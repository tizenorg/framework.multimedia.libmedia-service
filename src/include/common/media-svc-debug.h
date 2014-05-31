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



#ifndef _MEDIA_SVC_DEBUG_H_
#define _MEDIA_SVC_DEBUG_H_

#include <stdio.h>
#include <stdlib.h>
#include <dlog.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "MEDIA_SERVICE"

#define media_svc_debug(fmt, arg...) do { \
			LOGD(" "fmt"", ##arg);     \
		} while (0)

#define media_svc_error(fmt, arg...) do { \
			LOGE(" "fmt"", ##arg);     \
		} while (0)

#define media_svc_debug_func() do { \
			LOGD("");     \
		} while (0)

#define media_svc_retm_if(expr, fmt, arg...) do { \
			if(expr) { \
				LOGE(" "fmt"", ##arg);   \
				return; \
			} \
		} while (0)
#define media_svc_retv_if(expr, val) do { \
			if(expr) { \
				LOGE("");     \
				return (val); \
			} \
		} while (0)
#define media_svc_retvm_if(expr, val, fmt, arg...) do { \
			if(expr) { \
				LOGE(" "fmt"", ##arg); 	\
				return (val); \
			} \
		} while (0)

#define media_svc_retv_del_if(expr, val, p_str) do { \
			if(expr) { \
				LOGE("");     \
				_media_svc_destroy_content_info(p_str);        \
				return (val); \
			} \
		} while (0)

#define media_svc_sec_debug(fmt, arg...) do { \
			SECURE_LOGI(" "fmt"", ##arg);	 \
		} while (0)

#define media_svc_sec_warn(fmt, arg...) do { \
			SECURE_LOGW(" "fmt"", ##arg); 	\
		} while (0)

#define media_svct_sec_error(fmt, arg...) do { \
			SECURE_LOGE(" "fmt"", ##arg);	  \
		} while (0)

#ifdef _USE_LOG_FILE_
void mediainfo_init_file_debug();
void mediainfo_close_file_debug();
FILE* get_fp();
#define mediainfo_file_dbg(fmt,arg...)      fprintf( get_fp(), "[%s: %d] [%s]" fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##arg)

#endif


#ifdef _PERFORMANCE_CHECK_
long
mediainfo_get_debug_time(void);
void
mediainfo_reset_debug_time(void);
void
mediainfo_print_debug_time(char* time_string);
void
mediainfo_print_debug_time_ex(long start, long end, const char* func_name, char* time_string);
#endif

#endif /*_MEDIA_SVC_DEBUG_H_*/
