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
#ifndef __MEDIA_SVC_LOCALIZE_UTILS_H__
#define __MEDIA_SVC_LOCALIZE_UTILS_H__

#define array_sizeof(a) (sizeof(a) / sizeof(a[0]))

#define MEDIA_SVC_COMPARE_BETWEEN(left_range, value, right_range) (((left_range) <= (value)) && ((value) <= (right_range)))

int _media_svc_check_utf8(char c);

int SAFE_SNPRINTF(char **buf, int *buf_size, int len, const char *src);

int _media_svc_collation_str(const char *src, char **dest);

#endif // __MEDIA_SVC_LOCALIZE_UTILS_H__