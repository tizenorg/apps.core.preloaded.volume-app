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
#include <bluetooth.h>
#include <bluetooth_internal.h>
#include <bluetooth_extention.h>

#include "main.h"
#include "_util_log.h"
#include "view.h"
#include "control.h"
#include "sound.h"
#include "timer.h"

static void _bt_display_bt_volume_view(sound_type_e sound_type, int sound, int vibration, bool bt_opened);
static void _bt_volume_changed_cb(int volume, void *user_data);
static void _bt_state_changed_cb(int result, bool opened, void *user_data);
static int _bt_register_changed_cb(void);
static int _bt_unregister_changed_cb(void);

int bt_get_bt_volume(void)
{
	int ret = BT_ERROR_NONE;
	int bt_vol = 0;

	ret = bt_ag_get_speaker_gain(&bt_vol);
	if (ret != BT_ERROR_NONE)
	{
		_E("bt_ag_get_speaker_gain Failed");
		return -1;
	}
	_D("bt vol: %d", bt_vol);
	return bt_vol;
}

void bt_init_sco(void)
{
	_D("SCO volume initialize");
	if (BT_ERROR_NONE != bt_initialize())
		_E("BT initialize failed");

	if (_bt_register_changed_cb() != BT_ERROR_NONE)
		_E("volume bt register changed cb failed");
}

void bt_deinit_sco(void)
{
	_D("SCO volume Deinitialize");
	if (_bt_unregister_changed_cb() != BT_ERROR_NONE)
		_E("volume bt Unregister changed cb failed");

	if (BT_ERROR_NONE != bt_deinitialize())
		_E("BT Deinitialize failed");
}

static void _bt_display_bt_volume_view(sound_type_e sound_type, int sound, int vibration, bool bt_opened)
{
	if (VOLUME_ERROR_OK != volume_view_window_show())
		_E("Failed to show volume window");

	volume_view_volume_icon_set(sound_type, sound, vibration, bt_opened);
}

static void _bt_volume_changed_cb(int volume, void *user_data)
{
	bool bt_opened = false;
	int error = 0;
	int status = 0;
	int sound = 0;
	int lock = 1;
	int vibration = 0;
	sound_type_e sound_type = 0;
	_D("BT VOLUME is changed");

	status = volume_control_check_status(&lock, &sound_type);
	_D("status: %d, lock: %d, sound type : %d", status, lock, sound_type);

	sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	error = bt_ag_is_sco_opened(&bt_opened);
	if (error != BT_ERROR_NONE)
		_E("bt_ag_is_sco_opened return [%d]", error);
	_D("BT state %d", bt_opened);

	if (bt_opened == true && sound_type == SOUND_TYPE_CALL)
	{
		_bt_display_bt_volume_view(sound_type, sound, vibration, bt_opened);
		if (VOLUME_ERROR_OK != volume_view_slider_value_set(volume))
			_E("Failed to set slider value");

		volume_timer_add(3.0, TYPE_TIMER_BT);
	}
}

static void _bt_state_changed_cb(int result, bool opened, void *user_data)
{
	_D("SCO opened [%s]", opened ? "YES" : "NO");
}

static int _bt_register_changed_cb(void)
{
	int ret = 0;

	ret = bt_audio_initialize();
	if (ret != BT_ERROR_NONE)
	{
		_E("bt audio initialize failed");
		return -1;
	}

	ret = bt_ag_set_speaker_gain_changed_cb(_bt_volume_changed_cb, NULL);
	if (ret != BT_ERROR_NONE)
	{
		_E("register bt volume changed callback failed");
		return -1;
	}

	ret = bt_ag_set_sco_state_changed_cb(_bt_state_changed_cb, NULL);
	if (ret != BT_ERROR_NONE)
		_E("register bt state changed callback failed");

	return 0;
}

static int _bt_unregister_changed_cb(void)
{
	int ret = 0;

	ret = bt_audio_deinitialize();
	if (ret != BT_ERROR_NONE) {
		_E("bt audio initialize failed");
		return -1;
	}

	ret = bt_ag_unset_speaker_gain_changed_cb();
	if (ret != BT_ERROR_NONE) {
		_E("register bt volume changed callback failed");
		return -1;
	}

	ret = bt_ag_unset_sco_state_changed_cb();
	if (ret != BT_ERROR_NONE)
		_E("register bt state changed callback failed");

	return 0;
}

