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


/**
* @file 	utc_audio_svc_playlist_new_func.c
* @brief 	This is a suit of unit test cases to test audio_svc_playlist_new API function
* @author
* @version 	Initial Creation Version 0.1
* @date 	2010-09-13
*/
#include "utc_audio_svc_playlist_new_func.h"

void startup()
{
	UTC_AUDIO_SVC_OPEN();
}


void cleanup()
{
	UTC_AUDIO_SVC_CLOSE();
}

/**
* @brief	This tests int audio_svc_playlist_new() API with valid parameter
* 		Create a player handle with valid parameter & Test the handle by playing
* @par ID	utc_audio_svc_playlist_new_func_01
* @param	[in] &player = handle of player to be populated
* @return	This function returns zero on success, or negative value with error code
*/
void utc_audio_svc_playlist_new_func_01()
{
	int ret = AUDIO_SVC_ERROR_NONE;
	AudioHandleType *handle = NULL;
	int count = 5;

	//create playlists
	ret = audio_svc_playlist_new(&handle, count);
	dts_check_eq("audio_svc_playlist_new", ret, AUDIO_SVC_ERROR_NONE, "failed to create playlist.");

}


/**
* @brief 		This tests int audio_svc_playlist_new() API with invalid parameter
* 			Create a player handle with a NULL out param
* @par ID	utc_audio_svc_playlist_new_func_02
* @param	[in] &player = NULL
* @return	error code on success
*/
void utc_audio_svc_playlist_new_func_02()
{
	int ret = AUDIO_SVC_ERROR_NONE;
	AudioHandleType *handle = NULL;
	int count = 0;

	//create playlists
	ret = audio_svc_playlist_new(&handle, count);
	if (ret !=  AUDIO_SVC_ERROR_NONE)
	{
		dts_pass("audio_svc_item_get_val","abnormal condition test for invalid count parameter.");
	}
	else
	{
		dts_fail("audio_svc_playlist_new","Creating playlist should be failed because of the invalid count parameter.");
	}

}