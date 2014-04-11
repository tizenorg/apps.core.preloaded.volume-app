/*
 * org.tizen.volume
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __VOLUME_LOGIC_H__
#define __VOLUME_LOGIC_H__

#include "volume.h"
#include <bundle.h>

int _get_vconf_idlelock(void);
volume_type_t _get_volume_type(void);
int _close_volume(void *data);
int _app_create(struct appdata *ad);
int _app_pause(struct appdata *ad);
int _rotate_func(void *data);
int _app_reset(bundle *b, void *data);
Eina_Bool popup_timer_cb(void *data);
void _init_press_timers(void *data);
int _check_status(int *lock, int *type);
int _ungrab_key_new(struct appdata *ad);
int _grab_key_new(struct appdata *ad, Ecore_X_Window _xwin, int grab_mode);

#endif
/* __VOLUME_LOGIC_H__ */