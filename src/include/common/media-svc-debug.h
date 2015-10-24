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
#include <errno.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "MEDIA_SERVICE"

#define FONT_COLOR_RESET    "\033[0m"
#define FONT_COLOR_RED      "\033[31m"
#define FONT_COLOR_GREEN    "\033[32m"
#define FONT_COLOR_YELLOW   "\033[33m"
#define FONT_COLOR_BLUE     "\033[34m"
#define FONT_COLOR_PURPLE   "\033[35m"
#define FONT_COLOR_CYAN     "\033[36m"
#define FONT_COLOR_GRAY     "\033[37m"

#define media_svc_debug(fmt, arg...) do { \
			LOGD(FONT_COLOR_RESET" "fmt"", ##arg);     \
		} while (0)

#define media_svc_error(fmt, arg...) do { \
			LOGE(FONT_COLOR_RED" "fmt"", ##arg);     \
		} while (0)

#define media_svc_debug_fenter() do { \
			LOGD(FONT_COLOR_RESET"<ENTER>");     \
		} while (0)

#define media_svc_debug_fleave() do { \
			LOGD(FONT_COLOR_RESET"<LEAVE>");     \
		} while (0)

#define media_svc_retm_if(expr, fmt, arg...) do { \
			if(expr) { \
				LOGE(FONT_COLOR_RED" "fmt"", ##arg);   \
				return; \
			} \
		} while (0)
#define media_svc_retv_if(expr, val) do { \
			if(expr) { \
				LOGE(FONT_COLOR_RED"");     \
				return (val); \
			} \
		} while (0)
#define media_svc_retvm_if(expr, val, fmt, arg...) do { \
			if(expr) { \
				LOGE(FONT_COLOR_RED" "fmt"", ##arg); 	\
				return (val); \
			} \
		} while (0)

#define media_svc_retv_del_if(expr, val, p_str) do { \
			if(expr) { \
				LOGE(FONT_COLOR_RED"");     \
				_media_svc_destroy_content_info(p_str);        \
				return (val); \
			} \
		} while (0)

#define media_svc_sec_debug(fmt, arg...) do { \
			SECURE_LOGI(FONT_COLOR_RESET" "fmt"", ##arg);	 \
		} while (0)

#define media_svc_sec_warn(fmt, arg...) do { \
			SECURE_LOGW(FONT_COLOR_GREEN" "fmt"", ##arg); 	\
		} while (0)

#define media_svct_sec_error(fmt, arg...) do { \
			SECURE_LOGE(FONT_COLOR_RED" "fmt"", ##arg);	  \
		} while (0)

#define ERR_BUF_LENGHT 256
#define media_svc_stderror(fmt) do { \
			char media_svc_stderror_buf[ERR_BUF_LENGHT] = {0,}; \
			strerror_r(errno, media_svc_stderror_buf, ERR_BUF_LENGHT); \
			LOGE(FONT_COLOR_RED""fmt" : standard error= [%s]", media_svc_stderror_buf); \
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
