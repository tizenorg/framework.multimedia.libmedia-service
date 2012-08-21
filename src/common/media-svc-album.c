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

#include "media-svc-album.h"
#include "media-svc-error.h"
#include "media-svc-debug.h"
#include "media-svc-env.h"
#include "media-svc-util.h"
#include "media-svc-db-utils.h"

int _media_svc_get_album_id(sqlite3 *handle, const char *album, const char *artist, int * album_id)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *sql = NULL;

	media_svc_retvm_if(album == NULL, MEDIA_INFO_ERROR_INVALID_PARAMETER, "album is NULL");

	if(artist != NULL) {
		sql = sqlite3_mprintf("SELECT album_id FROM %s WHERE name = '%q' AND artist = '%q';", MEDIA_SVC_DB_TABLE_ALBUM, album, artist);
	} else {
		sql = sqlite3_mprintf("SELECT album_id FROM %s WHERE name = '%q' AND artist IS NULL;", MEDIA_SVC_DB_TABLE_ALBUM, album, artist);
	}

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MEDIA_INFO_ERROR_NONE) {
		if(ret == MEDIA_INFO_ERROR_DATABASE_NO_RECORD) {
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
	int ret = MEDIA_INFO_ERROR_NONE;
	sqlite3_stmt *sql_stmt = NULL;
	char *value = NULL;

	char *sql = sqlite3_mprintf("SELECT album_art FROM %s WHERE album_id=%d", MEDIA_SVC_DB_TABLE_ALBUM, album_id);

	ret = _media_svc_sql_prepare_to_step(handle, sql, &sql_stmt);

	if (ret != MEDIA_INFO_ERROR_NONE) {
		if(ret == MEDIA_INFO_ERROR_DATABASE_NO_RECORD) {
			media_svc_debug("there is no album_id.");
		}
		else {
			media_svc_error("error when _media_svc_get_folder_id_by_foldername. err = [%d]", ret);
		}
		return ret;
	}

	value = (char *)sqlite3_column_text(sql_stmt, 0);

	if (STRING_VALID(value)) {
		ret = __media_svc_malloc_and_strncpy(album_art, value);
		if (ret < 0) {
			media_svc_error("__media_svc_malloc_and_strncpy failed: %d", ret);
			SQLITE3_FINALIZE(sql_stmt);
			return ret;
		}
	} else {
		*album_art = NULL;
	}

	SQLITE3_FINALIZE(sql_stmt);

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_append_album(sqlite3 *handle, const char *album, const char *artist, const char *album_art, int * album_id)
{
	int err = -1;

	char *sql = sqlite3_mprintf("INSERT INTO %s (name, artist, album_art, album_art) values (%Q, %Q, %Q, %Q); ",
					     MEDIA_SVC_DB_TABLE_ALBUM, album, artist, album_art, album_art);
	err = _media_svc_sql_query(handle, sql);
	sqlite3_free(sql);
	if (err != SQLITE_OK) {
		media_svc_error("failed to insert albums");
		return MEDIA_INFO_ERROR_DATABASE_INTERNAL;
	}

	*album_id = sqlite3_last_insert_rowid(handle);

	return MEDIA_INFO_ERROR_NONE;
}
