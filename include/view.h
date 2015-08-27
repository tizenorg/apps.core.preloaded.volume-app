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

#include "tzsh_volume_service.h"

#ifndef __VOLUME_VIEW_H__
#define __VOLUME_VIEW_H__

Evas_Object *volume_view_win_get(void);
tzsh_h volume_view_tzsh_get(void);
tzsh_volume_service_h volume_view_service_get(void);
Evas_Object *volume_view_evas_get(void);
Evas_Object *volume_view_outer_layout_get(void);
Evas_Object *volume_view_icon_volume_get(void);
Evas_Object *volume_view_icon_setting_get(void);
Evas_Object *volume_view_slider_get(void);
Eina_Bool volume_view_is_registered_callback_get(void);
Eina_Bool volume_view_is_slider_touching_get(void);
void volume_view_is_registered_callback_set(Eina_Bool val);
int volume_mute_toggle_set(void);

volume_error_e volume_view_slider_value_set(int val);
void volume_view_volume_icon_set(sound_type_e sound_type, int sound, int vibration, bool bt_opened);
void volume_view_setting_icon_set(const char *file);
void volume_view_setting_icon_callback_del(void);

volume_error_e volume_view_window_show(void);
volume_error_e volume_view_window_hide(void);

volume_error_e volume_view_layout_create(Evas_Object *win);
Evas_Object *volume_view_window_create(void);

Evas_Object* show_lockscreen_splash(const char *bg_path);
volume_error_e hide_lockscreen_splash(void);

#endif /* __VOLUME_VIEW_H__ */
