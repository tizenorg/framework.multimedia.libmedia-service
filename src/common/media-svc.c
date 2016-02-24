/*
 * libmedia-service
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Hyunjun Ko <zzoon.ko@samsung.com>, Haejeong Kim <backto.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <string.h>
#include <errno.h>
#include <media-util.h>
#include "media-svc.h"
#include "media-svc-media.h"
#include "media-svc-debug.h"
#include "media-svc-util.h"
#include "media-svc-db-utils.h"
#include "media-svc-env.h"
#include "media-svc-media-folder.h"
#include "media-svc-album.h"
#include "media-svc-noti.h"
#include "media-svc-storage.h"

static __thread int g_media_svc_item_validity_data_cnt = 1;
static __thread int g_media_svc_item_validity_cur_data_cnt = 0;

static __thread int g_media_svc_move_item_data_cnt = 1;
static __thread int g_media_svc_move_item_cur_data_cnt = 0;

static __thread int g_media_svc_insert_item_data_cnt = 1;
static __thread int g_media_svc_insert_item_cur_data_cnt = 0;

static __thread int g_media_svc_update_item_data_cnt = 1;
static __thread int g_media_svc_update_item_cur_data_cnt = 0;

static __thread int g_media_svc_insert_folder_data_cnt = 1;
static __thread int g_media_svc_insert_folder_cur_data_cnt = 0;

/* Flag for items to be published by notification */
static __thread int g_insert_with_noti = FALSE;

#define DEFAULT_MEDIA_SVC_STORAGE_ID "media"

typedef struct{
	int media_type;
	char *path;
}media_svc_item_info_s;

static bool __media_svc_check_storage(media_svc_storage_type_e storage_type, bool check_all)
{
	if (check_all == TRUE) {
		if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL)
			&& (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)
			&& (storage_type != MEDIA_SVC_STORAGE_EXTERNAL_USB)
			&& (storage_type != MEDIA_SVC_STORAGE_CLOUD)) {
			media_svc_error("storage type is incorrect[%d]", storage_type);
			return FALSE;
		}
	} else {
		if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL)
			&& (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)
			&& (storage_type != MEDIA_SVC_STORAGE_EXTERNAL_USB)) {
			media_svc_error("storage type is incorrect[%d]", storage_type);
			return FALSE;
		}
	}

	return TRUE;
}

int media_svc_connect(MediaSvcHandle **handle)
{
	int ret = MS_MEDIA_ERR_NONE;
	MediaDBHandle *db_handle = NULL;

	media_svc_debug_fenter();

	ret = media_db_connect(0, &db_handle, FALSE);
	if(ret != MS_MEDIA_ERR_NONE)
		return ret;

	*handle = db_handle;
	return MS_MEDIA_ERR_NONE;
}

int media_svc_disconnect(MediaSvcHandle *handle)
{
	MediaDBHandle * db_handle = (MediaDBHandle *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	int ret = MS_MEDIA_ERR_NONE;

	ret = media_db_disconnect(db_handle);
	return ret;
}

int media_svc_create_table(MediaSvcHandle *handle)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	ret = _media_svc_init_table_query(MEDIA_SVC_DB_TABLE_MEDIA);
	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	/*create media table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_MEDIA, MEDIA_SVC_DB_LIST_MEDIA);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*create folder table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_FOLDER, MEDIA_SVC_DB_LIST_FOLDER);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*create playlist_map table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_PLAYLIST_MAP, MEDIA_SVC_DB_LIST_PLAYLIST_MAP);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*create playlist table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_PLAYLIST, MEDIA_SVC_DB_LIST_PLAYLIST);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/* create album table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_ALBUM, MEDIA_SVC_DB_LIST_ALBUM);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*create tag_map table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_TAG_MAP, MEDIA_SVC_DB_LIST_TAG_MAP);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*create tag table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_TAG, MEDIA_SVC_DB_LIST_TAG);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*create bookmark table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_BOOKMARK, MEDIA_SVC_DB_LIST_BOOKMARK);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*create storage table*/
	ret = _media_svc_make_table_query(db_handle, MEDIA_SVC_DB_TABLE_STORAGE, MEDIA_SVC_DB_LIST_STORAGE);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

#if 0
	/*init storage table*/
	ret = _media_svc_init_storage(db_handle);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);
#endif
	_media_svc_destroy_table_query();

	media_svc_debug_fleave();

	return MS_MEDIA_ERR_NONE;
}

int media_svc_get_storage_type(const char *path, media_svc_storage_type_e *storage_type)
{
	int ret = MS_MEDIA_ERR_NONE;
	media_svc_storage_type_e type;

	ret = _media_svc_get_storage_type_by_path(path, &type);
	media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "_media_svc_get_storage_type_by_path failed : %d", ret);

	*storage_type = type;

	return ret;
}

int media_svc_get_file_info(MediaSvcHandle *handle, const char *storage_id, const char *path, time_t *modified_time, unsigned long long *size)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");

	ret = _media_svc_get_fileinfo_by_path(db_handle, storage_id, path, modified_time, size);

	return ret;
}

int media_svc_check_item_exist_by_path(MediaSvcHandle *handle, const char *storage_id, const char *path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	int count = -1;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "Path is NULL");

	ret = _media_svc_count_record_with_path(db_handle, storage_id, path, &count);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	if(count > 0) {
		media_svc_debug("item is exist in database");
		return MS_MEDIA_ERR_NONE;
	} else {
		media_svc_debug("item is not exist in database");
		return MS_MEDIA_ERR_DB_NO_RECORD;
	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_insert_item_begin(MediaSvcHandle *handle, int data_cnt, int with_noti, int from_pid)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("Transaction data count : [%d]", data_cnt);

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(data_cnt < 1, MS_MEDIA_ERR_INVALID_PARAMETER, "data_cnt shuld be bigger than 1");

	g_media_svc_insert_item_data_cnt  = data_cnt;
	g_media_svc_insert_item_cur_data_cnt  = 0;

	/* Prepare for making noti item list */
	if (with_noti) {
		media_svc_debug("making noti list from pid[%d]", from_pid);
		if (_media_svc_create_noti_list(data_cnt) != MS_MEDIA_ERR_NONE) {
			return MS_MEDIA_ERR_OUT_OF_MEMORY;
		}

		_media_svc_set_noti_from_pid(from_pid);
		g_insert_with_noti = TRUE;
	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_insert_item_end(MediaSvcHandle *handle)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	if (g_media_svc_insert_item_cur_data_cnt  > 0) {

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_INSERT_ITEM);
		if (g_insert_with_noti) {
			media_svc_debug("sending noti list");
			_media_svc_publish_noti_list(g_media_svc_insert_item_cur_data_cnt);
			_media_svc_destroy_noti_list(g_media_svc_insert_item_cur_data_cnt);
			g_insert_with_noti = FALSE;
			_media_svc_set_noti_from_pid(-1);
		}
	}

	g_media_svc_insert_item_data_cnt  = 1;
	g_media_svc_insert_item_cur_data_cnt  = 0;

	return ret;
}

int media_svc_insert_item_bulk(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path, int is_burst)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};
	media_svc_media_type_e media_type;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set media info*/
	/* if drm_contentinfo is not NULL, the file is OMA DRM.*/
	ret = _media_svc_set_media_info(&content_info, storage_id, storage_type, path, &media_type, FALSE);
	if (ret != MS_MEDIA_ERR_NONE) {
		return ret;
	}

	if(media_type == MEDIA_SVC_MEDIA_TYPE_OTHER) {
		/*Do nothing.*/
	} else if(media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) {
		ret = _media_svc_extract_image_metadata(db_handle, &content_info);
	} else {
		ret = _media_svc_extract_media_metadata(db_handle, &content_info);
	}
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	/*Set or Get folder id*/
	ret = _media_svc_get_and_append_folder_id_by_path(db_handle, storage_id, path, storage_type, folder_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info.folder_uuid, folder_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	if (g_media_svc_insert_item_data_cnt == 1) {

		ret = _media_svc_insert_item_with_data(db_handle, storage_id, &content_info, is_burst, FALSE);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		if (g_insert_with_noti)
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt++);

	} else if(g_media_svc_insert_item_cur_data_cnt  < (g_media_svc_insert_item_data_cnt  - 1)) {

		ret = _media_svc_insert_item_with_data(db_handle, storage_id, &content_info, is_burst, TRUE);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		if (g_insert_with_noti)
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt);

		g_media_svc_insert_item_cur_data_cnt ++;

	} else if (g_media_svc_insert_item_cur_data_cnt  == (g_media_svc_insert_item_data_cnt  - 1)) {

		ret = _media_svc_insert_item_with_data(db_handle, storage_id, &content_info, is_burst, TRUE);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		if (g_insert_with_noti)
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt);

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_INSERT_ITEM);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		if (g_insert_with_noti) {
			_media_svc_publish_noti_list(g_media_svc_insert_item_cur_data_cnt + 1);
			_media_svc_destroy_noti_list(g_media_svc_insert_item_cur_data_cnt + 1);

			/* Recreate noti list */
			ret = _media_svc_create_noti_list(g_media_svc_insert_item_data_cnt);
			media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);
		}

		g_media_svc_insert_item_cur_data_cnt = 0;

	} else {
		media_svc_error("Error in media_svc_insert_item_bulk");
		_media_svc_destroy_content_info(&content_info);
		return MS_MEDIA_ERR_INTERNAL;
	}

	_media_svc_destroy_content_info(&content_info);

	return MS_MEDIA_ERR_NONE;
}

int media_svc_insert_item_immediately(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0, };
	media_svc_media_type_e media_type;
	int ini_val = _media_svc_get_ini_value();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set media info*/
	ret = _media_svc_set_media_info(&content_info, storage_id, storage_type, path, &media_type, FALSE);
	if (ret != MS_MEDIA_ERR_NONE) {
		return ret;
	}

	if(media_type == MEDIA_SVC_MEDIA_TYPE_OTHER) {
		/*Do nothing.*/
	} else if(media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) {
		ret = _media_svc_extract_image_metadata(db_handle, &content_info);
	} else {
		ret = _media_svc_extract_media_metadata(db_handle, &content_info);
	}

	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	/*Set or Get folder id*/
	ret = _media_svc_get_and_append_folder_id_by_path(db_handle, storage_id, path, storage_type, folder_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info.folder_uuid, folder_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);
#if 1
	/* Extracting thumbnail */
	if (ini_val == 1) {
		if (content_info.thumbnail_path == NULL) {
			if (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
				char thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0, };
				int width = 0;
				int height = 0;

				ret = _media_svc_request_thumbnail_with_origin_size(content_info.path, thumb_path, sizeof(thumb_path), &width, &height);
				if(ret == MS_MEDIA_ERR_NONE) {
					ret = __media_svc_malloc_and_strncpy(&(content_info.thumbnail_path), thumb_path);
				}

				if (content_info.media_meta.width <= 0)
					content_info.media_meta.width = width;

				if (content_info.media_meta.height <= 0)
					content_info.media_meta.height = height;
			}
		}
	}
#endif

	ret = _media_svc_insert_item_with_data(db_handle, storage_id, &content_info, FALSE, FALSE);

	if (ret == MS_MEDIA_ERR_NONE) {
		media_svc_debug("Insertion is successful. Sending noti for this");
		_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_INSERT, content_info.path, content_info.media_type, content_info.media_uuid, content_info.mime_type);
	} else if (ret == MS_MEDIA_ERR_DB_CONSTRAINT_FAIL) {
		media_svc_error("This item is already inserted. This may be normal operation because other process already did this");
	}

	_media_svc_destroy_content_info(&content_info);
	return ret;
}

int media_svc_move_item(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e src_storage, const char *src_path,
			media_svc_storage_type_e dest_storage, const char *dest_path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char *file_name = NULL;
	char *folder_path = NULL;
	int modified_time = 0;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};
	char old_thumb_path[MEDIA_SVC_PATHNAME_SIZE] = {0,};
	char new_thumb_path[MEDIA_SVC_PATHNAME_SIZE] = {0,};
	int media_type = -1;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(src_path), MS_MEDIA_ERR_INVALID_PARAMETER, "src_path is NULL");
	media_svc_retvm_if(!STRING_VALID(dest_path), MS_MEDIA_ERR_INVALID_PARAMETER, "dest_path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(src_storage, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid src_storage");
	media_svc_retvm_if(__media_svc_check_storage(dest_storage, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid dest_storage");

	/*check and update folder*/
	ret = _media_svc_get_and_append_folder_id_by_path(db_handle, storage_id, dest_path, dest_storage, folder_uuid);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*get filename*/
	file_name = g_path_get_basename(dest_path);

	/*get modified_time*/
	modified_time = _media_svc_get_file_time(dest_path);

	/*get thumbnail_path to update. only for Imgae and Video items. Audio share album_art(thumbnail)*/
	ret = _media_svc_get_media_type_by_path(db_handle, storage_id, src_path, &media_type);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("_media_svc_get_media_type_by_path failed");
		SAFE_FREE(file_name);
		return ret;
	}

	if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
		/*get old thumbnail_path*/
		ret = _media_svc_get_thumbnail_path_by_path(db_handle, storage_id, src_path, old_thumb_path);
		if ((ret != MS_MEDIA_ERR_NONE) && (ret != MS_MEDIA_ERR_DB_NO_RECORD)) {
			media_svc_error("_media_svc_get_thumbnail_path_by_path failed");
			SAFE_FREE(file_name);
			return ret;
		}

		/* If old thumb path is default or not */
		if (strncmp(old_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) == 0) {
			strncpy(new_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(new_thumb_path));
		} else {
			_media_svc_get_thumbnail_path(dest_storage, new_thumb_path, dest_path, THUMB_EXT);
		}
	}

	if (g_media_svc_move_item_data_cnt == 1) {

		/*update item*/
		if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
			ret = _media_svc_update_item_by_path(db_handle, storage_id, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, new_thumb_path, FALSE);
		} else {
			ret = _media_svc_update_item_by_path(db_handle, storage_id, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, NULL, FALSE);
		}
		SAFE_FREE(file_name);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		media_svc_debug("Move is successful. Sending noti for this");

		/* Get notification info */
		media_svc_noti_item *noti_item = NULL;
		ret = _media_svc_get_noti_info(db_handle, storage_id, dest_path, MS_MEDIA_ITEM_FILE, &noti_item);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		/* Send notification for move */
		_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_UPDATE, src_path, media_type, noti_item->media_uuid, noti_item->mime_type);
		_media_svc_destroy_noti_item(noti_item);

		/*update folder modified_time*/
		folder_path = g_path_get_dirname(dest_path);
		ret = _media_svc_update_folder_modified_time_by_folder_uuid(db_handle, folder_uuid, folder_path, FALSE);
		SAFE_FREE(folder_path);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		ret = _media_svc_update_folder_table(db_handle, storage_id);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	} else if (g_media_svc_move_item_cur_data_cnt  < (g_media_svc_move_item_data_cnt  - 1)) {

		/*update item*/
		if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
			ret = _media_svc_update_item_by_path(db_handle, storage_id, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, new_thumb_path, TRUE);
		} else {
			ret = _media_svc_update_item_by_path(db_handle, storage_id, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, NULL, TRUE);
		}
		SAFE_FREE(file_name);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		/*update folder modified_time*/
		folder_path = g_path_get_dirname(dest_path);
		ret = _media_svc_update_folder_modified_time_by_folder_uuid(db_handle, folder_uuid, folder_path, TRUE);
		SAFE_FREE(folder_path);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		g_media_svc_move_item_cur_data_cnt ++;

	} else if (g_media_svc_move_item_cur_data_cnt  == (g_media_svc_move_item_data_cnt  - 1)) {

		/*update item*/
		if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
			ret = _media_svc_update_item_by_path(db_handle, storage_id, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, new_thumb_path, TRUE);
		} else {
			ret = _media_svc_update_item_by_path(db_handle, storage_id, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, NULL, TRUE);
		}
		SAFE_FREE(file_name);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		/*update folder modified_time*/
		folder_path = g_path_get_dirname(dest_path);
		ret = _media_svc_update_folder_modified_time_by_folder_uuid(db_handle, folder_uuid, folder_path, TRUE);
		SAFE_FREE(folder_path);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		/*update db*/
		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_MOVE_ITEM);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		g_media_svc_move_item_cur_data_cnt = 0;

	} else {
		media_svc_error("Error in media_svc_move_item");
		return MS_MEDIA_ERR_INTERNAL;
	}

	/*rename thumbnail file*/
//	if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
		if((strlen(old_thumb_path) > 0) && (strncmp(old_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
			ret = _media_svc_rename_file(old_thumb_path,new_thumb_path);
			if (ret != MS_MEDIA_ERR_NONE)
				media_svc_error("_media_svc_rename_file failed : %d", ret);
		}
//	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_set_item_validity_begin(MediaSvcHandle *handle, int data_cnt)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("Transaction data count : [%d]", data_cnt);

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(data_cnt < 1, MS_MEDIA_ERR_INVALID_PARAMETER, "data_cnt shuld be bigger than 1");

	g_media_svc_item_validity_data_cnt  = data_cnt;
	g_media_svc_item_validity_cur_data_cnt  = 0;

	return MS_MEDIA_ERR_NONE;
}

int media_svc_set_item_validity_end(MediaSvcHandle *handle)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	if (g_media_svc_item_validity_cur_data_cnt  > 0) {

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_SET_ITEM_VALIDITY);
	}

	g_media_svc_item_validity_data_cnt  = 1;
	g_media_svc_item_validity_cur_data_cnt  = 0;

	return ret;
}

int media_svc_set_item_validity(MediaSvcHandle *handle, const char *storage_id, const char *path, int validity)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");

	media_svc_debug("path=[%s], validity=[%d]", path, validity);

	if (g_media_svc_item_validity_data_cnt  == 1) {

		return _media_svc_update_item_validity(db_handle, storage_id, path, validity, FALSE);

	} else if (g_media_svc_item_validity_cur_data_cnt  < (g_media_svc_item_validity_data_cnt  - 1)) {

		ret = _media_svc_update_item_validity(db_handle,storage_id,  path, validity, TRUE);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		g_media_svc_item_validity_cur_data_cnt ++;

	} else if (g_media_svc_item_validity_cur_data_cnt  == (g_media_svc_item_validity_data_cnt  - 1)) {

		ret = _media_svc_update_item_validity(db_handle, storage_id, path, validity, TRUE);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_SET_ITEM_VALIDITY);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		g_media_svc_item_validity_cur_data_cnt  = 0;

	} else {

		media_svc_error("Error in media_svc_set_item_validity");
		return MS_MEDIA_ERR_INTERNAL;
	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_delete_item_by_path(MediaSvcHandle *handle, const char *storage_id, const char *path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char thumb_path[MEDIA_SVC_PATHNAME_SIZE] = {0,};

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");

	int media_type = -1;
	ret = _media_svc_get_media_type_by_path(db_handle, storage_id, path, &media_type);
	media_svc_retv_if((ret != MS_MEDIA_ERR_NONE), ret);

#if 0
	if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
		/*Get thumbnail path to delete*/
		ret = _media_svc_get_thumbnail_path_by_path(db_handle, storage_id, path, thumb_path);
		media_svc_retv_if((ret != MS_MEDIA_ERR_NONE) && (ret != MS_MEDIA_ERR_DB_NO_RECORD), ret);
	} else if ((media_type == MEDIA_SVC_MEDIA_TYPE_SOUND) ||(media_type == MEDIA_SVC_MEDIA_TYPE_MUSIC)) {
		int count = 0;
		ret = _media_svc_get_media_count_with_album_id_by_path(db_handle, path, &count);
		media_svc_retv_if((ret != MS_MEDIA_ERR_NONE), ret);

		if (count == 1) {
			/*Get thumbnail path to delete*/
			ret = _media_svc_get_thumbnail_path_by_path(db_handle, storage_id, path, thumb_path);
			media_svc_retv_if((ret != MS_MEDIA_ERR_NONE) && (ret != MS_MEDIA_ERR_DB_NO_RECORD), ret);
		}
	}
#endif
	/*Get thumbnail path to delete*/
	ret = _media_svc_get_thumbnail_path_by_path(db_handle, storage_id, path, thumb_path);
	media_svc_retv_if((ret != MS_MEDIA_ERR_NONE) && (ret != MS_MEDIA_ERR_DB_NO_RECORD), ret);

	if (g_media_svc_insert_item_data_cnt == 1) {

		/* Get notification info */
		media_svc_noti_item *noti_item = NULL;
		ret = _media_svc_get_noti_info(db_handle, storage_id, path, MS_MEDIA_ITEM_FILE, &noti_item);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		/*Delete item*/
		ret = _media_svc_delete_item_by_path(db_handle, storage_id, path, FALSE);
		if (ret != MS_MEDIA_ERR_NONE) {
			media_svc_error("_media_svc_delete_item_by_path failed : %d", ret);
			_media_svc_destroy_noti_item(noti_item);

			return ret;
		}

		/* Send notification */
		media_svc_debug("Deletion is successful. Sending noti for this");
		_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_DELETE, path, media_type, noti_item->media_uuid, noti_item->mime_type);
		_media_svc_destroy_noti_item(noti_item);
	} else {
		ret = _media_svc_delete_item_by_path(db_handle, storage_id, path, TRUE);
		if (ret != MS_MEDIA_ERR_NONE) {
			media_svc_error("_media_svc_delete_item_by_path failed : %d", ret);
			return ret;
		}

	}

	/*Delete thumbnail*/
	if ((strlen(thumb_path) > 0) && (strncmp(thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
/*
		int thumb_count = 1;
		// Get count of media, which contains same thumbnail for music
		if ((media_type == MEDIA_SVC_MEDIA_TYPE_SOUND) ||(media_type == MEDIA_SVC_MEDIA_TYPE_MUSIC)) {
			ret = _media_svc_get_thumbnail_count(db_handle, storage_id, thumb_path, &thumb_count);
			if (ret != MS_MEDIA_ERR_NONE) {
				media_svc_error("Failed to get thumbnail count : %d", ret);
			}
		}

		if (thumb_count == 1) {
*/
			ret = _media_svc_remove_file(thumb_path);
			if(ret != MS_MEDIA_ERR_NONE) {
				media_svc_error("fail to remove thumbnail file.");
			}
//		}
	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_delete_all_items_in_storage(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("media_svc_delete_all_items_in_storage [%d]", storage_type);

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	ret = _media_svc_truncate_table(db_handle, storage_id, storage_type);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	if (storage_type != MEDIA_SVC_STORAGE_EXTERNAL_USB) {
		const char *dirpath = (storage_type == MEDIA_SVC_STORAGE_INTERNAL) ? MEDIA_SVC_THUMB_INTERNAL_PATH : MEDIA_SVC_THUMB_EXTERNAL_PATH;

		/* remove thumbnails */
		ret = _media_svc_remove_all_files_in_dir(dirpath);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);
	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_delete_invalid_items_in_storage(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	/*Delete from DB and remove thumbnail files*/
	return _media_svc_delete_invalid_items(db_handle, storage_id, storage_type);
}

int media_svc_delete_invalid_items_in_folder(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, bool is_recursive)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");

	/*Delete from DB and remove thumbnail files*/
	return _media_svc_delete_invalid_folder_items(db_handle, storage_id, folder_path, is_recursive);
}

int media_svc_set_all_storage_items_validity(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, int validity)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	return _media_svc_update_storage_item_validity(db_handle, storage_id, storage_type, validity);
}

int media_svc_set_folder_items_validity(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int validity, int recursive)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(folder_path), MS_MEDIA_ERR_INVALID_PARAMETER, "folder_path is NULL");

	if(recursive)
		return _media_svc_update_recursive_folder_item_validity(db_handle, storage_id, folder_path, validity);
	else
		return _media_svc_update_folder_item_validity(db_handle, storage_id, folder_path, validity);
}

int media_svc_refresh_item(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	media_svc_media_type_e media_type;
	int ini_val = _media_svc_get_ini_value();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set media info*/
	ret = _media_svc_set_media_info(&content_info, storage_id, storage_type, path, &media_type, TRUE);
	if (ret != MS_MEDIA_ERR_NONE) {
		return ret;
	}

	/* Initialize thumbnail information to remake thumbnail. */
	if(ini_val == 1) {
		char thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1];
		ret = _media_svc_get_thumbnail_path_by_path(db_handle, storage_id, path, thumb_path);
		if (ret != MS_MEDIA_ERR_NONE) {
			_media_svc_destroy_content_info(&content_info);
			return ret;
		}

		if (g_file_test(thumb_path, G_FILE_TEST_EXISTS) && (strncmp(thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
			ret = _media_svc_remove_file(thumb_path);
			if (ret != MS_MEDIA_ERR_NONE) {
				media_svc_error("_media_svc_remove_file failed : %s", thumb_path);
			}
		}

		ret = _media_svc_update_thumbnail_path(db_handle,  storage_id, path, NULL);
		if (ret != MS_MEDIA_ERR_NONE) {
			_media_svc_destroy_content_info(&content_info);
			return ret;
		}
	}

	/* Get notification info */
	media_svc_noti_item *noti_item = NULL;
	ret = _media_svc_get_noti_info(db_handle, storage_id, path, MS_MEDIA_ITEM_FILE, &noti_item);
	if (ret != MS_MEDIA_ERR_NONE) {
		_media_svc_destroy_content_info(&content_info);
		return ret;
	}

	media_type = noti_item->media_type;
	content_info.media_type = media_type;

	if(media_type == MEDIA_SVC_MEDIA_TYPE_OTHER) {
		/*Do nothing.*/
	} else if(media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) {
		ret = _media_svc_extract_image_metadata(db_handle, &content_info);
	} else {
		ret = _media_svc_extract_media_metadata(db_handle, &content_info);
	}

	if (ret != MS_MEDIA_ERR_NONE) {
		_media_svc_destroy_noti_item(noti_item);
		_media_svc_destroy_content_info(&content_info);
		return ret;
	}
#if 1
	/* Extracting thumbnail */
	if(ini_val == 1) {
		if (content_info.thumbnail_path == NULL) {
			if (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
				char thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0, };
				int width = 0;
				int height = 0;

				ret = _media_svc_request_thumbnail_with_origin_size(content_info.path, thumb_path, sizeof(thumb_path), &width, &height);
				if(ret == MS_MEDIA_ERR_NONE) {
					ret = __media_svc_malloc_and_strncpy(&(content_info.thumbnail_path), thumb_path);
				}

				if (content_info.media_meta.width <= 0)
					content_info.media_meta.width = width;

				if (content_info.media_meta.height <= 0)
					content_info.media_meta.height = height;
			}
		}
	} 

#endif

	ret = _media_svc_update_item_with_data(db_handle, storage_id, &content_info);

	if (ret == MS_MEDIA_ERR_NONE) {
		media_svc_debug("Update is successful. Sending noti for this");
		_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_UPDATE, content_info.path, noti_item->media_type, noti_item->media_uuid, noti_item->mime_type);
	} else {
		media_svc_error("_media_svc_update_item_with_data failed : %d", ret);
	}

	_media_svc_destroy_content_info(&content_info);
	_media_svc_destroy_noti_item(noti_item);

	return ret;
}

int media_svc_rename_folder(MediaSvcHandle *handle, const char *storage_id, const char *src_path, const char *dst_path)
{
	sqlite3 * db_handle = (sqlite3 *)handle;
	int ret = MS_MEDIA_ERR_NONE;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(src_path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "src_path is NULL");
	media_svc_retvm_if(dst_path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "dst_path is NULL");

	media_svc_debug("Src path : %s,  Dst Path : %s", src_path, dst_path);

	ret = _media_svc_sql_begin_trans(db_handle);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/* Update all folder record's modified date, which are changed above */
	char *update_folder_modified_time_sql = NULL;
	time_t date;
	time(&date);

	update_folder_modified_time_sql = sqlite3_mprintf("UPDATE folder SET modified_time = %d WHERE path LIKE '%q';", date, dst_path);

	ret = media_db_request_update_db_batch(update_folder_modified_time_sql);
	//ret = _media_svc_sql_query(db_handle, update_folder_modified_time_sql);
	sqlite3_free(update_folder_modified_time_sql);

	if (ret != SQLITE_OK) {
		media_svc_error("failed to update folder modified time");
		_media_svc_sql_rollback_trans(db_handle);

		return MS_MEDIA_ERR_DB_INTERNAL;
	}

	/* Update all items */
	char *select_all_sql = NULL;
	sqlite3_stmt *sql_stmt = NULL;
	char dst_child_path[MEDIA_SVC_PATHNAME_SIZE + 1];

	snprintf(dst_child_path, sizeof(dst_child_path), "%s/%%", dst_path);

	select_all_sql = sqlite3_mprintf("SELECT media_uuid, path, thumbnail_path, media_type from media where folder_uuid IN ( SELECT folder_uuid FROM folder where path='%q' or path like '%q');", dst_path, dst_child_path);

	media_svc_debug("[SQL query] : %s", select_all_sql);

	ret = _media_svc_sql_prepare_to_step_simple(db_handle, select_all_sql, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("error when media_svc_rename_folder. err = [%d]", ret);
		_media_svc_sql_rollback_trans(db_handle);
		return ret;
	}

	while (sqlite3_step(sql_stmt) == SQLITE_ROW) {
		char media_uuid[MEDIA_SVC_UUID_SIZE + 1] = {0, };
		char media_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0, };
		char media_thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0, };
		char media_new_thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0, };
		//int media_type;
		bool no_thumb = FALSE;

		if (STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 0))) {
			strncpy(media_uuid, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(media_uuid));
			media_uuid[sizeof(media_uuid) - 1] = '\0';
		} else {
			media_svc_error("media UUID is NULL");
			return MS_MEDIA_ERR_DB_INTERNAL;
		}

		if (STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 1))) {
			strncpy(media_path, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(media_path));
			media_path[sizeof(media_path) - 1] = '\0';
		} else {
			media_svc_error("media path is NULL");
			return MS_MEDIA_ERR_DB_INTERNAL;
		}

		if (STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 2))) {
			strncpy(media_thumb_path,	(const char *)sqlite3_column_text(sql_stmt, 2), sizeof(media_thumb_path));
			media_thumb_path[sizeof(media_thumb_path) - 1] = '\0';
		} else {
			media_svc_debug("media thumb path doesn't exist in DB");
			no_thumb = TRUE;
		}

		//media_type = sqlite3_column_int(sql_stmt, 3);

		/* Update path, thumbnail path of this item */
		char *replaced_path = NULL;
		replaced_path = _media_svc_replace_path(media_path, src_path, dst_path);
		if (replaced_path == NULL) {
			media_svc_error("_media_svc_replace_path failed");
			SQLITE3_FINALIZE(sql_stmt);
			_media_svc_sql_rollback_trans(db_handle);
			return MS_MEDIA_ERR_INTERNAL;
		}

		media_svc_debug("New media path : %s", replaced_path);
		media_svc_storage_type_e storage_type;

		if (!no_thumb) {
			/* If old thumb path is default or not */
			if (strncmp(media_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) == 0) {
				strncpy(media_new_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(media_new_thumb_path));
			} else {
				ret = _media_svc_get_storage_type_by_path(replaced_path, &storage_type);
				if (ret != MS_MEDIA_ERR_NONE) {
					media_svc_error("_media_svc_get_storage_type_by_path failed : %d", ret);
					SAFE_FREE(replaced_path);
					_media_svc_sql_rollback_trans(db_handle);
					return ret;
				}
	
				ret = _media_svc_get_thumbnail_path(storage_type, media_new_thumb_path, replaced_path, THUMB_EXT);
				if (ret != MS_MEDIA_ERR_NONE) {
					media_svc_error("_media_svc_get_thumbnail_path failed : %d", ret);
					SAFE_FREE(replaced_path);
					SQLITE3_FINALIZE(sql_stmt);
					_media_svc_sql_rollback_trans(db_handle);
					return ret;
				}
			}

			//media_svc_debug("New media thumbnail path : %s", media_new_thumb_path);
		}

		char *update_item_sql = NULL;

		if (no_thumb) {
			update_item_sql = sqlite3_mprintf("UPDATE media SET path='%q' WHERE media_uuid='%q'", replaced_path, media_uuid);
		} else {
#if 0
			if (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
				update_item_sql = sqlite3_mprintf("UPDATE media SET path='%q', thumbnail_path='%q' WHERE media_uuid='%q'", replaced_path, media_new_thumb_path, media_uuid);
			} else {
				update_item_sql = sqlite3_mprintf("UPDATE media SET path='%q', thumbnail_path='%q' WHERE media_uuid='%q'", replaced_path, media_thumb_path, media_uuid);
			}
#else
			update_item_sql = sqlite3_mprintf("UPDATE media SET path='%q', thumbnail_path='%q' WHERE media_uuid='%q'", replaced_path, media_new_thumb_path, media_uuid);
#endif
		}

		ret = media_db_request_update_db_batch(update_item_sql);
		//ret = _media_svc_sql_query(db_handle, update_item_sql);
		sqlite3_free(update_item_sql);
		SAFE_FREE(replaced_path);

		if (ret != SQLITE_OK) {
			media_svc_error("failed to update item");
			SQLITE3_FINALIZE(sql_stmt);
			_media_svc_sql_rollback_trans(db_handle);
	
			return MS_MEDIA_ERR_DB_INTERNAL;
		}

		/* Rename thumbnail file of file system */
//		if ((!no_thumb) && (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)
//				&& (strncmp(media_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
		if ((!no_thumb) && (strncmp(media_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
			ret = _media_svc_rename_file(media_thumb_path, media_new_thumb_path);
			if (ret != MS_MEDIA_ERR_NONE) {
				media_svc_error("_media_svc_rename_file failed : %d", ret);
			}
		}
	}

	SQLITE3_FINALIZE(sql_stmt);

	ret = _media_svc_sql_end_trans(db_handle);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("mb_svc_sqlite3_commit_trans failed.. Now start to rollback");
		_media_svc_sql_rollback_trans(db_handle);
		return ret;
	}

	media_svc_debug("Folder update is successful. Sending noti for this");
	/* Get notification info */
	media_svc_noti_item *noti_item = NULL;
	ret = _media_svc_get_noti_info(db_handle, storage_id, dst_path, MS_MEDIA_ITEM_DIRECTORY, &noti_item);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	_media_svc_publish_noti(MS_MEDIA_ITEM_DIRECTORY, MS_MEDIA_ITEM_UPDATE, src_path, -1, noti_item->media_uuid, NULL);
	_media_svc_destroy_noti_item(noti_item);

	return MS_MEDIA_ERR_NONE;
}

int media_svc_request_update_db(const char *db_query)
{
	media_svc_retvm_if(!STRING_VALID(db_query), MS_MEDIA_ERR_INVALID_PARAMETER, "db_query is NULL");

	return _media_svc_sql_query(NULL, db_query);
}

int media_svc_send_dir_update_noti(MediaSvcHandle *handle, const char *storage_id, const char *dir_path, const char *folder_id, media_item_update_type_e update_type, int pid)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char *uuid = NULL;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(dir_path), MS_MEDIA_ERR_INVALID_PARAMETER, "dir_path is NULL");

	/* Get notification info */
	media_svc_noti_item *noti_item = NULL;
	ret = _media_svc_get_noti_info(db_handle, storage_id, dir_path, MS_MEDIA_ITEM_DIRECTORY, &noti_item);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	if (folder_id != NULL) {
		uuid = folder_id;
	} else {
		uuid = noti_item->media_uuid;
	}

	ret = _media_svc_publish_dir_noti(MS_MEDIA_ITEM_DIRECTORY, MS_MEDIA_ITEM_UPDATE, dir_path, -1, noti_item->media_uuid, NULL, pid);
	ret = _media_svc_publish_dir_noti_v2(MS_MEDIA_ITEM_DIRECTORY, update_type, dir_path, -1, uuid, NULL, pid);
	_media_svc_destroy_noti_item(noti_item);

	return ret;
}

int media_svc_count_invalid_items_in_folder(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int *count)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(folder_path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "folder_path is NULL");
	media_svc_retvm_if(count == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "count is NULL");

	return _media_svc_count_invalid_folder_items(db_handle, storage_id, folder_path, count);
}

int media_svc_check_db_upgrade(MediaSvcHandle *handle, bool *need_full_scan)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	return _media_svc_check_db_upgrade(db_handle, need_full_scan);
}

int media_svc_check_db_corrupt(MediaSvcHandle *handle)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	return _media_db_check_corrupt(db_handle);
}

int media_svc_get_folder_list(MediaSvcHandle *handle, char* start_path, char ***folder_list, time_t **modified_time_list, int **item_num_list, int *count)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(count == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "count is NULL");

	return _media_svc_get_all_folders(db_handle, start_path, folder_list, modified_time_list, item_num_list, count);
}

int media_svc_update_folder_time(MediaSvcHandle *handle, const char *storage_id, const char *folder_path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	time_t sto_time = 0;
	int cur_time =  _media_svc_get_file_time(folder_path);
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	ret = _media_svc_get_folder_info_by_foldername(db_handle, storage_id, folder_path, folder_uuid, &sto_time);
	if (ret == MS_MEDIA_ERR_NONE) {
		if (sto_time != cur_time) {
			ret = _media_svc_update_folder_modified_time_by_folder_uuid(db_handle, folder_uuid, folder_path, FALSE);
		}
	}

	return ret;
}

int media_svc_update_item_begin(MediaSvcHandle *handle, int data_cnt)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("Transaction data count : [%d]", data_cnt);

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(data_cnt < 1, MS_MEDIA_ERR_INVALID_PARAMETER, "data_cnt shuld be bigger than 1");

	g_media_svc_update_item_data_cnt  = data_cnt;
	g_media_svc_update_item_cur_data_cnt  = 0;

	return MS_MEDIA_ERR_NONE;
}

int media_svc_update_item_end(MediaSvcHandle *handle)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	if (g_media_svc_update_item_cur_data_cnt  > 0) {

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_UPDATE_ITEM);
	}

	g_media_svc_update_item_data_cnt  = 1;
	g_media_svc_update_item_cur_data_cnt  = 0;

	return ret;
}

int media_svc_update_item_meta(MediaSvcHandle *handle, const char *file_path, int storage_type)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	media_svc_media_type_e media_type;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(file_path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set media info*/
	ret = _media_svc_set_media_info(&content_info, DEFAULT_MEDIA_SVC_STORAGE_ID, storage_type, file_path, &media_type, FALSE);
	if (ret != MS_MEDIA_ERR_NONE) {
		return ret;
	}

	if (media_type == MEDIA_SVC_MEDIA_TYPE_MUSIC) {
		ret = _media_svc_extract_music_metadata_for_update(db_handle, &content_info, media_type);
	} else {
		return MS_MEDIA_ERR_NONE;
	}

	if (ret != MS_MEDIA_ERR_NONE) {
		_media_svc_destroy_content_info(&content_info);
		return ret;
	}

	if (g_media_svc_update_item_data_cnt == 1) {

		ret = _media_svc_update_meta_with_data(db_handle, &content_info);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	} else if(g_media_svc_update_item_cur_data_cnt  < (g_media_svc_update_item_data_cnt  - 1)) {

		ret = _media_svc_update_meta_with_data(db_handle, &content_info);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		g_media_svc_update_item_cur_data_cnt ++;

	} else if (g_media_svc_update_item_cur_data_cnt  == (g_media_svc_update_item_data_cnt  - 1)) {

		ret = _media_svc_update_meta_with_data(db_handle, &content_info);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_UPDATE_ITEM);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		g_media_svc_update_item_cur_data_cnt = 0;

	} else {
		media_svc_error("Error in media_svc_update_item_meta");
		_media_svc_destroy_content_info(&content_info);
		return MS_MEDIA_ERR_INTERNAL;
	}

	_media_svc_destroy_content_info(&content_info);

	return ret;
}

int media_svc_publish_noti(MediaSvcHandle *handle, media_item_type_e update_item, media_item_update_type_e update_type, const char *path, media_type_e media_type, const char *uuid, const char *mime_type)
{
	return _media_svc_publish_noti(update_item, update_type, path, media_type, uuid, mime_type);
}

int media_svc_get_pinyin(MediaSvcHandle *handle, const char * src_str, char **pinyin_str)
{
	media_svc_retvm_if(!STRING_VALID(src_str), MS_MEDIA_ERR_INVALID_PARAMETER, "String is NULL");

	return _media_svc_get_pinyin_str(src_str, pinyin_str);
}

int media_svc_check_pinyin_support(bool *support)
{
	*support = _media_svc_check_pinyin_support();

	return MS_MEDIA_ERR_NONE;
}

int media_svc_set_storage_validity(MediaSvcHandle *handle, const char *storage_id, int validity)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	ret = _media_svc_update_storage_validity(db_handle, storage_id, validity);
	media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "update storage validity failed: %d", ret);

	ret = _media_svc_update_media_view(db_handle);
	media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "update media view failed : %d", ret);

	return ret;
}

int media_svc_get_storage_id(MediaSvcHandle *handle, const char *path, char *storage_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");

	ret = _media_svc_get_storage_uuid(db_handle, path, storage_id);

	return ret;
}

int media_svc_get_storage_path(MediaSvcHandle *handle, const char *storage_uuid, char **storage_path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(storage_uuid == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_uuid is NULL");

	ret = _media_svc_get_storage_path(db_handle, storage_uuid, storage_path);

	return ret;
}

int media_svc_get_storage_scan_status(MediaSvcHandle *handle, const char *storage_uuid, media_svc_scan_status_type_e *storage_status)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(storage_uuid == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_uuid is NULL");

	ret = _media_svc_get_storage_scan_status(db_handle, storage_uuid, storage_status);

	return ret;
}

int media_svc_set_storage_scan_status(MediaSvcHandle *handle, const char *storage_uuid, media_svc_scan_status_type_e storage_status)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	ret = _media_svc_set_storage_scan_status(db_handle, storage_uuid, storage_status);

	return ret;
}

int media_svc_get_storage_list(MediaSvcHandle *handle, char ***storage_list, char ***storage_id_list, int **scan_status_list, int *count)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(count == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "count is NULL");

	return _media_svc_get_all_storage(db_handle, storage_list, storage_id_list, scan_status_list, count);
}


static int __media_svc_copy_para_to_content(media_svc_content_info_s *content_info ,media_svc_content_info_s *new_content_info)
{
	media_svc_retvm_if(content_info == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(new_content_info == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	/*
	initialize media content datas
	*/
	int ret = MS_MEDIA_ERR_NONE;
	char * media_uuid = NULL;
	char * file_name = NULL;

	ret = __media_svc_malloc_and_strncpy(&new_content_info->path, content_info->path);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->storage_uuid, content_info->storage_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	/* Set default GPS value before extracting meta information */
	new_content_info->media_meta.longitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
	new_content_info->media_meta.latitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
	new_content_info->media_meta.altitude = MEDIA_SVC_DEFAULT_GPS_VALUE;

	/* Set default value before extracting meta information */
	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.title, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.description, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.copyright, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.track_num, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.album, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.artist, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

//	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.album_artist, MEDIA_SVC_TAG_UNKNOWN);
//	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.genre, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.composer, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.year, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	new_content_info->storage_type = content_info->storage_type;
	time(&new_content_info->added_time);

	media_uuid = _media_info_generate_uuid();
	media_svc_retvm_if(media_uuid == NULL, MS_MEDIA_ERR_INTERNAL, "Invalid UUID");

	ret = __media_svc_malloc_and_strncpy(&new_content_info->media_uuid, media_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	file_name = g_path_get_basename(content_info->path);
	ret = __media_svc_malloc_and_strncpy(&new_content_info->file_name, file_name);
	SAFE_FREE(file_name);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, new_content_info);

	new_content_info->played_count = 0;
	new_content_info->last_played_time= 0;
	new_content_info->last_played_position= 0;
	new_content_info->favourate= 0;
	new_content_info->media_meta.rating = 0;
	/*
	end of initializing
	*/

	new_content_info->size = content_info->size;
	new_content_info->added_time = content_info->added_time;
	new_content_info->modified_time = content_info->modified_time;
	new_content_info->is_drm = content_info->is_drm;
	new_content_info->media_type = content_info->media_type;

	if(STRING_VALID(content_info->mime_type))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->mime_type, content_info->mime_type);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy mime_type failed");
	}

	if(STRING_VALID(content_info->thumbnail_path))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->thumbnail_path, content_info->thumbnail_path);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy thumbnail_path failed");
	}

	if(STRING_VALID(content_info->media_meta.title))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.title, content_info->media_meta.title);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy title failed");
	}

	if(STRING_VALID(content_info->media_meta.album))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.album, content_info->media_meta.album);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy album failed");
	}

	if(STRING_VALID(content_info->media_meta.artist))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.artist, content_info->media_meta.artist);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy artist failed");
	}

/*
	if(STRING_VALID(content_info->media_meta.album_artist))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.album_artist, content_info->media_meta.album_artist);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy album_artist failed");
	}
*/

	if(STRING_VALID(content_info->media_meta.genre))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.genre, content_info->media_meta.genre);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy genre failed");
	}

	if(STRING_VALID(content_info->media_meta.composer))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.composer, content_info->media_meta.composer);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy composer failed");
	}

	if(STRING_VALID(content_info->media_meta.year))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.year, content_info->media_meta.year);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy year failed");
	}

	if(STRING_VALID(content_info->media_meta.recorded_date))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.recorded_date, content_info->media_meta.recorded_date);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy recorded_date failed");
	}

	if(STRING_VALID(content_info->media_meta.copyright))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.copyright, content_info->media_meta.copyright);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy copyright failed");
	}

	if(STRING_VALID(content_info->media_meta.track_num))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.track_num, content_info->media_meta.track_num);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy track_num failed");
	}

	if(STRING_VALID(content_info->media_meta.description))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.description, content_info->media_meta.description);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy description failed");
	}

	if(STRING_VALID(content_info->media_meta.weather))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.weather, content_info->media_meta.weather);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy description failed");
	}

	if(STRING_VALID(content_info->media_meta.datetaken))
	{
		ret = __media_svc_malloc_and_strncpy(&new_content_info->media_meta.datetaken, content_info->media_meta.datetaken);
		if(ret != MS_MEDIA_ERR_NONE)
			media_svc_error("strcpy datetaken failed");

		new_content_info->timeline = __media_svc_get_timeline_from_str(content_info->media_meta.datetaken);
		if (new_content_info->timeline == 0) {
			media_svc_error("Failed to get timeline : %s", new_content_info->media_meta.datetaken);
			new_content_info->timeline = new_content_info->modified_time;
		} else {
			media_svc_debug("Timeline : %ld", new_content_info->timeline);
		}
	}

	new_content_info->media_meta.bitrate = content_info->media_meta.bitrate;
	new_content_info->media_meta.samplerate = content_info->media_meta.samplerate;
	new_content_info->media_meta.channel = content_info->media_meta.channel;
	new_content_info->media_meta.duration = content_info->media_meta.duration;
	new_content_info->media_meta.longitude = content_info->media_meta.longitude;
	new_content_info->media_meta.latitude = content_info->media_meta.latitude;
	new_content_info->media_meta.altitude = content_info->media_meta.altitude;
	new_content_info->media_meta.width = content_info->media_meta.width;
	new_content_info->media_meta.height = content_info->media_meta.height;
	new_content_info->media_meta.orientation = content_info->media_meta.orientation;
	new_content_info->media_meta.rating = content_info->media_meta.rating;

	return 0;
}

int media_svc_insert_item_immediately_with_data(MediaSvcHandle *handle, media_svc_content_info_s *content_info)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};
	bool append_album = FALSE;

	/* Checking parameters if they are valid */
	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(content_info == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "content_info is NULL");
	media_svc_retvm_if(!STRING_VALID(content_info->path), MS_MEDIA_ERR_INVALID_PARAMETER, "file_path is NULL");
	media_svc_retvm_if(!STRING_VALID(content_info->mime_type), MS_MEDIA_ERR_INVALID_PARAMETER, "mime_type is NULL");

	if ((content_info->media_type < MEDIA_SVC_MEDIA_TYPE_IMAGE) || (content_info->media_type > MEDIA_SVC_MEDIA_TYPE_OTHER)) {
		media_svc_error("invalid media_type condition[%d]", content_info->media_type);
		return MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	if (content_info->size <= 0)
	{
		media_svc_error("invalid size condition[%d]", content_info->size);
		return MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	if (content_info->modified_time <= 0)
	{
		media_svc_error("invalid modified_time condition[%d]", content_info->modified_time);
		return MS_MEDIA_ERR_INVALID_PARAMETER;
	}
	media_svc_debug("storage[%d], path[%s], media_type[%d]", content_info->storage_type, content_info->path, content_info->media_type);

	media_svc_content_info_s _new_content_info;
	memset(&_new_content_info, 0, sizeof(media_svc_content_info_s));

	/* set othere data to the structure, which is passed as parameters */
	__media_svc_copy_para_to_content(content_info , &_new_content_info);

	/* Set or Get folder id */
	ret = _media_svc_get_and_append_folder_id_by_path(handle, _new_content_info.storage_uuid, _new_content_info.path, _new_content_info.storage_type, folder_uuid);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	ret = __media_svc_malloc_and_strncpy(&_new_content_info.folder_uuid, folder_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &_new_content_info);

	/* register album table data */

	int album_id = 0;
	if(_new_content_info.media_type == MEDIA_SVC_MEDIA_TYPE_SOUND || _new_content_info.media_type == MEDIA_SVC_MEDIA_TYPE_MUSIC)
	{
		ret = _media_svc_get_album_id(handle, _new_content_info.media_meta.album, _new_content_info.media_meta.artist, &album_id);

		if (ret != MS_MEDIA_ERR_NONE) {
			if (ret == MS_MEDIA_ERR_DB_NO_RECORD) {
				media_svc_debug("album does not exist. So start to make album art");
				append_album = TRUE;
			} else {
				media_svc_debug("other error");
				append_album = FALSE;
			}
		} else {
			_new_content_info.album_id = album_id;
			append_album = FALSE;

			if((!strncmp(_new_content_info.media_meta.album, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN))) ||
				(!strncmp(_new_content_info.media_meta.artist, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN)))) {

				media_svc_debug("Unknown album or artist already exists. Extract thumbnail for Unknown.");
			} else {

				media_svc_debug("album already exists. don't need to make album art");
				ret = _media_svc_get_album_art_by_album_id(handle, album_id, &_new_content_info.thumbnail_path);
				media_svc_retv_del_if((ret != MS_MEDIA_ERR_NONE) && (ret != MS_MEDIA_ERR_DB_NO_RECORD), ret, &_new_content_info);
			}
		}
	}

	if(append_album == TRUE) {

		if((strncmp(_new_content_info.media_meta.album, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN))) &&
			(strncmp(_new_content_info.media_meta.artist, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN))))
			ret = _media_svc_append_album(handle, _new_content_info.media_meta.album, _new_content_info.media_meta.artist, _new_content_info.thumbnail_path, &album_id);
		else
			ret = _media_svc_append_album(handle, _new_content_info.media_meta.album, _new_content_info.media_meta.artist, NULL, &album_id);

		_new_content_info.album_id = album_id;
	}

	/* Insert to db - calling _media_svc_insert_item_with_data */
	ret = _media_svc_insert_item_with_data(db_handle, _new_content_info.storage_uuid, &_new_content_info, FALSE, FALSE);

	if (ret == MS_MEDIA_ERR_NONE) {
		media_svc_debug("Insertion is successful.");
	}

	if (ret == MS_MEDIA_ERR_DB_CONSTRAINT_FAIL) {
		media_svc_error("This item is already inserted. This may be normal operation because other process already did this");
	}

	_media_svc_destroy_content_info(&_new_content_info);

	/* handling returned value - important */
	return ret;
}

void media_svc_destroy_content_info(media_svc_content_info_s *content_info)
{
	_media_svc_destroy_content_info(content_info);
}

int media_svc_generate_uuid(char **uuid)
{
	char *gen_uuid = NULL;
	gen_uuid = _media_info_generate_uuid();
	media_svc_retvm_if(gen_uuid == NULL, MS_MEDIA_ERR_INTERNAL, "Fail to generate uuid");

	*uuid = strdup(gen_uuid);

	return MS_MEDIA_ERR_NONE;
}

int media_svc_get_mmc_info(MediaSvcHandle *handle, char **storage_name, char **storage_path, int *validity, bool *info_exist)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	return _media_svc_get_mmc_info(db_handle, storage_name, storage_path, validity, info_exist);
}

int media_svc_check_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_name, char **storage_path, int *validity)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(storage_id == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(storage_path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_path is NULL");
	media_svc_retvm_if(validity == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "validity is NULL");

	return _media_svc_check_storage(db_handle, storage_id, storage_name, storage_path, validity);
}

int media_svc_update_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_path)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(storage_id == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(storage_path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_path is NULL");

	return _media_svc_update_storage_path(db_handle, storage_id, storage_path);
}

int media_svc_insert_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_name, const char *storage_path, const char *storage_account, media_svc_storage_type_e storage_type)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(storage_id == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(storage_path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	ret = _media_svc_append_storage(db_handle, storage_id, storage_name, storage_path, storage_account, storage_type);
	media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "append storage failed : %d", ret);

	if(strcmp(storage_id, MEDIA_SVC_DB_TABLE_MEDIA)) {
		ret = _media_svc_create_media_table_with_id(db_handle, storage_id);
		media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "create media table failed : %d", ret);

		ret = _media_svc_update_media_view(db_handle);
		media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "update media view failed : %d", ret);
	}

	return ret;
}

int media_svc_delete_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_name)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	media_svc_storage_type_e storage_type;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(storage_id == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");

	ret = _media_svc_get_storage_type(db_handle, storage_id, &storage_type);
	media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "_media_svc_get_storage_type_by_path failed : %d", ret);

	ret = _media_svc_delete_storage(db_handle, storage_id, storage_name);
	media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "remove storage failed : %d", ret);

	ret = _media_svc_delete_folder_by_storage_id(db_handle, storage_id, storage_type);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("fail to _media_svc_delete_folder_by_storage_id. error : [%d]", ret);
	}

	if (storage_type == MEDIA_SVC_STORAGE_EXTERNAL_USB) {
		ret = _media_svc_drop_media_table(db_handle, storage_id);
		media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "drop table failed : %d", ret);

		ret = _media_svc_update_media_view(db_handle);
		media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "update media view failed : %d", ret);
	}

	return ret;
}

int media_svc_insert_folder_begin(MediaSvcHandle *handle, int data_cnt)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("Transaction data count : [%d]", data_cnt);

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(data_cnt < 1, MS_MEDIA_ERR_INVALID_PARAMETER, "data_cnt shuld be bigger than 1");

	g_media_svc_insert_folder_data_cnt = data_cnt;
	g_media_svc_insert_folder_cur_data_cnt  = 0;

	return MS_MEDIA_ERR_NONE;
}

int media_svc_insert_folder_end(MediaSvcHandle *handle)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	if (g_media_svc_insert_folder_cur_data_cnt  > 0) {

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_INSERT_FOLDER);
	}

	g_media_svc_insert_folder_data_cnt  = 1;
	g_media_svc_insert_folder_cur_data_cnt  = 0;

	return ret;
}

int media_svc_insert_folder(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	if (g_media_svc_insert_folder_data_cnt  == 1) {

		ret = _media_svc_get_and_append_folder_id_by_folder_path(handle, storage_id, path, storage_type, folder_uuid, FALSE);

	} else if (g_media_svc_insert_folder_cur_data_cnt  < (g_media_svc_insert_folder_data_cnt  - 1)) {

		ret = _media_svc_get_and_append_folder_id_by_folder_path(handle, storage_id, path, storage_type, folder_uuid, TRUE);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		g_media_svc_insert_folder_cur_data_cnt ++;

	} else if (g_media_svc_insert_folder_cur_data_cnt  == (g_media_svc_insert_folder_data_cnt  - 1)) {

		ret = _media_svc_get_and_append_folder_id_by_folder_path(handle, storage_id, path, storage_type, folder_uuid, TRUE);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_INSERT_FOLDER);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		g_media_svc_insert_folder_cur_data_cnt  = 0;

	} else {
		media_svc_error("Error in media_svc_set_insert_folder");
		return MS_MEDIA_ERR_INTERNAL;
	}

	return ret;
}

int media_svc_delete_invalid_folder(MediaSvcHandle *handle, const char *storage_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	ret = _media_svc_delete_invalid_folder(db_handle, storage_id);

	return ret;
}

int media_svc_set_folder_validity(MediaSvcHandle *handle, const char *storage_id, const char *start_path, int validity, bool is_recursive)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	ret = _media_svc_set_folder_validity(db_handle, storage_id, start_path, validity, is_recursive);

	return ret;
}

int media_svc_insert_item_pass1(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path, int is_burst)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};
	media_svc_media_type_e media_type;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set basic media info*/
	ret = _media_svc_set_media_info(&content_info, storage_id, storage_type, path, &media_type, FALSE);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("_media_svc_set_media_info fail %d", ret);
		return ret;
	}

	//media_svc_debug("total %d, cur %d insert flag %d", g_media_svc_insert_item_data_cnt, 	g_media_svc_insert_item_cur_data_cnt, g_insert_with_noti);

	/*Set or Get folder id*/
	ret = _media_svc_get_and_append_folder_id_by_path(db_handle, storage_id, path, storage_type, folder_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info.folder_uuid, folder_uuid);
	media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

	if (g_media_svc_insert_item_data_cnt == 1) {

		ret = _media_svc_insert_item_pass1(db_handle, storage_id, &content_info, is_burst, FALSE);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		if (g_insert_with_noti)
		{
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt++);
		}
	} else if(g_media_svc_insert_item_cur_data_cnt  < (g_media_svc_insert_item_data_cnt  - 1)) {

		ret = _media_svc_insert_item_pass1(db_handle, storage_id, &content_info, is_burst, TRUE);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		if (g_insert_with_noti)
		{
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt);
		}
		media_svc_debug("g_media_svc_insert_item_cur_data_cnt %d", g_media_svc_insert_item_cur_data_cnt);
		g_media_svc_insert_item_cur_data_cnt ++;

	} else if (g_media_svc_insert_item_cur_data_cnt  == (g_media_svc_insert_item_data_cnt  - 1)) {

		ret = _media_svc_insert_item_pass1(db_handle, storage_id, &content_info, is_burst, TRUE);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		if (g_insert_with_noti)
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt);

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_INSERT_ITEM);
		media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);

		media_svc_debug("_media_svc_list_query_do over");

		if (g_insert_with_noti) {
			media_svc_debug("publish noti list %d", g_media_svc_insert_item_cur_data_cnt);
			_media_svc_publish_noti_list(g_media_svc_insert_item_cur_data_cnt + 1);
			_media_svc_destroy_noti_list(g_media_svc_insert_item_cur_data_cnt + 1);

			/* Recreate noti list */
			ret = _media_svc_create_noti_list(g_media_svc_insert_item_data_cnt);
			media_svc_retv_del_if(ret != MS_MEDIA_ERR_NONE, ret, &content_info);
		}

		g_media_svc_insert_item_cur_data_cnt = 0;

	} else {
		media_svc_error("Error in media_svc_insert_item_pass1");
		_media_svc_destroy_content_info(&content_info);
		return MS_MEDIA_ERR_INTERNAL;
	}

	_media_svc_destroy_content_info(&content_info);

	return MS_MEDIA_ERR_NONE;
}

int media_svc_insert_item_pass2(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, int scan_type, const char *extract_path, int is_burst)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	sqlite3_stmt *sql_stmt = NULL;
	media_svc_media_type_e media_type;
	media_svc_content_info_s content_info;
	GArray *db_data_array = NULL;
	media_svc_item_info_s *db_data = NULL;
	int idx = 0;
	char *sql = NULL;

	media_svc_debug_fenter();

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(__media_svc_check_storage(storage_type, FALSE) != TRUE, MS_MEDIA_ERR_INVALID_PARAMETER, "Invalid storage_type");

	if (scan_type == MS_MSG_DIRECTORY_SCANNING) {
		sql = sqlite3_mprintf("SELECT path, media_type FROM '%s' WHERE validity = 1 AND title IS NULL AND path LIKE '%q/%%'", storage_id, extract_path);
	} else if (scan_type == MS_MSG_DIRECTORY_SCANNING_NON_RECURSIVE) {
		char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0, };
		ret = _media_svc_get_folder_id_by_foldername(handle, storage_id, extract_path, folder_uuid);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

		sql = sqlite3_mprintf("SELECT path, media_type FROM '%s' WHERE validity = 1 AND title IS NULL AND folder_uuid = '%q'", storage_id, folder_uuid);
	} else { /*Storage Scanning*/
		sql = sqlite3_mprintf("SELECT path, media_type FROM '%s' WHERE validity = 1 AND title IS NULL", storage_id);
	}

	ret = _media_svc_sql_prepare_to_step_simple(handle, sql, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("error when get list. err = [%d]", ret);
		return ret;
	}

	db_data_array = g_array_new(FALSE, FALSE, sizeof(media_svc_item_info_s*));
	if (db_data_array == NULL) {
		media_svc_error("db_data_array is NULL. Out of memory");
		return MS_MEDIA_ERR_OUT_OF_MEMORY;
	}

	while (sqlite3_step(sql_stmt) == SQLITE_ROW) {
		db_data = NULL;
		db_data = malloc(sizeof(media_svc_item_info_s));
		if(db_data == NULL) {
			media_svc_error("Out of memory");
			continue;
		}

		if(STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 0)))
			db_data->path = strdup((const char *)sqlite3_column_text(sql_stmt, 0));

		db_data->media_type = (int)sqlite3_column_int(sql_stmt,1);

		g_array_append_val(db_data_array, db_data);
	}

	SQLITE3_FINALIZE(sql_stmt);

	while (db_data_array->len != 0) {
		db_data = NULL;
		db_data = g_array_index(db_data_array, media_svc_item_info_s*, 0);
		g_array_remove_index (db_data_array, 0);

		if ((db_data == NULL) ||(db_data->path == NULL)) {
			media_svc_error("invalid db data");
			continue;
		}

		media_type = db_data->media_type;
		media_svc_debug("path is %s, media type %d", db_data->path, media_type);
		memset(&content_info, 0, sizeof(media_svc_content_info_s));
		__media_svc_malloc_and_strncpy(&content_info.path, db_data->path);

		content_info.media_type = media_type;
		content_info.storage_type = storage_type;

		_media_svc_set_default_value(&content_info, FALSE);

		if (media_type == MEDIA_SVC_MEDIA_TYPE_OTHER) {
			/*Do nothing.*/
		} else if (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) {
			ret = _media_svc_extract_image_metadata(db_handle, &content_info);
		} else {
			ret = _media_svc_extract_media_metadata(db_handle, &content_info);
		}

		ret = _media_svc_insert_item_pass2(db_handle, storage_id, &content_info, is_burst, TRUE);

		_media_svc_destroy_content_info(&content_info);
		SAFE_FREE(db_data->path);
		SAFE_FREE(db_data);
		idx++;
	}

	if (idx > 0)
	{
		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_UPDATE_ITEM);
	}

	g_array_free(db_data_array, FALSE);
	db_data_array = NULL;

	media_svc_debug_fleave();

	return MS_MEDIA_ERR_NONE;
}

int media_svc_delete_invalid_folder_by_path(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int *delete_count)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");

	ret = _media_svc_delete_invalid_folder_by_path(db_handle, storage_id, folder_path, delete_count);

	return ret;
}

int media_svc_check_folder_exist_by_path(MediaSvcHandle *handle, const char *storage_id, const char *folder_path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	int count = -1;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(folder_path), MS_MEDIA_ERR_INVALID_PARAMETER, "Path is NULL");

	ret = _media_svc_count_folder_with_path(db_handle,  storage_id, folder_path, &count);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	if(count > 0) {
		media_svc_debug("item is exist in database");
		return MS_MEDIA_ERR_NONE;
	} else {
		media_svc_debug("item is not exist in database");
		return MS_MEDIA_ERR_DB_NO_RECORD;
	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_check_subfolder_by_path(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	int cnt = -1;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(storage_id), MS_MEDIA_ERR_INVALID_PARAMETER, "storage_id is NULL");
	media_svc_retvm_if(!STRING_VALID(folder_path), MS_MEDIA_ERR_INVALID_PARAMETER, "Path is NULL");

	*count = 0;
	ret = _media_svc_count_subfolder_with_path(db_handle,  storage_id, folder_path, &cnt);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);
	*count = cnt;

	if(cnt > 0) {
		media_svc_debug("item is exist in database");
		return MS_MEDIA_ERR_NONE;
	} else {
		media_svc_debug("item is not exist in database");
		return MS_MEDIA_ERR_DB_NO_RECORD;
	}

	return MS_MEDIA_ERR_NONE;
}

int media_svc_get_folder_id(MediaSvcHandle *handle, const char *storage_id, const char *path, char *folder_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");

	ret = _media_svc_get_folder_uuid(db_handle, storage_id, path, folder_id);

	return ret;
}

