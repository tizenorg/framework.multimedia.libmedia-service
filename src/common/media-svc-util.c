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

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/vfs.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <aul/aul.h>
#include <mm_file.h>
#include <mm_error.h>
#include <libexif/exif-data.h>
#include <media-thumbnail.h>
#include <media-util.h>
#include <uuid/uuid.h>
#include <vconf.h>
#include "media-svc-util.h"
#include "media-svc-db-utils.h"
#include "media-svc-error.h"
#include "media-svc-debug.h"
#include "media-svc-env.h"
#include "media-svc-hash.h"
#include "media-svc-album.h"
#include "media-svc-localize-utils.h"
#include "media-svc-localize_ch.h"
#include "media-svc-localize_tw.h"

#define MEDIA_SVC_FILE_EXT_LEN_MAX				6			/**<  Maximum file ext lenth*/

/* Define data structures for media type and mime type */
#define MEDIA_SVC_CATEGORY_UNKNOWN	0x00000000	/**< Default */
#define MEDIA_SVC_CATEGORY_ETC		0x00000001	/**< ETC category */
#define MEDIA_SVC_CATEGORY_IMAGE	0x00000002	/**< Image category */
#define MEDIA_SVC_CATEGORY_VIDEO	0x00000004	/**< Video category */
#define MEDIA_SVC_CATEGORY_MUSIC	0x00000008	/**< Music category */
#define MEDIA_SVC_CATEGORY_SOUND	0x00000010	/**< Sound category */

#define CONTENT_TYPE_NUM 5
#define MUSIC_MIME_NUM 29
#define SOUND_MIME_NUM 1
#define MIME_TYPE_LENGTH 255
#define MIME_LENGTH 50
#define _3GP_FILE ".3gp"
#define _MP4_FILE ".mp4"
#define _ASF_FILE ".asf"

typedef struct {
	char content_type[15];
	int category_by_mime;
} _media_svc_content_table_s;

static const _media_svc_content_table_s content_category[CONTENT_TYPE_NUM] = {
	{"audio", MEDIA_SVC_CATEGORY_SOUND},
	{"image", MEDIA_SVC_CATEGORY_IMAGE},
	{"video", MEDIA_SVC_CATEGORY_VIDEO},
	{"application", MEDIA_SVC_CATEGORY_ETC},
	{"text", MEDIA_SVC_CATEGORY_ETC},
};

static const char music_mime_table[MUSIC_MIME_NUM][MIME_LENGTH] = {
	/*known mime types of normal files*/
	"mpeg",
	"ogg",
	"x-ms-wma",
	"x-flac",
	"mp4",
	/* known mime types of drm files*/
	"mp3",
	"x-mp3", /*alias of audio/mpeg*/
	"x-mpeg", /*alias of audio/mpeg*/
	"3gpp",
	"x-ogg", /*alias of  audio/ogg*/
	"vnd.ms-playready.media.pya:*.pya", /*playready*/
	"wma",
	"aac",
	"x-m4a", /*alias of audio/mp4*/
	/* below mimes are rare*/
	"x-vorbis+ogg",
	"x-flac+ogg",
	"x-matroska",
	"ac3",
	"mp2",
	"x-ape",
	"x-ms-asx",
	"vnd.rn-realaudio",

	"x-vorbis", /*alias of audio/x-vorbis+ogg*/
	"vorbis", /*alias of audio/x-vorbis+ogg*/
	"x-oggflac",
	"x-mp2", /*alias of audio/mp2*/
	"x-pn-realaudio", /*alias of audio/vnd.rn-realaudio*/
	"vnd.m-realaudio", /*alias of audio/vnd.rn-realaudio*/
	"x-wav",
};

static const char sound_mime_table[SOUND_MIME_NUM][MIME_LENGTH] = {
	"x-smaf",
};

typedef enum {
	MEDIA_SVC_EXTRACTED_FIELD_NONE 			= 0x00000001,
	MEDIA_SVC_EXTRACTED_FIELD_TITLE 		= MEDIA_SVC_EXTRACTED_FIELD_NONE << 1,
	MEDIA_SVC_EXTRACTED_FIELD_DESC 			= MEDIA_SVC_EXTRACTED_FIELD_NONE << 2,
	MEDIA_SVC_EXTRACTED_FIELD_COPYRIGHT		= MEDIA_SVC_EXTRACTED_FIELD_NONE << 3,
	MEDIA_SVC_EXTRACTED_FIELD_AUTHOR		= MEDIA_SVC_EXTRACTED_FIELD_NONE << 4,
	MEDIA_SVC_EXTRACTED_FIELD_ARTIST		= MEDIA_SVC_EXTRACTED_FIELD_NONE << 5,
	MEDIA_SVC_EXTRACTED_FIELD_GENRE			= MEDIA_SVC_EXTRACTED_FIELD_NONE << 6,
	MEDIA_SVC_EXTRACTED_FIELD_ALBUM			= MEDIA_SVC_EXTRACTED_FIELD_NONE << 7,
	MEDIA_SVC_EXTRACTED_FIELD_TRACKNUM		= MEDIA_SVC_EXTRACTED_FIELD_NONE << 8,
	MEDIA_SVC_EXTRACTED_FIELD_YEAR			= MEDIA_SVC_EXTRACTED_FIELD_NONE << 9,
	MEDIA_SVC_EXTRACTED_FIELD_CATEGORY		= MEDIA_SVC_EXTRACTED_FIELD_NONE << 10,
	MEDIA_SVC_EXTRACTED_FIELD_ALBUM_ARTIST		= MEDIA_SVC_EXTRACTED_FIELD_NONE << 11,
} media_svc_extracted_field_e;


char *_media_info_generate_uuid(void)
{
	uuid_t uuid_value;
	static char uuid_unparsed[50];	

	uuid_generate(uuid_value);
	uuid_unparse(uuid_value, uuid_unparsed);

	//media_svc_debug("UUID : %s", uuid_unparsed);
	return uuid_unparsed;
}

void _strncpy_safe(char *x_dst, const char *x_src, int max_len)
{
	if (!x_src || strlen(x_src) == 0) {
		media_svc_error("x_src is NULL");
		return;
	}

	if (max_len < 1) {
		media_svc_error("length is Wrong");
		return;
	}

    strncpy(x_dst, x_src, max_len-1);
	x_dst[max_len-1] = '\0';
}

int __media_svc_malloc_and_strncpy(char **dst, const char *src)
{
	int len = 0;

	if (!STRING_VALID(src)) {
		media_svc_error("invalid src");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	SAFE_FREE(*dst);

	len = strlen(src) + 1;
	*dst = malloc(len);

	if (*dst == NULL) {
		media_svc_error("malloc failed");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	strncpy(*dst, src, len);
	char *p = *dst;
	p[len - 1] = '\0';

	return MEDIA_INFO_ERROR_NONE;
}

int __media_svc_malloc_and_strncpy_with_size(char **dst, const char *src, int copysize)
{
	if (!STRING_VALID(src)) {
		media_svc_error("invalid src");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	SAFE_FREE(*dst);

	*dst = malloc(copysize + 1);

	if (*dst == NULL) {
		media_svc_error("malloc failed");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	strncpy(*dst, src, copysize);
	char *p = *dst;
	p[copysize] = '\0';

	return MEDIA_INFO_ERROR_NONE;
}

static int __media_svc_split_to_double(char *input, double *arr)
{
	char tmp_arr[255] = {0, };
	int len = 0, idx = 0, arr_idx = 0, str_idx = 0;

	if(!STRING_VALID(input))
	{
		media_svc_error("Invalid parameter");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}
	memset(tmp_arr, 0x0, sizeof(tmp_arr));

	//media_svc_debug("input: [%s]", input);

	len = strlen(input);

	for (idx = 0; idx < (len+1); idx++)
	{
		if(input[idx] == ' ')
		{
			continue;
		}
		else if((input[idx] == ',') || (idx == len))
		{
			arr[arr_idx] = atof(tmp_arr);
			arr_idx++;
			str_idx = 0;
			//media_svc_debug("idx=[%d] arr_idx=[%d] tmp_attr[%s] atof(tmp_arr)=[%f]", idx, arr_idx, tmp_arr, atof(tmp_arr));
			memset(tmp_arr, 0x0, sizeof(tmp_arr));
		}
		else
		{
			tmp_arr[str_idx] = input[idx];
			str_idx++;
		}
	}

	if(arr_idx != 3)
	{
		media_svc_error("Error when parsing GPS [%d]", arr_idx);
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	return MEDIA_INFO_ERROR_NONE;
}

static int __media_svc_get_exif_info(ExifData *ed,
										char *buf,
										int *i_value,
										double *d_value,
										int ifdtype,
										long tagtype)
{
	ExifEntry *entry;
	ExifTag tag;

	if (ed == NULL) {
		//media_svc_debug("ExifData is NULL");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	tag = tagtype;

	entry = exif_data_get_entry(ed, tag);
	if (entry) {
		/* Get the contents of the tag in human-readable form */
		if (tag == EXIF_TAG_ORIENTATION ||
				tag == EXIF_TAG_PIXEL_X_DIMENSION ||
				tag == EXIF_TAG_PIXEL_Y_DIMENSION) {

			if (i_value == NULL) {
				media_svc_error("i_value is NULL");
				return MEDIA_INFO_ERROR_INVALID_PARAMETER;
			}

			ExifByteOrder mByteOrder = exif_data_get_byte_order(ed);
			short exif_value = exif_get_short(entry->data, mByteOrder);
			//media_svc_debug("%s : %d", exif_tag_get_name_in_ifd(tag,ifd), exif_value);
			*i_value = (int)exif_value;

		} else if (tag == EXIF_TAG_GPS_LATITUDE || tag == EXIF_TAG_GPS_LONGITUDE || tag == EXIF_TAG_GPS_ALTITUDE) {

			if (d_value == NULL) {
				media_svc_error("d_value is NULL");
				return MEDIA_INFO_ERROR_INVALID_PARAMETER;
			}

			/* Get the contents of the tag in human-readable form */
			char gps_buf[MEDIA_SVC_METADATA_LEN_MAX + 1] = {0, };
			exif_entry_get_value(entry, gps_buf, sizeof(gps_buf));
			gps_buf[strlen(gps_buf)] = '\0';
			int ret = MEDIA_INFO_ERROR_NONE;
			//media_svc_debug("%s: [%s]", exif_tag_get_name_in_ifd(tag, ifd), gps_buf);

			double tmp_arr[3] = { 0.0, 0.0, 0.0 };

			ret = __media_svc_split_to_double(gps_buf, tmp_arr);
			media_svc_retv_if(ret != MEDIA_INFO_ERROR_NONE, ret);

			*d_value = tmp_arr[0] + tmp_arr[1] / 60 + tmp_arr[2] / 3600;
			//media_svc_debug("GPS value is [%f], %f, %f, %f", *d_value, tmp_arr[0], tmp_arr[1], tmp_arr[2]);
		} else {

			if (buf == NULL) {
				media_svc_error("buf is NULL");
				return MEDIA_INFO_ERROR_INVALID_PARAMETER;
			}

			exif_entry_get_value(entry, buf, MEDIA_SVC_METADATA_LEN_MAX);
			buf[strlen(buf)] = '\0';
		}
	}

	return MEDIA_INFO_ERROR_NONE;
}

time_t __media_svc_get_timeline_from_str(const char *timstr)
{
	struct tm t;
	time_t modified_t = 0;
	time_t rawtime;
	struct tm timeinfo;

	if (!STRING_VALID(timstr))
	{
		media_svc_error("Invalid Parameter");
		return 0;
	}

	/*Exif Format : %Y:%m:%d %H:%M:%S
	Videoc Content Creation_time format of FFMpeg : %Y-%m-%d %H:%M:%S*/
	memset(&t, 0x00, sizeof(struct tm));

	tzset();
	time ( &rawtime );
	localtime_r (&rawtime, &timeinfo);

	if (strptime(timstr, "%Y:%m:%d %H:%M:%S", &t) || strptime(timstr, "%Y-%m-%d %H:%M:%S", &t))
	{
		t.tm_isdst = timeinfo.tm_isdst;
		if(t.tm_isdst != 0) {
			media_svc_error("DST %d", t.tm_isdst);
		}

		modified_t = mktime(&t);
		if(modified_t > 0)
		{
			return modified_t;
		}
		else
		{
			media_svc_error("Failed to get timeline : [%s] [%d:%d:%d: %d:%d:%d]", timstr, t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
		}
	}
	else
	{
		media_svc_error("Failed to get timeline : [%s]", timstr);
	}

	return 0;
}

static int __media_svc_get_content_type_from_mime(const char * path, const char * mimetype, int * category)
{
	int idx = 0;

	*category = MEDIA_SVC_CATEGORY_UNKNOWN;

	/*categorize from mimetype */
	for (idx = 0; idx < CONTENT_TYPE_NUM; idx++) {
		if (strstr(mimetype, content_category[idx].content_type) != NULL) {
			*category = (*category | content_category[idx].category_by_mime);
			break;
		}
	}

	/*in application type, exitst sound file ex) x-smafs */
	if (*category & MEDIA_SVC_CATEGORY_ETC) {
		int prefix_len = strlen(content_category[0].content_type);

		for (idx = 0; idx < SOUND_MIME_NUM; idx++) {
			if (strstr(mimetype + prefix_len, sound_mime_table[idx]) != NULL) {
				*category ^= MEDIA_SVC_CATEGORY_ETC;
				*category |= MEDIA_SVC_CATEGORY_SOUND;
				break;
			}
		}

		if(strncasecmp(mimetype, "text/x-iMelody", strlen("text/x-iMelody")) == 0) {
			*category ^= MEDIA_SVC_CATEGORY_ETC;
			*category |= MEDIA_SVC_CATEGORY_SOUND;
		}
	}

	/*check music file in soun files. */
	if (*category & MEDIA_SVC_CATEGORY_SOUND) {
		int prefix_len = strlen(content_category[0].content_type) + 1;

		for (idx = 0; idx < MUSIC_MIME_NUM; idx++) {
			if (strcmp(mimetype + prefix_len, music_mime_table[idx]) == 0) {
				*category ^= MEDIA_SVC_CATEGORY_SOUND;
				*category |= MEDIA_SVC_CATEGORY_MUSIC;
				break;
			}
		}

		/*m3u file is playlist but mime type is "audio/x-mpegurl". but It has to be classified into MS_CATEGORY_ETC since playlist is not a sound track*/
		if(strncasecmp(mimetype, "audio/x-mpegurl", strlen("audio/x-mpegurl")) == 0) {
			*category ^= MEDIA_SVC_CATEGORY_SOUND;
			*category |= MEDIA_SVC_CATEGORY_ETC;
		}
	} else if (*category & MEDIA_SVC_CATEGORY_VIDEO) {
		/*some video files don't have video stream. in this case it is categorize as music. */
		char *ext = NULL;
		/*"3gp" and "mp4" must check video stream and then categorize in directly. */
		ext = strrchr(path, '.');
		if (ext != NULL) {
			if ((strncasecmp(ext, _3GP_FILE, 4) == 0) || (strncasecmp(ext, _MP4_FILE, 5) == 0) || (strncasecmp(ext, _ASF_FILE, 5) == 0)) {
				int audio = 0;
				int video = 0;
				int err = 0;

				err = mm_file_get_stream_info(path, &audio, &video);
				if (err == 0) {
					if (audio > 0 && video == 0) {
						*category ^= MEDIA_SVC_CATEGORY_VIDEO;
						*category |= MEDIA_SVC_CATEGORY_MUSIC;
					}
				}
				//even though error occued in mm_file_get_stream_info return MEDIA_INFO_ERROR_NONE. fail means invalid media content.
			}
		}
	}

	return MEDIA_INFO_ERROR_NONE;
}

static int __media_svc_get_media_type(const char *path, const char *mime_type, media_svc_media_type_e *media_type)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	int category = 0;

	media_svc_media_type_e type;

	ret = __media_svc_get_content_type_from_mime(path, mime_type, &category);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("__media_svc_get_content_type_from_mime failed : %d", ret);
	}

	if (category & MEDIA_SVC_CATEGORY_SOUND)		type = MEDIA_SVC_MEDIA_TYPE_SOUND;
	else if (category & MEDIA_SVC_CATEGORY_MUSIC)	type = MEDIA_SVC_MEDIA_TYPE_MUSIC;
	else if (category & MEDIA_SVC_CATEGORY_IMAGE)	type = MEDIA_SVC_MEDIA_TYPE_IMAGE;
	else if (category & MEDIA_SVC_CATEGORY_VIDEO)	type = MEDIA_SVC_MEDIA_TYPE_VIDEO;
	else	type = MEDIA_SVC_MEDIA_TYPE_OTHER;

	*media_type = type;

	return ret;
}

/*
drm_contentifo is not NULL, if the file is OMA DRM.
If the file is not OMA DRM, drm_contentinfo must be NULL.
*/
static int __media_svc_get_mime_type(const char *path, char *mimetype)
{
	if (path == NULL)
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;

	/*in case of normal files or failure to get mime in drm */
	if (aul_get_mime_from_file(path, mimetype, 255) < 0) {
		media_svc_error("aul_get_mime_from_file fail");
		return MEDIA_INFO_ERROR_INVALID_MEDIA;
	}

	return MEDIA_INFO_ERROR_NONE;
}

static bool __media_svc_get_file_ext(const char *file_path, char *file_ext)
{
	int i = 0;

	for (i = strlen(file_path); i >= 0; i--) {
		if (file_path[i] == '.') {
			_strncpy_safe(file_ext, &file_path[i+1], MEDIA_SVC_FILE_EXT_LEN_MAX);
			return true;
		}

		if (file_path[i] == '/') {
			return false;
		}
	}
	return false;
}

static int __media_svc_get_location_value(MMHandleType tag, double *longitude, double *latitude, double *altitude)
{
	char *err_attr_name = NULL;
	double gps_value = 0.0;
	int mmf_error = MM_ERROR_NONE;

	mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_LONGITUDE, &gps_value, NULL);
	if (mmf_error == MM_ERROR_NONE) {
		if (longitude != NULL) {
			*longitude = (gps_value == 0.0) ? MEDIA_SVC_DEFAULT_GPS_VALUE : gps_value;
		}
	} else {
		SAFE_FREE(err_attr_name);
	}

	mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_LATIDUE, &gps_value, NULL);
	if (mmf_error == MM_ERROR_NONE) {
		if (latitude != NULL) {
			*latitude = (gps_value == 0.0) ? MEDIA_SVC_DEFAULT_GPS_VALUE : gps_value;
		}
	} else {
		SAFE_FREE(err_attr_name);
	}

	mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ALTIDUE, &gps_value, NULL);
	if (mmf_error == MM_ERROR_NONE) {
		if (altitude != NULL) {
			*altitude = (gps_value == 0.0) ? MEDIA_SVC_DEFAULT_GPS_VALUE : gps_value;
		}
	} else {
		SAFE_FREE(err_attr_name);
	}

	return MEDIA_INFO_ERROR_NONE;
}

static int _media_svc_save_image(void *image, int size, char *image_path)
{
	media_svc_debug("start save image, path [%s] image size [%d]", image_path, size);

	if (!image) {
		media_svc_error("invalid image..");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	struct statfs fs;
	if (-1 == statfs(MEDIA_SVC_THUMB_PATH_PREFIX, &fs)) {
		media_svc_error("error in statfs");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	long bsize_kbytes = fs.f_bsize >> 10;

	if ((bsize_kbytes * fs.f_bavail) < 1024) {
		media_svc_error("not enought space...");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	FILE *fp = NULL;
	int nwrite = -1;
	if (image != NULL && size > 0) {
		fp = fopen(image_path, "w");

		if (fp == NULL) {
			media_svc_error("failed to open file");
			return MEDIA_INFO_ERROR_INTERNAL;
		}

		nwrite = fwrite(image, 1, size, fp);
		if (nwrite != size) {
			media_svc_error("failed to write thumbnail");
			fclose(fp);
			return MEDIA_INFO_ERROR_INTERNAL;
		}
		fclose(fp);
	}

	return MEDIA_INFO_ERROR_NONE;
}

static char *_media_svc_get_title_from_filepath (const char *path)
{
	char *filename = NULL;
	char *title = NULL;
	char	*ext = NULL;
	int filename_len = -1;
	int new_title_len = -1;

	if (!path) {
		media_svc_error("path is NULL");
		return NULL;
	}

	filename = g_path_get_basename(path);
	if (!STRING_VALID(filename)) {
		media_svc_error("wrong file name");
		SAFE_FREE(filename);
		return NULL;
	}

	filename_len = strlen(filename);

	ext = g_strrstr(filename, ".");
	if (!ext) {
		media_svc_error("there is no file extention");
		return filename;
	}

	new_title_len = filename_len - strlen(ext);
	if (new_title_len < 1) {
		media_svc_error("title length is zero");
		SAFE_FREE(filename);
		return NULL;
	}

	title = g_strndup(filename, new_title_len < MEDIA_SVC_PATHNAME_SIZE ? new_title_len : MEDIA_SVC_PATHNAME_SIZE-1);

	SAFE_FREE(filename);

	media_svc_debug("extract title is [%s]", title);

	return title;
}

int _media_svc_rename_file( const char *old_name, const char *new_name)
{
	if((old_name == NULL) || (new_name == NULL))
	{
		media_svc_error("invalid file name");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	if (rename(old_name, new_name) < 0) {
		media_svc_error("err : [%s]", strerror(errno));
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_remove_file(const char *path)
{
	int result = -1;

	result = remove(path);
	if (result == 0) {
		media_svc_debug("success to remove file");
		return MEDIA_INFO_ERROR_NONE;
	} else {
		media_svc_error("fail to remove file result errno = %s", strerror(errno));
		return MEDIA_INFO_ERROR_INTERNAL;
	}
}

int _media_svc_remove_all_files_in_dir(const char *dir_path)
{
	struct dirent *entry = NULL;
	struct stat st;
	char filename[MEDIA_SVC_PATHNAME_SIZE] = {0};
	DIR *dir = NULL;

	dir = opendir(dir_path);
	if (dir == NULL) {
		media_svc_error("%s is not exist", dir_path);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		snprintf(filename, sizeof(filename), "%s/%s", dir_path, entry->d_name);

		if (stat(filename, &st) != 0) {
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			continue;
		}
		if (unlink(filename) != 0) {
			media_svc_error("failed to remove : %s", strerror(errno));
			closedir(dir);
			return MEDIA_INFO_ERROR_INTERNAL;
		}
	}

	closedir(dir);
	return MEDIA_INFO_ERROR_NONE;
}

static int __media_svc_check_thumb_dir(char *thumb_dir) {
	int ret = 0;
	DIR *dir = NULL;

	dir = opendir(thumb_dir);
	if (dir != NULL) {
		closedir(dir);
	} else {
		media_svc_error("opendir fail [%s]", strerror(errno));
		if (errno == ENOENT) {
			media_svc_error("[%s] is not exit. So, make it", thumb_dir);
			ret = mkdir(thumb_dir, 0777);
			if (ret < 0) {
				media_svc_error("make fail");
				goto ERROR;
			}
		} else {
			goto ERROR;
		}

		ret = chmod(thumb_dir, 0777);
		if (ret != 0) {
			media_svc_error("chmod failed [%s]", strerror(errno));
		}
		ret = chown(thumb_dir, 5000, 5000);
		if (ret != 0) {
			media_svc_error("chown failed [%s]", strerror(errno));
		}
	}

	return MEDIA_INFO_ERROR_NONE;

ERROR:
	return -1;
}

int _media_svc_get_thumbnail_path(media_svc_storage_type_e storage_type, char *thumb_path, const char *pathname, const char *img_format)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	char savename[MEDIA_SVC_PATHNAME_SIZE] = {0};
	char file_ext[MEDIA_SVC_FILE_EXT_LEN_MAX + 1] = {0};
	char *thumb_dir = NULL;
	char hash[255 + 1];
	char *thumbfile_ext = NULL;

	thumb_dir = (storage_type == MEDIA_SVC_STORAGE_INTERNAL) ? MEDIA_SVC_THUMB_INTERNAL_PATH : MEDIA_SVC_THUMB_EXTERNAL_PATH;

	ret = __media_svc_check_thumb_dir(thumb_dir);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("__media_svc_check_thumb_dir");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	memset(file_ext, 0, sizeof(file_ext));
	if (!__media_svc_get_file_ext(pathname, file_ext)) {
		media_svc_error("get file ext fail");
	}

	ret = mb_svc_generate_hash_code(pathname, hash, sizeof(hash));
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("mb_svc_generate_hash_code failed : %d", ret);
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	//media_svc_debug("img format is [%s]", img_format);

	if((strstr(img_format, "jpeg") != NULL) ||(strstr(img_format, "jpg") != NULL) ||(strstr(img_format, "JPG") != NULL)) {
		thumbfile_ext = "jpg";
	} else if((strstr(img_format, "png") != NULL) ||(strstr(img_format, "PNG") != NULL)) {
		thumbfile_ext = "png";
	} else if((strstr(img_format, "gif") != NULL) ||(strstr(img_format, "GIF") != NULL)) {
		thumbfile_ext = "gif";
	} else if((strstr(img_format, "bmp") != NULL) ||(strstr(img_format, "BMP") != NULL)) {
		thumbfile_ext = "bmp";
	} else {
		media_svc_error("Not proper img format");
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	snprintf(savename, sizeof(savename), "%s/.%s-%s.%s", thumb_dir, file_ext, hash, thumbfile_ext);
	_strncpy_safe(thumb_path, savename, MEDIA_SVC_PATHNAME_SIZE);
	//media_svc_debug("thumb_path is [%s]", thumb_path);

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_get_file_time(const char *full_path)
{
	struct stat statbuf;
	int fd = 0;

	memset(&statbuf, 0, sizeof(struct stat));
	fd = stat(full_path, &statbuf);
	if (fd == -1) {
		 media_svc_error("stat(%s) fails.", full_path);
		 return MEDIA_INFO_ERROR_INTERNAL;
	 }

	 return statbuf.st_mtime;
}

int _media_svc_set_media_info(media_svc_content_info_s *content_info, media_svc_storage_type_e storage_type,
			  const char *path, media_svc_media_type_e *media_type, bool refresh)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	char * media_uuid = NULL;
	char * file_name = NULL;
	struct stat st;
	bool drm_type = false;
	char mime_type[256] = {0};

	ret = __media_svc_malloc_and_strncpy(&content_info->path, path);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	memset(&st, 0, sizeof(struct stat));
	if (stat(path, &st) == 0) {
		content_info->modified_time = st.st_mtime;
		content_info->timeline = content_info->modified_time;
		content_info->size = st.st_size;
		//media_svc_debug("Modified time : [%d] Size : [%lld]", content_info->modified_time, content_info->size);
	} else {
		media_svc_error("stat failed : %s", strerror(errno));
	}

	/* Set default GPS value before extracting meta information */
	content_info->media_meta.longitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
	content_info->media_meta.latitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
	content_info->media_meta.altitude = MEDIA_SVC_DEFAULT_GPS_VALUE;

	/* Set filename to title for all media */
	char *title = NULL;
	title = _media_svc_get_title_from_filepath(content_info->path);
	if (title) {
		ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, title);
		if(ret != MEDIA_INFO_ERROR_NONE)
			media_svc_error("strcpy error");
		SAFE_FREE(title);
	} else {
		media_svc_error("Can't extract title");
		ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, MEDIA_SVC_TAG_UNKNOWN);
		media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);
	}

	/* Set default value before extracting meta information */
	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.description, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.copyright, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.track_num, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.album, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.artist, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.album_artist, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.genre, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.composer, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.year, MEDIA_SVC_TAG_UNKNOWN);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	/* refresh is TRUE when file modified. so only modified_time and size are changed*/
	if(refresh) {
		media_svc_debug("refresh");
		return MEDIA_INFO_ERROR_NONE;
	}

	content_info->storage_type = storage_type;
	time(&content_info->added_time);

	media_uuid = _media_info_generate_uuid();
	if(media_uuid == NULL) {
		_media_svc_destroy_content_info(content_info);
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	ret = __media_svc_malloc_and_strncpy(&content_info->media_uuid, media_uuid);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	file_name = g_path_get_basename(path);
	ret = __media_svc_malloc_and_strncpy(&content_info->file_name, file_name);
	SAFE_FREE(file_name);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	/* if the file is DRM file, drm_type value is DRM_TRUE(1).
	if drm_contentinfo is not NULL, the file is OMA DRM.*/
	ret = __media_svc_get_mime_type(path, mime_type);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	media_svc_debug("mime [%s]", mime_type);
	content_info->is_drm = drm_type;

	ret = __media_svc_get_media_type(path, mime_type, media_type);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	if ((*media_type < MEDIA_SVC_MEDIA_TYPE_IMAGE) || (*media_type > MEDIA_SVC_MEDIA_TYPE_OTHER)) {
		media_svc_error("invalid media_type condition[%d]", *media_type);
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	ret = __media_svc_malloc_and_strncpy(&content_info->mime_type, mime_type);
	media_svc_retv_del_if(ret != MEDIA_INFO_ERROR_NONE, ret, content_info);

	media_svc_sec_debug("storage[%d], path[%s], media_type[%d]", storage_type, path, *media_type);

	content_info->media_type = *media_type;

	content_info->played_count = 0;
	content_info->last_played_time= 0;
	content_info->last_played_position= 0;
	content_info->favourate= 0;
	content_info->media_meta.rating = 0;

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_extract_image_metadata(sqlite3 *handle, media_svc_content_info_s *content_info, media_svc_media_type_e media_type)
{
	if (content_info == NULL || media_type != MEDIA_SVC_MEDIA_TYPE_IMAGE) {
		media_svc_error("content_info == NULL || media_type != MEDIA_SVC_MEDIA_TYPE_IMAGE");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	char buf[MEDIA_SVC_METADATA_LEN_MAX + 1] = { '\0' };
	char description_buf[MEDIA_SVC_METADATA_DESCRIPTION_MAX + 1] = { '\0' };
	memset(buf, 0x00, sizeof(buf));
	memset(description_buf, 0x00, sizeof(description_buf));

	int ret = MEDIA_INFO_ERROR_NONE;
	double value = 0.0;
	int orient_value = 0;
	int exif_width = 0;
	int exif_height = 0;
	ExifData *ed = NULL;
	int has_datetaken = FALSE;
	int datetaken_size = 19;

	char *path = content_info->path;
	if (!STRING_VALID(path)) {
		media_svc_error("Invalid Path");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	/* Load an ExifData object from an EXIF file */
	ed = exif_data_new_from_file(path);

	if (!ed) {
		media_svc_sec_debug("There is no exif data in [ %s ]", path);
	}

	if (__media_svc_get_exif_info(ed, buf, NULL, NULL, EXIF_IFD_0, EXIF_TAG_GPS_LATITUDE_REF) == MEDIA_INFO_ERROR_NONE) {
		if (strlen(buf) != 0) {
			if (__media_svc_get_exif_info(ed, NULL, NULL, &value, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE) == MEDIA_INFO_ERROR_NONE) {

				if (strcmp(buf, "S") == 0) {
					value = -1 * value;
				}

				content_info->media_meta.latitude = value;
			} else {
				content_info->media_meta.latitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
			}
		} else {
			content_info->media_meta.latitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
		}
	} else {
		content_info->media_meta.latitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
	}

	memset(buf, 0x00, sizeof(buf));

	if (__media_svc_get_exif_info(ed, buf, NULL, NULL, EXIF_IFD_0, EXIF_TAG_GPS_LONGITUDE_REF) == MEDIA_INFO_ERROR_NONE) {
		if (strlen(buf) != 0) {
			if (__media_svc_get_exif_info(ed, NULL, NULL, &value, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE) == MEDIA_INFO_ERROR_NONE) {
				if (strcmp(buf, "W") == 0) {
					value = -1 * value;
				}
				content_info->media_meta.longitude = value;
			} else {
				content_info->media_meta.longitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
			}
		} else {
			content_info->media_meta.longitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
		}
	} else {
		content_info->media_meta.longitude = MEDIA_SVC_DEFAULT_GPS_VALUE;
	}

	memset(buf, 0x00, sizeof(buf));

	if (__media_svc_get_exif_info(ed, description_buf, NULL, NULL, EXIF_IFD_0, EXIF_TAG_IMAGE_DESCRIPTION) == MEDIA_INFO_ERROR_NONE) {
		if (strlen(description_buf) == 0) {
			//media_svc_debug("Use 'No description'");
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.description, MEDIA_SVC_TAG_UNKNOWN);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
		} else {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.description, description_buf);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
		}
	} else {
		ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.description, MEDIA_SVC_TAG_UNKNOWN);
		if(ret != MEDIA_INFO_ERROR_NONE)
			media_svc_error("strcpy error");
	}

	memset(buf, 0x00, sizeof(buf));

	if (!has_datetaken && __media_svc_get_exif_info(ed, buf, NULL, NULL, EXIF_IFD_0, EXIF_TAG_DATE_TIME_ORIGINAL) == MEDIA_INFO_ERROR_NONE) {
		if (strlen(buf) == 0) {
			//media_svc_debug("time  is NULL");
		} else {
#if 0
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.datetaken, buf);
#else
			ret = __media_svc_malloc_and_strncpy_with_size(&content_info->media_meta.datetaken, buf, datetaken_size);
#endif
			if(ret != MEDIA_INFO_ERROR_NONE) {
				media_svc_error("strcpy error");
			} else {
				has_datetaken = TRUE;
				/* This is same as recorded_date */
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.recorded_date, buf);
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");
			}
		}
	}

	memset(buf, 0x00, sizeof(buf));

	if (!has_datetaken && __media_svc_get_exif_info(ed, buf, NULL, NULL, EXIF_IFD_0, EXIF_TAG_DATE_TIME) == MEDIA_INFO_ERROR_NONE) {
		if (strlen(buf) == 0) {
			//media_svc_debug("time  is NULL");
		} else {
#if 0
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.datetaken, buf);
#else
			ret = __media_svc_malloc_and_strncpy_with_size(&content_info->media_meta.datetaken, buf, datetaken_size);
#endif
			if(ret != MEDIA_INFO_ERROR_NONE) {
				media_svc_error("strcpy error");
			} else {
				has_datetaken = TRUE;
				/* This is same as recorded_date */
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.recorded_date, buf);
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");
			}
		}
	}

	if (has_datetaken) {
		content_info->timeline = __media_svc_get_timeline_from_str(content_info->media_meta.datetaken);
		if (content_info->timeline == 0) {
			content_info->timeline = content_info->modified_time;
		} else {
			media_svc_debug("Timeline : %ld", content_info->timeline);
		}
	}

	/* Get orientation value from exif. */
	if (__media_svc_get_exif_info(ed, NULL, &orient_value, NULL, EXIF_IFD_0, EXIF_TAG_ORIENTATION) == MEDIA_INFO_ERROR_NONE) {
		if (orient_value >= NOT_AVAILABLE && orient_value <= ROT_270) {
			content_info->media_meta.orientation = orient_value;
		} else {
			content_info->media_meta.orientation = 0;
		}
	} else {
		content_info->media_meta.orientation = 0;
	}

	/* Get width value from exif. */
	if (__media_svc_get_exif_info(ed, NULL, &exif_width, NULL, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_X_DIMENSION) == MEDIA_INFO_ERROR_NONE) {
		if (exif_width > 0) {
			content_info->media_meta.width = exif_width;
		} else {
			content_info->media_meta.width = 0;
		}
	} else {
		content_info->media_meta.width = 0;
	}

	/* Get height value from exif. */
	if (__media_svc_get_exif_info(ed, NULL, &exif_height, NULL, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_Y_DIMENSION) == MEDIA_INFO_ERROR_NONE) {
		if (exif_height > 0) {
			content_info->media_meta.height = exif_height;
		} else {
			content_info->media_meta.height = 0;
		}
	} else {
		content_info->media_meta.height = 0;
	}

	if (ed != NULL) exif_data_unref(ed);

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_extract_music_metadata_for_update(sqlite3 *handle, media_svc_content_info_s *content_info, media_svc_media_type_e media_type)
{
	MMHandleType tag = 0;
	char *p = NULL;
	int size = -1;
	int extracted_field = MEDIA_SVC_EXTRACTED_FIELD_NONE;
	int mmf_error = MM_ERROR_NONE;
	char *err_attr_name = NULL;
	char *title = NULL;
	int album_id = 0;
	int ret = MEDIA_INFO_ERROR_NONE;

	/*Get Content Tag attribute ===========*/
	mmf_error = mm_file_create_tag_attrs(&tag, content_info->path);

	if (mmf_error == MM_ERROR_NONE) {
		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ALBUM, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_ALBUM)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.album, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");

			//media_svc_debug("album[%d] : %s", size, content_info->media_meta.album);
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("album - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ARTIST, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_ARTIST)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.artist, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			//media_svc_debug("artist[%d] : %s", size, content_info->media_meta.artist);
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("artist - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ALBUM_ARTIST, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_ALBUM_ARTIST)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.album_artist, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			//media_svc_debug("album_artist[%d] : %s", size, content_info->media_meta.album_artist);
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("album_artist - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_GENRE, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_GENRE)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.genre, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");

			//media_svc_debug("genre : %s", content_info->media_meta.genre);
			/* If genre is Ringtone, it's categorized as sound. But this logic is commented */
			/*
			if ((strcasecmp("Ringtone", p) == 0) | (strcasecmp("Alert tone", p) == 0)) {
				content_info->media_type = MEDIA_SVC_MEDIA_TYPE_SOUND;
			}
			*/
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("genre - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_TITLE, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_TITLE)) && (mmf_error == MM_ERROR_NONE) && (size > 0)/* && 	(!isspace(*p))*/) {
			if(!isspace(*p)) {
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, p);
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");

				extracted_field |= MEDIA_SVC_EXTRACTED_FIELD_TITLE;
			}
			else {
				int idx = 0;

				for(idx = 0; idx < size; idx++) {
					if(isspace(*p)) {
						media_svc_debug("SPACE [%s]", p);
						p++;
						continue;
					} else {
						media_svc_debug("Not SPACE [%s]", p);
						ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, p);
						if(ret != MEDIA_INFO_ERROR_NONE)
							media_svc_error("strcpy error");
						break;
					}
				}

				if(idx == size)
				{
					media_svc_debug("Can't extract title. All string is space");
					title = _media_svc_get_title_from_filepath(content_info->path);
					if (title) {
						ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, title);
						if(ret != MEDIA_INFO_ERROR_NONE)
							media_svc_error("strcpy error");
						SAFE_FREE(title);
					} else {
						media_svc_error("Can't extract title");
					}
				}
			}
		} else {
			SAFE_FREE(err_attr_name);
			title = _media_svc_get_title_from_filepath(content_info->path);
			if (title) {
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, title);
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");
				SAFE_FREE(title);
			} else {
				media_svc_error("Can't extract title");
			}
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_DESCRIPTION, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_DESC)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.description, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			//media_svc_debug("desc : %s", content_info->media_meta.description);
		} else {
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_AUTHOR, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_AUTHOR)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.composer, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			extracted_field |= MEDIA_SVC_EXTRACTED_FIELD_AUTHOR;
			//media_svc_debug("extract composer from content : %s", content_info->media_meta.composer);
		} else {
			//media_svc_debug("composer - unknown");
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_COPYRIGHT, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_COPYRIGHT)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.copyright, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			extracted_field |= MEDIA_SVC_EXTRACTED_FIELD_AUTHOR;
			//media_svc_debug("extract copyright from content : %s", content_info->media_meta.copyright);
		} else {
			//media_svc_debug("copyright - unknown");
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_destroy_tag_attrs(tag);
		if (mmf_error != MM_ERROR_NONE) {
			media_svc_error("fail to free tag attr - err(%x)", mmf_error);
		}
	}	else {
		/* in case of file size 0, MMFW Can't parsting tag info but add it to Music DB. */
		char *title = NULL;
		media_svc_error("no tag information");

		title = _media_svc_get_title_from_filepath(content_info->path);
		if (title) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, title);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			SAFE_FREE(title);
		} else {
			media_svc_error("Can't extract title");
		}

		content_info->album_id = album_id;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_extract_media_metadata(sqlite3 *handle, media_svc_content_info_s *content_info, media_svc_media_type_e media_type)
{
	MMHandleType content = 0;
	MMHandleType tag = 0;
	char *p = NULL;
	void *image = NULL;
	int size = -1;
	int extracted_field = MEDIA_SVC_EXTRACTED_FIELD_NONE;
	int mmf_error = MM_ERROR_NONE;
	bool thumb_extracted_from_drm = FALSE;
	char *err_attr_name = NULL;
	char *title = NULL;
	bool extract_thumbnail = FALSE;
	bool append_album = FALSE;
	int album_id = 0;
	int ret = MEDIA_INFO_ERROR_NONE;
	int cdis_value = 0;

	/*Get Content Tag attribute ===========*/
	mmf_error = mm_file_create_tag_attrs(&tag, content_info->path);

	if (mmf_error == MM_ERROR_NONE) {
		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ALBUM, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_ALBUM)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.album, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");

			//media_svc_debug("album[%d] : %s", size, content_info->media_meta.album);
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("album - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ARTIST, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_ARTIST)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.artist, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			//media_svc_debug("artist[%d] : %s", size, content_info->media_meta.artist);
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("artist - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ALBUM_ARTIST, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_ALBUM_ARTIST)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.album_artist, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			//media_svc_debug("album_artist[%d] : %s", size, content_info->media_meta.album_artist);
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("album_artist - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_GENRE, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_GENRE)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.genre, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");

			//media_svc_debug("genre : %s", content_info->media_meta.genre);
			/* If genre is Ringtone, it's categorized as sound. But this logic is commented */
			/*
			if ((strcasecmp("Ringtone", p) == 0) | (strcasecmp("Alert tone", p) == 0)) {
				content_info->media_type = MEDIA_SVC_MEDIA_TYPE_SOUND;
			}
			*/
		} else {
			SAFE_FREE(err_attr_name);
			//media_svc_debug("genre - unknown");
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_TITLE, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_TITLE)) && (mmf_error == MM_ERROR_NONE) && (size > 0)/* && 	(!isspace(*p))*/) {
			if(!isspace(*p)) {
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, p);
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");

				extracted_field |= MEDIA_SVC_EXTRACTED_FIELD_TITLE;
			}
			else {
				int idx = 0;

				for(idx = 0; idx < size; idx++) {
					if(isspace(*p)) {
						media_svc_debug("SPACE [%s]", p);
						p++;
						continue;
					} else {
						media_svc_debug("Not SPACE [%s]", p);
						ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, p);
						if(ret != MEDIA_INFO_ERROR_NONE)
							media_svc_error("strcpy error");
						break;
					}
				}

				if(idx == size)
				{
					media_svc_debug("Can't extract title. All string is space");
					title = _media_svc_get_title_from_filepath(content_info->path);
					if (title) {
						ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, title);
						if(ret != MEDIA_INFO_ERROR_NONE)
							media_svc_error("strcpy error");
						SAFE_FREE(title);
					} else {
						media_svc_error("Can't extract title");
					}
				}
			}
		} else {
			SAFE_FREE(err_attr_name);
			title = _media_svc_get_title_from_filepath(content_info->path);
			if (title) {
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, title);
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");
				SAFE_FREE(title);
			} else {
				media_svc_error("Can't extract title");
			}
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_DESCRIPTION, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_DESC)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.description, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			//media_svc_debug("desc : %s", content_info->media_meta.description);
		} else {
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_RECDATE, &p, &size, NULL);
		if ((mmf_error == MM_ERROR_NONE) && (size > 0)) {
			if(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
				/*Creation time format is 2013-01-01 00:00:00. change it to 2013:01:01 00:00:00 like exif time format*/
				char time_info[64] = {0, };
				char p_value[64] = {0, };
				int idx = 0;
				memset(time_info, 0x00, sizeof(time_info));
				memset(p_value, 0x00, sizeof(p_value));
				strncpy(p_value, p, size);
				for(idx =0; idx < size; idx++) {
					if(p_value[idx] == '-') {
						time_info[idx] = ':';
					} else if(p_value[idx] != '\0') {
						time_info[idx] = p_value[idx];
					} else {
						media_svc_error("strcpy error");
						break;
					}
				}
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.recorded_date, time_info);
			} else {
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.recorded_date, p);
			}

			if(ret != MEDIA_INFO_ERROR_NONE) {
				media_svc_error("strcpy error");
			} else {
				/* This is same as datetaken */
#if 0
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.datetaken, content_info->media_meta.recorded_date);
#else
				int datetaken_size = 19;
				ret = __media_svc_malloc_and_strncpy_with_size(&content_info->media_meta.datetaken, content_info->media_meta.recorded_date, datetaken_size);
#endif
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");

				content_info->timeline = __media_svc_get_timeline_from_str(content_info->media_meta.recorded_date);
				if (content_info->timeline == 0) {
					content_info->timeline = content_info->modified_time;
				} else {
					media_svc_debug("Timeline : %ld", content_info->timeline);
				}
			}
			//media_svc_debug("Recorded date : %s", content_info->media_meta.recorded_date);
		} else {
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_AUTHOR, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_AUTHOR)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.composer, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			extracted_field |= MEDIA_SVC_EXTRACTED_FIELD_AUTHOR;
			//media_svc_debug("extract composer from content : %s", content_info->media_meta.composer);
		} else {
			//media_svc_debug("composer - unknown");
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_COPYRIGHT, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_COPYRIGHT)) && (mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.copyright, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			extracted_field |= MEDIA_SVC_EXTRACTED_FIELD_AUTHOR;
			//media_svc_debug("extract copyright from content : %s", content_info->media_meta.copyright);
		} else {
			//media_svc_debug("copyright - unknown");
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_TRACK_NUM, &p, &size, NULL);
		if ((mmf_error == MM_ERROR_NONE) && (size > 0)) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.track_num, p);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
		} else {
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_DATE, &p, &size, NULL);
		if ((!(extracted_field & MEDIA_SVC_EXTRACTED_FIELD_YEAR)) && (mmf_error == MM_ERROR_NONE) && (size == 4)) {
			int year = 0;
			if((p != NULL) && (sscanf( p, "%d", &year))) {
				ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.year, p);
				if(ret != MEDIA_INFO_ERROR_NONE)
					media_svc_error("strcpy error");
			} else {
				media_svc_debug("Wrong Year Information [%s]", p);
			}
		} else {
			SAFE_FREE(err_attr_name);
		}

		mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_RATING, &p, &size, NULL);
		if ((mmf_error == MM_ERROR_NONE) && (size > 0)) {
			content_info->media_meta.rating = atoi(p);
		} else {
			SAFE_FREE(err_attr_name);
			content_info->media_meta.rating = 0;
		}

		/*Initialize album_id to 0. below code will set the album_id*/
		content_info->album_id = album_id;
		ret = _media_svc_get_album_id(handle, content_info->media_meta.album, content_info->media_meta.artist, &album_id);

		if (ret != MEDIA_INFO_ERROR_NONE) {
			if (ret == MEDIA_INFO_ERROR_DATABASE_NO_RECORD) {
				media_svc_debug("album does not exist. So start to make album art");
				extract_thumbnail = TRUE;
				append_album = TRUE;
			} else {
				extract_thumbnail = TRUE;
				append_album = FALSE;
			}
		} else {
			content_info->album_id = album_id;
			append_album = FALSE;

			if((!strncmp(content_info->media_meta.album, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN))) ||
				(!strncmp(content_info->media_meta.artist, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN)))) {

				media_svc_debug("Unknown album or artist already exists. Extract thumbnail for Unknown.");
				extract_thumbnail = TRUE;
			} else {
				media_svc_debug("album already exists. don't need to make album art");
				ret = _media_svc_get_album_art_by_album_id(handle, album_id, &content_info->thumbnail_path);
				media_svc_retv_del_if((ret != MEDIA_INFO_ERROR_NONE) && (ret != MEDIA_INFO_ERROR_DATABASE_NO_RECORD), ret, content_info);
				extract_thumbnail = TRUE;
			}
		}

		if ((!thumb_extracted_from_drm) && (extract_thumbnail == TRUE)) {
			mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ARTWORK, &image, &size, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				media_svc_error("fail to get tag artwork - err(%x)", mmf_error);
				SAFE_FREE(err_attr_name);
			} else {
				//media_svc_debug("artwork size1 [%d]", size);
			}

			mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ARTWORK_SIZE, &size, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				media_svc_error("fail to get artwork size - err(%x)", mmf_error);
				SAFE_FREE(err_attr_name);
			} else {
				//media_svc_debug("artwork size2 [%d]", size);
			}
			if (image != NULL && size > 0) {
				int ret = MEDIA_INFO_ERROR_NONE;
				char thumb_path[MEDIA_SVC_PATHNAME_SIZE] = "\0";
				int artwork_mime_size = -1;

				mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_ARTWORK_MIME, &p, &artwork_mime_size, NULL);
				if ((mmf_error == MM_ERROR_NONE) && (artwork_mime_size > 0)) {
					ret = _media_svc_get_thumbnail_path(content_info->storage_type, thumb_path, content_info->path, p);
					if (ret != MEDIA_INFO_ERROR_NONE) {
						media_svc_error("Fail to Get Thumbnail Path");
					}
				} else {
					SAFE_FREE(err_attr_name);
				}

				if(strlen(thumb_path) > 0)
				{
					ret = _media_svc_save_image(image, size, thumb_path);
					if (ret != MEDIA_INFO_ERROR_NONE) {
						media_svc_error("Fail to Save Thumbnail Image");
					}
					else {
						ret = __media_svc_malloc_and_strncpy(&content_info->thumbnail_path, thumb_path);
						if(ret != MEDIA_INFO_ERROR_NONE)
							media_svc_error("strcpy error");
					}
				}
			}
		}

		if(append_album == TRUE) {
			if((strncmp(content_info->media_meta.album, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN))) &&
				(strncmp(content_info->media_meta.artist, MEDIA_SVC_TAG_UNKNOWN, strlen(MEDIA_SVC_TAG_UNKNOWN))))
				ret = _media_svc_append_album(handle, content_info->media_meta.album, content_info->media_meta.artist, content_info->thumbnail_path, &album_id);
			else
				ret = _media_svc_append_album(handle, content_info->media_meta.album, content_info->media_meta.artist, NULL, &album_id);

			content_info->album_id = album_id;
		}

		if(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO) {
			double longitude = 0.0;
			double latitude = 0.0;
			double altitude = 0.0;

			__media_svc_get_location_value(tag, &longitude, &latitude, &altitude);
			content_info->media_meta.longitude = longitude;
			content_info->media_meta.latitude = latitude;
			content_info->media_meta.altitude = altitude;

			mmf_error = mm_file_get_attrs(tag, &err_attr_name, MM_FILE_TAG_CDIS, &cdis_value, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				cdis_value = 0;
				SAFE_FREE(err_attr_name);
			}

			media_svc_debug("CDIS : %d", cdis_value);
		}

		mmf_error = mm_file_destroy_tag_attrs(tag);
		if (mmf_error != MM_ERROR_NONE) {
			media_svc_error("fail to free tag attr - err(%x)", mmf_error);
		}
	}	else {
		/* in case of file size 0, MMFW Can't parsting tag info but add it to Music DB. */
		char *title = NULL;
		media_svc_error("no tag information");

		title = _media_svc_get_title_from_filepath(content_info->path);
		if (title) {
			ret = __media_svc_malloc_and_strncpy(&content_info->media_meta.title, title);
			if(ret != MEDIA_INFO_ERROR_NONE)
				media_svc_error("strcpy error");
			SAFE_FREE(title);
		} else {
			media_svc_error("Can't extract title");
		}

		content_info->album_id = album_id;
	}

	/*Get Content attribute ===========*/
	if (cdis_value == 1) {
		mmf_error = mm_file_create_content_attrs_safe(&content, content_info->path);
	} else {
		mmf_error = mm_file_create_content_attrs(&content, content_info->path);
	}

	if (mmf_error == MM_ERROR_NONE) {
		/*Common attribute*/
		mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_DURATION, &content_info->media_meta.duration, NULL);
		if (mmf_error != MM_ERROR_NONE) {
			SAFE_FREE(err_attr_name);
			media_svc_debug("fail to get duration attr - err(%x)", mmf_error);
		} else {
			//media_svc_debug("duration : %d", content_info->media_meta.duration);
		}

		/*Sound/Music attribute*/
		if((media_type == MEDIA_SVC_MEDIA_TYPE_SOUND) || (media_type == MEDIA_SVC_MEDIA_TYPE_MUSIC)) {

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_AUDIO_BITRATE, &content_info->media_meta.bitrate, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get audio bitrate attr - err(%x)", mmf_error);
			} else {
				//media_svc_debug("bit rate : %d", content_info->media_meta.bitrate);
			}

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_AUDIO_SAMPLERATE, &content_info->media_meta.samplerate, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get sample rate attr - err(%x)", mmf_error);
			} else {
				//media_svc_debug("sample rate : %d", content_info->media_meta.samplerate);
			}

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_AUDIO_CHANNELS, &content_info->media_meta.channel, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get audio channels attr - err(%x)", mmf_error);
			} else {
				//media_svc_debug("channel : %d", content_info->media_meta.channel);
			}

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_AUDIO_BITPERSAMPLE, &content_info->media_meta.bitpersample, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get audio bit per sample attr - err(%x)", mmf_error);
			} else {
				media_svc_debug("bitpersample : %d", content_info->media_meta.bitpersample);
			}
		}else if(media_type == MEDIA_SVC_MEDIA_TYPE_VIDEO)	{	/*Video attribute*/
			int audio_bitrate = 0;
			int video_bitrate = 0;

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_AUDIO_BITRATE, &audio_bitrate, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get audio bitrate attr - err(%x)", mmf_error);
			} else {
				//media_svc_debug("audio bit rate : %d", audio_bitrate);
			}

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_VIDEO_BITRATE, &video_bitrate, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get audio bitrate attr - err(%x)", mmf_error);
			} else {
				//media_svc_debug("video bit rate : %d", video_bitrate);
			}

			content_info->media_meta.bitrate = audio_bitrate + video_bitrate;

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_VIDEO_WIDTH, &content_info->media_meta.width, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get video width attr - err(%x)", mmf_error);
			} else {
				//media_svc_debug("width : %d", content_info->media_meta.width);
			}

			mmf_error = mm_file_get_attrs(content, &err_attr_name, MM_FILE_CONTENT_VIDEO_HEIGHT, &content_info->media_meta.height, NULL);
			if (mmf_error != MM_ERROR_NONE) {
				SAFE_FREE(err_attr_name);
				media_svc_debug("fail to get video height attr - err(%x)", mmf_error);
			} else {
				//media_svc_debug("height : %d", content_info->media_meta.height);
			}

		} else {
			media_svc_error("Not support type");
			mmf_error = mm_file_destroy_content_attrs(content);
			if (mmf_error != MM_ERROR_NONE) {
				media_svc_error("fail to free content attr - err(%x)", mmf_error);
			}
			return MEDIA_INFO_ERROR_INVALID_PARAMETER;
		}

		mmf_error = mm_file_destroy_content_attrs(content);
		if (mmf_error != MM_ERROR_NONE) {
			media_svc_error("fail to free content attr - err(%x)", mmf_error);
		}
	} else {
		media_svc_error("error in mm_file_create_content_attrs [%d]", mmf_error);
	}

	return MEDIA_INFO_ERROR_NONE;
}

void _media_svc_destroy_content_info(media_svc_content_info_s *content_info)
{
	media_svc_retm_if(content_info == NULL, "content info is NULL");

	/* Delete media_svc_content_info_s */
	SAFE_FREE(content_info->media_uuid);
	SAFE_FREE(content_info->path);
	SAFE_FREE(content_info->file_name);
	SAFE_FREE(content_info->mime_type);
	SAFE_FREE(content_info->folder_uuid);
	SAFE_FREE(content_info->thumbnail_path);

	/* Delete media_svc_content_meta_s */
	SAFE_FREE(content_info->media_meta.title);
	SAFE_FREE(content_info->media_meta.album);
	SAFE_FREE(content_info->media_meta.artist);
	SAFE_FREE(content_info->media_meta.album_artist);
	SAFE_FREE(content_info->media_meta.genre);
	SAFE_FREE(content_info->media_meta.composer);
	SAFE_FREE(content_info->media_meta.year);
	SAFE_FREE(content_info->media_meta.recorded_date);
	SAFE_FREE(content_info->media_meta.copyright);
	SAFE_FREE(content_info->media_meta.track_num);
	SAFE_FREE(content_info->media_meta.description);
	SAFE_FREE(content_info->media_meta.datetaken);
	SAFE_FREE(content_info->media_meta.weather);

	SAFE_FREE(content_info->media_meta.title_pinyin);
	SAFE_FREE(content_info->media_meta.album_pinyin);
	SAFE_FREE(content_info->media_meta.artist_pinyin);
	SAFE_FREE(content_info->media_meta.album_artist_pinyin);
	SAFE_FREE(content_info->media_meta.genre_pinyin);
	SAFE_FREE(content_info->media_meta.composer_pinyin);
	SAFE_FREE(content_info->media_meta.copyright_pinyin);
	SAFE_FREE(content_info->media_meta.description_pinyin);

	return;
}

int _media_svc_get_store_type_by_path(const char *path, media_svc_storage_type_e *storage_type)
{
	if(STRING_VALID(path))
	{
		if(strncmp(path, MEDIA_ROOT_PATH_INTERNAL, strlen(MEDIA_ROOT_PATH_INTERNAL)) == 0)
		{
			*storage_type = MEDIA_SVC_STORAGE_INTERNAL;
		}
		else if(strncmp (path, MEDIA_ROOT_PATH_SDCARD, strlen(MEDIA_ROOT_PATH_SDCARD)) == 0)
		{
			*storage_type = MEDIA_SVC_STORAGE_EXTERNAL;
		}
		else
		{
			media_svc_error("Invalid Path");
			return MEDIA_INFO_ERROR_INVALID_PARAMETER;
		}
	}
	else
	{
		media_svc_error("INVALID parameter");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	return MEDIA_INFO_ERROR_NONE;
}

char *_media_svc_replace_path(char *s, const char *olds, const char *news)
{
  char *result, *sr;
  size_t i, count = 0;
  size_t oldlen = strlen(olds); if (oldlen < 1) return s;
  size_t newlen = strlen(news);

  if (newlen != oldlen) {
    for (i = 0; s[i] != '\0';) {
      if (memcmp(&s[i], olds, oldlen) == 0) count++, i += oldlen;
      else i++;
    }   
  } else i = strlen(s);


  result = (char *) calloc(1, i + 1 + count * (newlen - oldlen));
  if (result == NULL) return NULL;

  sr = result;
  while (*s) {
    if (memcmp(s, olds, oldlen) == 0) {
      memcpy(sr, news, newlen);
      sr += newlen;
      s  += oldlen;
    } else *sr++ = *s++;
  }

  *sr = '\0';

  return result;
}

int _media_svc_error_convert(int error)
{
	if(error != MS_MEDIA_ERR_NONE)
		media_svc_error("error : [%d]", error);

	if(error == MS_MEDIA_ERR_NONE)							/*Error None*/
		return MEDIA_INFO_ERROR_NONE;
	else if(error == MS_MEDIA_ERR_DB_CONNECT_FAIL)			/*DB Connect Fail*/
		return MEDIA_INFO_ERROR_DATABASE_CONNECT;
	else if(error == MS_MEDIA_ERR_DB_DISCONNECT_FAIL)		/*DB Disconnect Fail*/
		return MEDIA_INFO_ERROR_DATABASE_DISCONNECT;
	else if(error == MS_MEDIA_ERR_SOCKET_CONN)				/*Socket Connect Fail*/
		return MEDIA_INFO_ERROR_SOCKET_CONN;
	else if(error == MS_MEDIA_ERR_INVALID_PARAMETER || error == MS_MEDIA_ERR_INVALID_PATH)
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	else if(error == MS_MEDIA_ERR_DB_CONSTRAINT_FAIL)
		return MEDIA_INFO_ERROR_DATABASE_CONSTRAINT;
	else if(error == MS_MEDIA_ERR_DB_BUSY_FAIL)
		return MEDIA_INFO_ERROR_DATABASE_BUSY;
	else if(error == MS_MEDIA_ERR_PERMISSION_DENIED)
		return MEDIA_INFO_ERROR_DATABASE_PERMISSION;

	return MEDIA_INFO_ERROR_INTERNAL;
}

bool _media_svc_is_drm_file(const char *path)
{
	return FALSE;
}

int _media_svc_request_thumbnail_with_origin_size(const char *path, char *thumb_path, int max_length, int *origin_width, int *origin_height)
{
	int ret = MEDIA_THUMB_ERROR_NONE;

	ret = thumbnail_request_from_db_with_size(path, thumb_path, max_length, origin_width, origin_height);

	if(ret != MEDIA_THUMB_ERROR_NONE)
	{
		media_svc_error("thumbnail_request_from_db failed: %d", ret);
		ret = MEDIA_INFO_ERROR_INTERNAL;
	}
	else
	{
		media_svc_sec_debug("thumbnail_request_from_db success: thumbnail path[%s]", thumb_path);
	}

	return ret;
}

int _media_svc_get_pinyin_str(const char *src_str, char **pinyin_str)
{
	int ret = MEDIA_INFO_ERROR_NONE;
	int size = 0;
	pinyin_name_s *pinyinname = NULL;
	char *lang = NULL;
	char *china_lang = "zh_CN";
	char *taiwan_lang = "zh_TW";
	char *hongkong_lang = "zh_HK";

	*pinyin_str = NULL;

	if(!STRING_VALID(src_str))
	{
		media_svc_debug("String is invalid");
		return ret;
	}

	lang = vconf_get_str(VCONFKEY_LANGSET);

	if (lang != NULL) {
		if (strncmp(lang, china_lang, strlen(china_lang)) == 0) {
			ret = _media_svc_convert_chinese_to_pinyin(src_str, &pinyinname, &size);
			if (ret == MEDIA_INFO_ERROR_NONE)
			{
				if(STRING_VALID(pinyinname[0].pinyin_name))
					*pinyin_str = strdup(pinyinname[0].pinyin_name);
				else
					*pinyin_str = strdup(src_str);	//Return Original Non China Character
			}

			_media_svc_pinyin_free(pinyinname, size);
		} else if (strncmp(lang, taiwan_lang, strlen(taiwan_lang)) == 0) {
			media_svc_bpmf_name_s *bpmf = NULL;

			ret = _media_svc_convert_chinese_to_bpmf(src_str, &bpmf);
			if (ret == MEDIA_INFO_ERROR_NONE && bpmf != NULL) {
				if(bpmf->bpmf_name != NULL) {
					*pinyin_str = strdup(bpmf->bpmf_name);
				}
				else
					*pinyin_str = NULL;	//Return Original Non China Character
			}

			_media_svc_bpmf_name_destroy(bpmf);
		} else if (strncmp(lang, hongkong_lang, strlen(hongkong_lang)) == 0) {
			char *sort_key = NULL;
			_media_svc_collation_str(src_str, &sort_key);
			*pinyin_str = strdup(sort_key);
			free(sort_key);
		}
	} else {
		ret = MEDIA_INFO_ERROR_INTERNAL;
	}

	SAFE_FREE(lang);

	return ret;
}

bool _media_svc_check_pinyin_support(void)
{
	/*Check CSC*/
	return TRUE;
}
