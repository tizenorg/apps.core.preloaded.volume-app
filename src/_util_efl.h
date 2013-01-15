/*
 * org.tizen.volume
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __VOLUME_UTIL_EFL_H__
#define __VOLUME_UTIL_EFL_H__

#include <Elementary.h>

Ecore_X_Window _add_input_window(void);
Evas_Object *_add_window(const char *name);
Evas_Object *_add_slider(Evas_Object *parent, int min, int max, int val);
Evas_Object *_add_layout(Evas_Object *parent, const char *file, const char *group);
Evas_Object *_add_button(Evas_Object *parent, const char *style, const char *text);
Evas_Object *_add_popup(Evas_Object *parent, const char *style);
Evas_Object *_add_label(Evas_Object *parent, const char *style, const char *text);
#endif
/* __VOLUME_UTIL_EFL_H__ */
