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
#include <media-util-err.h>
#include "media-svc-media-folder.h"
#include "media-svc-debug.h"
#include "media-svc-env.h"
#include "media-svc-util.h"
#include "media-svc-db-utils.h"

extern __thread GList *g_media_svc_move_item_query_list;
static __thread GList *g_media_svc_insert_folder_query_list;

static int __media_svc_is_root_path(const char *folder_path, bool *is_root)
{
	media_svc_retvm_if(!STRING_VALID(folder_path), MS_MEDIA_ERR_INVALID_PARAMETER, "folder_path is NULL");

	*is_root = FALSE;

	if (strcmp(folder_path, MEDIA_ROOT_PATH_INTERNAL) == 0 ||
		strcmp(folder_path, MEDIA_ROOT_PATH_SDCARD) == 0 ||
		strcmp(folder_path, MEDIA_ROOT_PATH_CLOUD) == 0) {
		media_svc_debug("ROOT PATH [%s]", folder_path);
		*is_root = TRUE;
	}

	return MS_MEDIA_ERR_NONE;
}

static int __media_svc_parent_is_ext_root_path(const char *folder_path, bool *is_root)
{
	char *parent_folder_path = NULL;

	media_svc_retvm_if(!STRING_VALID(folder_path), MS_MEDIA_ERR_INVALID_PARAMETER, "folder_path is NULL");

	*is_root = FALSE;

	parent_folder_path = g_path_get_dirname(folder_path);

	if(!STRING_VALID(parent_folder_path)) {
		media_svc_error("error : g_path_get_dirname falied");
		SAFE_FREE(parent_folder_path);
		return MS_MEDIA_ERR_OUT_OF_MEMORY;
	}

	if (strcmp(parent_folder_path, MEDIA_ROOT_PATH_EXTERNAL) == 0) {
		media_svc_debug("parent folder is ROOT PATH [%s]", parent_folder_path);
		*is_root = TRUE;
	}

	SAFE_FREE(parent_folder_path);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_get_folder_id_by_foldername(sqlite3 *handle, const char *storage_id, const char *folder_name, char *folder_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;
	char parent_folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0, };
	char *temp_parent_uuid = NULL;
	char *parent_folder_path = NULL;

	sql = sqlite3_mprintf("SELECT folder_uuid, parent_folder_uuid  FROM '%s' WHERE storage_uuid = '%q' AND path = '%q';", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, folder_name);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			media_svc_debug("there is no folder.");
		}
		else {
			media_svc_error("error when _media_svc_get_folder_id_by_foldername. err = [%d]", ret);
		}
		return ret;
	}

	memset(parent_folder_uuid, 0, sizeof(parent_folder_uuid));

	_strncpy_safe(folder_id, (const char *)sqlite3_column_text(sql_stmt, 0), MEDIA_SVC_UUID_SIZE+1);
	if (STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 1)))	/*root path can be null*/
		_strncpy_safe(parent_folder_uuid, (const char *)sqlite3_column_text(sql_stmt, 1), MEDIA_SVC_UUID_SIZE+1);

	SQLITE3_FINALIZE(sql_stmt);

	/*root path can be null*/
	if(!STRING_VALID(parent_folder_uuid)) {
		bool is_root = FALSE;

		ret = __media_svc_is_root_path(folder_name, &is_root);
		if (is_root)
			return MS_MEDIA_ERR_NONE;

		ret = __media_svc_parent_is_ext_root_path(folder_name, &is_root);
		if (is_root)
			return MS_MEDIA_ERR_NONE;
	}

	/* Notice : Below code is the code only for inserting parent folder uuid when upgrade the media db version 3 to 4  */
	/* Check parent folder uuid */
	if(!STRING_VALID(parent_folder_uuid)) {
		/* update parent_uuid */
		media_svc_error("[No-Error] there is no parent folder uuid. PLEASE CHECK IT");

		parent_folder_path = g_path_get_dirname(folder_name);
		if(!STRING_VALID(parent_folder_path)) {
			media_svc_error("error : g_path_get_dirname falied.");
			return MS_MEDIA_ERR_OUT_OF_MEMORY;
		}

		if(STRING_VALID(storage_id))
			sql = sqlite3_mprintf("SELECT folder_uuid  FROM '%s' WHERE storage_uuid = '%q' AND path = '%q';", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, parent_folder_path);
		else
			sql = sqlite3_mprintf("SELECT folder_uuid  FROM '%s' WHERE path = '%q';", MEDIA_SVC_DB_TABLE_FOLDER, parent_folder_path);

		SAFE_FREE(parent_folder_path);

		ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
		if (ret == MS_MEDIA_ERR_NONE) {
			temp_parent_uuid =  (char *)sqlite3_column_text(sql_stmt, 0);
			if (temp_parent_uuid != NULL) {
				_strncpy_safe(parent_folder_uuid, temp_parent_uuid, MEDIA_SVC_UUID_SIZE+1);
			}

			SQLITE3_FINALIZE(sql_stmt);
		}

		if(STRING_VALID(parent_folder_uuid)) {
			if(STRING_VALID(storage_id))
				sql = sqlite3_mprintf("UPDATE %q SET parent_folder_uuid  = '%q' WHERE storage_uuid = '%q' AND path = '%q';", MEDIA_SVC_DB_TABLE_FOLDER, parent_folder_uuid, storage_id, folder_name);
			else
				sql = sqlite3_mprintf("UPDATE %q SET parent_folder_uuid  = '%q' WHERE path = '%q';", MEDIA_SVC_DB_TABLE_FOLDER, parent_folder_uuid, folder_name);
			ret = _media_svc_sql_query(handle, sql);
			sqlite3_free(sql);
		} else {
			media_svc_error("error when get parent folder uuid");
		}
	}

	return ret;
}

static int __media_svc_append_folder(sqlite3 *handle, const char *storage_id, media_svc_storage_type_e storage_type,
				    const char *folder_id, const char *folder_path, const char *parent_folder_uuid, bool stack_query)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *folder_name = NULL;
	int folder_modified_date = 0;

	folder_name = g_path_get_basename(folder_path);
	folder_modified_date = _media_svc_get_file_time(folder_path);

	/*Update Pinyin If Support Pinyin*/
	char *folder_name_pinyin = NULL;
	if(_media_svc_check_pinyin_support())
		_media_svc_get_pinyin_str(folder_name, &folder_name_pinyin);

	char *sql = sqlite3_mprintf("INSERT INTO %s (folder_uuid, path, name, storage_uuid, storage_type, modified_time, name_pinyin, parent_folder_uuid) \
							values (%Q, %Q, %Q, %Q, '%d', '%d', %Q, %Q); ",
						     MEDIA_SVC_DB_TABLE_FOLDER, folder_id, folder_path, folder_name, storage_id, storage_type, folder_modified_date, folder_name_pinyin, parent_folder_uuid);

	if(!stack_query)
	{
		ret = _media_svc_sql_query(handle, sql);
		sqlite3_free(sql);
	}
	else
	{
		_media_svc_sql_query_add(&g_media_svc_insert_folder_query_list, &sql);
	}

	SAFE_FREE(folder_name);
	SAFE_FREE(folder_name_pinyin);

	return ret;
}

int _media_svc_update_folder_modified_time_by_folder_uuid(sqlite3 *handle, const char *folder_uuid, const char *folder_path, bool stack_query)
{
	int ret = MS_MEDIA_ERR_NONE;
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

static int __media_svc_get_and_append_parent_folder(sqlite3 *handle, const char *storage_id, const char *path, media_svc_storage_type_e storage_type, char *folder_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	unsigned int next_pos = 0;
	char *next = NULL;
	char *dir_path = NULL;
	const char *token = "/";
	char *folder_uuid = NULL;
	char parent_folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0, };
	bool folder_search_end = FALSE;

	memset(parent_folder_uuid, 0, sizeof(parent_folder_uuid));

	if(strncmp(path, MEDIA_ROOT_PATH_INTERNAL, strlen(MEDIA_ROOT_PATH_INTERNAL)) == 0)
		next_pos = strlen(MEDIA_ROOT_PATH_INTERNAL);
	else if (strncmp(path, MEDIA_ROOT_PATH_SDCARD, strlen(MEDIA_ROOT_PATH_SDCARD)) == 0)
		next_pos = strlen(MEDIA_ROOT_PATH_SDCARD);
	else if (strncmp(path, MEDIA_ROOT_PATH_CLOUD, strlen(MEDIA_ROOT_PATH_CLOUD)) == 0)
		next_pos = strlen(MEDIA_ROOT_PATH_CLOUD);
	else if (strncmp(path, MEDIA_ROOT_PATH_EXTERNAL, strlen(MEDIA_ROOT_PATH_EXTERNAL)) == 0)
		next_pos = strlen(MEDIA_ROOT_PATH_EXTERNAL);
	else
	{
		media_svc_error("Invalid Path");
		return MS_MEDIA_ERR_INTERNAL;
	}

	while (!folder_search_end)
	{
		next = strstr(path + next_pos , token);
		if (next != NULL)
		{
			next_pos = (next - path);
			dir_path = strndup(path, next_pos);
			next_pos++;
		}
		else
		{
			dir_path = strndup(path, strlen(path));
			folder_search_end = TRUE;
			media_svc_error("[No-Error] End Path [%s]", dir_path);
		}

		if (strcmp(dir_path, MEDIA_ROOT_PATH_EXTERNAL) == 0) {
			/*To avoid insert MEDIA_ROOT_PATH_EXTERNAL path*/
			continue;
		}

		ret = _media_svc_get_folder_id_by_foldername(handle, storage_id, dir_path, parent_folder_uuid);
		if (ret == MS_MEDIA_ERR_DB_NO_RECORD)
		{
			folder_uuid = _media_info_generate_uuid();
			if(folder_uuid == NULL )
			{
				media_svc_error("Invalid UUID");
				SAFE_FREE(dir_path);
				return MS_MEDIA_ERR_INTERNAL;
			}

			ret = __media_svc_append_folder(handle, storage_id, storage_type, folder_uuid, dir_path, parent_folder_uuid, FALSE);
			if (ret != MS_MEDIA_ERR_NONE) {
				media_svc_error("__media_svc_append_folder is failed");
			}

			media_svc_error("[No-Error] New Appended folder path [%s], folder_uuid [%s], parent_folder_uuid [%s]", dir_path, folder_uuid, parent_folder_uuid);
			_strncpy_safe(parent_folder_uuid, folder_uuid, MEDIA_SVC_UUID_SIZE+1);
		}
		else
		{
			media_svc_error("EXIST dir path : %s\n", dir_path);
		}

		SAFE_FREE(dir_path);
	}

	if(STRING_VALID(folder_uuid)) {
		_strncpy_safe(folder_id, folder_uuid, MEDIA_SVC_UUID_SIZE+1);
	} else {
		media_svc_error("Fail to get folder_uuid");
		return MS_MEDIA_ERR_INTERNAL;
	}

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_get_and_append_folder(sqlite3 *handle, const char *storage_id, const char *path, media_svc_storage_type_e storage_type, char *folder_id)
{
	int ret = MS_MEDIA_ERR_NONE;

	ret = _media_svc_get_folder_id_by_foldername(handle, storage_id, path, folder_id);

	if (ret == MS_MEDIA_ERR_DB_NO_RECORD)
	{
		ret = __media_svc_get_and_append_parent_folder(handle, storage_id, path, storage_type, folder_id);
	}

	return ret;
}

int _media_svc_get_and_append_folder_id_by_path(sqlite3 *handle, const char *storage_id, const char *path, media_svc_storage_type_e storage_type, char *folder_id)
{
	char *dir_path = NULL;
	int ret = MS_MEDIA_ERR_NONE;

	dir_path = g_path_get_dirname(path);

	ret =  _media_svc_get_and_append_folder(handle, storage_id, dir_path, storage_type, folder_id);

	SAFE_FREE(dir_path);

	return ret;
}

int _media_svc_update_folder_table(sqlite3 *handle, const char *storage_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;

	sql = sqlite3_mprintf("DELETE FROM '%s' WHERE folder_uuid IN (SELECT folder_uuid FROM '%s' WHERE folder_uuid NOT IN (SELECT folder_uuid FROM '%s'))",
	     MEDIA_SVC_DB_TABLE_FOLDER, MEDIA_SVC_DB_TABLE_FOLDER, storage_id);

	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);

	return ret;
}

static int __media_svc_count_all_folders(sqlite3 *handle, char* start_path, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = sqlite3_mprintf("SELECT count(*) FROM '%s' WHERE path LIKE '%q%%'", MEDIA_SVC_DB_TABLE_FOLDER, start_path);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("error when _media_svc_sql_prepare_to_step. err = [%d]", ret);
		return ret;
	}

	*count = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_get_all_folders(sqlite3 *handle, char *start_path, char ***folder_list, time_t **modified_time_list, int **item_num_list, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	int idx = 0;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;
	int cnt =0;
	char **folder_uuid = NULL;
	int i =0;

	ret  = __media_svc_count_all_folders(handle, start_path, &cnt);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("error when __media_svc_count_all_folders. err = [%d]", ret);
		return ret;
	}

	if (cnt > 0) {
		sql = sqlite3_mprintf("SELECT path, modified_time, folder_uuid FROM '%s' WHERE path LIKE '%q%%'", MEDIA_SVC_DB_TABLE_FOLDER, start_path);
	} else {
		*folder_list = NULL;
		*modified_time_list = NULL;
		*item_num_list = NULL;
		return MS_MEDIA_ERR_NONE;
	}

	*folder_list = malloc(sizeof(char *) * cnt);
	*modified_time_list = malloc(sizeof(int) * cnt);
	*item_num_list = malloc(sizeof(int) * cnt);
	folder_uuid = malloc(sizeof(char *) * cnt);

	if((*folder_list == NULL) || (*modified_time_list == NULL) || (*item_num_list == NULL) ||(folder_uuid == NULL)) {
		media_svc_error("Out of memory");
		goto ERROR;
	}
	memset(folder_uuid, 0x0, sizeof(char *) * cnt);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
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
			if (ret != MS_MEDIA_ERR_NONE) {
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
		ret = MS_MEDIA_ERR_INTERNAL;
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

int _media_svc_get_folder_info_by_foldername(sqlite3 *handle, const char *storage_id, const char *folder_name, char *folder_id, time_t *modified_time)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;

	char *sql = sqlite3_mprintf("SELECT folder_uuid, modified_time FROM %s WHERE (storage_uuid = '%q' AND path = '%q');", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, folder_name);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
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

int _media_svc_get_and_append_folder_id_by_folder_path(sqlite3 *handle, const char *storage_id, const char *path, media_svc_storage_type_e storage_type, char *folder_id, bool stack_query)
{
	char *path_name = NULL;
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;

	path_name = strdup(path);
	if (path_name == NULL) {
		media_svc_error("out of memory");
		return MS_MEDIA_ERR_OUT_OF_MEMORY;
	}

	ret = _media_svc_get_folder_id_by_foldername(handle, storage_id, path_name, folder_id);
	if (ret == MS_MEDIA_ERR_DB_NO_RECORD) {
		bool is_root = FALSE;
		bool is_parent_root = FALSE;

		ret = __media_svc_is_root_path(path_name, &is_root);
		ret = __media_svc_parent_is_ext_root_path(path_name, &is_parent_root);

		char parent_folder_uuid[MEDIA_SVC_UUID_SIZE+1] = {0, };
		memset(parent_folder_uuid, 0x00, sizeof(parent_folder_uuid));

		if ((is_root == FALSE) && (is_parent_root == FALSE)) {
			/*get parent folder id*/
			char *parent_path_name = g_path_get_dirname(path_name);

			ret = _media_svc_get_folder_id_by_foldername(handle, storage_id, parent_path_name, parent_folder_uuid);
			if (ret != MS_MEDIA_ERR_NONE) {
				if (ret == MS_MEDIA_ERR_DB_NO_RECORD) {
				/*if No-Root directory scanning start before doing storage scanning, parent_folder_uuid can be NULL.
				You can remove above logic if use below logic always. but I keep above code for the performance issue.
				Most case (except No-Root directory scanning start before doing storage scanning) get parent_folder_uuid well*/
				media_svc_error("[No-Error] There is no proper parent_folder_uuid. so try to make it");
				ret = _media_svc_get_and_append_folder(handle, storage_id, path, storage_type, folder_id);
				} else {
					media_svc_error("error when _media_svc_get_parent_folder_id_by_foldername. err = [%d]", ret);
				}

				SAFE_FREE(path_name);
				SAFE_FREE(parent_path_name);

				return ret;
			}
		}

		char *folder_uuid = _media_info_generate_uuid();
		if (folder_uuid == NULL) {
			media_svc_error("Invalid UUID");
			SAFE_FREE(path_name);
			return MS_MEDIA_ERR_INTERNAL;
		}

		ret = __media_svc_append_folder(handle, storage_id, storage_type, folder_uuid, path_name, parent_folder_uuid, stack_query);

		_strncpy_safe(folder_id, folder_uuid, MEDIA_SVC_UUID_SIZE+1);

	} else {
		sql = sqlite3_mprintf("UPDATE '%s' SET validity=1 WHERE storage_uuid = '%q' AND path = '%q'", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, path);

		if(!stack_query)
		{
			ret = _media_svc_sql_query(handle, sql);
			sqlite3_free(sql);
		}
		else
		{
			_media_svc_sql_query_add(&g_media_svc_insert_folder_query_list, &sql);
		}
	}

	SAFE_FREE(path_name);

	return ret;
}

int _media_svc_delete_invalid_folder(sqlite3 *handle, const char *storage_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;

	sql = sqlite3_mprintf("DELETE FROM '%s' WHERE storage_uuid = '%q' AND validity = 0", MEDIA_SVC_DB_TABLE_FOLDER, storage_id);
	ret = _media_svc_sql_query(handle, sql);

	sqlite3_free(sql);

	return ret;
}

int _media_svc_set_folder_validity(sqlite3 *handle, const char *storage_id, const char *start_path, int validity, bool is_recursive)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;
	char start_path_id[MEDIA_SVC_UUID_SIZE+1] = {0,};

	if (is_recursive) {
		ret = _media_svc_get_folder_id_by_foldername(handle, storage_id, start_path, start_path_id);
		media_svc_retvm_if(ret != MS_MEDIA_ERR_NONE, ret, "_media_svc_get_folder_id_by_foldername fail");		
		media_svc_retvm_if(!STRING_VALID(start_path_id), MS_MEDIA_ERR_INVALID_PARAMETER, "start_path_id is NULL");

		sql = sqlite3_mprintf("UPDATE '%s' SET validity = %d WHERE storage_uuid = '%q' AND (parent_folder_uuid = '%q' OR folder_uuid ='%q')",
						MEDIA_SVC_DB_TABLE_FOLDER, validity, storage_id, start_path_id, start_path_id);
	} else {
		sql = sqlite3_mprintf("UPDATE '%s' SET validity = %d WHERE storage_uuid = '%q' AND path = '%q'",
						MEDIA_SVC_DB_TABLE_FOLDER, validity, storage_id, start_path);
	}

	ret = _media_svc_sql_query(handle, sql);

	sqlite3_free(sql);

	return ret;
}

int _media_svc_delete_folder_by_storage_id(sqlite3 *handle, const char *storage_id, media_svc_storage_type_e storage_type)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;

	sql = sqlite3_mprintf("DELETE FROM '%s' WHERE storage_uuid = '%q' AND storage_type = %d", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, storage_type);
	ret = _media_svc_sql_query(handle, sql);

	sqlite3_free(sql);

	return ret;
}

GList ** _media_svc_get_folder_list_ptr(void)
{
	return &g_media_svc_insert_folder_query_list;
}

int _media_svc_delete_invalid_folder_by_path(sqlite3 *handle, const char *storage_id, const char *folder_path, int *delete_count)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;
	int del_count = 0;
	sqlite3_stmt *sql_stmt = NULL;

	if (folder_path == NULL)
		return MS_MEDIA_ERR_INVALID_PARAMETER;

	/*check the number of the deleted folder*/
	sql = sqlite3_mprintf("SELECT count(*) FROM '%s' WHERE (storage_uuid = '%q' AND validity = 0 AND PATH LIKE '%q/%%')", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, folder_path);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	del_count = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);
	sql = NULL;

	/*delete invalid folder*/
	sql = sqlite3_mprintf("DELETE FROM '%s' WHERE (storage_uuid = '%q' AND validity = 0 AND PATH LIKE '%q%%')", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, folder_path);
	ret = _media_svc_sql_query(handle, sql);

	sqlite3_free(sql);

	*delete_count = del_count;

	return ret;
}

int _media_svc_count_folder_with_path(sqlite3 *handle,  const char *storage_id, const char *path, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;

	char *sql = sqlite3_mprintf("SELECT count(*) FROM '%s' WHERE (storage_uuid='%q' AND path='%q')", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, path);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	*count = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_count_subfolder_with_path(sqlite3 *handle,  const char *storage_id, const char *path, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;

	char *sql = sqlite3_mprintf("SELECT count(*) FROM '%s' WHERE (storage_uuid='%q' AND path LIKE'%q/%%')", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, path);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	*count = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_get_folder_uuid(sqlite3 *handle, const char *storage_id, const char *path, char *folder_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	sql = sqlite3_mprintf("SELECT folder_uuid FROM '%s' WHERE (storage_uuid='%q' AND path='%q')", MEDIA_SVC_DB_TABLE_FOLDER, storage_id, path);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	_strncpy_safe(folder_id, (const char *)sqlite3_column_text(sql_stmt, 0), MEDIA_SVC_UUID_SIZE+1);

	SQLITE3_FINALIZE(sql_stmt);

	if(!STRING_VALID(folder_id))
	{
		media_svc_error("Not found valid storage id [%s]", path);
		ret = MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	return ret;
}

