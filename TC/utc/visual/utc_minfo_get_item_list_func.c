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
* @file 	utc_minfo_get_item_list_func.c
* @brief 	This is a suit of unit test cases to test minfo_get_item_list API function
* @author 		
* @version 	Initial Creation Version 0.1
* @date 	2010-10-10
*/

#include "utc_minfo_get_item_list_func.h"

static int _ite_fn( Mitem* item, void* user_data) 
{
	GList** list = (GList**) user_data;
	*list = g_list_append( *list, item );

	return 0;
}

/**
* @brief	This tests int minfo_get_item_list() API with valid parameter
* 		Get glist including Mitem members.
* @par ID	utc_minfo_get_item_list_func_01
* @param	[in] 
* @return	This function returns zero on success, or negative value with error code
*/
void utc_minfo_get_item_list_func_01()
{
	int ret = 0;
	const char *cluster_uuid = "8ac1df34-efa8-4143-a47e-5b6f4bac8c96";

	GList *p_list = NULL;
	minfo_item_filter item_filter = {MINFO_ITEM_VIDEO,MINFO_MEDIA_SORT_BY_DATE_ASC,0,5,true, MINFO_MEDIA_FAV_ALL};

	ret = minfo_get_item_list(handle, cluster_uuid, item_filter, _ite_fn, &p_list);

	if (ret == MB_SVC_ERROR_DB_NO_RECORD) {
		dts_pass(API_NAME, "No record. This is normal operation");
	}

	dts_check_ge(API_NAME, ret, MB_SVC_ERROR_NONE, "unable to get media records. error code->%d", ret);
}


/**
* @brief 		This tests int minfo_get_item_list() API with invalid parameter
* 			Get glist including Mitem members.
* @par ID	utc_minfo_get_item_list_func_02
* @param	[in] 
* @return	error code on success 
*/
void utc_minfo_get_item_list_func_02()
{	
	int ret = 0;
	const char *cluster_uuid = NULL;

	GList *p_list = NULL;
	minfo_item_filter item_filter = {MINFO_ITEM_VIDEO,MINFO_MEDIA_SORT_BY_DATE_ASC,0,5,true,MINFO_MEDIA_FAV_ALL};

	ret = minfo_get_item_list(handle, cluster_uuid, item_filter, NULL, &p_list);
	dts_check_lt(API_NAME, ret, MB_SVC_ERROR_NONE,"getting media records should be failed because of the item_filter parameter.");
}
