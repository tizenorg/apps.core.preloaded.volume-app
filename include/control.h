/*
 * Copyright (c) 2009-2014 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __VOLUME_CONTROL_H__
#define __VOLUME_CONTROL_H__

enum {
	IDLELOCK_ERROR = -1,
	IDLELOCK_OFF = 0,
	IDLELOCK_ON = 1,
	IDLELOCK_MAX,
};

/* _check_status() return value */
enum{
	LOCK_AND_NOT_MEDIA = -0x1,
	UNLOCK_STATUS,
	LOCK_AND_MEDIA,
};

extern void volume_control_check_syspopup();
extern int volume_register_shape_timer();
extern void volume_control_check_once(void);
extern int volume_control_get_viewport_height();
extern int volume_control_get_viewport_width();
extern void volume_control_show_hide_worning();
extern Eina_Bool volume_control_viewport_is_warning_visible();

extern bundle *volume_control_reset_get_bundle(void);
extern Eina_Bool volume_control_get_is_deleting(void);
extern Eina_Bool volume_control_get_is_launching(void);
extern int volume_control_get_current_angle(void);
extern sound_type_e volume_control_get_sound_type_at_show(void);

extern volume_error_e volume_control_cache_flush(void);

extern volume_error_e volume_control_app_launch_with_bundle(const char *op_type, const char *operation, const char *pkgname);
extern volume_error_e volume_control_mode_syspopup_launch(void);
extern int volume_control_get_vconf_idlelock(void);

extern int volume_control_check_status(int *lock, sound_type_e *sound_type);

extern Eina_Bool volume_control_show_view(int status, sound_type_e sound_type, int sound, bool bt_opened);
extern int volume_control_hide_view(void);
extern volume_error_e volume_control_close_bt_display();

extern void volume_control_register_vconfkey(void);
extern void volume_control_unregister_vconfkey(void);

extern volume_error_e volume_control_pause(void);
extern volume_error_e volume_control_reset(bundle *b);
extern volume_error_e volume_control_initialize(void);
extern void volume_control_deinitialize(void);

#endif /* __VOLUME_CONTROL_H__ */
