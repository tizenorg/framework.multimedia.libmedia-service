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



#ifndef _MEDIA_SVC_TYPES_H_
#define _MEDIA_SVC_TYPES_H_

#include <time.h>

typedef void MediaSvcHandle;		/**< Handle */

/**
 * Type definition for storage type
 */
typedef enum{
	MEDIA_SVC_STORAGE_INTERNAL = 0,		/**< The device's internal storage */
	MEDIA_SVC_STORAGE_EXTERNAL = 1,		/**< The device's external storage like sd card*/
	MEDIA_SVC_STORAGE_EXTERNAL_USB = 2,	/**< The external USB storage (Since 2.4) */
	MEDIA_SVC_STORAGE_CLOUD = 100,		/**< The Cloud storage (Since 2.4) */
	MEDIA_SVC_STORAGE_MAX,				/**< Invalid storage*/
}media_svc_storage_type_e;

/**
 * Type definition for content type
 */
typedef enum{
	MEDIA_SVC_MEDIA_TYPE_IMAGE	= 0,	/**< Image Content*/
	MEDIA_SVC_MEDIA_TYPE_VIDEO	= 1,	/**< Video Content*/
	MEDIA_SVC_MEDIA_TYPE_SOUND	= 2,	/**< Sound Content like Ringtone*/
	MEDIA_SVC_MEDIA_TYPE_MUSIC	= 3,	/**< Music Content like mp3*/
	MEDIA_SVC_MEDIA_TYPE_OTHER	= 4,	/**< Not media Content*/
}media_svc_media_type_e;

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
	char	*	exposure_time;		/**< exposure_time*/
	float		fnumber;			/**< fnumber*/
	int 		iso;				/**< iso*/
	char	*	model;				/**< model*/
	int 		width;				/**< width*/
	int 		height;				/**< height*/
	char	*	datetaken;			/**< datetaken*/
	int		orientation;			/**< orientation*/
	int		rating;				/**< user defined rating */
	char	*	weather;				/**< weather of image */
	int		bitpersample;				/**< bitrate*/

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
	char	*	storage_uuid;					/**< Unique ID of storage */
	media_svc_content_meta_s	media_meta;	/**< meta data structure for audio files */
} media_svc_content_info_s;

/**
 * Type definition for scanning status
 */
typedef enum{
	MEDIA_SVC_SCAN_PREPARE		= 0,	/**< Prepare scanning*/
	MEDIA_SVC_SCAN_PROCESSING	= 1,	/**< Process scanning*/
	MEDIA_SVC_SCAN_STOP			= 2,	/**< Stop scanning*/
	MEDIA_SVC_SCAN_COMPLETE		= 3,	/**< Complete scanning*/
	MEDIA_SVC_SCAN_MAX			= 4,	/**< Invalid status*/
}media_svc_scan_status_type_e;

#endif /*_MEDIA_SVC_TYPES_H_*/
