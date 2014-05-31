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

#include <glib/gstdio.h>
#include "media-svc-media-folder.h"
#include "media-svc-error.h"
#include "media-svc-debug.h"
#include "media-svc-env.h"
#include "media-svc-util.h"
#include "media-svc-db-utils.h"

extern __thread GList *g_media_svc_move_item_query_list;

int _media_svc_get_folder_id_by_foldername(sqlite3 *handle, const char *folder_name, char *folder_id)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3_stmt *sql_stmt = NULL;

	char *sql = sqlite3_mprintf("SELECT folder_uuid FROM %s WHERE path = '%q';", MEDIA_SVC_DB_TABLE_FOLDER, folder_name);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MEDIA_INFO_ERROR_NONE) {
		if(ret == MEDIA_INFO_ERROR_DATABASE_NO_RECORD) {
			media_svc_error("there is no folder.");
		}
		else {
			media_svc_error("error when _media_svc_get_folder_id_by_foldername. err = [%d]", ret);
		}
		return ret;
	}

	_strncpy_safe(folder_id, (const char *)sqlite3_column_text(sql_stmt, 0), MEDIA_SVC_UUID_SIZE+1);

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}

int _media_svc_append_folder(sqlite3 *handle, media_svc_storage_type_e storage_type,
				    const char *folder_id, const char *path_name, const char *folder_name, int modified_date)
{
	int ret = MEDIA_INFO_ERROR_NONE;

	/*Update Pinyin If Support Pinyin*/
	char *folder_name_pinyin = NULL;
	if(_media_svc_check_pinyin_support())
		_media_svc_get_pinyin_str(folder_name, &folder_name_pinyin);

	char *sql = sqlite3_mprintf("INSERT INTO %s (folder_uuid, path, name, storage_type, modified_time, name_pinyin) values (%Q, %Q, %Q, '%d', '%d', %Q); ",
					     MEDIA_SVC_DB_TABLE_FOLDER, folder_id, path_name, folder_name, storage_type, modified_date, folder_name_pinyin);
	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);

	SAFE_FREE(folder_name_pinyin);

	return ret;
}

int _media_svc_update_folder_modified_time_by_folder_uuid(sqlite3 *handle, const char *folder_uuid, const char *folder_path, bool stack_query)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	int modified_time = 0;

	modified_time = _media_svc_get_file_time(folder_path);

	char *sql = sqlite3_mprintf("UPDATE %s SET modified_time=%d WHERE folder_uuid=%Q;", MEDIA_SVC_DB_TABLE_FOLDER, modified_time, folder_uuid);

	if(!stack_query) {
		ret = _media_svc_sql_query(handle, sql);
		sqlite3_free(sql);
	} else {
		_media_svc_sql_query_add(&g_media_svc_move_item_query_list, &sql);
	}

	return ret;
}

int _media_svc_get_and_append_folder_id_by_path(sqlite3 *handle, const char *path, media_svc_storage_type_e storage_type, char *folder_id)
{
	char *path_name = NULL;
	int ret = MEDIA_INFO_ERROR_NONE;

	path_name = g_path_get_dirname(path);

	ret = _media_svc_get_folder_id_by_foldername(handle, path_name, folder_id);

	if(ret == MEDIA_INFO_ERROR_DATABASE_NO_RECORD) {
		char *folder_name = NULL;
		int folder_modified_date = 0;
		char *folder_uuid = _media_info_generate_uuid();
		if(folder_uuid == NULL ) {
			media_svc_error("Invalid UUID");
			SAFE_FREE(path_name);
			return MEDIA_INFO_ERROR_INTERNAL;
		}

		folder_name = g_path_get_basename(path_name);
		folder_modified_date = _media_svc_get_file_time(path_name);

		ret = _media_svc_append_folder(handle, storage_type, folder_uuid, path_name, folder_name, folder_modified_date);
		if (ret != MEDIA_INFO_ERROR_NONE) {
			media_svc_error("_media_svc_append_folder is failed");
		}
		SAFE_FREE(folder_name);
		_strncpy_safe(folder_id, folder_uuid, MEDIA_SVC_UUID_SIZE+1);
	}

	SAFE_FREE(path_name);

	return ret;
}

int _media_svc_update_folder_table(sqlite3 *handle)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	char *sql = NULL;

	sql = sqlite3_mprintf("DELETE FROM %s WHERE folder_uuid IN (SELECT folder_uuid FROM %s WHERE folder_uuid NOT IN (SELECT folder_uuid FROM %s))",
	     MEDIA_SVC_DB_TABLE_FOLDER, MEDIA_SVC_DB_TABLE_FOLDER, MEDIA_SVC_DB_TABLE_MEDIA);

	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);

	return ret;
}

static int __media_svc_count_all_folders(sqlite3 *handle, char* start_path, int *count)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = sqlite3_mprintf("SELECT count(*) FROM %s WHERE path LIKE '%q%%'", MEDIA_SVC_DB_TABLE_FOLDER, start_path);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("error when _media_svc_sql_prepare_to_step. err = [%d]", ret);
		return ret;
	}

	*count = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_get_all_folders(sqlite3 *handle, char *start_path, char ***folder_list, time_t **modified_time_list, int **item_num_list, int *count)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	int idx = 0;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;
	int cnt =0;
	char **folder_uuid = NULL;
	int i =0;

	ret  = __media_svc_count_all_folders(handle, start_path, &cnt);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("error when __media_svc_count_all_folders. err = [%d]", ret);
		return ret;
	}

	if (cnt > 0) {
		sql = sqlite3_mprintf("SELECT path, modified_time, folder_uuid FROM %s WHERE path LIKE '%q%%'", MEDIA_SVC_DB_TABLE_FOLDER, start_path);
	} else {
		*folder_list = NULL;
		*modified_time_list = NULL;
		*item_num_list = NULL;
		return MEDIA_INFO_ERROR_NONE;
	}

	*folder_list = malloc(sizeof(char *) * cnt);
	*modified_time_list = malloc(sizeof(int) * cnt);
	*item_num_list = malloc(sizeof(int) * cnt);
	folder_uuid = malloc(sizeof(char *) * cnt);
	memset(folder_uuid, 0x0, sizeof(char *) * cnt);

	if((*folder_list == NULL) || (*modified_time_list == NULL) || (*item_num_list == NULL) ||(folder_uuid == NULL)) {
		media_svc_error("Out of memory");
		goto ERROR;
	}

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("prepare error [%s]", sqlite3_errmsg(handle));
		goto ERROR;
	}

	media_svc_debug("QEURY OK");

	while (1) {
		if(STRING_VALID((char *)sqlite3_column_text(sql_stmt, 0)))
			(*folder_list)[idx] = strdup((char *)sqlite3_column_text(sql_stmt, 0));

		(*modified_time_list)[idx] = (int)sqlite3_column_int(sql_stmt, 1);

		/* get the folder's id */
		if(STRING_VALID((char *)sqlite3_column_text(sql_stmt, 2)))
			folder_uuid[idx] = strdup((char *)sqlite3_column_text(sql_stmt, 2));

		idx++;

		if(sqlite3_step(sql_stmt) != SQLITE_ROW)
			break;
	}
	SQLITE3_FINALIZE(sql_stmt);

	/*get the numbder of item in the folder by using folder's id */
	for (i = 0; i < idx; i ++) {
		if(STRING_VALID(folder_uuid[i])) {
			sql = sqlite3_mprintf("SELECT COUNT(*) FROM %s WHERE (folder_uuid='%q' AND validity = 1)", MEDIA_SVC_DB_TABLE_MEDIA, folder_uuid[i]);
			ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
			if (ret != MEDIA_INFO_ERROR_NONE) {
				media_svc_error("prepare error [%s]", sqlite3_errmsg(handle));
				goto ERROR;
			}

			(*item_num_list)[i] = (int)sqlite3_column_int(sql_stmt, 0);

			SQLITE3_FINALIZE(sql_stmt);
		} else
		{
			media_svc_error("Invalid Folder Id");
		}
	}

	if (cnt == idx) {
		*count = cnt;
		media_svc_debug("Get Folder is OK");
	} else {
		media_svc_error("Fail to get folder");
		ret = MEDIA_INFO_ERROR_INTERNAL;
		goto ERROR;
	}

	/* free all data */
	for (i  = 0; i < idx; i ++) {
		SAFE_FREE(folder_uuid[i]);
	}
	SAFE_FREE(folder_uuid);

	return ret;

ERROR:

	/* free all data */
	for (i  = 0; i < idx; i ++) {
		SAFE_FREE((*folder_list)[i]);
		SAFE_FREE(folder_uuid[i]);
	}
	SAFE_FREE(*folder_list);
	SAFE_FREE(*modified_time_list);
	SAFE_FREE(*item_num_list);
	SAFE_FREE(folder_uuid);

	*count = 0;

	return ret;
}

int _media_svc_get_folder_info_by_foldername(sqlite3 *handle, const char *folder_name, char *folder_id, time_t *modified_time)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3_stmt *sql_stmt = NULL;

	char *sql = sqlite3_mprintf("SELECT folder_uuid, modified_time FROM %s WHERE path = '%q';", MEDIA_SVC_DB_TABLE_FOLDER, folder_name);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MEDIA_INFO_ERROR_NONE) {
		if(ret == MEDIA_INFO_ERROR_DATABASE_NO_RECORD) {
			media_svc_debug("there is no folder.");
		}
		else {
			media_svc_error("error when _media_svc_get_folder_id_by_foldername. err = [%d]", ret);
		}
		return ret;
	}

	_strncpy_safe(folder_id, (const char *)sqlite3_column_text(sql_stmt, 0), MEDIA_SVC_UUID_SIZE+1);
	*modified_time = (int)sqlite3_column_int(sql_stmt, 1);

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}