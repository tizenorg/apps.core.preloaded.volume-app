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

#include <Ecore.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <feedback.h>
#include <bluetooth.h>
#include <bluetooth_internal.h>
#include <bluetooth_extention.h>
#include <app.h>

#include "main.h"
#include "_util_log.h"
#include "_util_efl.h"
#include "view.h"
#include "control.h"
#include "sound.h"
#include "timer.h"
#include "key_event.h"
#include "bt.h"

#define VCONFKEY_ALARM_VOLUME_POPUP_ENABLE  "memory/alarm/volume_popup_enable"
#define VOLUME_INPUT_WIN_NAME "volumekey-input-window"

struct _key_event_s_info {
	Ecore_X_Window input_win;

	Ecore_Event_Handler *handler_volume_up;
	Ecore_Event_Handler *handler_volume_down;
	Ecore_Event_Handler *handler_qp_state_check;

	Eina_Bool is_mute;
	Eina_Bool is_pressing;

	int count_grabed;

	int last_value_in_media;
};

static struct _key_event_s_info key_event_info = {
	.input_win = 0,

	.handler_volume_up = NULL,
	.handler_volume_down = NULL,
	.handler_qp_state_check = NULL,

	.is_mute = EINA_FALSE,
	.is_pressing = EINA_FALSE,

	.count_grabed = 0,

	.last_value_in_media = 0,
};

static Eina_Bool _key_release_cb(void *data, int type, void *event);
static Eina_Bool _key_press_cb(void *data, int type, void *event);
static volume_error_e _volume_popup_check_in_alarm_type(sound_type_e sound_type);
static volume_error_e _volume_down_key_press(sound_type_e sound_type, int sound, bool bt_opened);
static volume_error_e _volume_up_key_press(sound_type_e sound_type, int sound, bool bt_opened);
static volume_error_e _mute_key_press();

Ecore_Event_Handler* volume_key_event_handler_volume_up_get(void)
{
	return key_event_info.handler_volume_up;
}

Ecore_Event_Handler* volume_key_event_handler_volume_down_get(void)
{
	return key_event_info.handler_volume_down;
}

Eina_Bool volume_key_event_is_pressing_get(void)
{
	return key_event_info.is_pressing;
}

int volume_key_event_count_grabed_get(void)
{
	return key_event_info.count_grabed;
}

void volume_key_event_handler_add(void)
{
	if(!key_event_info.handler_volume_up)
		key_event_info.handler_volume_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_release_cb, NULL);

	if(!key_event_info.handler_volume_down)
		key_event_info.handler_volume_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_press_cb, NULL);
}

void volume_key_event_handler_del(void)
{
	ret_if(key_event_info.handler_volume_up == NULL);
	ecore_event_handler_del(key_event_info.handler_volume_up);
	key_event_info.handler_volume_up= NULL;

	ret_if(key_event_info.handler_volume_down == NULL);
	ecore_event_handler_del(key_event_info.handler_volume_down);
	key_event_info.handler_volume_down = NULL;
}

static volume_error_e _mute_key_press()
{
	int lastval = -1;
	int lock = IDLELOCK_ON;
	int status = 0;
	int sound = 0;
	int error = 0;
	bool bt_opened = false;
	sound_type_e sound_type = 0;

	status = volume_control_check_status(&lock, &sound_type);
	_D("status: %d, lock: %d, sound type : %d", status, lock, sound_type);

	sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	error = bt_ag_is_sco_opened(&bt_opened);
	if(error != BT_ERROR_NONE)
		_E("bt_ag_is_sco_opened return [%d]", error);

	_D("BT state %d", bt_opened);

	volume_control_show_view(status, sound_type, sound, bt_opened);

	if (sound_type == SOUND_TYPE_MEDIA) {
		if (key_event_info.is_mute == EINA_FALSE) {
			_D("media is playing. set media volume to 0.");
			lastval = volume_sound_level_get(sound_type);
			retv_if(lastval == -1, VOLUME_ERROR_FAIL);

			key_event_info.last_value_in_media = lastval;
			volume_sound_level_set(sound_type, 0);
			if (VOLUME_ERROR_OK != volume_view_slider_value_set(0)) {
				_E("Failed to set slider value");
				return VOLUME_ERROR_FAIL;
			}
			key_event_info.is_mute = EINA_TRUE;
			return VOLUME_ERROR_OK;
		} else {
			_D("toggle the mute key to normal in media. last value in media : %d", key_event_info.last_value_in_media);
			volume_sound_level_set(sound_type, lastval);
			if (VOLUME_ERROR_OK != volume_view_slider_value_set(lastval)) {
				_E("Failed to set slider value");
				return VOLUME_ERROR_FAIL;
			}
			key_event_info.is_mute = EINA_FALSE;
			return VOLUME_ERROR_OK;
		}
	} else {
		if (lock == IDLELOCK_ON) {
			_D("lock is on, block the MUTE key");
			return VOLUME_ERROR_OK;
		}

		if (volume_mute_toggle_set())
			volume_sound_feedback_play(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_GENERAL);

		return VOLUME_ERROR_OK;
	}
}

static volume_error_e _volume_up_key_press(sound_type_e sound_type, int sound, bool bt_opened)
{
	int sound_step = 0;
	int sound_level = 0;
	int vibration = 0;

	_D("Volume Up Key Pressed");
	key_event_info.is_mute = EINA_FALSE;

	sound_step = volume_sound_step_get();
	_D("sound step : %d", sound_step);

	sound_level = volume_sound_level_get(sound_type);
	retv_if(sound_level == -1, VOLUME_ERROR_FAIL);
	_D("sound level : %d", sound_level);

	vibration = volume_sound_vconf_status_get(TYPE_VCONF_VIBRATION_STATUS);
	_D("vibration : %d", vibration);

	if (elm_object_disabled_get(volume_view_slider_get()))
		elm_object_disabled_set(volume_view_slider_get(), EINA_FALSE);

	if (sound_type == SOUND_TYPE_RINGTONE) {
		if (!sound) {
			/* Check sound status change case. */
			if (!vibration) {
				_D("mute -> vib.");
				volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 1);
				volume_sound_vib_play();
			}
			else {
				_D("vib -> sound");
				volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 1);
				volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 0);
				volume_sound_level_set(sound_type, sound_level+1);
				volume_view_slider_value_set(sound_level+1);
				_D("new sound value: %d", sound_level+1);
			}
		}
		else {
			/*adjust the sound level normally */
			if (sound_level != sound_step) {
				volume_sound_level_set(sound_type, sound_level+1);
				volume_view_slider_value_set(sound_level+1);
				_D("new sound value: %d", sound_level+1);
			}
		}
	}
	else if (sound_type == SOUND_TYPE_NOTIFICATION) {
		if (!sound) {
			/* No sound in notification type. */
			volume_view_slider_value_set(0);
			elm_object_disabled_set(volume_view_slider_get(), EINA_TRUE);
		}
		else {
			/*adjust the sound level normally */
			if (sound_level != sound_step) {
				volume_sound_level_set(sound_type, sound_level+1);
				volume_view_slider_value_set(sound_level+1);
				_D("new sound value: %d", sound_level+1);
			}
		}
	}
	/* Sound type is not ringtone. Need to adjust sound level */
	else if (sound_type == SOUND_TYPE_CALL && bt_opened) {
		int bt_vol = 0;
		if (bt_ag_get_speaker_gain(&bt_vol) != BT_ERROR_NONE)
			_E("Getting bt volume is failed");

		_D("BT VOLUME : %d", bt_vol);

		if (bt_vol != sound_step) {
			if(bt_ag_notify_speaker_gain(bt_vol+1) != BT_ERROR_NONE)
				_E("Setting bt volume is failed");
			volume_view_slider_value_set(bt_vol+1);
			_D("New BT VOLUME : %d", bt_vol+1);
		}
	}
	else {
		if (sound_level != sound_step ) {
			volume_sound_level_set(sound_type, sound_level+1);
			volume_view_slider_value_set(sound_level+1);
			_D("new sound value: %d", sound_level+1);
		}
	}

	if (sound_type != SOUND_TYPE_ALARM)
		volume_sound_play();

	volume_timer_del(TYPE_TIMER_SU);
	volume_timer_del(TYPE_TIMER_SD);
	volume_timer_add(0.5, TYPE_TIMER_SU);

	if (!volume_timer_su_timer_get()) {
		_E("Failed to get SUTIMER");
		return VOLUME_ERROR_FAIL;
	}

	return VOLUME_ERROR_OK;
}

static volume_error_e _volume_down_key_press(sound_type_e sound_type, int sound, bool bt_opened)
{
	key_event_info.is_mute = EINA_FALSE;

	int val = volume_sound_level_get(sound_type);
	retv_if(val == -1, VOLUME_ERROR_FAIL);

	int sound_st = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound_st);

	int vibration = volume_sound_vconf_status_get(TYPE_VCONF_VIBRATION_STATUS);
	_D("vibration : %d", vibration);

	if (elm_object_disabled_get(volume_view_slider_get()))
		elm_object_disabled_set(volume_view_slider_get(), EINA_FALSE);

	if(sound_type == SOUND_TYPE_RINGTONE){
		if (!sound)
			/* Check sound status change case. */
			_D("Do nothing.");
		else {
			if (val != 0) {
				volume_sound_level_set(sound_type, val - 1);
				volume_view_slider_value_set(val - 1);
				/*adjust the sound level normally */
				if (val == 1) {
					volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 0);
					volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 1);
					volume_sound_vib_play();
				}
			}
		}
	}
	else if (sound_type == SOUND_TYPE_NOTIFICATION) {
		if(!sound) {
			/* No sound in notification type. */
			volume_view_slider_value_set(0);
			elm_object_disabled_set(volume_view_slider_get(), EINA_TRUE);
		}
		else {
			/*adjust the sound level normally */
			if (val != 0) {
				volume_sound_level_set(sound_type, val-1);
				volume_view_slider_value_set(val-1);
				_D("new sound value: %d", val-1);
			}
		}
	}
	else if(sound_type == SOUND_TYPE_CALL && bt_opened) {
		int bt_vol = 0;
		if(bt_ag_get_speaker_gain(&bt_vol) != BT_ERROR_NONE)
			_E("Getting bt volume is failed");

		_D("BT VOLUME : %d", bt_vol);
		if(bt_ag_notify_speaker_gain(bt_vol-1) != BT_ERROR_NONE)
			_E("Setting bt volume is failed");
		volume_view_slider_value_set(bt_vol-1);

		_D("New BT VOLUME : %d", bt_vol-1);
	}
	/* Sound type is not ringtone. Need to adjust sound level */
	else {
		if (val != 0) {
			volume_sound_level_set(sound_type, val - 1);
			volume_view_slider_value_set(val - 1);
			_D("new sound value: %d", val-1);
		}
	}

	if(sound_type != SOUND_TYPE_ALARM)
		volume_sound_play();

	volume_timer_del(TYPE_TIMER_SD);
	volume_timer_del(TYPE_TIMER_SU);
	volume_timer_add(0.5, TYPE_TIMER_SD);

	return VOLUME_ERROR_OK;
}

static volume_error_e _volume_popup_check_in_alarm_type(sound_type_e sound_type)
{
	int is_enabled = 0;

	if(sound_type == SOUND_TYPE_ALARM) {
		_D("Sound type is Alarm Type");
		if(vconf_get_bool(VCONFKEY_ALARM_VOLUME_POPUP_ENABLE, &is_enabled) < 0) {
			_E("Failed to get vconfkey : VCONFKEY_ALARM_VOLUME_POPUP_ENABLE");
			return VOLUME_ERROR_FAIL;
		}
		_D("volume popup enabled in alarm type : %d", is_enabled);

		if(!is_enabled) {
			_D("alarm type but vconf for the volume popup is disabled");
			return VOLUME_ERROR_FAIL;
		}
	}

	return VOLUME_ERROR_OK;
}

static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	int sound = 0;
	int lock = IDLELOCK_ON;
	int key_status = 0;
	int status = 0;
	int error = 0;
	bool bt_opened = false;
	sound_type_e sound_type = 0;
	Evas_Object *win = NULL;
	Ecore_Event_Key *ev = NULL;

	ev = (Ecore_Event_Key*) event;
	retv_if(!ev, ECORE_CALLBACK_CANCEL);

	_D("Key Press CB : %s", ev->keyname);

	win = volume_view_win_get();
	retv_if(!win, ECORE_CALLBACK_CANCEL);

	if(!strncmp(ev->keyname, KEY_CANCEL, strlen(KEY_CANCEL)) || !strncmp(ev->keyname, KEY_BACK, strlen(KEY_BACK))) {
		_D("%s is pressed", ev->keyname);
		return ECORE_CALLBACK_CANCEL;
	}

	if(volume_view_is_slider_touching_get()) {
		_E("Failed to show volume : is_slider_touching is EINA_TRUE");
		return ECORE_CALLBACK_CANCEL;
	}

	if(vconf_get_int(VCONFKEY_STARTER_USE_VOLUME_KEY, &key_status) < 0) {
		_E("Failed to get vconf : VCONFKEY_STATER_USE_VOLUME_KEY");
		return ECORE_CALLBACK_CANCEL;
	}
	retvm_if(key_status == 2, ECORE_CALLBACK_CANCEL, "starter use volume key. status : 2");

#ifndef FEATURE_SDK
	int is_call = -1;
	if(!strncmp(ev->keyname, KEY_MUTE, strlen(KEY_MUTE))) {
		_D("MUTE key is pressed");
		if(VOLUME_ERROR_OK != _mute_key_press())
			_E("Failed to press MUTE key");
		return ECORE_CALLBACK_CANCEL;
	}

	if(vconf_get_int(VCONFKEY_TELEPHONY_CALL_STATE, &is_call) < 0 || is_call < 0) {
		_E("Failed to get call state vconf");
		return ECORE_CALLBACK_CANCEL;
	}
	if(is_call>0) {
		_D("Call is active");
		return ECORE_CALLBACK_CANCEL;
	}
#endif

	status = volume_control_check_status(&lock, &sound_type);
	_D("status: %d, lock: %d, sound type : %d", status, lock, sound_type);

	sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	error = bt_ag_is_sco_opened(&bt_opened);
	if(error != BT_ERROR_NONE)
		_E("bt_ag_is_sco_opened return [%d]", error);

	_D("BT state %d", bt_opened);

	if(VOLUME_ERROR_OK != _volume_popup_check_in_alarm_type(sound_type)) {
		_E("Failed to set volume popup");
		return ECORE_CALLBACK_CANCEL;
	}

	volume_control_show_view(status, sound_type, sound, bt_opened);

	key_event_info.is_pressing = EINA_TRUE;

	volume_timer_del(TYPE_TIMER_POPUP);

	if (!strncmp(ev->keyname, KEY_VOLUMEUP, strlen(KEY_VOLUMEUP))) {
		if(VOLUME_ERROR_OK != _volume_up_key_press(sound_type, sound, bt_opened))
			_E("Failed to press volume up key");
	}
	else if (!strncmp(ev->keyname, KEY_VOLUMEDOWN, strlen(KEY_VOLUMEDOWN))) {
		if(VOLUME_ERROR_OK != _volume_down_key_press(sound_type, sound, bt_opened))
			_E("Failed to press volume down key");
	}

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _key_release_cb(void *data, int type, void *event)
{
	Ecore_Event_Key *ev = event;
	retv_if(ev == NULL, ECORE_CALLBACK_CANCEL);
	_D("Key Release CB : %s", ev->keyname);

	Evas_Object *win = volume_view_win_get();
	retv_if(win == NULL, ECORE_CALLBACK_CANCEL);

	key_event_info.is_pressing = EINA_FALSE;

	if(!strncmp(ev->keyname, KEY_CANCEL, strlen(KEY_CANCEL))) {
		_D("%s is released", ev->keyname);
		if(VOLUME_ERROR_OK != volume_control_hide_view())
			_E("Failed to close volume");
		if(VOLUME_ERROR_OK != volume_control_cache_flush())
			_E("Failed to flush cache");
		return ECORE_CALLBACK_CANCEL;
	}

	if(!strncmp(ev->keyname, KEY_BACK, strlen(KEY_BACK))) {
		_D("BACK Key is released");
		return ECORE_CALLBACK_CANCEL;
	}

	if (!strncmp(ev->keyname, KEY_VOLUMEUP, strlen(KEY_VOLUMEUP))) {
		_D("up key released and del timer");
		volume_timer_del(TYPE_TIMER_SU);
	}
	else if (!strncmp(ev->keyname, KEY_VOLUMEDOWN, strlen(KEY_VOLUMEDOWN))) {
		_D("down key released and del timer");
		volume_timer_del(TYPE_TIMER_SD);
	}

	volume_timer_del(TYPE_TIMER_POPUP);

	if(volume_view_is_slider_touching_get() == EINA_FALSE)
		volume_timer_add(3.0, TYPE_TIMER_POPUP);

	_D("key release fini");
	return ECORE_CALLBACK_CANCEL;
}

