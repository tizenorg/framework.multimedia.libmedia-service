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

#ifndef _MEDIA_SVC_MEDIA_FOLDER_H_
#define _MEDIA_SVC_MEDIA_FOLDER_H_

#include <sqlite3.h>
#include <stdbool.h>
#include "media-svc-types.h"

int _media_svc_get_folder_id_by_foldername(sqlite3 *handle, const char *folder_name, char *folder_id);
int _media_svc_append_folder(sqlite3 *handle, media_svc_storage_type_e storage_type, const char *folder_id, const char *path_name, const char *folder_name, int modified_date);
int _media_svc_update_folder_modified_time_by_folder_uuid(sqlite3 *handle, const char *folder_uuid, const char *folder_path, bool stack_query);
int _media_svc_get_and_append_folder_id_by_path(sqlite3 *handle, const char *path, media_svc_storage_type_e storage_type, char *folder_id);
int _media_svc_update_folder_table(sqlite3 *handle);
int _media_svc_get_all_folders(sqlite3 *handle, char* start_path, char ***folder_list, time_t **modified_time_list, int **item_num_list, int *count);
int _media_svc_get_folder_info_by_foldername(sqlite3 *handle, const char *folder_name, char *folder_id, time_t *modified_time);

#endif /*_MEDIA_SVC_MEDIA_FOLDER_H_*/
