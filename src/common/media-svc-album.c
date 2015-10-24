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

#include <media-util-err.h>
#include "media-svc-album.h"
#include "media-svc-debug.h"
#include "media-svc-env.h"
#include "media-svc-util.h"
#include "media-svc-db-utils.h"

int _media_svc_get_album_id(sqlite3 *handle, const char *album, const char *artist, int * album_id)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	media_svc_retvm_if(album == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "album is NULL");

	if(artist != NULL) {
		sql = sqlite3_mprintf("SELECT album_id FROM %s WHERE name = '%q' AND artist = '%q';", MEDIA_SVC_DB_TABLE_ALBUM, album, artist);
	} else {
		sql = sqlite3_mprintf("SELECT album_id FROM %s WHERE name = '%q' AND artist IS NULL;", MEDIA_SVC_DB_TABLE_ALBUM, album, artist);
	}

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			media_svc_debug("there is no album.");
		}
		else {
			media_svc_error("error when _media_svc_get_album_id. err = [%d]", ret);
		}
		return ret;
	}

	*album_id = sqlite3_column_int(sql_stmt, 0);

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}

int _media_svc_get_album_art_by_album_id(sqlite3 *handle, int album_id, char **album_art)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *value = NULL;

	char *sql = sqlite3_mprintf("SELECT album_art FROM %s WHERE album_id=%d", MEDIA_SVC_DB_TABLE_ALBUM, album_id);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			media_svc_debug("there is no album_id.");
		}
		else {
			media_svc_error("error when get album_art. err = [%d]", ret);
		}
		return ret;
	}

	value = (char *)sqlite3_column_text(sql_stmt, 0);

	if (STRING_VALID(value)) {
		ret = __media_svc_malloc_and_strncpy(album_art, value);
		if (ret != MS_MEDIA_ERR_NONE) {
			SQLITE3_FINALIZE(sql_stmt);
			return ret;
		}
	} else {
		*album_art = NULL;
	}

	SQLITE3_FINALIZE(sql_stmt);

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_append_album(sqlite3 *handle, const char *album, const char *artist, const char *album_art, int * album_id)
{
	int ret = MS_MEDIA_ERR_NONE;

	char *sql = sqlite3_mprintf("INSERT INTO %s (name, artist, album_art, album_art) values (%Q, %Q, %Q, %Q); ",
					     MEDIA_SVC_DB_TABLE_ALBUM, album, artist, album_art, album_art);
	ret = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	//*album_id = sqlite3_last_insert_rowid(handle);
	int inserted_album_id = 0;
	ret = _media_svc_get_album_id(handle, album, artist, &inserted_album_id);
	media_svc_retv_if(ret != MS_MEDIA_ERR_NONE, ret);

	*album_id = inserted_album_id;

	return MS_MEDIA_ERR_NONE;
}

int _media_svc_get_media_count_with_album_id_by_path(sqlite3 *handle, const char *path, int *count)
{
	int ret = MS_MEDIA_ERR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	media_svc_retvm_if(path == NULL, MS_MEDIA_ERR_INVALID_PARAMETER, "path is NULL");

	sql = sqlite3_mprintf("select count(media_uuid) from %s INNER JOIN (select album_id from %s where path=%Q and album_id > 0) as album ON album.album_id=media.album_id;", MEDIA_SVC_DB_TABLE_MEDIA, MEDIA_SVC_DB_TABLE_MEDIA, path);
	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MS_MEDIA_ERR_NONE) {
		if(ret == MS_MEDIA_ERR_DB_NO_RECORD) {
			media_svc_debug("there is no media in relted to this media's album.");
		}
		else {
			media_svc_error("error when _media_svc_get_media_count_with_album_id_by_path. err = [%d]", ret);
		}
		return ret;
	}

	*count = sqlite3_column_int(sql_stmt, 0);
	media_svc_debug("Media count : %d", *count);

	SQLITE3_FINALIZE(sql_stmt);

	return ret;
}
