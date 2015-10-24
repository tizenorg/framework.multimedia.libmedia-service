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

#ifndef _MEDIA_SVC_STORAGE_H_
#define _MEDIA_SVC_STORAGE_H_

#include <sqlite3.h>

int _media_svc_init_storage(sqlite3 *handle);
int _media_svc_get_mmc_info(MediaSvcHandle *handle, char **storage_name, char **storage_path, int *validity, bool *info_exist);
int _media_svc_check_storage(sqlite3 *handle, const char *storage_id, const char *storage_name, char **storage_path, int *validity);
int _media_svc_append_storage(sqlite3 *handle, const char *storage_id, const char *storage_name, const char *storage_path, const char *storage_account, media_svc_storage_type_e storage_type);
int _media_svc_update_storage_path(sqlite3 *handle, const char *storage_id, const char *path);
int _media_svc_delete_storage(sqlite3 *handle, const char *storage_id, const char *storage_name);
int _media_svc_update_storage_validity(sqlite3 *handle, const char *storage_id, int validity);
int _media_svc_get_storage_uuid(sqlite3 *handle, const char *path, char *storage_id);
int _media_svc_get_storage_type(sqlite3 *handle, const char *storage_id, media_svc_storage_type_e *storage_type);
int _media_svc_get_storage_path(sqlite3 *handle, const char *storage_id, char **storage_path);
int _media_svc_get_storage_scan_status(sqlite3 *handle, const char*storage_id, media_svc_scan_status_type_e *scan_status);
int _media_svc_set_storage_scan_status(sqlite3 *handle, const char*storage_id, media_svc_scan_status_type_e scan_status);
int _media_svc_get_all_storage(sqlite3 *handle, char ***storage_list, char ***storage_id_list, int **scan_status_list, int *count);


#endif /*_MEDIA_SVC_STORAGE_H_*/
