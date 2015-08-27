/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 */


#ifndef __VOLUME_UTIL_EFL_H__
#define __VOLUME_UTIL_EFL_H__

#include <Elementary.h>

Ecore_X_Window _add_input_window(void);
Evas_Object *add_volume_window(const char *name);
Evas_Object *add_slider(Evas_Object *parent, int min, int max, int val);
Evas_Object *add_layout(Evas_Object *parent, const char *file, const char *group);
Evas_Object *add_button(Evas_Object *parent, const char *style, const char *text);
Evas_Object *add_popup(Evas_Object *parent, const char *style);
Evas_Object *add_label(Evas_Object *parent, const char *text);
#endif
/* __VOLUME_UTIL_EFL_H__ */
