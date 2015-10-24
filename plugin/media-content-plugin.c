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
#include <sys/stat.h>
#include <mm_file.h>
#include <media-thumbnail.h>
#include "media-svc.h"
#include "media-svc-util.h"

#define MEDIA_SVC_PLUGIN_ERROR_NONE		0
#define MEDIA_SVC_PLUGIN_ERROR			-1

#define STRING_VALID(str)	\
	((str != NULL && strlen(str) > 0) ? TRUE : FALSE)
#define STORAGE_VALID(storage)\
	(((storage == MEDIA_SVC_STORAGE_INTERNAL) || (storage == MEDIA_SVC_STORAGE_EXTERNAL) || (storage == MEDIA_SVC_STORAGE_EXTERNAL_USB)) ? TRUE : FALSE)


typedef enum{
	ERR_HANDLE = 1,
	ERR_FILE_PATH,
	ERR_FOLDER_PATH,
	ERR_MIME_TYPE,
	ERR_NOT_MEDIAFILE,
	ERR_STORAGE_TYPE,
	ERR_CHECK_ITEM,
	ERR_MAX,
}media_svc_error_type_e;

static void __set_error_message(int err_type, char ** err_msg);

static void __set_error_message(int err_type, char ** err_msg)
{
	if (err_msg)
		*err_msg = NULL;
	else
		return;

	if(err_type == ERR_HANDLE)
		*err_msg = strdup("invalid handle");
	else if(err_type == ERR_FILE_PATH)
		*err_msg = strdup("invalid file path");
	else if(err_type == ERR_FOLDER_PATH)
		*err_msg = strdup("invalid folder path");
	else if(err_type == ERR_MIME_TYPE)
		*err_msg = strdup("invalid mime type");
	else if(err_type == ERR_NOT_MEDIAFILE)
		*err_msg = strdup("not media content");
	else if(err_type == ERR_STORAGE_TYPE)
		*err_msg = strdup("invalid storage type");
	else if(err_type == ERR_CHECK_ITEM)
		*err_msg = strdup("item does not exist");
	else if(err_type == MS_MEDIA_ERR_DB_CONNECT_FAIL)
		*err_msg = strdup("DB connect error");
	else if(err_type == MS_MEDIA_ERR_DB_DISCONNECT_FAIL)
		*err_msg = strdup("DB disconnect error");
	else if(err_type == MS_MEDIA_ERR_INVALID_PARAMETER)
		*err_msg = strdup("invalid parameter");
	else if(err_type == MS_MEDIA_ERR_DB_INTERNAL)
		*err_msg = strdup("DB internal error");
	else if(err_type == MS_MEDIA_ERR_DB_NO_RECORD)
		*err_msg = strdup("not found in DB");
	else if(err_type == MS_MEDIA_ERR_INTERNAL)
		*err_msg = strdup("media service internal error");
	else if(err_type == MS_MEDIA_ERR_DB_CORRUPT)
		*err_msg = strdup("DB corrupt error");
	else
		*err_msg = strdup("error unknown");

	return;
}

int connect_db(void ** handle, char ** err_msg)
{
	int ret = media_svc_connect(handle);

	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int disconnect_db(void * handle, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_disconnect(handle);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int check_item_exist(void* handle, const char *storage_id, const char *file_path, bool *modified, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;
	*modified = TRUE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	time_t modified_time = 0;
	unsigned long long file_size = 0;
	struct stat st;

	ret = media_svc_get_file_info(handle, storage_id, file_path, &modified_time, &file_size);
	if(ret == MS_MEDIA_ERR_NONE) {
		memset(&st, 0, sizeof(struct stat));
		if (stat(file_path, &st) == 0) {
			if((st.st_mtime != modified_time) || (st.st_size != file_size))
				*modified = TRUE;
			else
				*modified = FALSE;
		}

		return MEDIA_SVC_PLUGIN_ERROR_NONE;	//exist
	}

	__set_error_message(ERR_CHECK_ITEM, err_msg);

	return MEDIA_SVC_PLUGIN_ERROR;		//not exist
}

int insert_item_begin(void * handle, int item_cnt, int with_noti, int from_pid, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_item_begin(handle, item_cnt, with_noti, from_pid);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_item_end(void * handle, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_item_end(handle);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_item(void * handle, const char *storage_id, const char *file_path, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_item_bulk(handle, storage_id, storage_type, file_path, FALSE);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_item_immediately(void * handle, const char *storage_id, const char *file_path, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_item_immediately(handle, storage_id, storage_type, file_path);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_burst_item(void * handle, const char *storage_id, const char *file_path, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_item_bulk(handle, storage_id, storage_type, file_path, TRUE);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_all_storage_items_validity(void * handle,  const char *storage_id, int storage_type, int validity, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_all_storage_items_validity(handle, storage_id, storage_type, validity);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_folder_item_validity(void * handle, const char *storage_id, const char * folder_path, int validity, int recursive, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(folder_path)) {
		__set_error_message(ERR_FOLDER_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_folder_items_validity(handle, storage_id, folder_path, validity, recursive);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_item_validity_begin(void * handle, int item_cnt, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_item_validity_begin(handle, item_cnt);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_item_validity_end(void * handle, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_item_validity_end(handle);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_item_validity(void * handle, const char *storage_id, const char *file_path, int storage_type, int validity, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_item_validity(handle, storage_id, file_path, validity);

	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int delete_item(void * handle, const char *storage_id, const char *file_path, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_check_item_exist_by_path(handle, storage_id, file_path);
	if(ret == 0) {
		ret = media_svc_delete_item_by_path(handle, storage_id, file_path);

		if(ret < 0) {
			__set_error_message(ret, err_msg);
			return MEDIA_SVC_PLUGIN_ERROR;
		}
		else
			return MEDIA_SVC_PLUGIN_ERROR_NONE;
	}

	__set_error_message(ERR_CHECK_ITEM, err_msg);	//not exist in DB so can't delete item.
	return MEDIA_SVC_PLUGIN_ERROR;
}

int delete_all_items_in_storage(void * handle, const char *storage_id, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_delete_all_items_in_storage(handle, storage_id, storage_type);
	if(ret < 0) {
			__set_error_message(ret, err_msg);
			return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int delete_all_invalid_items_in_storage(void * handle, const char *storage_id, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_delete_invalid_items_in_storage(handle, storage_id, storage_type);
	if(ret < 0) {
			__set_error_message(ret, err_msg);
			return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int delete_all_invalid_items_in_folder(void * handle, const char *storage_id, const char *folder_path, bool is_recursve, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(folder_path)) {
		__set_error_message(ERR_FOLDER_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_delete_invalid_items_in_folder(handle, storage_id, folder_path, is_recursve);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}


int update_begin(void)
{
	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int update_end(const char *start_path)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	ret = thumbnail_request_extract_all_thumbs(start_path);
	if (ret < 0) {
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int send_dir_update_noti(void * handle, const char *storage_id, const char *dir_path, const char *folder_id, int update_type, int pid, char **err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if (!STRING_VALID(dir_path)) {
		__set_error_message(ERR_FOLDER_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_send_dir_update_noti(handle, storage_id, dir_path, folder_id, (media_item_update_type_e)update_type, pid);
	if (ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int count_delete_items_in_folder(void * handle, const char *storage_id, const char *folder_path, int *count, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(count == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(folder_path)) {
		__set_error_message(ERR_FOLDER_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_count_invalid_items_in_folder(handle, storage_id, folder_path, count);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int check_db(void * handle, bool *need_full_scan, char **err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	/*check db schema*/
	ret = media_svc_create_table(handle);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	/*check db version*/
	ret = media_svc_check_db_upgrade(handle, need_full_scan);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int check_db_corrupt(void * handle, char **err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	/*check db version*/
	ret = media_svc_check_db_corrupt(handle);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int get_folder_list(void * handle, const char *storage_id, char* start_path, char ***folder_list, int **modified_time_list, int **item_num_list, int *count, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(count == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_get_folder_list(handle, start_path, folder_list, (time_t**)modified_time_list, item_num_list, count);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int update_folder_time(void * handle, const char *storage_id, char *folder_path, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(folder_path == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_update_folder_time(handle, storage_id, folder_path);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int get_uuid(void * handle, char **uuid, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_generate_uuid(uuid);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int get_mmc_info(void * handle, char **storage_name, char **storage_path, int *validity, bool *info_exist, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_get_mmc_info(handle, storage_name, storage_path, validity, info_exist);
	if(ret < 0) {
		__set_error_message(MS_MEDIA_ERR_DB_NO_RECORD, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int check_storage(void * handle, const char *storage_id, const char *storage_name, char **storage_path, int *validity, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_check_storage(handle, storage_id, storage_name, storage_path, validity);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_storage(void * handle, const char *storage_id, int storage_type, const char *storage_name, const char *storage_path, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_storage(handle, storage_id, storage_name, storage_path, NULL, storage_type);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int update_storage(void * handle, const char *storage_id, const char *storage_path, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_update_storage(handle, storage_id, storage_path);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int delete_storage(void * handle, const char *storage_id, const char *storage_name, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_delete_storage(handle, storage_id, storage_name);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_storage_validity(void * handle, const char *storage_id, int validity, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_storage_validity(handle, storage_id, validity);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_all_storage_validity(void * handle, int validity, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_storage_validity(handle, NULL, validity);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int get_storage_id(void * handle, const char *path, char *storage_id, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_get_storage_id(handle, path, storage_id);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int get_storage_scan_status(void * handle, const char *storage_id, int *status, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;
	media_svc_scan_status_type_e storage_status = 0;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_get_storage_scan_status(handle, storage_id, &storage_status);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	*status = storage_status;

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_storage_scan_status(void * handle, const char *storage_id, int status, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;
	media_svc_scan_status_type_e storage_status = status;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_storage_scan_status(handle, storage_id, storage_status);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int get_storage_list(void * handle, char ***storage_list, char ***storage_id_list,int **scan_status_list, int *count, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(count == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_get_storage_list(handle, storage_list, storage_id_list, scan_status_list, count);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int update_item_begin(void * handle, int item_cnt, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_update_item_begin(handle, item_cnt);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int update_item_end(void * handle, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_update_item_end(handle);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int update_item_meta(void * handle, const char *file_path, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(file_path == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_update_item_meta(handle, file_path, storage_type);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_item_scan(void * handle, const char *storage_id, const char *file_path, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_item_pass1(handle, storage_id, storage_type, file_path, FALSE);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int update_item_extract(void * handle, const char *storage_id, int storage_type, int scan_type, const char *path, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_item_pass2(handle, storage_id, storage_type, scan_type, path, FALSE);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_folder_begin(void * handle, int item_cnt, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_folder_begin(handle, item_cnt);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_folder_end(void * handle, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_folder_end(handle);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int insert_folder(void * handle, const char *storage_id, const char *file_path, int storage_type, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if (!STRING_VALID(file_path)) {
		__set_error_message(ERR_FILE_PATH, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	if(!STORAGE_VALID(storage_type)) {
		__set_error_message(ERR_STORAGE_TYPE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_insert_folder(handle, storage_id, storage_type, file_path);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int delete_invalid_folder(void * handle, const char *storage_id, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_delete_invalid_folder(handle, storage_id);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int set_folder_validity(void * handle, const char *storage_id, const char* start_path, int validity, bool is_recursive, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_set_folder_validity(handle, storage_id, start_path, validity, is_recursive);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int delete_invalid_folder_by_path(void * handle, const char *storage_id, const char *folder_path, int *delete_count, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_delete_invalid_folder_by_path(handle, storage_id, folder_path, delete_count);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int check_folder_exist(void * handle, const char *storage_id, const char *folder_path, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_check_folder_exist_by_path(handle, storage_id, folder_path);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int count_subfolder(void * handle, const char *storage_id, const char *folder_path, int *count, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;
	int cnt = 0;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_check_subfolder_by_path(handle, storage_id, folder_path, &cnt);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	*count = cnt;

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}

int get_folder_id(void * handle, const char *storage_id, const char *path, char *folder_id, char ** err_msg)
{
	int ret = MEDIA_SVC_PLUGIN_ERROR_NONE;

	if(handle == NULL) {
		__set_error_message(ERR_HANDLE, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	ret = media_svc_get_folder_id(handle, storage_id, path, folder_id);
	if(ret < 0) {
		__set_error_message(ret, err_msg);
		return MEDIA_SVC_PLUGIN_ERROR;
	}

	return MEDIA_SVC_PLUGIN_ERROR_NONE;
}



