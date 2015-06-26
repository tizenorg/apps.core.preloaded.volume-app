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


#include <appcore-common.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <feedback.h>

#include "main.h"
#include "_util_log.h"
#include "view.h"
#include "control.h"
#include "sound.h"
#include "bt.h"

struct _sound_s_info {
	int sound_step;
	int sound_status;
	int vibration_status;
	Eina_Bool is_vibration;
};

static struct _sound_s_info sound_info = {
	.sound_step = 0,
	.sound_status = 0,
	.vibration_status = 0,
	.is_vibration = EINA_FALSE,
};

static void _sound_status_changed_cb(keynode_t *key, void *data);
static int _setting_sound_enabled(void);

int volume_sound_step_get(void)
{
	return sound_info.sound_step;
}

int volume_sound_sound_status_get(void)
{
	return sound_info.sound_status;
}

int volume_sound_vibration_status_get(void)
{
	return sound_info.vibration_status;
}

Eina_Bool volume_sound_is_vibration_get(void)
{
	return sound_info.is_vibration;
}

void volume_sound_is_vibration_set(Eina_Bool val)
{
	sound_info.is_vibration = val;
}

void volume_sound_feedback_play(int feedback_type, int pattern)
{
	_D("Feedback type : %d / pattern : %d", feedback_type, pattern);

	if (FEEDBACK_ERROR_NONE != feedback_play_type(feedback_type, pattern))
		_E("Failed to play feedback");
}

void volume_sound_vib_play(void)
{
	ret_if(sound_info.is_vibration);

	_D("Play Feedback : vibration");

	//if (FEEDBACK_ERROR_NONE != feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_VIBRATION_ON))
	//	_E("Failed to play feedback");

	sound_info.is_vibration = EINA_TRUE;
}

void volume_sound_play(void)
{
	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	ret_if(sound_type == SOUND_TYPE_ALARM);
	ret_if(sound_type == SOUND_TYPE_MEDIA);
	ret_if(sound_type == SOUND_TYPE_VOICE);
	ret_if(sound_type == SOUND_TYPE_NOTIFICATION);

	_D("Play Feedback : sound");
	if(FEEDBACK_ERROR_NONE != feedback_play(FEEDBACK_PATTERN_VOLUME_KEY)) {
		_E("Failed to play feedback");
	}

	return;
}

void volume_sound_setting_sound_play(void)
{
	int snd_enabled = _setting_sound_enabled();
	_D("snd_enabled(%d)", snd_enabled);

	if(snd_enabled == 1)
		volume_sound_feedback_play(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TOUCH_TAP);
}

int volume_sound_sound_manager_step_get(sound_type_e type)
{
	_D("volume sound manager step get");
	int ret = -1, step = 0;

	ret = sound_manager_get_max_volume(type, &step);
	retvm_if(ret < 0, -1, "Failed to get step");

	sound_info.sound_step = step;

	return step;
}

int volume_sound_sound_manager_volume_get(sound_type_e type)
{
	int ret = -1, val = 0;
	ret = sound_manager_get_volume(type, &val);
	retvm_if(ret < 0, -1, "Failed to get volume");
	return val;
}

sound_type_e volume_sound_sound_manager_type_get(void)
{
	_D(" Volume sound manager type get");
	int ret = 0;
	sound_type_e sound_type = -1;

	ret = sound_manager_get_current_sound_type(&sound_type);
	_D("ret: %d, sound type: %d", ret, sound_type);
	switch (ret) {
	case SOUND_MANAGER_ERROR_NONE:
		_D("Error none");
		break;
	case SOUND_MANAGER_ERROR_NO_PLAYING_SOUND:
		_D("NO playing sound");
		sound_type = SOUND_TYPE_RINGTONE;
		break;
	default:
		_E("Failed to get sound type : %d", ret);
		sound_type = SOUND_TYPE_RINGTONE;
		return sound_type;
	}

	if(sound_type == SOUND_TYPE_SYSTEM)
		sound_type = SOUND_TYPE_RINGTONE;

	return sound_type;
}

void _mm_func(sound_type_e type, unsigned int volume, void *data)
{
	bool bt_opened = false;

	_D("mm func is called type : %d, volume : %d", type, volume);
	if (type == volume_control_get_sound_type_at_show()) {
		_D("current show type : %d, volume : %d", type, volume);
		/* Need to check sound type & device in media type volume. */
		if (type == SOUND_TYPE_MEDIA) {
			int sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
			_D("sound status : %d", sound);

			int vibration = volume_sound_vconf_status_get(TYPE_VCONF_VIBRATION_STATUS);
			_D("vibration : %d", vibration);
			volume_view_volume_icon_set(type, sound, vibration, bt_opened);
		}
		if (VOLUME_ERROR_OK != volume_view_slider_value_set(volume))
			_E("Failed to set slider value");
	}
}

void volume_sound_mm_sound_init(void)
{
	_D("MM sound Init");
	int ret = sound_manager_set_volume_changed_cb(_mm_func, NULL);
	if (ret != SOUND_MANAGER_ERROR_NONE)
		_E("Failed to set volume changed event[%d]", ret);
}

int volume_sound_level_get(sound_type_e type)
{
	int val = -1;
	if (sound_manager_get_volume(type, &val) < 0) {
		_E("Failed to get Volume step");
		return -1;
	}

	return val;
}

int volume_sound_level_set(sound_type_e type, int val)
{
	int ret = -1;
	ret = sound_manager_set_volume(type, val);

	return ret;
}

void volume_sound_change_set(int val)
{
	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	int sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	if (val) {
		if (sound_type != SOUND_TYPE_MEDIA) {
			if (!volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS)) {
				volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 1);
				volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 0);
			}
		}
		if (volume_sound_level_set(sound_type, val) != 0)
			volume_sound_level_set(sound_type, 9);

		volume_sound_play();
	} else {
		if (sound_type != SOUND_TYPE_MEDIA)
			volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 0);
		volume_view_slider_value_set(0);
		volume_sound_level_set(sound_type, 0);

		if (sound_type == SOUND_TYPE_RINGTONE) {
			volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 1);
			volume_sound_vib_play();
		}
	}
}

void volume_sound_vconf_status_set(volume_vconf_type_e type, int val)
{
	char *vconfkey = NULL;
	switch (type)
	{
	case TYPE_VCONF_SOUND_STATUS:
		vconfkey = VCONFKEY_SETAPPL_SOUND_STATUS_BOOL;
		break;
	case TYPE_VCONF_VIBRATION_STATUS:
		vconfkey = VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL;
		break;
	default:
		_E("Failed to set vconfkey : Type error");
		return;
	}

	if (vconf_set_bool(vconfkey, val) < 0)
		_E("Failed to set vconfkey : %s", vconfkey);
}

int volume_sound_vconf_status_get(volume_vconf_type_e type)
{
	int ret = -1;
	char *vconfkey = NULL;

	switch (type)
	{
	case TYPE_VCONF_SOUND_STATUS:
		vconfkey = VCONFKEY_SETAPPL_SOUND_STATUS_BOOL;
		break;
	case TYPE_VCONF_VIBRATION_STATUS:
		vconfkey = VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL;
		break;
	default:
		_E("Failed to get vconfkey : Type error");
		return -1;
	}

	if (vconf_get_bool(vconfkey, &ret) < 0) {
		_E("Failed to get vconfkey : %s", vconfkey);
		return -1;
	}

	return ret;
}

void volume_sound_vconfkey_register(void)
{
	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, _sound_status_changed_cb, NULL) < 0)
		_E("Failed to notify vconfkey : VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, _sound_status_changed_cb, NULL) < 0)
		_E("Failed to notify vconfkey : VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL");
	_sound_status_changed_cb(NULL, NULL);
}

void volume_sound_vconfkey_unregister(void)
{
	if (vconf_ignore_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, _sound_status_changed_cb) < 0)
		_E("Failed to ignore vconfkey : VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

	if (vconf_ignore_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, _sound_status_changed_cb) < 0)
		_E("Failed to ignore vconfkey : VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL");
}

static int _setting_sound_enabled(void)
{
	int snd_status = 0;

	if (vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd_status) < 0)
		_E("Failed to get vconfkey : VCONFKEY_SETAPPL_SOUND_STATUS_BOOL");

	if (snd_status == 1)
		return 1;

	return 0;
}

static void _sound_status_changed_cb(keynode_t *key, void *data)
{
	bool bt_opened = false;
	const char *keyname = vconf_keynode_get_name(key);
	ret_if(!keyname);
	_D("keyname : %s", keyname);

	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	int sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);
	sound_info.sound_status = sound;

	int vibration = volume_sound_vconf_status_get(TYPE_VCONF_VIBRATION_STATUS);
	_D("vibration : %d", vibration);
	sound_info.vibration_status = vibration;

	if (sound == 0 && vibration == 1)
		_D("vibration mode");
	else if (sound == 1 && vibration == 0) {
		_D("sound mode");
		sound_info.is_vibration = EINA_FALSE;
	} else if (sound == 0 && vibration == 0) {
		_D("mute mode");
		sound_info.is_vibration = EINA_FALSE;
	}

	volume_view_volume_icon_set(sound_type, sound, vibration, bt_opened);
}

