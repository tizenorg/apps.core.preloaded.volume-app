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


#ifndef __VOLUME_SOUND_H__
#define __VOLUME_SOUND_H__

extern int volume_sound_sound_status_get(void);
extern int volume_sound_vibration_status_get(void);
extern int volume_sound_step_get(void);
extern Eina_Bool volume_sound_is_vibration_get(void);
extern void volume_sound_is_vibration_set(Eina_Bool val);

extern void volume_sound_mm_sound_init(void);
extern int volume_sound_slider_value_set(void *data, int val);
extern volume_error_e volume_sound_level_get(sound_type_e type);
extern int volume_sound_sound_manager_step_get(sound_type_e type);
extern int volume_sound_sound_manager_volume_get(sound_type_e type);
extern sound_type_e volume_sound_sound_manager_type_get(void);
extern void volume_sound_feedback_play(int feedback_type, int pattern);
extern void volume_sound_vib_play(void);
extern void volume_sound_play(void);
extern int volume_sound_icon_set(void *data, int val);
extern int volume_sound_level_set(sound_type_e type, int val);
extern void volume_sound_setting_sound_play(void);
extern void volume_sound_change_set(int val);

extern void volume_sound_vconf_status_set(volume_vconf_type_e type, int val);
extern int volume_sound_vconf_status_get(volume_vconf_type_e type);
extern void volume_sound_vconfkey_register(void);
extern void volume_sound_vconfkey_unregister(void);

#endif
/* __VOLUME_SOUND_H__ */
