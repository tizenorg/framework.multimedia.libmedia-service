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
 
#include <stdio.h>
#include <unistd.h>
#include <media-svc.h>
#include <media-svc-noti.h>

GMainLoop *g_loop = NULL;
MediaSvcHandle *g_db_handle = NULL;

void _noti_cb(int pid,
                media_item_type_e update_item,
                media_item_update_type_e update_type,
                char *path,
                char *uuid,
                media_type_e content_type,
                char *mime_type,
                void *user_data)
{
	media_svc_debug("Noti from PID(%d)", pid);

	if (update_item == MS_MEDIA_ITEM_FILE) {
		media_svc_debug("Noti item : MS_MEDIA_ITEM_FILE");
	} else if (update_item == MS_MEDIA_ITEM_DIRECTORY) {
		media_svc_debug("Noti item : MS_MEDIA_ITEM_DIRECTORY");
	}

	if (update_type == MS_MEDIA_ITEM_INSERT) {
		media_svc_debug("Noti type : MS_MEDIA_ITEM_INSERT");
	} else if (update_type == MS_MEDIA_ITEM_DELETE) {
		media_svc_debug("Noti type : MS_MEDIA_ITEM_DELETE");
	} else if (update_type == MS_MEDIA_ITEM_UPDATE) {
		media_svc_debug("Noti type : MS_MEDIA_ITEM_UPDATE");
	}

	//media_svc_debug("content type : %d", content_type);
	printf("content type : %d\n", content_type);

	if (path)
		printf("path : %s\n", path);
	else
		printf("path not");

	if (mime_type)
		printf("mime_type : %s", mime_type);
	else
		printf("mime not");

    if (user_data) printf("String : %s\n", (char *)user_data);
	else
		printf("user not");

    return;
}

#if 1
gboolean _send_noti_batch_operations(gpointer data)
{
    int ret = MEDIA_INFO_ERROR_NONE;

    /* First of all, noti subscription */
    char *user_str = strdup("hi");
    media_db_update_subscribe(_noti_cb, (void*)user_str);

    /* 1. media_svc_insert_item_immediately */
    char *path = "/opt/usr/media/test/image1.jpg";

	media_svc_storage_type_e storage_type;

	ret = media_svc_get_storage_type(path, &storage_type);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_get_storage_type failed : %d (%s)", ret, path);
        return FALSE;
	}

	int idx = 0;
	char *file_list[10];

	ret = media_svc_insert_item_begin(g_db_handle, 100, TRUE, getpid());
	//ret = media_svc_insert_item_begin(g_db_handle, 100);
	for (idx = 0; idx < 16; idx++) {
		char filepath[255] = {0,};
		snprintf(filepath, sizeof(filepath), "%s%d.jpg", "/opt/usr/media/test/image", idx+1);
		media_svc_debug("File : %s\n", filepath);
		file_list[idx] = strdup(filepath);
		ret = media_svc_insert_item_bulk(g_db_handle, storage_type, file_list[idx], FALSE);
		if (ret != 0) {
			media_svc_error("media_svc_insert_item_bulk[%d] failed", idx);
		} else {
			media_svc_debug("media_svc_insert_item_bulk[%d] success", idx);
		}
	}

	ret = media_svc_insert_item_end(g_db_handle);

	return FALSE;
}
#endif

gboolean _send_noti_operations(gpointer data)
{
	int ret = MEDIA_INFO_ERROR_NONE;

	/* First of all, noti subscription */
	char *user_str = strdup("hi");
	media_db_update_subscribe(_noti_cb, (void*)user_str);

	/* 1. media_svc_insert_item_immediately */
	char *path = "/opt/usr/media/test/image1.jpg";
	media_svc_storage_type_e storage_type;

	ret = media_svc_get_storage_type(path, &storage_type);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_get_storage_type failed : %d (%s)", ret, path);
		return FALSE;
	}

	ret = media_svc_insert_item_immediately(g_db_handle, storage_type, path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_insert_item_immediately failed : %d", ret);
		return FALSE;
	}

	media_svc_debug("media_svc_insert_item_immediately success");

	/* 2. media_svc_refresh_item */
	ret = media_svc_refresh_item(g_db_handle, storage_type, path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_refresh_item failed : %d", ret);
		return FALSE;
	}
	media_svc_debug("media_svc_refresh_item success");

	/* 2. media_svc_move_item */
	const char *dst_path = "/opt/usr/media/test/image11.jpg";
	ret = media_svc_move_item(g_db_handle, storage_type, path, storage_type, dst_path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_move_item failed : %d", ret);
		return FALSE;
	}
	media_svc_debug("media_svc_move_item success");

	ret = media_svc_move_item(g_db_handle, storage_type, dst_path, storage_type, path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_move_item failed : %d", ret);
		return FALSE;
	}
	media_svc_debug("media_svc_move_item success");

	/* 4. media_svc_delete_item_by_path */
	ret = media_svc_delete_item_by_path(g_db_handle, path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_delete_item_by_path failed : %d", ret);
		return FALSE;
	}
	media_svc_debug("media_svc_delete_item_by_path success");

	/* Rename folder */
	const char *src_folder_path = "/opt/usr/media/test";
	const char *dst_folder_path = "/opt/usr/media/test_test";
	ret = media_svc_rename_folder(g_db_handle, src_folder_path, dst_folder_path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_rename_folder failed : %d", ret);
		return FALSE;
	}
	media_svc_debug("media_svc_rename_folder success");

	/* Rename folder again */
	ret = media_svc_rename_folder(g_db_handle, dst_folder_path, src_folder_path);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_rename_folder failed : %d", ret);
		return FALSE;
	}
	media_svc_debug("media_svc_rename_folder success");

	return FALSE;
}

int test_noti()
{
	GSource *source = NULL;
	GMainContext *context = NULL;

	g_loop = g_main_loop_new(NULL, FALSE);
	context = g_main_loop_get_context(g_loop);
	source = g_idle_source_new();
#if 0
	g_source_set_callback (source, _send_noti_operations, NULL, NULL);
#else
	g_source_set_callback (source, _send_noti_batch_operations, NULL, NULL);
#endif
	g_source_attach (source, context);

	g_main_loop_run(g_loop);

	g_main_loop_unref(g_loop);
    media_db_update_unsubscribe();

	return MEDIA_INFO_ERROR_NONE;
}

int main()
{
	int ret = MEDIA_INFO_ERROR_NONE;
	ret = media_svc_connect(&g_db_handle);
	if (ret != MEDIA_INFO_ERROR_NONE) {
		media_svc_error("media_svc_connect failed : %d", ret);
	} else {
		media_svc_debug("media_svc_connect success");
	}

	ret = test_noti();
	if (ret < MEDIA_INFO_ERROR_NONE) {
		media_svc_error("test_noti failed : %d", ret);
	} else {
		media_svc_debug("test_noti success");
	}

	media_svc_disconnect(g_db_handle);
	return ret;
}

