 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the License);
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an AS IS BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */

#include <appcore-common.h>
#include <mm_sound.h>
#include <vconf.h>
#include <svi.h>

#include "volume.h"
#include "_util_log.h"
#include "_logic.h"


void _play_vib(int handle)
{
	svi_play_vib(handle, SVI_VIB_OPERATION_VIBRATION);
}

void _play_sound(int type, int handle)
{
	if (type == VOLUME_TYPE_MEDIA) {

	} else {
		svi_play_sound(handle, SVI_SND_TOUCH_TOUCH1);
	}
}

int _init_svi(void *data)
{
	int ret, handle;

	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invaild argument: appdata is NULL\n");

	ret = svi_init(&handle);
	if (ret != SVI_SUCCESS) {
		_E("Failed to init svi\n");
		svi_fini(handle);
		return -1;
	}
	ad->sh = handle;
	return 0;
}

int _set_slider_value(void *data, int val)
{
	double angle;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	if (ad->flag_touching == EINA_TRUE) {
		return 0;
	}
	if (ad->sl) {
		elm_slider_min_max_set(ad->sl, 0, ad->step);
		elm_slider_value_set(ad->sl, val);
	}

	return 0;
}

int _set_icon(void *data, int val)
{
	int snd, vib;
	double angle;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd);
	vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &vib);

	if (val == 0) {
		if (vib) {
			if (ad->type == VOLUME_TYPE_MEDIA) {
				edje_object_signal_emit(elm_layout_edje_get(ad->ly), "mute", "icon");
			} else {
				edje_object_signal_emit(elm_layout_edje_get(ad->ly), "vib", "icon");
				_play_vib(ad->sh);
			}

		} else {
			edje_object_signal_emit(elm_layout_edje_get(ad->ly), "mute", "icon");

		}

	} else {
		edje_object_signal_emit(elm_layout_edje_get(ad->ly), "default", "icon");

	}

}

void _set_popup_title(void *data, int type)
{
	char buf[64] = {0, };
	char name[64] = {0, };
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	_get_title(type, buf, sizeof(buf));
	elm_object_part_text_set(ad->pu, "title,text", buf);

}

int _get_title(volume_type_t type, char *label, int size)
{
	char *text = NULL;

	text = D_("IDS_COM_BODY_UNKNOWN");

	switch (type) {
	case VOLUME_TYPE_SYSTEM:
		text = D_("IDS_COM_BODY_SYSTEM");
		break;
	case VOLUME_TYPE_NOTIFICATION:
		text = D_("IDS_COM_HEADER_NOTIFICATION");
		break;
	case VOLUME_TYPE_ALARM:
		text = D_("IDS_COM_BODY_ALARM");
		break;
	case VOLUME_TYPE_RINGTONE:
		text = D_("IDS_COM_BODY_RINGTONE");
		break;
	case VOLUME_TYPE_MEDIA:
		text = T_("IDS_COM_BODY_MEDIA");
		break;
	case VOLUME_TYPE_CALL:
		text = D_("IDS_COM_BODY_CALL");
		break;
	case VOLUME_TYPE_EXT_ANDROID:
		/* this enum is different from mm_sound.h and avsys-audio.h */
		text = T_("IDS_COM_BODY_ANDROID");
		break;
	case VOLUME_TYPE_EXT_JAVA:
		text = T_("IDS_COM_BODY_JAVA");
		break;
	default:
		text = D_("IDS_COM_BODY_SYSTEM");
		break;
	}
	snprintf(label, size, "%s", text);
	_D("get title(%s)\n", label);

	return 0;
}



double _get_angle_with_value(int val, int step)
{
_D("func\n");
	double angle;

	angle = 360.0 * (val + 1) / step;
	return angle;
}

void _mm_func(void *data)
{
	int type, val;
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	/* function could be activated when window exists */
	if (ad->win) {
		ad->step = _get_step(ad->type);
		mm_sound_volume_get_value(ad->type, &val);

		_set_slider_value(ad, val);
		_set_popup_title(ad, ad->type);
		_set_icon(ad, val);
		_D("type(%d) val(%d)\n", ad->type, val);

	}
}

void _mm_system_cb(void *data)
{
	_mm_func(data);
}

void _mm_notification_cb(void *data)
{
	_mm_func(data);
}

void _mm_alarm_cb(void *data)
{
	_mm_func(data);
}

void _mm_ringtone_cb(void *data)
{
	_mm_func(data);
}

void _mm_media_cb(void *data)
{
	_mm_func(data);
}

void _mm_call_cb(void *data)
{
	_mm_func(data);
}

void _mm_ext_android_cb(void *data)
{
	_mm_func(data);
}

void _mm_ext_java_cb(void *data)
{
	_mm_func(data);
}

int _get_volume_type_max(void)
{
	return VOLUME_TYPE_MAX;
}

int _init_mm_sound(void *data)
{
	int i;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	mm_sound_volume_add_callback(VOLUME_TYPE_SYSTEM,
			_mm_system_cb, (void *)ad);
	mm_sound_volume_add_callback(VOLUME_TYPE_NOTIFICATION,
			_mm_notification_cb, (void *)ad);
	mm_sound_volume_add_callback(VOLUME_TYPE_ALARM,
			_mm_alarm_cb, (void *)ad);
	mm_sound_volume_add_callback(VOLUME_TYPE_RINGTONE,
			_mm_ringtone_cb, (void *)ad);
	mm_sound_volume_add_callback(VOLUME_TYPE_MEDIA,
			_mm_media_cb, (void *)ad);
	mm_sound_volume_add_callback(VOLUME_TYPE_CALL,
			_mm_call_cb, (void *)ad);
	mm_sound_volume_add_callback(VOLUME_TYPE_EXT_ANDROID,
			_mm_ext_android_cb, (void *)ad);
	mm_sound_volume_add_callback(VOLUME_TYPE_EXT_JAVA,
			_mm_ext_java_cb, (void *)ad);

	return 0;
}

int _get_sound_level(volume_type_t type, int *val)
{
	mm_sound_volume_get_value(type, val);
	return 0;
}

int _set_sound_level(volume_type_t type, int val)
{
	mm_sound_volume_set_value(type, val);
	return 0;
}

int _get_step(int type)
{
	int ret, step;

	ret = mm_sound_volume_get_step(type, &step);
	retvm_if(ret < 0, -1, "Failed to get step\n");
	step -= 1;
	return step;
}


