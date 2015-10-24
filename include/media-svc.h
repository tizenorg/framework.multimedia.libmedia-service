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



#ifndef _MEDIA_SVC_H_
#define _MEDIA_SVC_H_

#include "media-svc-types.h"
#include <media-util-noti.h>
#include <time.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif


int media_svc_connect(MediaSvcHandle **handle);
int media_svc_disconnect(MediaSvcHandle *handle);
int media_svc_create_table(MediaSvcHandle *handle);
int media_svc_check_item_exist_by_path(MediaSvcHandle *handle, const char *storage_id, const char *path);
int media_svc_insert_folder(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path);
int media_svc_insert_item_begin(MediaSvcHandle *handle, int with_noti, int data_cnt, int from_pid);
int media_svc_insert_item_end(MediaSvcHandle *handle);
int media_svc_insert_item_bulk(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path, int is_burst);
int media_svc_insert_item_immediately(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path);
int media_svc_move_item(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e src_storage, const char *src_path, media_svc_storage_type_e dest_storage, const char *dest_path);
int media_svc_set_item_validity_begin(MediaSvcHandle *handle, int data_cnt);
int media_svc_set_item_validity_end(MediaSvcHandle *handle);
int media_svc_set_item_validity(MediaSvcHandle *handle, const char *storage_id, const char *path, int validity);
int media_svc_delete_item_by_path(MediaSvcHandle *handle, const char *storage_id, const char *path);
int media_svc_delete_all_items_in_storage(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type);
int media_svc_delete_invalid_items_in_storage(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type);
int media_svc_delete_invalid_items_in_folder(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, bool is_recursive);
int media_svc_set_all_storage_items_validity(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, int validity);
int media_svc_set_folder_items_validity(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int validity, int recursive);
int media_svc_refresh_item(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path);
int media_svc_rename_folder(MediaSvcHandle *handle, const char *storage_id, const char *src_path, const char *dst_path);
int media_svc_request_update_db(const char *db_query);
int media_svc_get_storage_type(const char *path, media_svc_storage_type_e *storage_type);
int media_svc_get_file_info(MediaSvcHandle *handle, const char *storage_id, const char *path, time_t *modified_time, unsigned long long *size);
int media_svc_send_dir_update_noti(MediaSvcHandle *handle, const char *storage_id, const char *dir_path, const char *folder_id, media_item_update_type_e update_type, int pid);
int media_svc_count_invalid_items_in_folder(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int *count);
int media_svc_check_db_upgrade(MediaSvcHandle *handle, bool *need_full_scan);
int media_svc_check_db_corrupt(MediaSvcHandle *handle);
int media_svc_get_folder_list(MediaSvcHandle *handle, char* start_path, char ***folder_list, time_t **modified_time_list, int **item_num_list, int *count);
int media_svc_update_folder_time(MediaSvcHandle *handle, const char *storage_id, const char *folder_path);
int media_svc_publish_noti(MediaSvcHandle *handle, media_item_type_e update_item, media_item_update_type_e update_type, const char *path, media_type_e media_type, const char *uuid, const char *mime_type);
int media_svc_get_pinyin(MediaSvcHandle *handle, const char * src_str, char **pinyin_str);
int media_svc_check_pinyin_support(bool *support);
int media_svc_update_item_begin(MediaSvcHandle *handle, int data_cnt);
int media_svc_update_item_end(MediaSvcHandle *handle);
int media_svc_update_item_meta(MediaSvcHandle *handle, const char *file_path, int storage_type);
int media_svc_insert_item_immediately_with_data(MediaSvcHandle *handle, media_svc_content_info_s *content_info);
void media_svc_destroy_content_info(media_svc_content_info_s *content_info);

int media_svc_generate_uuid(char **uuid);
int media_svc_get_mmc_info(MediaSvcHandle *handle, char **storage_name, char **storage_path, int *validity, bool *info_exist);
int media_svc_check_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_name, char **storage_path, int *validity);
int media_svc_insert_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_name, const char *storage_path, const char *storage_account, media_svc_storage_type_e storage_type);
int media_svc_update_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_path);
int media_svc_delete_storage(MediaSvcHandle *handle, const char *storage_id, const char *storage_name);
int media_svc_set_storage_validity(MediaSvcHandle *handle, const char *storage_id, int validity);
int media_svc_get_storage_id(MediaSvcHandle *handle, const char *path, char *storage_id);
int media_svc_get_storage_path(MediaSvcHandle *handle, const char *storage_uuid, char **storage_path);
int media_svc_get_storage_scan_status(MediaSvcHandle *handle, const char *storage_uuid, media_svc_scan_status_type_e *storage_status);
int media_svc_set_storage_scan_status(MediaSvcHandle *handle, const char *storage_uuid, media_svc_scan_status_type_e storage_status);
int media_svc_get_storage_list(MediaSvcHandle *handle, char ***storage_list, char ***storage_id_list, int **scan_status_list, int *count);
int media_svc_delete_invalid_folder(MediaSvcHandle *handle, const char *storage_id);
int media_svc_set_folder_validity(MediaSvcHandle *handle, const char *storage_id, const char *start_path, int validity, bool is_recursive);

int media_svc_insert_item_pass1(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, const char *path, int is_burst);
int media_svc_insert_item_pass2(MediaSvcHandle *handle, const char *storage_id, media_svc_storage_type_e storage_type, int scan_type, const char* extract_path, int is_burst);

int media_svc_insert_folder_begin(MediaSvcHandle *handle, int data_cnt);
int media_svc_insert_folder_end(MediaSvcHandle *handle);

int media_svc_delete_invalid_folder_by_path(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int *delete_count);
int media_svc_check_folder_exist_by_path(MediaSvcHandle *handle, const char *storage_id, const char *folder_path);
int media_svc_check_subfolder_by_path(MediaSvcHandle *handle, const char *storage_id, const char *folder_path, int *count);
int media_svc_get_folder_id(MediaSvcHandle *handle, const char *storage_id, const char *path, char *folder_id);


#ifdef __cplusplus
}
#endif

#endif /*_MEDIA_SVC_H_*/
