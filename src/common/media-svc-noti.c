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
#include "media-svc-noti.h"
#include "media-svc-util.h"

static __thread media_svc_noti_item *g_inserted_noti_list = NULL;
static __thread int g_noti_from_pid = -1;

media_svc_noti_item *_media_svc_get_noti_list()
{
	return g_inserted_noti_list;
}

void _media_svc_set_noti_from_pid(int pid)
{
	g_noti_from_pid = pid;
}

int _media_svc_create_noti_list(int count)
{
	SAFE_FREE(g_inserted_noti_list);

	g_inserted_noti_list = calloc(count, sizeof(media_svc_noti_item));
	if (g_inserted_noti_list == NULL) {
		media_svc_error("Failed to prepare noti items");
		return MEDIA_INFO_ERROR_OUT_OF_MEMORY;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_insert_item_to_noti_list(media_svc_content_info_s *content_info, int cnt)
{
	media_svc_noti_item *noti_list = g_inserted_noti_list;

	if (noti_list && content_info) {
		noti_list[cnt].pid = g_noti_from_pid;
		noti_list[cnt].update_item = MS_MEDIA_ITEM_INSERT; // INSERT
		noti_list[cnt].update_type = MS_MEDIA_ITEM_FILE;
		noti_list[cnt].media_type = content_info->media_type;
		if (content_info->media_uuid)
			noti_list[cnt].media_uuid = strdup(content_info->media_uuid);
		if (content_info->path)
			noti_list[cnt].path = strdup(content_info->path);
		if (content_info->mime_type)
			noti_list[cnt].mime_type = strdup(content_info->mime_type);
	}

	return MEDIA_INFO_ERROR_NONE;
}


int _media_svc_destroy_noti_list(int all_cnt)
{
	int i = 0;
	media_svc_noti_item *noti_list = g_inserted_noti_list;

	if (noti_list) {
		for (i = 0; i < all_cnt; i++) {
			SAFE_FREE(noti_list[i].media_uuid);
			SAFE_FREE(noti_list[i].path);
			SAFE_FREE(noti_list[i].mime_type);
		}

		SAFE_FREE(g_inserted_noti_list);
		g_inserted_noti_list = NULL;
	}

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_publish_noti_list(int all_cnt)
{
	int err = MEDIA_INFO_ERROR_NONE;
	int i = 0;
	media_svc_noti_item *noti_list = g_inserted_noti_list;

	if (noti_list) {
		for (i = 0; i < all_cnt; i++) {
			err = _media_svc_publish_noti_by_item(&(noti_list[i]));
			if (err < 0) {
				media_svc_error("_media_svc_publish_noti failed : %d", err);
			}
		}
	}

	return err;
}


int _media_svc_create_noti_item(media_svc_content_info_s *content_info,
							int pid,
							media_item_type_e update_item,
							media_item_update_type_e update_type,
							media_svc_noti_item **item)
{
	media_svc_noti_item *_item = NULL;

	if (item == NULL || content_info == NULL) {
		media_svc_error("_media_svc_create_noti_item : invalid param");
		return MEDIA_INFO_ERROR_INVALID_PARAMETER;
	}

	_item = calloc(1, sizeof(media_svc_noti_item));

	if (_item == NULL) {
		media_svc_error("Failed to prepare noti items");
		return MEDIA_INFO_ERROR_OUT_OF_MEMORY;
	}

	_item->pid = pid;
	_item->update_item = update_item;
	_item->update_type = update_type;
	_item->media_type = content_info->media_type;

	if (content_info->media_uuid)
		_item->media_uuid = strdup(content_info->media_uuid);
	if (content_info->path)
		_item->path = strdup(content_info->path);
	if (content_info->mime_type)
		_item->mime_type = strdup(content_info->mime_type);

	*item = _item;

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_destroy_noti_item(media_svc_noti_item *item)
{
	if (item) {
		SAFE_FREE(item->media_uuid);
		SAFE_FREE(item->path);
		SAFE_FREE(item->mime_type);

		SAFE_FREE(item);
	}

	return MEDIA_INFO_ERROR_NONE;
}

int _media_svc_publish_noti_by_item(media_svc_noti_item *noti_item)
{
	int err = MEDIA_INFO_ERROR_NONE;

	if (noti_item && noti_item->path) {
		err = media_db_update_send(noti_item->pid,
								noti_item->update_item,
								noti_item->update_type,
								noti_item->path,
								noti_item->media_uuid,
								noti_item->media_type,
								noti_item->mime_type);
		if (err < 0) {
			media_svc_error("media_db_update_send failed : %d [%s]", err, noti_item->path);
		} else {
			media_svc_debug("media_db_update_send success");
		}
	}

	return err;
}

int _media_svc_publish_noti(media_item_type_e update_item,
							media_item_update_type_e update_type,
							const char *path,
							media_type_e media_type,
							const char *uuid,
							const char *mime_type
)
{
	int err = MEDIA_INFO_ERROR_NONE;

	if (path) {
		err = media_db_update_send(getpid(),
								update_item,
								update_type,
								(char *)path,
								(char *)uuid,
								media_type,
								(char *)mime_type);
		if (err < 0) {
			media_svc_error("media_db_update_send failed : %d [%s]", err, path);
			return MEDIA_INFO_ERROR_SEND_NOTI_FAIL;
		} else {
			media_svc_debug("media_db_update_send success");
		}
	}

	return err;
}