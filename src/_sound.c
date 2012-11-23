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

#define STRBUF_SIZE 64
#define PATHBUF_SIZE 256

int _set_sound_level(volume_type_t type, int val);

void _play_vib(int handle)
{
	svi_play_vib(handle, SVI_VIB_OPERATION_VIBRATION);
}

void _play_sound(int type, int handle)
{
	if (type == VOLUME_TYPE_MEDIA || type == VOLUME_TYPE_RINGTONE) {

	} else {
		svi_play_sound(handle, SVI_SND_TOUCH_TOUCH1);
	}
}

int _init_svi(void *data)
{
	int ret = -1, handle = -1;

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
	int snd=0, vib=0;
	char *img = NULL;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	system_audio_route_device_t device = 0;
	mm_sound_route_get_playing_device(&device);

	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd);
	vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &vib);

	switch(ad->type){
		case VOLUME_TYPE_RINGTONE:
			img = IMG_VOLUME_ICON_CALL;
			if(val == 0){
				if(vib)
					img = IMG_VOLUME_ICON_VIB;
				else
					img = IMG_VOLUME_ICON_MUTE;
			}
			if(!snd){
				img = IMG_VOLUME_ICON_MUTE;
				if(vib)img = IMG_VOLUME_ICON_VIB;
			}
			break;
		case VOLUME_TYPE_MEDIA:
			if(device == SYSTEM_AUDIO_ROUTE_PLAYBACK_DEVICE_EARPHONE)
				img = IMG_VOLUME_ICON_HEADPHONE;
			else
				img = IMG_VOLUME_ICON_MEDIA;
			break;
		default:
			img = IMG_VOLUME_ICON;
			if(val == 0){
				if(vib)
					img = IMG_VOLUME_ICON_VIB;
				else
					img = IMG_VOLUME_ICON_MUTE;
			}
			if(!snd){
				img = IMG_VOLUME_ICON_MUTE;
				if(vib)img = IMG_VOLUME_ICON_VIB;
			}
			break;
	}
	if (ad->ic ) {
		elm_icon_file_set(ad->ic, EDJ_APP, img);
	}
	return 1;
}

Eina_Bool _delete_message_ticker_notification(void *data)
{
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	if(ad->noti_id != 0)
	{
		noti_err = notification_delete_by_priv_id(PKGNAME, NOTIFICATION_TYPE_NOTI, ad->noti_id);
		retvm_if(noti_err != NOTIFICATION_ERROR_NONE, ECORE_CALLBACK_RENEW, "Fail to notification_delete : %d\n", noti_err);
	}
	ad->noti_id = 0;
	return ECORE_CALLBACK_CANCEL;
}

int _insert_message_ticker_notification(void)
{
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	int priv_id = 0;

	noti = notification_new(NOTIFICATION_TYPE_NOTI, NOTIFICATION_GROUP_ID_NONE, NOTIFICATION_PRIV_ID_NONE);
	retvm_if(noti == NULL, 0, "notification_new is failed\n");

	noti_err = notification_set_display_applist(noti, NOTIFICATION_DISPLAY_APP_TICKER);
	retvm_if(noti == NULL, 0, "notification_set_display_applist\n");

	noti_err = notification_set_application(noti, PKGNAME);
	retvm_if(noti_err != NOTIFICATION_ERROR_NONE, 0, "Fail to notification_set_application : %d\n", noti_err);

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, IMG_VOLUME_PACKAGE_ICON);
	retvm_if(noti_err != NOTIFICATION_ERROR_NONE, 0, "Fail to notification_set_image : %d\n", noti_err);

	noti_err = notification_set_text_domain(noti, PACKAGE, LOCALEDIR);
	retvm_if(noti_err != NOTIFICATION_ERROR_NONE, 0, "Fail to notification_set_text_domain : %d\n", noti_err);

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, STR_MEDIA_MSG, IDS_MEDIA_MSG, NOTIFICATION_VARIABLE_TYPE_NONE);
	retvm_if(noti_err != NOTIFICATION_ERROR_NONE, 0, "Fail to notification_set_text : %d\n", noti_err);

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, STR_WARNING_MSG, IDS_WARNING_MSG, NOTIFICATION_VARIABLE_TYPE_NONE);
	retvm_if(noti_err != NOTIFICATION_ERROR_NONE, 0, "Fail to notification_set_text : %d\n", noti_err);

	noti_err = notification_insert(noti, &priv_id);
	retvm_if(noti_err != NOTIFICATION_ERROR_NONE, 0, "Fail to notification_set_text_domain : %d\n", noti_err);

	return priv_id;
}

void _set_device_warning(void *data, int val, int device)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	switch (device) {
		case SYSTEM_AUDIO_ROUTE_PLAYBACK_DEVICE_EARPHONE:
			if (val >= 10 && ad->type == VOLUME_TYPE_MEDIA) {
				if(ad->noti_id == 0 && ad->noti_seen == EINA_FALSE){
					ad->noti_id = _insert_message_ticker_notification();
					DEL_TIMER(ad->warntimer);
					ADD_TIMER(ad->warntimer, 2.0, _delete_message_ticker_notification, data);
					ad->noti_seen = EINA_TRUE;
				}
			}
			else {
				_delete_message_ticker_notification(data);
				ad->noti_seen = EINA_FALSE;
			}
			break;
		default:
			_delete_message_ticker_notification(data);
			ad->noti_seen = EINA_FALSE;
			break;
	}
}

int _get_title(volume_type_t type, char *label, int size)
{
	char *text = NULL;

	text = S_("IDS_COM_BODY_UNKNOWN");

	switch (type) {
	case VOLUME_TYPE_SYSTEM:
		text = S_("IDS_COM_BODY_SYSTEM");
		break;
	case VOLUME_TYPE_NOTIFICATION:
		text = S_("IDS_COM_HEADER_NOTIFICATION");
		break;
	case VOLUME_TYPE_ALARM:
		text = S_("IDS_COM_BODY_ALARM");
		break;
	case VOLUME_TYPE_RINGTONE:
		text = S_("IDS_COM_BODY_RINGTONE");
		break;
	case VOLUME_TYPE_MEDIA:
		text = T_("IDS_COM_BODY_MEDIA");
		break;
	case VOLUME_TYPE_CALL:
		text = S_("IDS_COM_BODY_CALL");
		break;
	case VOLUME_TYPE_EXT_ANDROID:
		/* this enum is different from mm_sound.h and avsys-audio.h */
		text = T_("IDS_COM_BODY_ANDROID");
		break;
	case VOLUME_TYPE_EXT_JAVA:
		text = T_("IDS_COM_BODY_JAVA");
		break;
	default:
		text = S_("IDS_COM_BODY_SYSTEM");
		break;
	}
	snprintf(label, size, "%s", text);
	_D("get title(%s)\n", label);

	return 0;
}

int _get_step(int type)
{
	int ret = -1, step = 0;
	ret = mm_sound_volume_get_step(type, &step);
	retvm_if(ret < 0, -1, "Failed to get step\n");
	step -= 1;
	return step;
}

void _mm_func(void *data)
{
	_D("%s\n", __func__);
	int val = 0, snd = 0;
	system_audio_route_device_t device = 0;
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	retm_if(ad->win == NULL, "Failed to get window\n");

	/* function could be activated when window exists */
	ad->type = _get_volume_type();
	ad->step = _get_step(ad->type);
	mm_sound_route_get_playing_device(&device);
	mm_sound_volume_get_value(ad->type, (unsigned int*)(&val));

	/* apply earphone safety concept */
	if(ad->device != device){
		if(device == SYSTEM_AUDIO_ROUTE_PLAYBACK_DEVICE_EARPHONE && val > 9)
			val = 9;
			_set_sound_level(ad->type, val);
		ad->device = device;
	}
	if(ad->type == VOLUME_TYPE_RINGTONE){
		if(val == 0)
			vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, EINA_FALSE);
		else{
			vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd);
			if(snd == EINA_FALSE){
				vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, EINA_TRUE);
			}
		}
	}

	_set_slider_value(ad, val);
	_set_icon(ad, val);
	_set_device_warning(ad, val, device);
	_D("type(%d) val(%d)\n", ad->type, val);
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
	mm_sound_volume_get_value(type, (unsigned int*)val);
	return 0;
}

int _set_sound_level(volume_type_t type, int val)
{
	mm_sound_volume_set_value(type, val);
	return 0;
}
