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

#ifndef _MEDIA_SVC_NOTI_H_
#define _MEDIA_SVC_NOTI_H_

#include "media-svc-types.h"
#include "media-svc-env.h"
#include "media-svc-debug.h"
#include <media-util-noti.h>
#include <media-util-noti-internal.h>

typedef struct _media_svc_noti_item media_svc_noti_item;

struct _media_svc_noti_item {
	int pid;
	media_item_type_e update_item;
	media_item_update_type_e update_type;
	media_type_e media_type;
	char *media_uuid;
	char *path;
	char *mime_type;
};

void _media_svc_set_noti_from_pid(int pid);
int _media_svc_create_noti_list(int count);
int _media_svc_insert_item_to_noti_list(media_svc_content_info_s *content_info, int cnt);
int _media_svc_destroy_noti_list(int all_cnt);
int _media_svc_publish_noti_list(int all_cnt);
int _media_svc_destroy_noti_item(media_svc_noti_item *item);
int _media_svc_publish_noti(media_item_type_e update_item,
							media_item_update_type_e update_type,
							const char *path,
							media_type_e media_type,
							const char *uuid,
							const char *mime_type
);
int _media_svc_publish_dir_noti(media_item_type_e update_item,
							media_item_update_type_e update_type,
							const char *path,
							media_type_e media_type,
							const char *uuid,
							const char *mime_type,
							int pid
);
int _media_svc_publish_dir_noti_v2(media_item_type_e update_item,
							media_item_update_type_e update_type,
							const char *path,
							media_type_e media_type,
							const char *uuid,
							const char *mime_type,
							int pid
);





#endif /*_MEDIA_SVC_NOTI_H_*/

