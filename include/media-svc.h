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



#ifndef _MEDIA_SVC_H_
#define _MEDIA_SVC_H_

#include "media-svc-types.h"
#include "media-svc-error.h"
#ifdef __cplusplus
extern "C" {
#endif


/**
	@defgroup	MEDIA_SVC	Media Information Service
	 @{
	  * @file			media-svc.h
	  * @brief		This file defines API's for media service.
	  * @version	 	1.0
 */

/**
        @defgroup MEDIA_SVC_API    Media Database API
        @{

        @par
        manage the service database.
 */


/**
 *	media_svc_connect:
 *	Connect to the media database. This is the function that an user who wants to get a handle to access the media database. 
 *
 *  @param 		handle [out]		Handle to access database.
 *	@return		This function returns zero(MEDIA_INFO_ERROR_NONE) on success, or negative value with error code.
 *				Please refer 'media-info-error.h' to know the exact meaning of the error.
 *	@see		media_svc_disconnect
 *	@pre		None
 *	@post		call media_svc_disconnect to disconnect media database.
 *	@remark	The database name is "/opt/usr/dbspace/.media.db".
 * 	@par example
 * 	@code

#include <media-info.h>

void connect_media_db()
{
	int ret = MEDIA_INFO_ERROR_NONE;
	MediaSvcHandle* my_handle = NULL;

	// connect to the media database
	ret = media_svc_connect(&my_handle);

	if (ret < 0)
	{
		printf("Fatal error to connect DB\n");
		return;
	}

	return;
}

 * 	@endcode
 */
int media_svc_connect(MediaSvcHandle **handle);


/**
 *	media_svc_disconnect:
 *	Disconnect to the media database. This is the function that an user who wants to disconnect the media database. 
 *
 *  @param 		handle [in]		Handle to access database.
 *	@return		This function returns zero(MEDIA_INFO_ERROR_NONE) on success, or negative value with error code.
 *				Please refer 'media-info-error.h' to know the exact meaning of the error.
 *	@see		media_svc_connect
 *	@pre		call media_svc_connect to connect media database.
 *	@post		None
 *	@remark	The database name is "/opt/usr/dbspace/.media.db".
 * 	@par example
 * 	@code

#include <media-info.h>

void disconnect_media_db()
{
	int ret = MEDIA_INFO_ERROR_NONE;
	MediaSvcHandle* my_handle = NULL;

	// connect to the media database
	ret = media_svc_connect(&my_handle);

	if (ret < 0)
	{
		printf("Fatal error to connect DB\n");
		return;
	}

	//
	// Do something using my_handle
	//
	

	ret = media_svc_disconnect(my_handle);
	if (ret < 0)
	{
		printf("Fatal error to disconnect DB\n");
	}

	return;
}

 * 	@endcode
 */
int media_svc_disconnect(MediaSvcHandle *handle);


/**
 *	media_svc_create_table:
 *	Create table of media database and set Index and Triggers.
 *
 *  @param 		handle [in]		Handle to access database.
 *	@return		This function returns zero(MEDIA_INFO_ERROR_NONE) on success, or negative value with error code.
 *				Please refer 'media-info-error.h' to know the exact meaning of the error.
 *	@see		None
 *	@pre		call media_svc_connect to connect media database.
 *	@post		call media_svc_disconnect to disconnect media database.
 *	@remark	The database name is "/opt/usr/dbspace/.media.db".
 * 	@par example
 * 	@code

#include <media-info.h>

void create_media_db_table()
{
	int ret = MEDIA_INFO_ERROR_NONE;
	MediaSvcHandle* my_handle = NULL;

	// connect to the media database
	ret = media_svc_connect(&my_handle);

	if (ret < 0)
	{
		printf("Fatal error to connect DB\n");
		return;
	}

	ret = media_svc_create_table(my_handle);
	if (ret < 0)
	{
		printf("Fatal error to create DB table\n");
	}

	ret = media_svc_disconnect(my_handle);
	if (ret < 0)
	{
		printf("Fatal error to disconnect DB\n");
	}

	return;
}

 * 	@endcode
 */

int media_svc_create_table(MediaSvcHandle *handle);

int media_svc_check_item_exist_by_path(MediaSvcHandle *handle, const char *path);

int media_svc_insert_folder(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path);

int media_svc_insert_item_begin(MediaSvcHandle *handle, int with_noti, int data_cnt, int from_pid);

int media_svc_insert_item_end(MediaSvcHandle *handle);

int media_svc_insert_item_bulk(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path, int is_burst);

int media_svc_insert_item_immediately(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path);

int media_svc_move_item_begin(MediaSvcHandle *handle, int data_cnt);

int media_svc_move_item_end(MediaSvcHandle *handle);

int media_svc_move_item(MediaSvcHandle *handle, media_svc_storage_type_e src_storage, const char *src_path, media_svc_storage_type_e dest_storage, const char *dest_path);

int media_svc_set_item_validity_begin(MediaSvcHandle *handle, int data_cnt);

int media_svc_set_item_validity_end(MediaSvcHandle *handle);

int media_svc_set_item_validity(MediaSvcHandle *handle, const char *path, int validity);

int media_svc_delete_item_by_path(MediaSvcHandle *handle, const char *path);

int media_svc_delete_all_items_in_storage(MediaSvcHandle *handle, media_svc_storage_type_e storage_type);

int media_svc_delete_invalid_items_in_storage(MediaSvcHandle *handle, media_svc_storage_type_e storage_type);

int media_svc_delete_invalid_items_in_folder(MediaSvcHandle *handle, const char *folder_path);

int media_svc_set_all_storage_items_validity(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, int validity);

int media_svc_set_folder_items_validity(MediaSvcHandle *handle, const char *folder_path, int validity, int recursive);

int media_svc_refresh_item(MediaSvcHandle *handle, media_svc_storage_type_e storage_type, const char *path);

int media_svc_rename_folder(MediaSvcHandle *handle, const char *src_path, const char *dst_path);

int media_svc_request_update_db(const char *db_query);

int media_svc_get_storage_type(const char *path, media_svc_storage_type_e *storage_type);

int media_svc_get_mime_type(const char *path, char *mimetype);

int media_svc_get_media_type(const char *path, const char *mime_type, media_svc_media_type_e *media_type);

int media_svc_get_file_info(MediaSvcHandle *handle, const char *path, time_t *modified_time, unsigned long long *size);

int media_svc_send_dir_update_noti(MediaSvcHandle *handle, const char *dir_path);

int media_svc_count_invalid_items_in_folder(MediaSvcHandle *handle, const char *folder_path, int *count);

/** @} */

/**
	@}
 */

#ifdef __cplusplus
}
#endif

#endif /*_MEDIA_SVC_H_*/
