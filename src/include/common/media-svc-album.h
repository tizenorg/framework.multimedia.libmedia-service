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

#ifndef _MEDIA_SVC_ALBUM_H_
#define _MEDIA_SVC_ALBUM_H_

#include <sqlite3.h>

int _media_svc_get_album_id(sqlite3 *handle, const char *album, const char *artist, int * album_id);
int _media_svc_get_album_art_by_album_id(sqlite3 *handle, int album_id, char **album_art);
int _media_svc_append_album(sqlite3 *handle, const char *album, const char *artist, const char *album_art, int * album_id);
int _media_svc_get_media_count_with_album_id_by_path(sqlite3 *handle, const char *path, int *count);

#endif /*_MEDIA_SVC_ALBUM_H_*/
