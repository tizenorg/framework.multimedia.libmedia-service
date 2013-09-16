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
#include <drm_client.h>
#include <media-util.h>
#include "media-svc.h"
#include "media-svc-media.h"
#include "media-svc-debug.h"
#include "media-svc-util.h"
#include "media-svc-db-utils.h"
#include "media-svc-media-folder.h"
#include "media-svc-album.h"
#include "media-svc-noti.h"

static __thread int g_media_svc_item_validity_data_cnt = 1;
static __thread int g_media_svc_item_validity_cur_data_cnt = 0;

static __thread int g_media_svc_move_item_data_cnt = 1;
static __thread int g_media_svc_move_item_cur_data_cnt = 0;

static __thread int g_media_svc_insert_item_data_cnt = 1;
static __thread int g_media_svc_insert_item_cur_data_cnt = 0;

/* Flag for items to be published by notification */
static __thread int g_insert_with_noti = FALSE;

int media_svc_connect(MediaSvcHandle **handle)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	MediaDBHandle *db_handle = NULL;

	media_svc_debug_func();

#if 1	//Use libmedia_utils API
	ret = media_db_connect(&db_handle);
	if(ret != MS_MEDIA_ERR_NONE)
		return _media_svc_error_convert(ret);

#else
	sqlite3 * db_handle = NULL;

	ret = _media_svc_connect_db_with_handle(&db_handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);
#endif

	*handle = db_handle;
	return MEDIA_INFO_ERROR_NONE;

}

int media_svc_disconnect(MediaSvcHandle *handle)
{
	MediaDBHandle * db_handle = (MediaDBHandle *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

#if 1	//Use libmedia_utils API
	int ret = MEDIA_INFO_ERROR_NONE;

	ret = media_db_disconnect(db_handle);
	return _media_svc_error_convert(ret);
#else
	sqlite3 * db_handle = (sqlite3 *)handle;

	return _media_svc_disconnect_db_with_handle(db_handle);
#endif
}

int media_svc_create_table(MediaSvcHandle *handle)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

	/*create media table*/
	ret = _media_svc_create_media_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/*create folder table*/
	ret = _media_svc_create_folder_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/*create playlist table*/
	ret = _media_svc_create_playlist_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/* create album table*/
	ret = _media_svc_create_album_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/*create tag table*/
	ret = _media_svc_create_tag_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/*create bookmark table*/
	ret = _media_svc_create_bookmark_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

#if 0	/*unify media and custom table*/
	/*create custom table*/
	ret = _media_svc_create_custom_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);
#endif

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_get_storage_type(const char *path, media_svc_storage_type_e *storage_type)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	media_svc_storage_type_e type;

	ret = _media_svc_get_store_type_by_path(path, &type);
	media_svc_retvm_if(ret < MEDIA_INFO_ERROR_NONE, ret, "_media_svc_get_store_type_by_path failed : %d", ret);

	*storage_type = type;

	return ret;
}

int media_svc_get_mime_type(const char *path, char *mimetype)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	drm_bool_type_e is_drm = DRM_FALSE;
	drm_content_info_s *drm_contentInfo = NULL;

	ret = _media_svc_get_mime_type(path, mimetype, &is_drm, &drm_contentInfo);

	SAFE_FREE(drm_contentInfo);

	return ret;
}

int media_svc_get_media_type(const char *path, const char *mime_type, media_svc_media_type_e *media_type)
{
	int ret = MEDIA_INFO_ERROR_NONE;

	ret = _media_svc_get_media_type(path, mime_type, media_type);

	return ret;
}

int media_svc_get_file_info(MediaSvcHandle *handle, const char *path, time_t *modified_time, unsigned long long *size)
{
	int ret = MEDIA_INFO_ERROR_NONE;

	ret = _media_svc_get_fileinfo_by_path(handle, path, modified_time, size);

	return ret;
}

int media_svc_check_item_exist_by_path(MediaSvcHandle *handle, const char *path)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	int count = -1;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "Path is NULL");

	ret = _media_svc_count_record_with_path(db_handle, path, &count);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	if(count > 0) {
		media_svc_debug("item is exist in database");
		return MEDIA_INFO_ERROR_NONE;
	} else {
		media_svc_debug("item is not exist in database");
		return MEDIA_INFO_ERROR_DATABASE_NO_RECORD;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_insert_item_begin(MediaSvcHandle *handle, int data_cnt, int with_noti, int from_pid)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("Transaction data count : [%d]", data_cnt);

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(data_cnt < 1, MEDIA_INFO_ERROR_INVALID_PARAMETER, "data_cnt shuld be bigger than 1");

	g_media_svc_insert_item_data_cnt  = data_cnt;
	g_media_svc_insert_item_cur_data_cnt  = 0;

	/* Prepare for making noti item list */
	if (with_noti) {
		media_svc_debug("making noti list from pid[%d]", from_pid);
		if (_media_svc_create_noti_list(data_cnt) != MEDIA_INFO_ERROR_NONE) {
			return MEDIA_INFO_ERROR_OUT_OF_MEMORY;
		}

		_media_svc_set_noti_from_pid(from_pid);
		g_insert_with_noti = TRUE;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_insert_item_end(MediaSvcHandle *handle)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

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

int media_svc_insert_item_bulk(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path, int is_burst)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};
	media_svc_media_type_e media_type;
	drm_content_info_s *drm_contentInfo = NULL;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "path is NULL");

	if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL) && (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("storage type is incorrect[%d]", storage_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set media info*/
	/* if drm_contentinfo is not NULL, the file is OMA DRM.*/
	ret = _media_svc_set_media_info(&content_info, storage_type, path, &media_type, FALSE, &drm_contentInfo);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	if(media_type == MEDIA_SVC_MEDIA_TYPE_OTHER) {
		/*Do nothing.*/
	} else if(media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) {
		ret = _media_svc_extract_image_metadata(&content_info, media_type);
	} else {
		ret = _media_svc_extract_media_metadata(handle, &content_info, media_type, drm_contentInfo);
	}
	SAFE_FREE(drm_contentInfo);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/*Set or Get folder id*/
	ret = _media_svc_get_and_append_folder_id_by_path(handle, path, storage_type, folder_uuid);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	ret = __media_svc_malloc_and_strncpy(&content_info.folder_uuid, folder_uuid);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, &content_info);

	if (g_media_svc_insert_item_data_cnt == 1) {

		ret = _media_svc_insert_item_with_data(db_handle, &content_info, is_burst, FALSE);
		media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, &content_info);

		if (g_insert_with_noti)
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt++);

	} else if(g_media_svc_insert_item_cur_data_cnt  < (g_media_svc_insert_item_data_cnt  - 1)) {

		ret = _media_svc_insert_item_with_data(db_handle, &content_info, is_burst, TRUE);
		media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, &content_info);

		if (g_insert_with_noti)
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt);

		g_media_svc_insert_item_cur_data_cnt ++;

	} else if (g_media_svc_insert_item_cur_data_cnt  == (g_media_svc_insert_item_data_cnt  - 1)) {

		ret = _media_svc_insert_item_with_data(db_handle, &content_info, is_burst, TRUE);
		media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, &content_info);

		if (g_insert_with_noti)
			_media_svc_insert_item_to_noti_list(&content_info, g_media_svc_insert_item_cur_data_cnt);

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_INSERT_ITEM);
		media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, &content_info);

		if (g_insert_with_noti) {
			_media_svc_publish_noti_list(g_media_svc_insert_item_cur_data_cnt + 1);
			_media_svc_destroy_noti_list(g_media_svc_insert_item_cur_data_cnt + 1);

			/* Recreate noti list */
			if (_media_svc_create_noti_list(g_media_svc_insert_item_data_cnt) != MEDIA_INFO_ERROR_NONE) {
				return MEDIA_INFO_ERROR_OUT_OF_MEMORY;
			}
		}

		g_media_svc_insert_item_cur_data_cnt = 0;

	} else {
		media_svc_error("Error in media_svc_insert_item_bulk");
		_media_svc_destroy_content_info(&content_info);
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	_media_svc_destroy_content_info(&content_info);

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_insert_item_immediately(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};
	media_svc_media_type_e media_type;
	drm_content_info_s *drm_contentInfo = NULL;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "path is NULL");

	if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL) && (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("storage type is incorrect[%d]", storage_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set media info*/
	ret = _media_svc_set_media_info(&content_info, storage_type, path, &media_type, FALSE, &drm_contentInfo);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	if(media_type == MEDIA_SVC_MEDIA_TYPE_OTHER) {
		/*Do nothing.*/
	} else if(media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) {
		ret = _media_svc_extract_image_metadata(&content_info, media_type);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);
	} else {
		ret = _media_svc_extract_media_metadata(handle, &content_info, media_type, drm_contentInfo);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);
	}

	/*Set or Get folder id*/
	ret = _media_svc_get_and_append_folder_id_by_path(handle, path, storage_type, folder_uuid);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	ret = __media_svc_malloc_and_strncpy(&content_info.folder_uuid, folder_uuid);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, &content_info);
#if 1
	/* Extracting thumbnail */
	if (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
		char thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0, };
		int width = 0;
		int height = 0;

		ret = thumbnail_request_from_db_with_size(content_info.path, thumb_path, sizeof(thumb_path), &width, &height);
		if (ret < 0) {
			media_svc_error("thumbnail_request_from_db failed: %d", ret);
		} else {
			media_svc_debug("thumbnail_request_from_db success: %s", thumb_path);
			ret = __media_svc_malloc_and_strncpy(&(content_info.thumbnail_path), thumb_path);
			if (ret < 0) {
				media_svc_error("__media_svc_malloc_and_strncpy failed : %d", ret);
			}
		}

		if (content_info.media_meta.width <= 0)
			content_info.media_meta.width = width;

		if (content_info.media_meta.height <= 0)
			content_info.media_meta.height = height;
	}
#endif
	ret = _media_svc_insert_item_with_data(db_handle, &content_info, FALSE, FALSE);

	if (ret == MEDIA_INFO_ERROR_NONE) {
		media_svc_debug("Insertion is successful. Sending noti for this");
		_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_INSERT, content_info.path, content_info.media_type, content_info.media_uuid, content_info.mime_type);
	}

	_media_svc_destroy_content_info(&content_info);
	return ret;
}

int media_svc_insert_folder(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "path is NULL");

	if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL) && (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("storage type is incorrect[%d]", storage_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	media_svc_debug("storage[%d], folder_path[%s]", storage_type, path);

	/*Get folder info*/
	char *folder_name = NULL;
	int folder_modified_date = 0;
	char *folder_uuid = _media_info_generate_uuid();
	if(folder_uuid == NULL ) {
		media_svc_error("Invalid UUID");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	folder_name = g_path_get_basename(path);
	folder_modified_date = _media_svc_get_file_time(path);

	ret = _media_svc_append_folder(handle, storage_type, folder_uuid, path, folder_name, folder_modified_date);
	SAFE_FREE(folder_name);

	if (ret < MEDIA_INFO_ERROR_NONE) {
		media_svc_error("_media_svc_append_folder error [%d]", ret);
		return ret;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_move_item_begin(MediaSvcHandle *handle, int data_cnt)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("Transaction data count : [%d]", data_cnt);

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(data_cnt < 1, MEDIA_INFO_ERROR_INVALID_PARAMETER, "data_cnt shuld be bigger than 1");

	g_media_svc_move_item_data_cnt  = data_cnt;
	g_media_svc_move_item_cur_data_cnt  = 0;

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_move_item_end(MediaSvcHandle *handle)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

	if (g_media_svc_move_item_cur_data_cnt  > 0) {

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_MOVE_ITEM);
	}

	/*clean up old folder path*/
	ret = _media_svc_update_folder_table(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	g_media_svc_move_item_data_cnt  = 1;
	g_media_svc_move_item_cur_data_cnt  = 0;

	return ret;
}

int media_svc_move_item(MediaSvcHandle *handle, media_svc_storage_type_e src_storage, const char *src_path,
			media_svc_storage_type_e dest_storage, const char *dest_path)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char *file_name = NULL;
	char *folder_path = NULL;
	int modified_time = 0;
	char folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0,};
	char old_thumb_path[MEDIA_SVC_PATHNAME_SIZE] = {0,};
	char new_thumb_path[MEDIA_SVC_PATHNAME_SIZE] = {0,};
	int media_type = -1;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(src_path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "src_path is NULL");
	media_svc_retvm_if(!STRING_VALID(dest_path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "dest_path is NULL");

	if ((src_storage != MEDIA_SVC_STORAGE_INTERNAL) && (src_storage != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("src_storage type is incorrect[%d]", src_storage);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}
	if ((dest_storage != MEDIA_SVC_STORAGE_INTERNAL) && (dest_storage != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("dest_storage type is incorrect[%d]", dest_storage);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	/*check and update folder*/
	ret = _media_svc_get_and_append_folder_id_by_path(handle, dest_path, dest_storage, folder_uuid);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/*get filename*/
	file_name = g_path_get_basename(dest_path);

	/*get modified_time*/
	modified_time = _media_svc_get_file_time(dest_path);

	/*get thumbnail_path to update. only for Imgae and Video items. Audio share album_art(thumbnail)*/
	ret = _media_svc_get_media_type_by_path(handle, src_path, &media_type);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
		/*get old thumbnail_path*/
		ret = _media_svc_get_thumbnail_path_by_path(handle, src_path, old_thumb_path);
		media_svc_retv_if((ret != MEDIA_INFO_ERROR_NONE) && (ret != MEDIA_INFO_ERROR_DATABASE_NO_RECORD), ret);

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
			ret = _media_svc_update_item_by_path(handle, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, new_thumb_path, FALSE);
		} else {
			ret = _media_svc_update_item_by_path(handle, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, NULL, FALSE);
		}
		SAFE_FREE(file_name);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		media_svc_debug("Move is successful. Sending noti for this");

		/* Get notification info */
		media_svc_noti_item *noti_item = NULL;
		ret = _media_svc_get_noti_info(handle, dest_path, MS_MEDIA_ITEM_FILE, &noti_item);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		/* Send notification for move */
		_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_UPDATE, src_path, media_type, noti_item->media_uuid, noti_item->mime_type);
		_media_svc_destroy_noti_item(noti_item);

		/*update folder modified_time*/
		folder_path = g_path_get_dirname(dest_path);
		ret = _media_svc_update_folder_modified_time_by_folder_uuid(handle, folder_uuid, folder_path, FALSE);
		SAFE_FREE(folder_path);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		ret = _media_svc_update_folder_table(handle);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	} else if (g_media_svc_move_item_cur_data_cnt  < (g_media_svc_move_item_data_cnt  - 1)) {

		/*update item*/
		if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
			ret = _media_svc_update_item_by_path(handle, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, new_thumb_path, TRUE);
		} else {
			ret = _media_svc_update_item_by_path(handle, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, NULL, TRUE);
		}
		SAFE_FREE(file_name);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		/*update folder modified_time*/
		folder_path = g_path_get_dirname(dest_path);
		ret = _media_svc_update_folder_modified_time_by_folder_uuid(handle, folder_uuid, folder_path, TRUE);
		SAFE_FREE(folder_path);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		g_media_svc_move_item_cur_data_cnt ++;

	} else if (g_media_svc_move_item_cur_data_cnt  == (g_media_svc_move_item_data_cnt  - 1)) {

		/*update item*/
		if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
			ret = _media_svc_update_item_by_path(handle, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, new_thumb_path, TRUE);
		} else {
			ret = _media_svc_update_item_by_path(handle, src_path, dest_storage, dest_path, file_name, modified_time, folder_uuid, NULL, TRUE);
		}
		SAFE_FREE(file_name);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		/*update folder modified_time*/
		folder_path = g_path_get_dirname(dest_path);
		ret = _media_svc_update_folder_modified_time_by_folder_uuid(handle, folder_uuid, folder_path, TRUE);
		SAFE_FREE(folder_path);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		/*update db*/
		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_MOVE_ITEM);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		g_media_svc_move_item_cur_data_cnt = 0;

	} else {
		media_svc_error("Error in media_svc_move_item");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	/*rename thumbnail file*/
	if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
		if((strlen(old_thumb_path) > 0) && (strncmp(old_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
			ret = _media_svc_rename_file(old_thumb_path,new_thumb_path);
			if (ret < 0)
				media_svc_error("_media_svc_rename_file failed : %d", ret);
		}
	}

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_set_item_validity_begin(MediaSvcHandle *handle, int data_cnt)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug("Transaction data count : [%d]", data_cnt);

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(data_cnt < 1, MEDIA_INFO_ERROR_INVALID_PARAMETER, "data_cnt shuld be bigger than 1");

	g_media_svc_item_validity_data_cnt  = data_cnt;
	g_media_svc_item_validity_cur_data_cnt  = 0;

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_set_item_validity_end(MediaSvcHandle *handle)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

	if (g_media_svc_item_validity_cur_data_cnt  > 0) {

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_SET_ITEM_VALIDITY);
	}

	g_media_svc_item_validity_data_cnt  = 1;
	g_media_svc_item_validity_cur_data_cnt  = 0;

	return ret;
}

int media_svc_set_item_validity(MediaSvcHandle *handle, const char *path, int validity)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "path is NULL");

	media_svc_debug("path=[%s], validity=[%d]", path, validity);

	if (g_media_svc_item_validity_data_cnt  == 1) {

		return _media_svc_update_item_validity(db_handle, path, validity, FALSE);

	} else if (g_media_svc_item_validity_cur_data_cnt  < (g_media_svc_item_validity_data_cnt  - 1)) {

		ret = _media_svc_update_item_validity(db_handle, path, validity, TRUE);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		g_media_svc_item_validity_cur_data_cnt ++;

	} else if (g_media_svc_item_validity_cur_data_cnt  == (g_media_svc_item_validity_data_cnt  - 1)) {

		ret = _media_svc_update_item_validity(db_handle, path, validity, TRUE);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		ret = _media_svc_list_query_do(db_handle, MEDIA_SVC_QUERY_SET_ITEM_VALIDITY);
		media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

		g_media_svc_item_validity_cur_data_cnt  = 0;

	} else {

		media_svc_error("Error in media_svc_set_item_validity");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_delete_item_by_path(MediaSvcHandle *handle, const char *path)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char thumb_path[MEDIA_SVC_PATHNAME_SIZE] = {0,};

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "path is NULL");

	int media_type = -1;
	ret = _media_svc_get_media_type_by_path(db_handle, path, &media_type);
	media_svc_retv_if((ret != MEDIA_INFO_ERROR_NONE), ret);

	if((media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) ||(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)) {
		/*Get thumbnail path to delete*/
		ret = _media_svc_get_thumbnail_path_by_path(db_handle, path, thumb_path);
		media_svc_retv_if((ret != MEDIA_INFO_ERROR_NONE) && (ret != MEDIA_INFO_ERROR_DATABASE_NO_RECORD), ret);
	} else if ((media_type == MEDIA_SVC_MEDIA_TYPE_SOUND) ||(media_type == MEDIA_SVC_MEDIA_TYPE_MUSIC)) {
		int count = 0;
		ret = _media_svc_get_media_count_with_album_id_by_path(db_handle, path, &count);
		media_svc_retv_if((ret != MEDIA_INFO_ERROR_NONE), ret);

		if (count == 1) {
			/*Get thumbnail path to delete*/
			ret = _media_svc_get_thumbnail_path_by_path(db_handle, path, thumb_path);
			media_svc_retv_if((ret != MEDIA_INFO_ERROR_NONE) && (ret != MEDIA_INFO_ERROR_DATABASE_NO_RECORD), ret);
		}
	}

	/* Get notification info */
	media_svc_noti_item *noti_item = NULL;
	ret = _media_svc_get_noti_info(handle, path, MS_MEDIA_ITEM_FILE, &noti_item);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/*Delete item*/
	ret = _media_svc_delete_item_by_path(db_handle, path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("_media_svc_delete_item_by_path failed : %d", ret);
		_media_svc_destroy_noti_item(noti_item);

		return ret;
	}

	/* Send notification */
	media_svc_debug("Deletion is successful. Sending noti for this");
	_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_DELETE, path, media_type, noti_item->media_uuid, noti_item->mime_type);
	_media_svc_destroy_noti_item(noti_item);

	/*Delete thumbnail*/
	if ((strlen(thumb_path) > 0) && (strncmp(thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
		int thumb_count = 1;
		/* Get count of media, which contains same thumbnail for music */
		if ((media_type == MEDIA_SVC_MEDIA_TYPE_SOUND) ||(media_type == MEDIA_SVC_MEDIA_TYPE_MUSIC)) {
			ret = _media_svc_get_thumbnail_count(db_handle, thumb_path, &thumb_count);
			if (ret < MEDIA_INFO_ERROR_NONE) {
				media_svc_error("Failed to get thumbnail count : %d", ret);
			}
		}

		if (thumb_count == 1) {
			if (_media_svc_remove_file(thumb_path) == FALSE) {
				media_svc_error("fail to remove thumbnail file.");
			}
		}
	}

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_delete_all_items_in_storage(MediaSvcHandle *handle, media_svc_storage_type_e storage_type)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	char * dirpath = NULL;

	media_svc_debug("media_svc_delete_all_items_in_storage [%d]", storage_type);

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

	if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL) && (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("storage type is incorrect[%d]", storage_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	ret = _media_svc_truncate_table(db_handle, storage_type);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	dirpath = (storage_type == MEDIA_SVC_STORAGE_INTERNAL) ? MEDIA_SVC_THUMB_INTERNAL_PATH : MEDIA_SVC_THUMB_EXTERNAL_PATH;

	/* remove thumbnails */
	ret = _media_svc_remove_all_files_in_dir(dirpath);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_delete_invalid_items_in_storage(MediaSvcHandle *handle, media_svc_storage_type_e storage_type)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

	if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL) && (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("storage type is incorrect[%d]", storage_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	/*Delete from DB and remove thumbnail files*/
	return _media_svc_delete_invalid_items(db_handle, storage_type);
}

int media_svc_delete_invalid_items_in_folder(MediaSvcHandle *handle, const char *folder_path)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

	/*Delete from DB and remove thumbnail files*/
	return _media_svc_delete_invalid_folder_items(db_handle, folder_path);
}

int media_svc_set_all_storage_items_validity(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, int validity)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");

	if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL) && (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("storage type is incorrect[%d]", storage_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	return _media_svc_update_storage_item_validity(db_handle, storage_type, validity);
}

int media_svc_set_folder_items_validity(MediaSvcHandle *handle, const char *folder_path, int validity, int recursive)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(folder_path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "folder_path is NULL");

	if(recursive)
		return _media_svc_update_recursive_folder_item_validity(db_handle, folder_path, validity);
	else
		return _media_svc_update_folder_item_validity(db_handle, folder_path, validity);
}

int media_svc_refresh_item(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;
	media_svc_media_type_e media_type;
	drm_content_info_s *drm_contentInfo = NULL;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(!STRING_VALID(path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "path is NULL");

	if ((storage_type != MEDIA_SVC_STORAGE_INTERNAL) && (storage_type != MEDIA_SVC_STORAGE_EXTERNAL)) {
		media_svc_error("storage type is incorrect[%d]", storage_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}


	media_svc_content_info_s content_info;
	memset(&content_info, 0, sizeof(media_svc_content_info_s));

	/*Set media info*/
	ret = _media_svc_set_media_info(&content_info, storage_type, path, &media_type, TRUE, &drm_contentInfo);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/* Initialize thumbnail information to remake thumbnail. */
	char thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1];
	ret = _media_svc_get_thumbnail_path_by_path(handle, path, thumb_path);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	if (g_file_test(thumb_path, G_FILE_TEST_EXISTS) && (strncmp(thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
		ret = _media_svc_remove_file(thumb_path);
		if (ret < MEDIA_INFO_ERROR_NONE) {
			media_svc_error("_media_svc_remove_file failed : %s", thumb_path);
		}
	}

	ret = _media_svc_update_thumbnail_path(handle, path, NULL);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/* Get notification info */
	media_svc_noti_item *noti_item = NULL;
	ret = _media_svc_get_noti_info(handle, path, MS_MEDIA_ITEM_FILE, &noti_item);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	media_type = noti_item->media_type;

	if(media_type == MEDIA_SVC_MEDIA_TYPE_OTHER) {
		/*Do nothing.*/
	} else if(media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE) {
		ret = _media_svc_extract_image_metadata(&content_info, media_type);
		if (ret != MEDIA_INFO_ERROR_NONE) {
			_media_svc_destroy_noti_item(noti_item);
			return ret;
		}
	} else {
		ret = _media_svc_extract_media_metadata(handle, &content_info, media_type, drm_contentInfo);
		if (ret != MEDIA_INFO_ERROR_NONE) {
			_media_svc_destroy_noti_item(noti_item);
			return ret;
		}
	}
#if 1
	/* Extracting thumbnail */
	if (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
		char thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0, };
		int width = 0;
		int height = 0;

		ret = thumbnail_request_from_db_with_size(content_info.path, thumb_path, sizeof(thumb_path), &width, &height);
		if (ret < 0) {
			media_svc_error("thumbnail_request_from_db failed: %d", ret);
		} else {
			media_svc_debug("thumbnail_request_from_db success: %s", thumb_path);
			ret = __media_svc_malloc_and_strncpy(&(content_info.thumbnail_path), thumb_path);
			if (ret < 0) {
				media_svc_error("__media_svc_malloc_and_strncpy failed : %d", ret);
			}
		}

		if (content_info.media_meta.width <= 0)
			content_info.media_meta.width = width;

		if (content_info.media_meta.height <= 0)
			content_info.media_meta.height = height;
	}
#endif
	ret = _media_svc_update_item_with_data(db_handle, &content_info);

	if (ret == MEDIA_INFO_ERROR_NONE) {
		media_svc_debug("Update is successful. Sending noti for this");
		_media_svc_publish_noti(MS_MEDIA_ITEM_FILE, MS_MEDIA_ITEM_UPDATE, content_info.path, noti_item->media_type, noti_item->media_uuid, noti_item->mime_type);
	} else {
		media_svc_error("_media_svc_update_item_with_data failed : %d", ret);
	}

	_media_svc_destroy_content_info(&content_info);
	_media_svc_destroy_noti_item(noti_item);

	return ret;
}

int media_svc_rename_folder(MediaSvcHandle *handle, const char *src_path, const char *dst_path)
{
	sqlite3 * db_handle = (sqlite3 *)handle;
	int ret = MEDIA_INFO_ERROR_NONE;

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(src_path == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "src_path is NULL");
	media_svc_retvm_if(dst_path == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "dst_path is NULL");

	media_svc_debug("Src path : %s,  Dst Path : %s", src_path, dst_path);

	/* Get notification info */
	media_svc_noti_item *noti_item = NULL;
	ret = _media_svc_get_noti_info(handle, src_path, MS_MEDIA_ITEM_DIRECTORY, &noti_item);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	ret = _media_svc_sql_begin_trans(handle);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	/* Update all folder record's path, which are matched by old parent path */
	char *update_folder_path_sql = NULL;
	char src_path_slash[MEDIA_SVC_PATHNAME_SIZE + 1];
	char dst_path_slash[MEDIA_SVC_PATHNAME_SIZE + 1];

	snprintf(src_path_slash, sizeof(src_path_slash), "%s/", src_path);
	snprintf(dst_path_slash, sizeof(dst_path_slash), "%s/", dst_path);

	update_folder_path_sql = sqlite3_mprintf("UPDATE folder SET path = REPLACE( path, '%q', '%q');", src_path_slash, dst_path_slash);

	//ret = _media_svc_sql_query(handle, update_folder_path_sql);
	ret = media_db_request_update_db_batch(update_folder_path_sql);
	sqlite3_free(update_folder_path_sql);

	if (ret != SQLITE_OK) {
		media_svc_error("failed to update folder path");
		_media_svc_sql_rollback_trans(handle);

		return MEDIA_INFO_ERROR_DATABASE_INTERNAL;
	}

	/* Update all folder record's modified date, which are changed above */
	char *update_folder_modified_time_sql = NULL;
	time_t date;
	time(&date);

	update_folder_modified_time_sql = sqlite3_mprintf("UPDATE folder SET modified_time = %d where path like '%q';", date, dst_path);

	ret = media_db_request_update_db_batch(update_folder_modified_time_sql);
	//ret = _media_svc_sql_query(handle, update_folder_modified_time_sql);
	sqlite3_free(update_folder_modified_time_sql);

	if (ret != SQLITE_OK) {
		media_svc_error("failed to update folder modified time");
		_media_svc_sql_rollback_trans(handle);

		return MEDIA_INFO_ERROR_DATABASE_INTERNAL;
	}

	/* Update all items */
	char *select_all_sql = NULL;
	sqlite3_stmt *sql_stmt = NULL;
	char dst_child_path[MEDIA_SVC_PATHNAME_SIZE + 1];

	snprintf(dst_child_path, sizeof(dst_child_path), "%s/%%", dst_path);

	select_all_sql = sqlite3_mprintf("SELECT media_uuid, path, thumbnail_path, media_type from media where folder_uuid IN ( SELECT folder_uuid FROM folder where path='%q' or path like '%q');", dst_path, dst_child_path);

	media_svc_debug("[SQL query] : %s", select_all_sql);

	ret = sqlite3_prepare_v2(handle, select_all_sql, -1, &sql_stmt, NULL);
	sqlite3_free((char *)select_all_sql);

	if (ret != SQLITE_OK) {
		media_svc_error ("prepare error [%s]", sqlite3_errmsg(handle));
		_media_svc_sql_rollback_trans(handle);
		return MEDIA_INFO_ERROR_DATABASE_INTERNAL;
	}

	while (1) {
		ret = sqlite3_step(sql_stmt);
		if (ret != SQLITE_ROW) {
			media_svc_debug("end of iteration");
			break;
		}

		char media_uuid[MEDIA_SVC_UUID_SIZE + 1] = {0,};
		char media_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0,};
		char media_thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0,};
		char media_new_thumb_path[MEDIA_SVC_PATHNAME_SIZE + 1] = {0,};
		int media_type;
		bool no_thumb = FALSE;

		if (STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 0))) {
			strncpy(media_uuid,	(const char *)sqlite3_column_text(sql_stmt, 0), sizeof(media_uuid));
			media_uuid[sizeof(media_uuid) - 1] = '\0';
		} else {
			media_svc_error("media UUID is NULL");
			return MEDIA_INFO_ERROR_DATABASE_INVALID;
		}

		if (STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 1))) {
			strncpy(media_path,	(const char *)sqlite3_column_text(sql_stmt, 1), sizeof(media_path));
			media_path[sizeof(media_path) - 1] = '\0';
		} else {
			media_svc_error("media path is NULL");
			return MEDIA_INFO_ERROR_DATABASE_INVALID;
		}

		if (STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 2))) {
			strncpy(media_thumb_path,	(const char *)sqlite3_column_text(sql_stmt, 2), sizeof(media_thumb_path));
			media_thumb_path[sizeof(media_thumb_path) - 1] = '\0';
		} else {
			media_svc_debug("media thumb path doesn't exist in DB");
			no_thumb = TRUE;
		}

		media_type = sqlite3_column_int(sql_stmt, 3);

		/* Update path, thumbnail path of this item */
		char *replaced_path = NULL;
		replaced_path = _media_svc_replace_path(media_path, src_path, dst_path);
		if (replaced_path == NULL) {
			media_svc_error("_media_svc_replace_path failed");
			SQLITE3_FINALIZE(sql_stmt);
			_media_svc_sql_rollback_trans(handle);
			return MEDIA_INFO_ERROR_INTERNAL;
		}

		media_svc_debug("New media path : %s", replaced_path);
		media_svc_storage_type_e storage_type;

		if (!no_thumb) {
			/* If old thumb path is default or not */
			if (strncmp(media_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) == 0) {
				strncpy(media_new_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(media_new_thumb_path));
			} else {
				ret = _media_svc_get_store_type_by_path(replaced_path, &storage_type);
				if (ret < 0) {
					media_svc_error("_media_svc_get_store_type_by_path failed : %d", ret);
					SAFE_FREE(replaced_path);
					_media_svc_sql_rollback_trans(handle);
					return MEDIA_INFO_ERROR_DATABASE_INTERNAL;
				}
	
				ret = _media_svc_get_thumbnail_path(storage_type, media_new_thumb_path, replaced_path, THUMB_EXT);
				if (ret < 0) {
					media_svc_error("_media_svc_get_thumbnail_path failed : %d", ret);
					SAFE_FREE(replaced_path);
					SQLITE3_FINALIZE(sql_stmt);
					_media_svc_sql_rollback_trans(handle);
					return MEDIA_INFO_ERROR_INTERNAL;
				}
			}

			//media_svc_debug("New media thumbnail path : %s", media_new_thumb_path);
		}

		char *update_item_sql = NULL;

		if (no_thumb) {
			update_item_sql = sqlite3_mprintf("UPDATE media SET path='%q' WHERE media_uuid='%q'", replaced_path, media_uuid);
		} else {
			if (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
				update_item_sql = sqlite3_mprintf("UPDATE media SET path='%q', thumbnail_path='%q' WHERE media_uuid='%q'", replaced_path, media_new_thumb_path, media_uuid);
			} else {
				update_item_sql = sqlite3_mprintf("UPDATE media SET path='%q', thumbnail_path='%q' WHERE media_uuid='%q'", replaced_path, media_thumb_path, media_uuid);
			}
		}

		ret = media_db_request_update_db_batch(update_item_sql);
		//ret = _media_svc_sql_query(handle, update_item_sql);
		sqlite3_free(update_item_sql);
		SAFE_FREE(replaced_path);

		if (ret != SQLITE_OK) {
			media_svc_error("failed to update item");
			SQLITE3_FINALIZE(sql_stmt);
			_media_svc_sql_rollback_trans(handle);
	
			return MEDIA_INFO_ERROR_DATABASE_INTERNAL;
		}

		/* Rename thumbnail file of file system */
		if ((!no_thumb) && (media_type == MEDIA_SVC_MEDIA_TYPE_IMAGE || media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)
				&& (strncmp(media_thumb_path, MEDIA_SVC_THUMB_DEFAULT_PATH, sizeof(MEDIA_SVC_THUMB_DEFAULT_PATH)) != 0)) {
			ret = _media_svc_rename_file(media_thumb_path, media_new_thumb_path);
			if (ret < 0) {
				media_svc_error("_media_svc_rename_file failed : %d", ret);
			}
		}
	}

	SQLITE3_FINALIZE(sql_stmt);

	ret = _media_svc_sql_end_trans(handle);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("mb_svc_sqlite3_commit_trans failed.. Now start to rollback\n");
		_media_svc_sql_rollback_trans(handle);
		return ret;
	}

	media_svc_debug("Folder update is successful. Sending noti for this");
	_media_svc_publish_noti(MS_MEDIA_ITEM_DIRECTORY, MS_MEDIA_ITEM_UPDATE, src_path, -1, noti_item->media_uuid, NULL);
	_media_svc_destroy_noti_item(noti_item);

	return MEDIA_INFO_ERROR_NONE;
}

int media_svc_request_update_db(const char *db_query)
{
	int ret = MEDIA_INFO_ERROR_NONE;

	media_svc_retvm_if(!STRING_VALID(db_query), MEDIA_INFO_ERROR_INVALID_PARAMETER, "db_query is NULL");

	ret = _media_svc_request_update_db(db_query);

	return _media_svc_error_convert(ret);
}

int media_svc_send_dir_update_noti(MediaSvcHandle *handle, const char *dir_path)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_retvm_if(!STRING_VALID(dir_path), MEDIA_INFO_ERROR_INVALID_PARAMETER, "dir_path is NULL");

	/* Get notification info */
	media_svc_noti_item *noti_item = NULL;
	ret = _media_svc_get_noti_info(db_handle, dir_path, MS_MEDIA_ITEM_DIRECTORY, &noti_item);
	media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

	ret = _media_svc_publish_noti(MS_MEDIA_ITEM_DIRECTORY, MS_MEDIA_ITEM_UPDATE, dir_path, -1, noti_item->media_uuid, NULL);
	_media_svc_destroy_noti_item(noti_item);

	return ret;
}

int media_svc_count_invalid_items_in_folder(MediaSvcHandle *handle, const char *folder_path, int *count)
{
	sqlite3 * db_handle = (sqlite3 *)handle;

	media_svc_debug_func();

	media_svc_retvm_if(db_handle == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "Handle is NULL");
	media_svc_retvm_if(folder_path == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "folder_path is NULL");
	media_svc_retvm_if(count == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "count is NULL");

	return _media_svc_count_invalid_folder_items(db_handle, folder_path, count);
}
