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



#ifndef _MEDIA_SVC_ERROR_H_
#define _MEDIA_SVC_ERROR_H_

/**
	@addtogroup MEDIA_SVC
	 @{
	 * @file		media-svc-error.h
	 * @brief	This file defines error codes for media service.

 */

/**
        @defgroup MEDIA_SVC_COMMON  Global data structure and error code
        @{

        @par
         type definition and error code
 */


#define MEDIA_INFO_ERROR_NONE							0			/**< No Error */

#define MEDIA_INFO_ERROR_INVALID_PARAMETER			-1			/**< Invalid parameter */
#define MEDIA_INFO_ERROR_INVALID_MEDIA				-2			/**< Invalid media */
#define MEDIA_INFO_ERROR_INVALID_FILE_FORMAT			-3			/**< Invalid file format */
#define MEDIA_INFO_ERROR_INVALID_PATH				-4			/**< Invalid file path */
#define MEDIA_INFO_ERROR_OUT_OF_MEMORY				-5			/**< Out of memory */
#define MEDIA_INFO_ERROR_OUT_OF_STORAGE				-6			/**< Out of storage */
#define MEDIA_INFO_ERROR_INSERT_FAIL					-7			/**< Insert failed  */
#define MEDIA_INFO_ERROR_DRM_INSERT_FAIL				-8			/**< DRM file insert failed */

#define MEDIA_INFO_ERROR_ITEM_NOT_FOUND				-11			/**< Item not found */
#define MEDIA_INFO_ERROR_FILE_NOT_FOUND				-12			/**< File not found */
#define MEDIA_INFO_ERROR_APPEND_ITEM_FAILED			-13			/**< Append item failed */
#define MEDIA_INFO_ERROR_REMOVE_ITEM_FAILED			-14			/**< Remove item failed */
#define MEDIA_INFO_ERROR_GET_ITEM_FAILED				-15			/**< Get item failed */
#define MEDIA_INFO_ERROR_REMOVE_FILE_FAILED			-16			/**< Remove file failed */
#define MEDIA_INFO_ERROR_EXTRACT_FAILED				-17			/**< Extract Failed */
#define MEDIA_INFO_ERROR_MAKE_PLAYLIST_NAME_FAILED	-18			/**< fail to make new playlist name */

#define MEDIA_INFO_ERROR_DATABASE_CONNECT			-100		/**< DB connect error */
#define MEDIA_INFO_ERROR_DATABASE_DISCONNECT		-101		/**< DB disconnect error */
#define MEDIA_INFO_ERROR_DATABASE_CORRUPT		-102		/**< DB corrut error */
#define MEDIA_INFO_ERROR_DATABASE_QUERY				-104		/**< DB query error */
#define MEDIA_INFO_ERROR_DATABASE_TABLE_OPEN		-105		/**< DB table open error */
#define MEDIA_INFO_ERROR_DATABASE_INVALID			-106		/**< DB invalid error */
#define MEDIA_INFO_ERROR_DATABASE_INTERNAL			-107		/**< DB internal error */
#define MEDIA_INFO_ERROR_DATABASE_NO_RECORD		-108		/**< Item not found in DB */
#define MEDIA_INFO_ERROR_DATABASE_CONSTRAINT		-109		/**< DB constraint fail. In case of INSERT, this means the record already exist */

#define MEDIA_INFO_ERROR_SOCKET_CONN					-201		/**< Socket connect error */
#define MEDIA_INFO_ERROR_SOCKET_MSG					-202		/**< Socket message error */
#define MEDIA_INFO_ERROR_SOCKET_SEND					-203		/**< Socket send error */
#define MEDIA_INFO_ERROR_SOCKET_RECEIVE				-204		/**< Socket receive error */
#define MEDIA_INFO_ERROR_SOCKET_RECEIVE_TIMEOUT		-205		/**< Socket time out */

#define MEDIA_INFO_ERROR_SEND_NOTI_FAIL				-301		/**< Sending Notifications fail */

#define MEDIA_INFO_ERROR_INTERNAL						-998		/**< Internal error */
#define MEDIA_INFO_ERROR_UNKNOWN					-999		/**< Unknown error */
#define MEDIA_INFO_ERROR_NOT_IMPLEMENTED			-200		/**< Not implemented */
/**
	@}
*/

/**
	@}
*/

#endif /*_MEDIA_SVC_ERROR_H_*/
