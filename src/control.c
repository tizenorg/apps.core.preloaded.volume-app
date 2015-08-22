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

#include <vconf.h>
#include <vconf-keys.h>
#include <app_manager.h>
#include <bluetooth.h>
#include <bluetooth_internal.h>
#include <app.h>
#include <bluetooth_extention.h>

#include "main.h"
#include "_util_efl.h"
#include "_util_log.h"
#include "view.h"
#include "control.h"
#include "sound.h"
#include "timer.h"
#include "key_event.h"
#include "bt.h"

#define VCONF_KEY_FMRADIO_RECORDING "memory/private/Sound/FMRadioRecording"

struct _control_s_info{
	bundle *volume_bundle;

	Eina_Bool is_deleting;
	Eina_Bool is_launching;
	Eina_Bool is_new;
	Eina_Bool is_warning_visible;
	Eina_Bool reset_once;
	Eina_Bool show_once;

	int current_angle;
	int viewport_width;
	int viewport_height;

	sound_type_e sound_type_at_show;
};
static struct _control_s_info control_info = {
	.volume_bundle = NULL,

	.is_deleting = EINA_FALSE,
	.is_launching = EINA_FALSE,
	.is_new = EINA_FALSE,
	.is_warning_visible = EINA_FALSE,
	.reset_once = EINA_FALSE,
	.show_once = EINA_FALSE,

	.current_angle = 0,
	.viewport_width = 0,
	.viewport_height= 0,

	.sound_type_at_show = SOUND_TYPE_RINGTONE,
};

static void _notify_pm_lcdoff_cb(keynode_t * node, void *data);
static void _idle_lock_state_vconf_changed_cb(keynode_t *key, void *data);
static void _starter_user_volume_key_vconf_changed_cb(keynode_t *key, void *data);
static void _control_set_window_rotation(Evas_Object *win);
static void _rotate_changed_cb(void *data, Evas_Object *obj, void *event_info);

bundle* volume_control_reset_get_bundle(void)
{
	return control_info.volume_bundle;
}

Eina_Bool volume_control_get_is_deleting(void)
{
	return control_info.is_deleting;
}

Eina_Bool volume_control_get_is_launching(void)
{
	return control_info.is_launching;
}

int volume_control_get_current_angle(void)
{
	return control_info.current_angle;
}

sound_type_e volume_control_get_sound_type_at_show(void)
{
	return control_info.sound_type_at_show;
}

int volume_control_get_viewport_height()
{
	return control_info.viewport_height;
}

int volume_control_get_viewport_width()
{
	return control_info.viewport_width;
}

Eina_Bool volume_control_viewport_is_warning_visible()
{
    return control_info.is_warning_visible;
}

volume_error_e volume_control_cache_flush(void)
{
	Evas_Object *win = volume_view_win_get();
	retv_if(win == NULL, VOLUME_ERROR_FAIL);

	Evas *evas = NULL;
	int file_cache = -1;
	int collection_cache = -1;
	int image_cache = -1;
	int font_cache = -1;

	evas = evas_object_evas_get(win);
	retv_if(!evas, VOLUME_ERROR_FAIL);

	file_cache = edje_file_cache_get();
	collection_cache = edje_collection_cache_get();
	image_cache = evas_image_cache_get(evas);
	font_cache = evas_font_cache_get(evas);

	edje_file_cache_set(file_cache);
	edje_collection_cache_set(collection_cache);
	evas_image_cache_set(evas, 0);
	evas_font_cache_set(evas, 0);

	evas_image_cache_flush(evas);
	evas_render_idle_flush(evas);
	evas_font_cache_flush(evas);

	edje_file_cache_flush();
	edje_collection_cache_flush();

	edje_file_cache_set(file_cache);
	edje_collection_cache_set(collection_cache);
	evas_image_cache_set(evas, image_cache);
	evas_font_cache_set(evas, font_cache);

	return VOLUME_ERROR_OK;
}

/* rotation event callback func. */
volume_error_e volume_control_app_launch_with_bundle(const char *op_type, const char *operation, const char *pkgname)
{
	app_control_h app_control;
	int ret = 0;

	ret = app_control_create(&app_control);
	if (ret != 0) {
		_E("Failed to create app control");
		return VOLUME_ERROR_FAIL;
	}

	ret = app_control_set_app_id(app_control, pkgname);
	if (ret != 0)
	{
		_E("Failed to set appid");
		app_control_destroy(app_control);
		return VOLUME_ERROR_FAIL;
	}

	ret = app_control_add_extra_data(app_control, op_type, operation);
	if (ret != 0)
	{
		_E("Failed to add extra data");
		app_control_destroy(app_control);
		return VOLUME_ERROR_FAIL;
	}

	ret = app_control_send_launch_request(app_control, NULL, NULL);
	_D("launch app with service : [%s][%d]", pkgname, ret);

	app_control_destroy(app_control);

	return VOLUME_ERROR_OK;
}

int volume_control_get_vconf_idlelock(void)
{
	int lock = IDLELOCK_OFF;
	int pm_state = VCONFKEY_PM_STATE_NORMAL;

	/* Check Idle-Lock */
	if(vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock) < 0)
	{
		_E("Failed to get vconfkey : VCONFKEY_IDLE_LOCK_STATE");
		return IDLELOCK_ERROR;
	}
	_D("idlelock vconf : %d", lock);

	/* Check PM state */
	if(vconf_get_int(VCONFKEY_PM_STATE, &pm_state) < 0)
	{
		_E("Failed to get vconfkey : VCONFKEY_PM_STATE");
		return IDLELOCK_ERROR;
	}
	_D("PM STATE vconf : %d", pm_state);

	return (lock == VCONFKEY_IDLE_LOCK ||
		pm_state == VCONFKEY_PM_STATE_LCDOFF ||
		pm_state  == VCONFKEY_PM_STATE_SLEEP
		) ? IDLELOCK_ON : IDLELOCK_OFF;
}


int volume_control_check_status(int *lock, sound_type_e *sound_type)
{
	*lock = volume_control_get_vconf_idlelock();
	*sound_type = volume_sound_sound_manager_type_get();
	_D("lock : %d / sound_type : %d", *lock, *sound_type);

	if(*lock == IDLELOCK_ON)
	{
		if(*sound_type == SOUND_TYPE_RINGTONE)
		{
			_D("IDLELOCK is ON / sound type is Ringtone");
			return LOCK_AND_NOT_MEDIA;
		}

		if(*sound_type != SOUND_TYPE_RINGTONE)
		{
			_D("IDLELOCK is ON / sound type is not Ringtone(media or alaram)");
			return LOCK_AND_MEDIA;
		}
	}

	_D("IDLELOCK is OFF / normal case");

	return UNLOCK_STATUS;
}

void volume_control_show_hide_worning()
{
	Evas_Object *ly_outer = volume_view_outer_layout_get();
	sound_type_e sound_type = volume_sound_sound_manager_type_get();
	int volume = volume_sound_sound_manager_volume_get(sound_type);

	if(sound_type == SOUND_TYPE_MEDIA
        && volume>VOLUME_MAX_SAFETY_VOLUME_LEVEL)
	{
		if(!control_info.is_warning_visible)
		{
			control_info.is_warning_visible = EINA_TRUE;

			if(control_info.current_angle == 90 || control_info.current_angle == 270)
			{
				elm_object_signal_emit(ly_outer, "show_warning_l", "clipper"); //landscape
			}
			else
			{
				elm_object_signal_emit(ly_outer, "show_warning", "clipper"); //landscape
			}
		}
	}
	else if(control_info.is_warning_visible)
	{
		control_info.is_warning_visible = EINA_FALSE;

		if(control_info.current_angle == 90 || control_info.current_angle == 270)
		{
			elm_object_signal_emit(ly_outer, "hide_warning_l", "clipper"); //landscape
		}
		else
		{
			elm_object_signal_emit(ly_outer, "hide_warning", "clipper"); //landscape
		}
	}
}

Eina_Bool volume_control_show_view(int status, sound_type_e sound_type, int sound, bool bt_opened)
{
	_D("Volume control show");
	Evas_Object *win = NULL;
	int volume = 0;
	int vibration = 0;

	retv_if(control_info.is_deleting, EINA_FALSE);

	control_info.is_new = EINA_TRUE;

	win = volume_view_win_get();
	retv_if(!win, EINA_FALSE);

	if(status == LOCK_AND_NOT_MEDIA)
	{
		_D("Lock and Not Media");
		if(evas_object_visible_get(win))
		{
			if(VOLUME_ERROR_OK != volume_control_hide_view())
			{
				_E("Failed to close volume");
			}

			if(VOLUME_ERROR_OK != volume_control_cache_flush())
			{
				_E("Failed to flush cache");
			}
		}
		return EINA_FALSE;
	} else {
		_D("UNLOCK or LOCK_AND_MEDIA");
		control_info.sound_type_at_show = sound_type;

		if(status == UNLOCK_STATUS)	{
			if(VOLUME_ERROR_OK != volume_view_window_show()) {
				_E("Failed to show volume window");
			}
		}

		control_info.is_launching = EINA_TRUE;

		if(bt_opened == 1 && sound_type == SOUND_TYPE_CALL)
		{
			_D("bt is opened and is calling");
			volume = bt_get_bt_volume();
			_D("bt volume is : %d", volume);

			if(VOLUME_ERROR_OK != volume_view_slider_value_set(volume))
			{
				_E("Failed to set volume value to slider");
			}
		}
		else
		{
			volume = volume_sound_sound_manager_volume_get(sound_type);
			_D("volume : %d", volume);

			vibration = volume_sound_vconf_status_get(TYPE_VCONF_VIBRATION_STATUS);
			_D("vibration : %d", vibration);

			if(((vibration == 1 && sound == 0) || sound == 0) && sound_type == SOUND_TYPE_RINGTONE)
			{
				volume = 0;
			}

			if(VOLUME_ERROR_OK != volume_view_slider_value_set(volume))
			{
				_E("Failed to set volume value to slider");
			}
		}

		//@TODO: need to check
		volume_view_volume_icon_set(sound_type, sound, vibration, bt_opened);

		return EINA_TRUE;
	}
}

volume_error_e volume_control_close_bt_display(void)
{
	retv_if(volume_control_get_is_deleting(), VOLUME_ERROR_FAIL);

	_D("Start closing bt display");

	control_info.is_deleting = EINA_TRUE;

	/* hide window */
	if(VOLUME_ERROR_OK != volume_view_window_hide())
	{
		_E("Failed to hide window");
	}

	control_info.is_deleting = EINA_FALSE;

	_D("End closing bt display");
	volume_timer_del(TYPE_TIMER_BT);

	return 0;
}

volume_error_e volume_control_hide_view(void)
{
	retv_if(volume_control_get_is_deleting(), VOLUME_ERROR_FAIL);

	_D("Start closing volume view");

	control_info.is_deleting = EINA_TRUE;

	volume_timer_del(TYPE_TIMER_SU);
	volume_timer_del(TYPE_TIMER_SD);
	volume_timer_del(TYPE_TIMER_SLIDER);
	volume_timer_del(TYPE_TIMER_POPUP);

	/* hide window */
	if (VOLUME_ERROR_OK != volume_view_window_hide()) {
		_E("Failed to hide window");
	}

	control_info.is_deleting = EINA_FALSE;
	control_info.is_launching = EINA_FALSE;

	_D("End closing volume view");

	return 0;
}

void volume_control_register_vconfkey(void)
{
	/* other app grab volume key => close volume */
	if(vconf_notify_key_changed(VCONFKEY_STARTER_USE_VOLUME_KEY, _starter_user_volume_key_vconf_changed_cb, NULL) != 0)
	{
		_E("Failed to register callback function : VCONFKEY_STARTER_USE_VOLUME_KEY");
	}

	/* Lock screen status vconf changed callback */
	if(vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE, _idle_lock_state_vconf_changed_cb, NULL) != 0)
	{
		_E("Failed to notify vconfkey : VCONFKEY_IDLE_LOCK_STATE");
	}

	if (vconf_notify_key_changed(VCONFKEY_PM_LCDOFF_SOURCE, _notify_pm_lcdoff_cb, NULL) != 0) {
		_E("Failed to notify vconfkey : VCONFKEY_PM_LCDOFF_SOURCE");
	}
}

void volume_control_unregister_vconfkey(void)
{
	/* other app grab volume key => close volume */
	if(vconf_ignore_key_changed(VCONFKEY_STARTER_USE_VOLUME_KEY, _starter_user_volume_key_vconf_changed_cb) < 0)
	{
		_E("Failed to ignore vconfkey : VCONFKEY_STARTER_USE_VOLUME_KEY");
	}

	/* Lock screen status vconf changed callback */
	if(vconf_ignore_key_changed(VCONFKEY_IDLE_LOCK_STATE, _idle_lock_state_vconf_changed_cb) < 0)
	{
		_E("Failed to ignore vconfkey : VCONFKEY_IDLE_LOCK_STATE");
	}

	if (vconf_ignore_key_changed
		(VCONFKEY_PM_LCDOFF_SOURCE, _notify_pm_lcdoff_cb) != 0) {
		_E("Fail vconf_ignore_key_changed : VCONFKEY_PM_LCDOFF_SOURCE");
	}
}

volume_error_e volume_control_pause(void)
{
	Evas_Object *win = volume_view_win_get();
	retv_if(!win, VOLUME_ERROR_FAIL);

	if(evas_object_visible_get(win)) {
		if(VOLUME_ERROR_OK != volume_control_hide_view())
		{
			_E("Failed to close volume");
		}

		if(VOLUME_ERROR_OK != volume_control_cache_flush())
		{
			_E("Failed to flush cache");
		}
	}

	return VOLUME_ERROR_OK;
}

volume_error_e volume_control_reset(bundle *b)
{
	_D("Volume control reset");
	Evas_Object *win = volume_view_win_get();
	retv_if(!win, VOLUME_ERROR_FAIL);

	int lock = IDLELOCK_ON;
	int status = 0;
	int volume = 0;
	int sound = 0;
	int error = 0;
	bool bt_opened = false;
	sound_type_e sound_type = 0;
	const char *show_volume = NULL;

	status = volume_control_check_status(&lock, &sound_type);
	_D("status: %d, lock: %d, sound type : %d", status, lock, sound_type);

	volume = volume_sound_sound_manager_volume_get(sound_type);
	_D("volume : %d", volume);

	sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	error = bt_ag_is_sco_opened(&bt_opened);
	if(error != BT_ERROR_NONE)
	{
		_E("bt_ag_is_sco_opened return [%d]", error);
	}
	_D("BT state %d", bt_opened);

	show_volume = bundle_get_val(b, SHOWVOLUME);
	retv_if(!show_volume, VOLUME_ERROR_FAIL);

	if(!strncasecmp(show_volume, ISTRUE, strlen(ISTRUE)))
	{
		_D("Bundle : %s", show_volume);
		if(lock == IDLELOCK_OFF)
		{
			_D("Show Volume");
			volume_timer_add(3.0, TYPE_TIMER_POPUP);
			volume_control_show_view(status, sound_type, sound, bt_opened);
		}
	}

	return VOLUME_ERROR_OK;
}

volume_error_e volume_control_initialize(void)
{
	_D("Volume control initialize");

	/* Create main window */
	Evas_Object *win = volume_view_window_create();
	retv_if(!win, VOLUME_ERROR_FAIL);

	/* Create volume layout */
	if(VOLUME_ERROR_OK != volume_view_layout_create(win)) {
		_E("Failed to create volume layout");
		return VOLUME_ERROR_FAIL;
	}

	elm_win_screen_size_get(win, NULL, NULL, &(control_info.viewport_width), &(control_info.viewport_height));


	/* Set available rotations */
	_control_set_window_rotation(win);

	/* Register vconfkey changed callback
	 * : VCONFKEY_STARTER_USE_VOLUME_KEY
	 * : VCONFKEY_IDLE_LOCK_STATE
	 * : VCONFKEY_LOCKSCREEN_SVIEW_STATE
	 * */
	volume_control_register_vconfkey();

	/* Register vconfkey changed callback
	 * : VCONFKEY_SETAPPL_SOUND_STATUS_BOOL
	 * : VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL
	 * */
	volume_sound_vconfkey_register();

	/* Add key event handler */
	volume_key_event_handler_add();

	/* Register volume changed callback */
	volume_sound_mm_sound_init();

	/* BT initialize and register changed callback */
	bt_init_sco();

	return VOLUME_ERROR_OK;
}

void volume_control_deinitialize(void)
{
	/* Unregister vconfkey changed callback */
	volume_control_unregister_vconfkey();

	/* Unregister sound vconfkey changed callback */
	volume_sound_vconfkey_unregister();

	/* Unregister bt changed callback */
	bt_deinit_sco();
}

static void _rotate_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	static int current_angle = -1;
	int changed_angle = elm_win_rotation_get(obj);
	LOGD("MIK");

	Evas_Object *ly_outer = volume_view_outer_layout_get();
	ret_if(!ly_outer);

	_D("window rotated [%d] => [%d]", current_angle, changed_angle);
	if(current_angle != changed_angle) {
		current_angle = changed_angle;
		control_info.current_angle = current_angle;
		switch(current_angle){
		case 90 :
		case 270 :
			_D("show,landscape");
			elm_object_signal_emit(ly_outer, "show,landscape", "bg");
			if(control_info.is_warning_visible)
			{
				elm_object_signal_emit(ly_outer, "show_warning_l", "clipper");
			}
			break;
		default :
			_D("show,portrait");
			elm_object_signal_emit(ly_outer, "show,portrait", "bg");
			if(control_info.is_warning_visible)
			{
				elm_object_signal_emit(ly_outer, "show_warning", "clipper");
			}
			break;
		}
	}
}

static void _control_set_window_rotation(Evas_Object *win)
{
	ret_if(!win);

	if (elm_win_wm_rotation_supported_get(win)) {
		const int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, (const int *)&rots, 4);
		_D("set available rotations");
	}

	/* rotation event callback */
	evas_object_smart_callback_add(win, "wm,rotation,changed", _rotate_changed_cb, NULL);

	/* initialize degree */
	_rotate_changed_cb(NULL, win, NULL);
}

static void _starter_user_volume_key_vconf_changed_cb(keynode_t *key, void *data)
{
	int ret = EINA_FALSE;

	if(vconf_get_int(VCONFKEY_STARTER_USE_VOLUME_KEY, &ret) < 0)
	{
		_E("Failed to get vconfkey : VCONFKEY_STARTER_USE_VOLUME_KEY");
		return;
	}
	_D("ret : %d", ret);

	if(ret == 1)
	{
		_D("any other App grab volume hard key");
		if(VOLUME_ERROR_OK != volume_control_hide_view()) {
			_E("Failed to close volume");
			return;
		}
		if(VOLUME_ERROR_OK != volume_control_cache_flush()) {
			_E("Failed to flush cache");
			return;
		}
		if(vconf_set_int(VCONFKEY_STARTER_USE_VOLUME_KEY, 0) < 0) {
			_E("Failed to get vconfkey : VCONFKEY_STATER_USE_VOLUME_KEY");
			return;
		}
	}
	else if(ret == 2)
	{
		_D("setting App grab volume hard key");
	}
}

static void _idle_lock_state_vconf_changed_cb(keynode_t *key, void *data)
{
	int lock = VCONFKEY_IDLE_UNLOCK;

	if(vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock) < 0)
	{
		_E("Failed to get vconfkey : VCONFKEY_IDLE_LOCK_STATE");
		return;
	}
	_D("idle lock state : %d", lock);

	if(lock == VCONFKEY_IDLE_LAUNCHING_LOCK)
	{
		if(VOLUME_ERROR_OK != volume_view_window_hide())
		{
			_E("Failed to hide window");
		}
	}
}

static void _notify_pm_lcdoff_cb(keynode_t * node, void *data)
{
	if(VOLUME_ERROR_OK != volume_control_hide_view())
	{
		_E("Failed to close volume");
	}

	if(VOLUME_ERROR_OK != volume_control_cache_flush())
	{
		_E("Failed to flush cache");
	}
}
