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

#include <Ecore.h>
#include <feedback.h>
#include <vconf.h>
#include <vconf-keys.h>

#include "main.h"
#include "_util_efl.h"
#include "_util_log.h"
#include "view.h"
#include "control.h"
#include "timer.h"
#include "key_event.h"
#include "sound.h"
#include "tzsh_volume_service.h"

struct _view_s_info {
	Evas_Object *win;
	tzsh_h tzsh;
	tzsh_volume_service_h volume_service;
	Evas_Object *evas;
	Evas_Object *ly_outer;
	Evas_Object *icon_volume;
	Evas_Object *icon_setting;
	Evas_Object *slider;

	Evas_Object *lockscreen_splash;

	Evas_Object *ao_settings;

	Eina_Bool is_registered_callback;
	Eina_Bool is_slider_touching;
	Eina_Bool is_warning_displayed;
	sound_type_e pre_sound_type;
};
static struct _view_s_info view_info = {
	.win = NULL,
	.tzsh = NULL,
	.volume_service = NULL,
	.evas = NULL,
	.ly_outer = NULL,
	.icon_volume = NULL,
	.icon_setting = NULL,
	.slider = NULL,

	.lockscreen_splash = NULL,

	.ao_settings = NULL,

	.is_registered_callback = EINA_FALSE,
	.is_slider_touching = EINA_FALSE,
	.is_warning_displayed = EINA_FALSE,
	.pre_sound_type = SOUND_TYPE_RINGTONE
};

static void _button_cb(void *data, Evas_Object *obj, void *event_info);
static void _button_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _app_control_error_check(int ret);
static void _hide_launcher(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _slider_stop_cb(void *data, Evas_Object *obj, void *event_info);
static void _slider_start_cb(void *data, Evas_Object *obj, void *event_info);
static void _slider_changed_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Object* _slider_make();
static Evas_Object* _volume_icon_make();
static Evas_Object* _setting_icon_make();

Evas_Object* volume_view_win_get(void)
{
	return view_info.win;
}

tzsh_h volume_view_tzsh_get(void)
{
	return view_info.tzsh;
}

tzsh_volume_service_h volume_view_service_get(void)
{
	return view_info.volume_service;
}

Evas_Object* volume_view_evas_get(void)
{
	return view_info.evas;
}

Evas_Object* volume_view_outer_layout_get(void)
{
	return view_info.ly_outer;
}

Evas_Object* volume_view_icon_volume_get(void)
{
	return view_info.icon_volume;
}

Evas_Object* volume_view_icon_setting_get(void)
{
	return view_info.icon_setting;
}

Evas_Object* volume_view_slider_get(void)
{
	return view_info.slider;
}

Eina_Bool volume_view_is_registered_callback_get(void)
{
	return view_info.is_registered_callback;
}

Eina_Bool volume_view_is_slider_touching_get(void)
{
	return view_info.is_slider_touching;
}

sound_type_e volume_view_pre_sound_type_get(void)
{
	return view_info.pre_sound_type;
}

volume_error_e volume_view_set_default_slider(){
	Evas_Object *slider = volume_view_slider_get();
	elm_object_style_set(slider, "default");
	return VOLUME_ERROR_OK;
}

volume_error_e volume_view_set_warning_slider(){
	Evas_Object *slider = volume_view_slider_get();
	elm_object_style_set(slider, "warning");
	Edje_Message_Float_Set *msg = alloca(sizeof(Edje_Message_Float_Set) + (sizeof(double)));
	msg->count = 1;
	msg->val[0] = 0.66;
	edje_object_message_send(elm_layout_edje_get(slider), EDJE_MESSAGE_FLOAT_SET, 0, msg);

	return VOLUME_ERROR_OK;
}

int volume_mute_toggle_set()
{
	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	int sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	int vibration = volume_sound_vconf_status_get(TYPE_VCONF_VIBRATION_STATUS);
	_E("vibration : %d", vibration);

	if (sound == 1 || vibration == 1) {
		volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 0);
		volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 0);

		if(VOLUME_ERROR_OK != volume_view_slider_value_set(0))
			_E("Failed to set slider value");

		return 0;
	}
	else {
		volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 1);
		volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 0);

		int val = volume_sound_level_get(sound_type);
		retv_if(val == -1, 0);

		if (val == 0) {
			if (VOLUME_ERROR_OK != volume_view_slider_value_set(1))
				_E("Failed to set slider value");
			volume_sound_level_set(sound_type, 1);
		} else {
			if (VOLUME_ERROR_OK != volume_view_slider_value_set(val))
				_E("Failed to set slider value");
		}

		return 1;
	}
}

volume_error_e volume_change_slider_max_value(sound_type_e type)
{
	_D("Slider max change for state: %d", type);
	int ret = 0;
	int step = 0;

	ret = sound_manager_get_max_volume(type, &step);
	if (ret < 0) {
		_E("Failed to get max volume for sound_type: %d", type);
		return VOLUME_ERROR_FAIL;
	}
	_D("Sound type: %d, max: %d", type, step);

	elm_slider_min_max_set(view_info.slider, 0, step);

	view_info.pre_sound_type = type;

	return VOLUME_ERROR_OK;
}

volume_error_e volume_view_slider_value_set(int val)
{
	_D("Slider value set : %d", val);
	retv_if(view_info.is_slider_touching, VOLUME_ERROR_FAIL);
	retv_if(val<0, VOLUME_ERROR_FAIL);

	elm_slider_value_set(view_info.slider, val);

	return VOLUME_ERROR_OK;
}

void volume_view_volume_icon_set(sound_type_e sound_type, int sound, int vibration, bool bt_opened)
{
	_D("Volume icon set");
	char *img = NULL;

	if (sound == -1 || vibration == -1) {
		img = IMG_VOLUME_ICON_MUTE;
		_D("img : %s", img);
		elm_image_file_set(view_info.icon_volume, EDJ_APP, img);
		return;
	}

	switch (sound_type)
	{
	case SOUND_TYPE_RINGTONE:
		if (sound)
			img = IMG_VOLUME_ICON;
		else if (vibration)
			img = IMG_VOLUME_ICON_VIB;
		else
			img = IMG_VOLUME_ICON_MUTE;
		break;
	case SOUND_TYPE_MEDIA:
		img = IMG_VOLUME_ICON_MEDIA;
		break;
	case SOUND_TYPE_CALL:
		if (bt_opened)
			img = IMG_VOLUME_ICON_CALL;
		else
			img = IMG_VOLUME_ICON_CALL;
		break;
	case SOUND_TYPE_NOTIFICATION:
		if (sound)
			img = IMG_VOLUME_ICON_NOTI;
		else if(vibration)
			img = IMG_VOLUME_ICON_NOTI_VIB;
		else
			img = IMG_VOLUME_ICON_NOTI_MUTE;
		break;
	case SOUND_TYPE_ALARM:
		img = IMG_VOLUME_ICON_MEDIA;
		break;
	default:
		img = IMG_VOLUME_ICON;
		break;
	}

	volume_view_set_default_slider();

	_D("img : %s", img);
	elm_image_file_set(view_info.icon_volume, EDJ_APP, img);
}

void volume_view_setting_icon_set(const char *file)
{
	_D("Setting icon image set");
	ret_if(!file);

	Evas_Object *icon_setting = view_info.icon_setting;
	ret_if(!icon_setting);

	if (EINA_TRUE != elm_image_file_set(icon_setting, EDJ_APP, file)) {
		_E("Failed to set image file : %s, Group", EDJ_APP, file);
	};
}

void volume_view_setting_icon_callback_add(void)
{
	_D("Setting callback add");
	ret_if(view_info.is_registered_callback);

	Evas_Object *icon_setting = view_info.icon_setting;
	ret_if(!icon_setting);

	evas_object_event_callback_add(icon_setting, EVAS_CALLBACK_MOUSE_DOWN, _button_mouse_down_cb, NULL);
	evas_object_smart_callback_add(icon_setting, "clicked", _button_cb, NULL);

	view_info.is_registered_callback = EINA_TRUE;
}

void volume_view_setting_icon_callback_del(void)
{
	_D("Setting callback del");
	ret_if(!view_info.is_registered_callback);

	Evas_Object *icon_setting = view_info.icon_setting;
	ret_if(!icon_setting);

	evas_object_smart_callback_del(icon_setting, "clicked", _button_cb );
	evas_object_event_callback_del(icon_setting, EVAS_CALLBACK_MOUSE_DOWN, _button_mouse_down_cb);

	view_info.is_registered_callback = EINA_FALSE;
}

Evas_Object *add_slider(Evas_Object *parent, int min, int max, int val)
{
	retv_if(!parent, NULL);
	Evas_Object *slider = elm_slider_add(parent);
	retvm_if(!slider, NULL, "Failed to add slider");

	elm_slider_horizontal_set(slider, EINA_TRUE);
	elm_slider_indicator_show_set(slider, EINA_FALSE);
	elm_slider_indicator_format_set(slider, "%.0f");
	evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, 0.5);
	elm_slider_min_max_set(slider, min, max);
	elm_slider_value_set(slider, val);

	return slider;
}

volume_error_e volume_view_window_show(sound_type_e type)
{
	_D("Volume view window SHOW is [%p]", view_info.win);

	elm_win_iconified_set(view_info.win, EINA_FALSE);

	if (type == SOUND_TYPE_CALL) {
		_D("Sound type is Call");
	} else {
		volume_view_setting_icon_callback_add();
	}

	return VOLUME_ERROR_OK;
}

volume_error_e volume_view_window_hide(void)
{
	_D("Volume view window HIDE");
	elm_win_iconified_set(view_info.win, EINA_TRUE);
	volume_view_setting_icon_callback_del();

	return VOLUME_ERROR_OK;
}

Evas_Object *add_layout(Evas_Object *parent, const char *file, const char *group)
{
	retvm_if(!parent, NULL, "Invalid argument: parent is NULL\n");
	retvm_if(!file, NULL, "Invalid argument: file is NULL\n");
	retvm_if(!group, NULL, "Invalid argument: group is NULL\n");

	Evas_Object *eo = elm_layout_add(parent);
	retvm_if(!eo, NULL, "Failed to add layout\n");

	int r = -1;
	r = elm_layout_file_set(eo, file, group);
	if (!r) {
		_E("Failed to set file[%s]\n", file);
		evas_object_del(eo);
		return NULL;
	}

	evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(eo);

	return eo;
}

void _connect_to_wm(Evas_Object *win)
{
	_D("Mack connection with window manager");

	tzsh_window tz_win;

	view_info.tzsh = tzsh_create(TZSH_TOOLKIT_TYPE_EFL);
	if (!view_info.tzsh) {
		_E("Failed to get connection to Tizen window manager");
	}

	tz_win = elm_win_window_id_get(win);
	if (!tz_win) {
		_E("Failed to get Tizen window manager");
	}

	view_info.volume_service = tzsh_volume_service_create(view_info.tzsh, tz_win);
	if (!view_info.volume_service) {
		_E("Failed to get volume service");
	}
}

static void _down_for_hide(void *data, Evas_Object *obj, const char* emission, const char* source)
{
	LOGD("Down for HIDE");

	volume_control_hide_view();
}

volume_error_e volume_view_layout_create(Evas_Object *win)
{
	LOGD("Layout create");

	retv_if(!win, VOLUME_ERROR_FAIL);

	Evas_Object *ly_outer = add_layout(win, EDJ_APP, "volume_layout");
	retv_if(!ly_outer, VOLUME_ERROR_FAIL);
	elm_win_resize_object_add(win, ly_outer);
	elm_object_signal_callback_add(ly_outer, "hide,popup", "event", _hide_launcher, NULL);
	view_info.ly_outer = ly_outer;

	/* make setting icon */
	Evas_Object *icon_setting = _setting_icon_make();
	retv_if(!icon_setting, VOLUME_ERROR_FAIL);
	elm_object_part_content_set(ly_outer, "ic_setting", icon_setting);
	view_info.icon_setting = icon_setting;

	/* make volume icon */
	Evas_Object *icon_volume = _volume_icon_make();
	retv_if(!icon_volume, VOLUME_ERROR_FAIL);
	elm_object_part_content_set(ly_outer, "ic_sound", icon_volume);
	view_info.icon_volume = icon_volume;

	/* make slider */
	Evas_Object *slider = _slider_make();
	retv_if(!slider, VOLUME_ERROR_FAIL);
	view_info.slider = slider;
	elm_object_part_content_set(ly_outer, "sw.slider", slider);

	/* add callback for hide */
	elm_object_signal_callback_add(ly_outer, "hide,volume", "hide", _down_for_hide, NULL);

	return VOLUME_ERROR_OK;
}

static void _iconified_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("ICONIFIED IS CALLED");
}

Evas_Object *add_volume_window(const char *name)
{
	Evas_Object *eo = NULL;
	Evas *evas = NULL;
	int x, y, w, h = 0;

	eo = elm_win_add(NULL, name, ELM_WIN_NOTIFICATION);
	retv_if(!eo, NULL);
	evas = evas_object_evas_get(eo);
	retv_if(!evas, NULL);
	view_info.evas = evas;

	elm_win_alpha_set(eo, EINA_TRUE);
	elm_win_title_set(eo, name);
	elm_win_borderless_set(eo, EINA_TRUE);
	ecore_evas_name_class_set(ecore_evas_ecore_evas_get(evas), "SYSTEM_POPUP", "SYSTEM_POPUP");
	elm_win_prop_focus_skip_set(eo, EINA_TRUE);
	elm_win_role_set(eo, "no-dim");

	elm_win_screen_size_get(eo, &x, &y, &w, &h);
	_D("volume screen size => x: %d, y: %d, w: %d, h: %d", x, y, w, h);

	evas_object_smart_callback_add(eo, "iconified", _iconified_cb, NULL);

	return eo;
}

static Eina_Bool _key_grab_cb(void *data)
{
	int ret_up = 0;
	int ret_down = 0;
	Evas_Object *win = data;

	_D("keygrab window is [%p]", win);
	ret_up = elm_win_keygrab_set(win, KEY_VOLUMEUP, 0, 0, 0, ELM_WIN_KEYGRAB_SHARED);
	_D("Result of volume up keygrab set : %d", ret_up);
	ret_down = elm_win_keygrab_set(win, KEY_VOLUMEDOWN, 0, 0, 0, ELM_WIN_KEYGRAB_SHARED);
	_D("Result of volume down keygrab set : %d", ret_down);

	return EINA_FALSE;
}

Evas_Object *volume_view_window_create(void)
{
	Evas_Object *win = add_volume_window(PACKAGE);
	retv_if(!win, NULL);
	_D("window is [%p]", win);

	view_info.win = win;
	_D("view_info.win is [%p]", view_info.win);

	evas_object_show(win);

	ecore_timer_add(1.0f, _key_grab_cb, win);

	_connect_to_wm(win);

	elm_win_iconified_set(win, EINA_TRUE);

	return win;
}

void _lock_sound_check(void)
{
	int lcdoff_source = -1;
	int lock_sound = -1;
	int lock_type = -1;

	if (vconf_get_int(VCONFKEY_PM_LCDOFF_SOURCE, &lcdoff_source) < 0) {
		_E("Failed to get vconfkey : VCONFKEY_PM_LCDOFF_SOURCE");
		return;
	}
	_D("lcd off source : %d", lcdoff_source);

	if (lcdoff_source != VCONFKEY_PM_LCDOFF_BY_POWERKEY) {
		_E("Should not play lock sound");
		return;
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_SOUND_LOCK_BOOL, &lock_sound) < 0) {
		_E("Failed to get vconfkey : VCONFKEY_SETAPPL_SOUND_LOCK_BOOL");
		return;
	}
	_D("lock sound : %d", lock_sound);

	if (lock_sound) {
		if (vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_type) < 0) {
			_E("Failed to get vconfkey : VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT");
			lock_type = 0;
		}
		_D("lock type : %d", lock_type);

		feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_LOCK);
	}
}

Evas_Object* show_lockscreen_splash(const char* bg_path)
{
	_D(" ");

	retv_if(bg_path == NULL, NULL);

	if (view_info.lockscreen_splash) {
		_E("There is remain fake_bg : remove old one");
		evas_object_del(view_info.lockscreen_splash);
		view_info.lockscreen_splash = NULL;
	}

	/* Check Lock Sound */
	_lock_sound_check();

	/* hide volume window */
	if(VOLUME_ERROR_OK != volume_view_window_hide())
		_E("Failed to hide window");

	Evas_Object *win_splash = elm_win_add(NULL,"lockscreen_splash", ELM_WIN_NOTIFICATION);

	if (!win_splash) {
		_E("Failed to add splash window");
		return NULL;
	}

	_D("Splash window created");

	Evas_Object *bg = elm_bg_add(win_splash);
	if (!bg) {
		_D("Failed to get background");
		evas_object_del(win_splash);
		return NULL;
	}

	elm_win_resize_object_add(win_splash, bg);
	elm_bg_file_set(bg, bg_path, NULL);
	evas_object_show(bg);

	evas_object_show(win_splash);

	_D("Splash window shown");

	view_info.lockscreen_splash = win_splash;

	return win_splash;
}

volume_error_e hide_lockscreen_splash(void)
{
	_D(" ");

	if (!view_info.lockscreen_splash) {
		_E("No splash window found");
		return VOLUME_ERROR_FAIL;
	}

	evas_object_del(view_info.lockscreen_splash);
	view_info.lockscreen_splash = NULL;
	_D("Splash window closed");

	return VOLUME_ERROR_OK;
}
static void _slider_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	view_info.is_slider_touching = EINA_TRUE;

	volume_timer_del(TYPE_TIMER_POPUP);

	volume_timer_add(0.2, TYPE_TIMER_SLIDER);
}

static void _slider_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	volume_timer_del(TYPE_TIMER_POPUP);

	double val = 0;

	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	val = elm_slider_value_get(view_info.slider);
	val += 0.5;
	_D("slider value : %d", val);

	if (view_info.is_slider_touching)
		return;

	volume_sound_change_set((int)val);

	if (!volume_key_event_is_pressing_get()) {
		volume_timer_add(3.0, TYPE_TIMER_POPUP);
		return;
	}

	if (!view_info.is_slider_touching) {
		volume_timer_add(3.0, TYPE_TIMER_POPUP);
		return;
	}
}

static void _slider_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	view_info.is_slider_touching = EINA_FALSE;

	volume_timer_del(TYPE_TIMER_SLIDER);

	Evas_Object *slider = volume_view_slider_get();
	ret_if(slider == NULL);

	double val = 0;

	val = elm_slider_value_get(slider);
	val += 0.5;
	_D("slider value : %d", (int)val);

	volume_sound_change_set((int)val);

	volume_timer_add(3.0, TYPE_TIMER_POPUP);
}

static void _hide_launcher(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	evas_object_hide(view_info.win);
}

static void _button_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	volume_view_setting_icon_set(IMG_VOLUME_ICON_SETTINGS_PRESSED);
}

static void _app_control_error_check(int ret)
{
	if (ret != APP_CONTROL_ERROR_NONE) {
		switch (ret)
		{
		case APP_CONTROL_ERROR_INVALID_PARAMETER :
			_E("error : APP_CONTROL_ERROR_INVALID_PARAMETER");
			break;
		case APP_CONTROL_ERROR_OUT_OF_MEMORY :
			_E("error : APP_CONTROL_ERROR_OUT_OF_MEMORY");
			break;
		case APP_CONTROL_ERROR_APP_NOT_FOUND :
			_E("error : APP_CONTROL_ERROR_APP_NOT_FOUND");
			break;
		case APP_CONTROL_ERROR_LAUNCH_REJECTED :
			_E("error : APP_CONTROL_ERROR_LAUNCH_REJECTED");
			break;
		default :
			_E("error : %d", ret);
			break;
		}
	}
}

static void _button_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("volume setting is clicked");
	int ret = -1;
	app_control_h svc;

	if (evas_object_visible_get(view_info.win)) {
		volume_timer_del(TYPE_TIMER_POPUP);

		ret = app_control_create(&svc);
		_app_control_error_check(ret);

		ret = app_control_set_app_id(svc, "org.tizen.setting.volume");
		_app_control_error_check(ret);

		ret = app_control_send_launch_request(svc, NULL, NULL);
		_app_control_error_check(ret);

		if (VOLUME_ERROR_OK != volume_control_pause())
			_E("Failed to pause volume");

		app_control_destroy(svc);
	}
}

static Evas_Object* _setting_icon_make()
{
	_D("Setting ICON make");
	Evas_Object *icon_setting = elm_icon_add(view_info.ly_outer);
	retv_if(!icon_setting, NULL);

	evas_object_size_hint_aspect_set(icon_setting, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	elm_image_resizable_set(icon_setting, EINA_TRUE, EINA_TRUE);

	view_info.icon_setting = icon_setting;

	volume_view_setting_icon_set(IMG_VOLUME_ICON_SETTINGS);
	evas_object_show(icon_setting);

	return icon_setting;
}

static Evas_Object* _volume_icon_make()
{
	Evas_Object *icon_volume = elm_icon_add(view_info.ly_outer);
	retv_if(!icon_volume, NULL);

	evas_object_size_hint_aspect_set(icon_volume, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	elm_image_resizable_set(icon_volume, EINA_TRUE, EINA_TRUE);
	evas_object_show(icon_volume);

	return icon_volume;
}

static Evas_Object* _slider_make()
{
	_D("Volume Slider Make");
	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	int sound_step = volume_sound_sound_manager_step_get(sound_type);
	_D("sound step : %d", sound_step);

	int sound_val = volume_sound_level_get(sound_type);
	if (sound_val == -1) {
		_E("Failed to get volume level");
		sound_val = 0;
	}
	_D("sound value : %d", sound_val);

	Evas_Object *slider = add_slider(view_info.ly_outer, 0, sound_step, sound_val);
	retv_if(!slider, NULL);
	evas_object_smart_callback_add(slider, "slider,drag,start", _slider_start_cb, NULL);
	evas_object_smart_callback_add(slider, "changed", _slider_changed_cb, NULL);
	evas_object_smart_callback_add(slider, "slider,drag,stop", _slider_stop_cb, NULL);

	return slider;
}

