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


#ifndef __VOLUME_KEY_EVENT_H__
#define __VOLUME_KEY_EVENT_H__

#define KEY_VOLUMEUP "XF86AudioRaiseVolume"
#define KEY_VOLUMEDOWN "XF86AudioLowerVolume"
#define KEY_MUTE "XF86AudioMute"
#define KEY_BACK "XF86Back"
#define KEY_CANCEL "Cancel"

extern Ecore_X_Window volume_key_event_input_window_get(void);
extern Ecore_Event_Handler *volume_key_event_handler_volume_up_get(void);
extern Ecore_Event_Handler *volume_key_event_handler_volume_down_get(void);
extern Ecore_Event_Handler *volume_key_event_handler_qp_state_check(void);
extern Eina_Bool volume_key_event_is_pressing_get(void);
extern int volume_key_event_count_grabed_get(void);
extern void volume_key_event_count_grabed_set(int val);

extern sound_type_e volume_key_event_sound_type_at_key_pressed_get(void);

extern int volume_key_event_key_grab(Ecore_X_Window _xwin, int grab_mode);
extern volume_error_e volume_key_event_key_ungrab(void);

extern void volume_key_event_handler_add(void);
extern void volume_key_event_handler_del(void);
extern volume_error_e volume_key_event_input_window_create(void);

#endif /* __VOLUME_KEY_EVENT_H__ */
