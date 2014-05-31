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



#ifndef _MEDIA_SVC_ENV_H_
#define _MEDIA_SVC_ENV_H_

#include <time.h>
#include <media-util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DB information
 */

#define MEDIA_SVC_DB_NAME 						MEDIA_DB_NAME		/**<  media db name*/
#define LATEST_VERSION_NUMBER					2

/**
 * DB table information
 */

#define MEDIA_SVC_DB_TABLE_MEDIA					"media"				/**<  media table*/
#define MEDIA_SVC_DB_TABLE_FOLDER					"folder"				/**<  media_folder table*/
#define MEDIA_SVC_DB_TABLE_PLAYLIST				"playlist"				/**<  playlist table*/
#define MEDIA_SVC_DB_TABLE_PLAYLIST_MAP			"playlist_map"			/**<  playlist_map table*/
#define MEDIA_SVC_DB_TABLE_ALBUM					"album"				/**<  album table*/
#define MEDIA_SVC_DB_TABLE_TAG					"tag"				/**<  tag table*/
#define MEDIA_SVC_DB_TABLE_TAG_MAP				"tag_map"			/**<  tag_map table*/
#define MEDIA_SVC_DB_TABLE_BOOKMARK				"bookmark"			/**<  bookmark table*/

#define MEDIA_SVC_METADATA_LEN_MAX			128						/**<  Length of metadata*/
#define MEDIA_SVC_METADATA_DESCRIPTION_MAX	512						/**<  Length of description*/
#define MEDIA_SVC_PATHNAME_SIZE				4096					/**<  Length of Path name. */
#define MEDIA_SVC_UUID_SIZE		    				36 						/**< Length of UUID*/

#define MEDIA_SVC_TAG_UNKNOWN				"Unknown"
#define MEDIA_SVC_MEDIA_PATH				MEDIA_THUMB_ROOT_PATH			/**<  Media path*/
#define MEDIA_SVC_THUMB_PATH_PREFIX			MEDIA_SVC_MEDIA_PATH"/.thumb"			/**< Thumbnail path prefix*/
#define MEDIA_SVC_THUMB_INTERNAL_PATH 		MEDIA_SVC_THUMB_PATH_PREFIX"/phone"	/**<  Phone thumbnail path*/
#define MEDIA_SVC_THUMB_EXTERNAL_PATH 		MEDIA_SVC_THUMB_PATH_PREFIX"/mmc"		/**<  MMC thumbnail path*/
#define MEDIA_SVC_THUMB_DEFAULT_PATH		MEDIA_SVC_THUMB_PATH_PREFIX"/thumb_default.png" /** default thumbnail */

#define MEDIA_SVC_DEFAULT_GPS_VALUE			-200			/**<  Default GPS Value*/
#define THUMB_EXT 	"jpg"

enum Exif_Orientation {
    NOT_AVAILABLE=0,
    NORMAL  =1,
    HFLIP   =2,
    ROT_180 =3,
    VFLIP   =4,
    TRANSPOSE   =5,
    ROT_90  =6,
    TRANSVERSE  =7,
    ROT_270 =8
};

/**
 * Media meta data information
 */
typedef struct {
	char	*	title;				/**< track title*/
	char	*	album;				/**< album name*/
	char	*	artist;				/**< artist name*/
	char	*	album_artist;		/**< artist name*/
	char	*	genre;				/**< genre of track*/
	char	*	composer;			/**< composer name*/
	char	*	year;				/**< year*/
	char	*	recorded_date;		/**< recorded date*/
	char	*	copyright;			/**< copyright*/
	char	*	track_num;			/**< track number*/
	char	*	description;			/**< description*/
	int		bitrate;				/**< bitrate*/
	int		samplerate;			/**< samplerate*/
	int		channel;				/**< channel*/
	int		duration;			/**< duration*/
	float		longitude;			/**< longitude*/
	float		latitude;				/**< latitude*/
	float		altitude;				/**< altitude*/
	int 		width;				/**< width*/
	int 		height;				/**< height*/
	char	*	datetaken;			/**< datetaken*/
	int		orientation;			/**< orientation*/
	int		rating;				/**< user defined rating */
	char	*	weather;				/**< weather of image */

	char	*	file_name_pinyin;				/**< pinyin for file_name*/
	char	*	title_pinyin;					/**< pinyin for title*/
	char	*	album_pinyin;				/**< pinyin for album*/
	char	*	artist_pinyin;					/**< pinyin for artist*/
	char	*	album_artist_pinyin;			/**< pinyin for album_artist*/
	char	*	genre_pinyin;					/**< pinyin for genre*/
	char	*	composer_pinyin;				/**< pinyin for composer*/
	char	*	copyright_pinyin;				/**< pinyin for copyright*/
	char	*	description_pinyin;			/**< pinyin for description*/
} media_svc_content_meta_s;


/**
 * Media data information
 */
typedef struct {
	char	*	media_uuid;					/**< Unique ID of item */
	char	*	path;						/**< Full path of media file */
	char	*	file_name;					/**< File name of media file. Display name */
	char	*	file_name_pinyin;				/**< File name pinyin of media file. Display name */
	int		media_type;					/**< Type of media file : internal/external */
	char	*	mime_type;					/**< Full path and file name of media file */
	unsigned long long	size;							/**< size */
	time_t	added_time;					/**< added time, time_t */
	time_t	modified_time;				/**< modified time, time_t */
	time_t	timeline;					/**< timeline of media, time_t */
	char	*	folder_uuid;					/**< Unique ID of folder */
	int		album_id;					/**< Unique ID of album */
	char	*	thumbnail_path;				/**< Thumbnail image file path */
	int		played_count;				/**< played count */
	int		last_played_time;				/**< last played time */
	int		last_played_position;			/**< last played position */
	int		favourate;					/**< favourate. o or 1 */
	int		is_drm;						/**< is_drm. o or 1 */
	int		sync_status;						/**< sync_status  */
	int		storage_type;					/**< Storage of media file : internal/external */
	media_svc_content_meta_s	media_meta;	/**< meta data structure for audio files */
} media_svc_content_info_s;

typedef enum{
	MEDIA_SVC_QUERY_INSERT_ITEM,
	MEDIA_SVC_QUERY_SET_ITEM_VALIDITY,
	MEDIA_SVC_QUERY_MOVE_ITEM,
} media_svc_query_type_e;

#ifdef __cplusplus
}
#endif

#endif /*_MEDIA_SVC_ENV_H_*/
