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

#include "media-util-err.h"
#include "media-svc-debug.h"
#include "media-svc-env.h"
#include "media-svc-db-utils.h"
#include "media-svc-util.h"
#include "media-svc-storage.h"

int _media_svc_init_storage(sqlite3 *handle)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;
	sqlite3_stmt *sql_stmt = NULL;
	int storage_cnt = 0;

	/*Add Internal storage*/
	sql = sqlite3_mprintf("SELECT COUNT(*) FROM '%s' WHERE storage_uuid='%s' AND storage_name='%s'", MEDIA_SVC_DB_TABLE_STORAGE, MEDIA_SVC_DB_TABLE_MEDIA, MEDIA_SVC_DB_TABLE_MEDIA);
	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	storage_cnt = sqlite3_column_int(sql_stmt, 0);
	SQLITE3_FINALIZE(sql_stmt);

	if(storage_cnt == 0)
	{
		sql = sqlite3_mprintf("INSERT INTO %s (storage_uuid, storage_name, storage_path, storage_type) VALUES ('%s', '%s', '%s', 0);",
			MEDIA_SVC_DB_TABLE_STORAGE, MEDIA_SVC_DB_TABLE_MEDIA, MEDIA_SVC_DB_TABLE_MEDIA, MEDIA_ROOT_PATH_INTERNAL);

		ret = _media_svc_sql_query(handle, sql);
		sqlite3_free(sql);
		media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);
	}

	return ret;
}

int _media_svc_get_mmc_info(MediaSvcHandle *handle, char **storage_name, char **storage_path, int *validity, bool *info_exist)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	sql = sqlite3_mprintf("SELECT * FROM '%s' WHERE storage_uuid=%Q AND storage_name IS NOT '%s'", MEDIA_SVC_DB_TABLE_STORAGE, MEDIA_SVC_DB_TABLE_MEDIA, MEDIA_SVC_DB_TABLE_MEDIA);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		*storage_name = NULL;
		*storage_path = NULL;
		*validity = 0;
		*info_exist = FALSE;

		if (ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			*info_exist = FALSE;
		}

		return ret;
	}

	if(STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 1)))
		*storage_name = strdup((const char *)sqlite3_column_text(sql_stmt, 1));

	if(STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 2)))
		*storage_path = strdup((const char *)sqlite3_column_text(sql_stmt, 2));

	*validity = sqlite3_column_int(sql_stmt, 6);

	*info_exist = TRUE;

	SQLITE3_FINALIZE(sql_stmt);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_check_storage(sqlite3 *handle, const char *storage_id, const char *storage_name, char **storage_path, int *validity)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	*storage_path = NULL;
	*validity = 0;

	if(storage_name != NULL)
		sql = sqlite3_mprintf("SELECT * FROM '%s' WHERE storage_uuid=%Q AND storage_name=%Q", MEDIA_SVC_DB_TABLE_STORAGE, storage_id, storage_name);
	else
		sql = sqlite3_mprintf("SELECT * FROM '%s' WHERE storage_uuid=%Q", MEDIA_SVC_DB_TABLE_STORAGE, storage_id);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	if(STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 2)))
		*storage_path = strdup((const char *)sqlite3_column_text(sql_stmt, 2));

	*validity = sqlite3_column_int(sql_stmt, 6);

	SQLITE3_FINALIZE(sql_stmt);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_append_storage(sqlite3 *handle, const char *storage_id, const char *storage_name, const char *storage_path, const char *storage_account, media_svc_storage_type_e storage_type)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = sqlite3_mprintf("INSERT INTO %s (storage_uuid, storage_name, storage_path, storage_account, storage_type) values (%Q, %Q, %Q, %Q, %d); ",
							 MEDIA_SVC_DB_TABLE_STORAGE, storage_id, storage_name, storage_path, storage_account, storage_type);

	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);

	return ret;
}

int _media_svc_update_storage_path(sqlite3 *handle, const char *storage_id, const char *path)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;
	char *old_storage_path = NULL;
	int validity = 0;

	/*Get old path*/
	ret = _media_svc_check_storage(handle, storage_id, NULL, &old_storage_path, &validity);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*Storage table update*/
	sql = sqlite3_mprintf("UPDATE '%s' SET storage_path=%Q WHERE storage_uuid=%Q", MEDIA_SVC_DB_TABLE_STORAGE, path, storage_id);
	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*Folder table update*/
	sql = sqlite3_mprintf("UPDATE '%s' SET path=REPLACE(path, %Q, %Q) WHERE storage_uuid=%Q", MEDIA_SVC_DB_TABLE_FOLDER, old_storage_path, path, storage_id);
	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	/*Media table update*/
	sql = sqlite3_mprintf("UPDATE '%s' SET path=REPLACE(path, %Q, %Q)", storage_id, old_storage_path, path);
	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	return ret;
}

int _media_svc_delete_storage(sqlite3 *handle, const char *storage_id, const char *storage_name)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;

	if (storage_name != NULL)
			sql = sqlite3_mprintf("DELETE FROM '%s' WHERE storage_uuid=%Q AND storage_name=%Q", MEDIA_SVC_DB_TABLE_STORAGE, storage_id, storage_name);
	else if (storage_id != NULL)
		sql = sqlite3_mprintf("DELETE FROM '%s' WHERE storage_uuid=%Q", MEDIA_SVC_DB_TABLE_STORAGE, storage_id);

	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);

	return ret;
}

int _media_svc_update_storage_validity(sqlite3 *handle, const char *storage_id, int validity)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;

	if (storage_id == NULL) {
		sql = sqlite3_mprintf("UPDATE '%s' SET validity=%d WHERE storage_uuid != 'media' AND storage_type != %d", MEDIA_SVC_DB_TABLE_STORAGE, validity, MEDIA_SVC_STORAGE_CLOUD);
	} else {
		sql = sqlite3_mprintf("UPDATE '%s' SET validity=%d WHERE storage_uuid=%Q;", MEDIA_SVC_DB_TABLE_STORAGE, validity, storage_id);
	}

	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);

	return ret;
}

int _media_svc_get_storage_uuid(sqlite3 *handle, const char *path, char *storage_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;
	char *storage_path = NULL;
	char *remain_path = NULL;
	int remain_len = 0;

	if(strncmp(path, MEDIA_ROOT_PATH_INTERNAL, strlen(MEDIA_ROOT_PATH_INTERNAL)) == 0)
	{
		_strncpy_safe(storage_id, MEDIA_SVC_DB_TABLE_MEDIA, MEDIA_SVC_UUID_SIZE+1);
		return MS_MEDIA_ERR_NONE;
	}

	sql = sqlite3_mprintf("SELECT storage_uuid, storage_path FROM '%s' WHERE validity=1", MEDIA_SVC_DB_TABLE_STORAGE);

	ret = _media_svc_sql_prepare_to_step_simple(handle, sql, &sql_stmt);

	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	remain_path = strstr(path+strlen(MEDIA_ROOT_PATH_USB) +1, "/");
	if (remain_path != NULL)
		remain_len = strlen(remain_path);

	storage_path = strndup(path, strlen(path) - remain_len);

	while(sqlite3_step(sql_stmt) == SQLITE_ROW)
	{
		if(STRING_VALID((const char *)sqlite3_column_text(sql_stmt, 1)))
		{
			if (strlen(storage_path)  == strlen((const char *)sqlite3_column_text(sql_stmt, 1)))
			{
				if(strncmp(storage_path, (const char *)sqlite3_column_text(sql_stmt, 1), strlen(storage_path)) == 0)
				{
					_strncpy_safe(storage_id, (const char *)sqlite3_column_text(sql_stmt, 0), MEDIA_SVC_UUID_SIZE+1);
					break;
				}
			}
		}

	}

	SQLITE3_FINALIZE(sql_stmt);
	SAFE_FREE(storage_path);

	if(!STRING_VALID(storage_id))
	{
		media_svc_error("Not found valid storage id [%s]", path);
		ret = MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	return ret;
}

int _media_svc_get_storage_type(sqlite3 *handle, const char *storage_id, media_svc_storage_type_e *storage_type)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	if(!STRING_VALID(storage_id))
	{
		media_svc_error("Invalid storage_idid");
		ret = MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	sql = sqlite3_mprintf("SELECT storage_type FROM '%s' WHERE (storage_uuid=%Q AND validity=1)", MEDIA_SVC_DB_TABLE_STORAGE, storage_id);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			media_svc_debug("there is no storage.");
		}
		else {
			media_svc_error("error when _media_svc_get_storage_type. err = [%d]", ret);
		}
		return ret;
	}

	*storage_type = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}

int _media_svc_get_storage_path(sqlite3 *handle, const char *storage_id, char **storage_path)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	if(!STRING_VALID(storage_id))
	{
		media_svc_error("Invalid storage_idid");
		ret = MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	sql = sqlite3_mprintf("SELECT storage_path FROM '%s' WHERE (storage_uuid=%Q AND validity=1)", MEDIA_SVC_DB_TABLE_STORAGE, storage_id);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			media_svc_debug("there is no storage.");
		}
		else {
			media_svc_error("error when _media_svc_get_storage_type. err = [%d]", ret);
		}
		return ret;
	}

	*storage_path = strdup((char *)sqlite3_column_text(sql_stmt, 0));

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}

int _media_svc_get_storage_scan_status(sqlite3 *handle, const char *storage_id, media_svc_scan_status_type_e *scan_status)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	if(!STRING_VALID(storage_id))
	{
		media_svc_error("Invalid storage_id");
		return MS_MEDIA_ERR_INVALID_PARAMETER;
	}

	sql = sqlite3_mprintf("SELECT scan_status FROM '%s' WHERE (storage_uuid=%Q AND validity=1)", MEDIA_SVC_DB_TABLE_STORAGE, storage_id);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			media_svc_debug("there is no storage.");
		}
		else {
			media_svc_error("error when _media_svc_get_storage_scan_status. err = [%d]", ret);
		}
		return ret;
	}

	*scan_status = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}

int _media_svc_set_storage_scan_status(sqlite3 *handle, const char *storage_id, media_svc_scan_status_type_e scan_status)
{
	int ret = MS_MEDIA_ERR_NONE;
	char *sql = NULL;

	if (storage_id == NULL) {
		sql = sqlite3_mprintf("UPDATE '%s' SET scan_status=%d WHERE storage_uuid != 'media'", MEDIA_SVC_DB_TABLE_STORAGE, scan_status);
	} else {
		sql = sqlite3_mprintf("UPDATE '%s' SET scan_status=%d WHERE storage_uuid=%Q", MEDIA_SVC_DB_TABLE_STORAGE, scan_status, storage_id);
	}

	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);

	return ret;
}

static int __media_svc_count_all_storage(sqlite3 *handle, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = sqlite3_mprintf("SELECT count(*) FROM '%s' WHERE validity = 1", MEDIA_SVC_DB_TABLE_STORAGE);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("error when _media_svc_sql_prepare_to_step. err = [%d]", ret);
		return ret;
	}

	*count = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_get_all_storage(sqlite3 *handle, char ***storage_list, char ***storage_id_list, int **scan_status_list, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	int idx = 0;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;
	int cnt =0;

	ret  = __media_svc_count_all_storage(handle, &cnt);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("error when __media_svc_count_all_folders. err = [%d]", ret);
		return ret;
	}

	if (cnt > 0) {
		sql = sqlite3_mprintf("SELECT storage_path, storage_uuid, scan_status FROM '%s' WHERE validity = 1", MEDIA_SVC_DB_TABLE_STORAGE);
	} else {
		*storage_list = NULL;
		*scan_status_list = NULL;
		return MS_MEDIA_ERR_NONE;
	}

	*storage_list = malloc(sizeof(char *) * cnt);
	*storage_id_list = malloc(sizeof(char *) * cnt);
	*scan_status_list = malloc(sizeof(int) * cnt);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);
	if (ret != MS_MEDIA_ERR_NONE) {
		media_svc_error("prepare error [%s]", sqlite3_errmsg(handle));
		SAFE_FREE(*storage_list);
		SAFE_FREE(*scan_status_list);
		return ret;
	}

	media_svc_debug("QEURY OK");

	while (1) {
		(*storage_list)[idx] = strdup((char *)sqlite3_column_text(sql_stmt, 0));
		(*storage_id_list)[idx] = strdup((char *)sqlite3_column_text(sql_stmt, 1));
		(*scan_status_list)[idx] = (int)sqlite3_column_int(sql_stmt, 2);
		if(sqlite3_step(sql_stmt) != SQLITE_ROW)
			break;
		idx++;
	}

	if (cnt == idx + 1) {
		*count = cnt;
		media_svc_debug("OK");
	} else {
		/* free all data */
		int i =0;
		for (i  = 0; i < idx; i ++) {
			SAFE_FREE((*storage_list)[i]);
			SAFE_FREE((*storage_id_list)[i]);
		}
		SAFE_FREE(*storage_list);
		SAFE_FREE(*storage_id_list);
		SAFE_FREE(*scan_status_list);
		*count = 0;
		ret = MS_MEDIA_ERR_INTERNAL;
	}

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}
