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
#ifndef __MEDIA_SERVICE_LOCALIZE_CH_H__
#define __MEDIA_SERVICE_LOCALIZE_CH_H__

#define CHINESE_PINYIN_SPELL_MAX_LEN	15

typedef struct {
	char *pinyin_initial;
	char *pinyin_name;
} pinyin_name_s;

int _media_svc_convert_chinese_to_pinyin(const char *src, pinyin_name_s **name, int *size);
void _media_svc_pinyin_free(pinyin_name_s *pinyinname, int size);

bool _media_svc_has_chinese(const char *src);

#endif // __MEDIA_SERVICE_LOCALIZE_CH_H__