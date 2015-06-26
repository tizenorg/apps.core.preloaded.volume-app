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

#include "main.h"
#include "_util_log.h"
#include "view.h"
#include "control.h"
#include "sound.h"
#include "timer.h"

#define DEL_TIMER(x) \
	if (x) {\
		_D("DELTIMER x : %p\n", x);\
		ecore_timer_del(x);\
		x = NULL;\
	}
#define ADD_TIMER(x, time, _timer_cb, data) \
	if(x != NULL) DEL_TIMER(x); \
	x = ecore_timer_add(time, _timer_cb, data);\
	_D("ADDTIMER x : %p\n", x);\


struct _timer_s_info {
	Ecore_Timer *popup_timer; /* pop-up timer */
	Ecore_Timer *slider_timer; /* slider timer */
	Ecore_Timer *su_timer; /* short up timer */
	Ecore_Timer *sd_timer; /* short down timer */
	Ecore_Timer *lu_timer; /* long up timer */
	Ecore_Timer *ld_timer; /* long down timer */
	Ecore_Timer *bt_timer; /* long down timer */
};

static struct _timer_s_info timer_info = {
	.popup_timer = NULL,
	.slider_timer = NULL,
	.su_timer = NULL,
	.sd_timer = NULL,
	.lu_timer = NULL,
	.ld_timer = NULL,
	.bt_timer = NULL,
};

static Eina_Bool _timer_short_down_cb(void *data);
static Eina_Bool _timer_short_up_cb(void *data);
static Eina_Bool _timer_slider_cb(void *data);
static Eina_Bool _timer_popup_cb(void *data);
static Eina_Bool _timer_bt_cb(void *data);

Ecore_Timer *volume_timer_bt_timer_get(void)
{
	return timer_info.bt_timer;
}

Ecore_Timer *volume_timer_popup_timer_get(void)
{
	return timer_info.popup_timer;
}

Ecore_Timer *volume_timer_slider_timer_get(void)
{
	return timer_info.slider_timer;
}

Ecore_Timer *volume_timer_su_timer_get(void)
{
	return timer_info.su_timer;
}

Ecore_Timer *volume_timer_sd_timer_get(void)
{
	return timer_info.sd_timer;
}

void volume_timer_add(double time, volume_timer_type type)
{
	_D("VOLUME TIMER ADD");
	if (type == TYPE_TIMER_POPUP) {
		ADD_TIMER(timer_info.popup_timer, time, _timer_popup_cb, NULL);
	} else if (type == TYPE_TIMER_SLIDER) {
		ADD_TIMER(timer_info.slider_timer, time, _timer_slider_cb, NULL);
	} else if (type == TYPE_TIMER_SD) {
		ADD_TIMER(timer_info.sd_timer, time, _timer_short_down_cb, NULL);
	} else if (type == TYPE_TIMER_SU) {
		ADD_TIMER(timer_info.su_timer, time, _timer_short_up_cb, NULL);
	} else if (type == TYPE_TIMER_BT) {
		ADD_TIMER(timer_info.bt_timer, time, _timer_bt_cb, NULL);
	} else {
		_E("Failed to get type : type error(%d)", type);
		return;
	}
}

void volume_timer_del(volume_timer_type type)
{
	if (type == TYPE_TIMER_POPUP) {
		DEL_TIMER(timer_info.popup_timer);
	} else if (type == TYPE_TIMER_SLIDER) {
		DEL_TIMER(timer_info.slider_timer);
	} else if (type == TYPE_TIMER_SD) {
		DEL_TIMER(timer_info.sd_timer);
	} else if (type == TYPE_TIMER_SU) {
		DEL_TIMER(timer_info.su_timer);
	} else if (type == TYPE_TIMER_BT) {
		DEL_TIMER(timer_info.bt_timer);
	} else {
		_E("Failed to get type : type error(%d)", type);
		return;
	}
}

static Eina_Bool _timer_bt_cb(void *data)
{
	if (VOLUME_ERROR_OK != volume_control_close_bt_display())
		_E("Failed to close volume");
	if (VOLUME_ERROR_OK != volume_control_cache_flush())
		_E("Failed to flush cache");

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _timer_popup_cb(void *data)
{
	if (VOLUME_ERROR_OK != volume_control_hide_view())
		_E("Failed to close volume");
	if (VOLUME_ERROR_OK != volume_control_cache_flush())
		_E("Failed to flush cache");

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _timer_slider_cb(void *data)
{
	Evas_Object *slider = volume_view_slider_get();
	if (slider == NULL) {
		timer_info.slider_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	double val = 0;

	int sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

#if 0
	int vibration = volume_sound_vconf_status_get(TYPE_VCONF_VIBRATION_STATUS);
	_D("vibration : %d", vibration);
#endif

	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	val = elm_slider_value_get(slider);
	val += 0.5;
	_D("slider value : %d", (int)val);

	if ((int)val != 0) {
		if (sound_type != SOUND_TYPE_MEDIA) {
			volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 1);
			volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 0);
		}
		volume_sound_is_vibration_set(EINA_FALSE);
	}
	if (val < 1) {
		if (sound_type != SOUND_TYPE_MEDIA) {
			volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 0);

			if (sound) {
				volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 1);
				volume_sound_vib_play();
			}
		}
		elm_slider_value_set(slider, 0);
		volume_sound_level_set(sound_type, 1);
	}

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _timer_short_up_cb(void *data)
{
	_D("volume is in LongPress");
	Evas_Object *win = volume_view_win_get();
	if (!win) {
		_E("Window is NULL");
		return ECORE_CALLBACK_CANCEL;
	}

	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	if (!evas_object_visible_get(win) && sound_type == SOUND_TYPE_RINGTONE) {
		_E("Window is hidden");
		return ECORE_CALLBACK_CANCEL;
	}

	if (volume_view_is_slider_touching_get())
		return ECORE_CALLBACK_RENEW;

	ecore_timer_interval_set(timer_info.su_timer, 0.1);
	volume_timer_del(TYPE_TIMER_SLIDER);

	int sound_step = volume_sound_step_get();
	_D("sound step : %d", sound_step);

	int sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	int val = volume_sound_level_get(sound_type);
	if (val == -1) {
		_E("Failed to get volume level");
		return ECORE_CALLBACK_CANCEL;
	}
	_D("sound value : %d", val);

	if (val == sound_step) {
		_D("already sound value : %d", sound_step);
		return ECORE_CALLBACK_RENEW;
	}

	if (!sound && sound_type == SOUND_TYPE_NOTIFICATION) {
		_D("Do not adjust the noti type in no sound.");
		return ECORE_CALLBACK_RENEW;
	}

	if (!sound && sound_type == SOUND_TYPE_RINGTONE) {
		volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 1);
		volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 0);
	}

	if (volume_sound_level_set(sound_type, val+1 > sound_step ? sound_step : val+1)) {
		_D("[SAFETY_SOUND] release timer");
		volume_timer_add(3.0, TYPE_TIMER_POPUP);
	}
	val = volume_sound_level_get(sound_type);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _timer_short_down_cb(void *data)
{
	Evas_Object *win = volume_view_win_get();
	if (!win) {
		_E("Window is NULL");
		return ECORE_CALLBACK_CANCEL;
	}

	sound_type_e sound_type = volume_control_get_sound_type_at_show();
	_D("sound type at show : %d", sound_type);

	if (!evas_object_visible_get(win) && sound_type == SOUND_TYPE_RINGTONE) {
		_E("Window is hidden");;
		return ECORE_CALLBACK_CANCEL;
	}

	if (volume_view_is_slider_touching_get())
		return ECORE_CALLBACK_RENEW;

	ecore_timer_interval_set(timer_info.sd_timer, 0.1);
	volume_timer_del(TYPE_TIMER_SLIDER);

	int sound = volume_sound_vconf_status_get(TYPE_VCONF_SOUND_STATUS);
	_D("sound status : %d", sound);

	int val = volume_sound_level_get(sound_type);
	if (val == -1) {
		_E("Failed to get volume level");
		return ECORE_CALLBACK_CANCEL;
	}

	if (!sound && sound_type == SOUND_TYPE_NOTIFICATION) {
		_D("Do not adjust the noti type in no sound.");
		return ECORE_CALLBACK_RENEW;
	}
	_D("sound value : %d", val);

	if (val == 1) {
		if (sound && sound_type == SOUND_TYPE_RINGTONE) {
			_D("Set sound status to vibration in long press");
			volume_sound_vconf_status_set(TYPE_VCONF_SOUND_STATUS, 0);
			volume_sound_vconf_status_set(TYPE_VCONF_VIBRATION_STATUS, 1);
			volume_sound_vib_play();
		}
	} else if (!val) {
		_D("already sound value : 0");
		return ECORE_CALLBACK_RENEW;
	}

	if (sound || sound_type != SOUND_TYPE_RINGTONE)
		volume_sound_level_set(sound_type, val-1 <= 0 ? 0 : val-1);

	return ECORE_CALLBACK_RENEW;
}

