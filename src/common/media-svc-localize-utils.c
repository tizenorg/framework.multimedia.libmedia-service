/*
 * Media Service
 *
 * Copyright (c) 2010 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/ucol.h>

#include <vconf.h>

#include "media-svc-error.h"
#include "media-svc-debug.h"
#include "media-svc-localize-utils.h"

#define SAFE_STRLEN(src) ((src)?strlen(src):0)

int _media_svc_check_utf8(char c)
{
	if ((c & 0xff) < (128 & 0xff))
		return 1;
	else if ((c & (char)0xe0) == (char)0xc0)
		return 2;
	else if ((c & (char)0xf0) == (char)0xe0)
		return 3;
	else if ((c & (char)0xf8) == (char)0xf0)
		return 4;
	else if ((c & (char)0xfc) == (char)0xf8)
		return 5;
	else if ((c & (char)0xfe) == (char)0xfc)
		return 6;
	else
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
}

int SAFE_SNPRINTF(char **buf, int *buf_size, int len, const char *src)
{
	int remain;
	int temp_len;

	if (len < 0)
		return -1;

	remain = *buf_size - len;
	if (remain > strlen(src) + 1) {
		temp_len = snprintf((*buf)+len, remain, "%s", src);
		return temp_len;
	}
	else {
		char *temp;
		while(1) {
			temp = realloc(*buf, *buf_size*2);
			if (NULL == temp)
				return -1;
			*buf = temp;
			*buf_size = *buf_size * 2;
			remain = *buf_size - len;
			if (remain > strlen(src) + 1)
				break;
		}
		temp_len = snprintf((*buf)+len, remain, "%s", src);
		return temp_len;
	}
}

static int __media_svc_remove_special_char(const char *src, char *dest, int dest_size)
{
	int s_pos=0, d_pos=0, char_type, src_size;

	if (NULL == src) {
		media_svc_error("The parameter(src) is NULL");
		dest[d_pos] = '\0';
		return 0;
	}
	src_size = strlen(src);

	while (src[s_pos] != 0) {
		char_type = _media_svc_check_utf8(src[s_pos]);

		if (0 < char_type && char_type < dest_size - d_pos && char_type <= src_size - s_pos) {
			memcpy(dest+d_pos, src+s_pos, char_type);
			d_pos += char_type;
			s_pos += char_type;
		}
		else {
			media_svc_error("The parameter(src:%s) has invalid character set", src);
			dest[d_pos] = '\0';
			return MEDIA_INFO_ERROR_INVALID_PARAMETER;
		}
	}

	dest[d_pos] = '\0';
	return d_pos;
}

static inline int __media_svc_collation_str(const char *src, char **dest)
{
	int32_t size = 0;
	UErrorCode status = U_ZERO_ERROR;
	UChar *tmp_result = NULL;
	UCollator *collator;
	char region[50] = {0};
	char *lang = NULL;

	lang = vconf_get_str(VCONFKEY_LANGSET);
	if(lang != NULL) {
		if(strlen(lang) < 50) {
			strncpy (region, lang, strlen(lang));
			free(lang);
		} else {
			media_svc_error("Lang size error(%s)", lang);
			free(lang);
		}
	} else {
		strcpy(region, "en_US.UTF-8");
	}

	char *dot = strchr(region, '.');
	if (dot)
		*dot = '\0';

	collator = ucol_open(region, &status);

	media_svc_retvm_if(U_FAILURE(status), MEDIA_INFO_ERROR_INTERNAL,
			"ucol_open() Failed(%s)", u_errorName(status));

	u_strFromUTF8(NULL, 0, &size, src, strlen(src), &status);
	if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR) {
		media_svc_error("u_strFromUTF8 to get the dest length Failed(%s)", u_errorName(status));
		ucol_close(collator);
		return MEDIA_INFO_ERROR_INTERNAL;
	}
	status = U_ZERO_ERROR;
	tmp_result = calloc(1, sizeof(UChar) * (size + 1));
	u_strFromUTF8(tmp_result, size + 1, NULL, src, -1, &status);
	if (U_FAILURE(status)){
		media_svc_error("u_strFromUTF8 Failed(%s)", u_errorName(status));
		free(tmp_result);
		ucol_close(collator);
		return MEDIA_INFO_ERROR_INTERNAL;
	}

	size = ucol_getSortKey(collator, tmp_result, -1, NULL, 0);
	*dest = calloc(1, sizeof(uint8_t) * (size + 1));
	size = ucol_getSortKey(collator, tmp_result, -1, (uint8_t *)*dest, size + 1);

	ucol_close(collator);
	free(tmp_result);
	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_collation_str(const char *src, char **dest)
{
	int ret;
	char temp[SAFE_STRLEN(src) + 1];

	ret = __media_svc_remove_special_char(src, temp, sizeof(temp));
	media_svc_retvm_if(ret < MEDIA_INFO_ERROR_NONE, ret, "__ctsvc_remove_special_char() Failed(%d)", ret);

	return __media_svc_collation_str(temp, dest);
}

