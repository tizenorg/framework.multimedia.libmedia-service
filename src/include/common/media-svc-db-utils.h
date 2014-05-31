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


#ifndef _MEDIA_SVC_DB_UTILS_H_
#define _MEDIA_SVC_DB_UTILS_H_

#include <sqlite3.h>
#include <glib.h>

#define SQLITE3_FINALIZE(x)       if(x  != NULL) {sqlite3_finalize(x);}

int _media_svc_create_media_table(sqlite3 *db_handle);
int _media_svc_create_folder_table(sqlite3 *db_handle);
int _media_svc_create_playlist_table(sqlite3 *db_handle);
int _media_svc_create_album_table(sqlite3 *db_handle);
int _media_svc_create_tag_table(sqlite3 *db_handle);
int _media_svc_create_bookmark_table(sqlite3 *db_handle);
int _media_svc_create_db_version(sqlite3 *db_handle);
int _media_svc_sql_query(sqlite3 *db_handle, const char *sql_str);
int _media_svc_sql_prepare_to_step(sqlite3 *handle, const char *sql_str, sqlite3_stmt** stmt);
int _media_svc_sql_prepare_to_step_simple(sqlite3 *handle, const char *sql_str, sqlite3_stmt** stmt);
int _media_svc_sql_begin_trans(sqlite3 *handle);
int _media_svc_sql_end_trans(sqlite3 *handle);
int _media_svc_sql_rollback_trans(sqlite3 *handle);
int _media_svc_sql_query_list(sqlite3 *handle, GList **query_list);
void _media_svc_sql_query_add(GList **query_list, char **query);
void _media_svc_sql_query_release(GList **query_list);
int _media_svc_check_db_upgrade(sqlite3 *db_handle);
int _media_db_check_corrupt(sqlite3 *db_handle);

#endif /*_MEDIA_SVC_DB_UTILS_H_*/
