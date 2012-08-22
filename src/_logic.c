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


#include <math.h>

#include <Ecore.h>
#include <Ecore_X.h>
#include <utilX.h>
#include <vconf.h>
#include <ui-gadget.h>

#include "volume.h"
#include "_util_log.h"
#include "_util_efl.h"
#include "_sound.h"
#include "_button.h"

#define STRBUF_SIZE 128

enum {
	IDLELOCK_OFF = 0x0,
	IDLELOCK_ON,
	IDLELOCK_MAX,
};

void _ungrab_key(struct appdata *ad);

static void _sound_cb(keynode_t *node, void *data)
{
}

static void _vibration_cb(keynode_t *node, void *data)
{
}

int _init_vconf(struct appdata *ad)
{
	int ret;
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, _sound_cb, ad);
	retvm_if(ret < 0, -1, "Failed to notify sound status\n");
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, _vibration_cb, ad);
	retvm_if(ret < 0, -1, "Failed to notifi vibration status\n");
	return 0;
}

int _close_volume(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->flag_deleting == EINA_TRUE, -1, "Closing volume\n");

	_D("start closing volume\n");
	ad->flag_deleting = EINA_TRUE;

	_ungrab_key(ad);

	DEL_TIMER(ad->sutimer)
	DEL_TIMER(ad->lutimer)
	DEL_TIMER(ad->sdtimer)
	DEL_TIMER(ad->ldtimer)
	DEL_TIMER(ad->ptimer)

	if (ad->pu)
		evas_object_hide(ad->pu);
	if (ad->win)
		evas_object_hide(ad->win);
	appcore_flush_memory();

	ad->flag_deleting = EINA_FALSE;
	_D("end closing volume\n");
	return 0;
}

Eina_Bool popup_timer_cb(void *data)
{
	_D("%s\n", __func__);
	_close_volume(data);
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool _lu_timer_cb(void *data)
{
	int val;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	if (ad->win == NULL || evas_object_visible_get(ad->win) == EINA_FALSE){
		_D("win is NULL or hide state, so long press pass\n");
		return ECORE_CALLBACK_CANCEL;
	}
	DEL_TIMER(ad->stimer)

	_get_sound_level(ad->type, &val);
	_set_sound_level(ad->type, val +1 > ad->step ? ad->step : val + 1);
	_D("down, type(%d), step(%d) val[%d]\n", ad->type, ad->step, val+1);
	return ECORE_CALLBACK_RENEW;
}

Eina_Bool _su_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	ADD_TIMER(ad->lutimer, 0.0, _lu_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool _ld_timer_cb(void *data)
{
	int val;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	if (ad->win == NULL || evas_object_visible_get(ad->win) == EINA_FALSE){
		_D("win is NULL or hide state, so long press pass\n");
		return ECORE_CALLBACK_CANCEL;
	}
	DEL_TIMER(ad->stimer)

	_get_sound_level(ad->type, &val);
	_set_sound_level(ad->type, val -1 <= 0 ? 0 : val - 1);
	_D("down, type(%d), step(%d) val[%d]\n", ad->type, ad->step, val+1);
	return ECORE_CALLBACK_RENEW;
}

Eina_Bool _sd_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	_D("add long down timer\n");
	ADD_TIMER(ad->ldtimer, 0.0, _ld_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	_D("%s\n", __func__);
	int val=0, snd=0;
	Ecore_Event_Key *ev = event;
	struct appdata *ad = (struct appdata *)data;

	retvm_if(ev == NULL, ECORE_CALLBACK_CANCEL, "Invalid arguemnt: event is NULL\n");
	retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->win == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: window is NULL\n");

	if (ad->flag_touching == EINA_TRUE) {
		return ECORE_CALLBACK_CANCEL;
	}

	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd);

	ad->flag_pressing = EINA_TRUE;

	DEL_TIMER(ad->ptimer)

	if (!strcmp(ev->keyname, KEY_VOLUMEUP)) {
		_get_sound_level(ad->type, &val);
		if (val == ad->step) {
			_set_sound_level(ad->type, ad->step);
			_play_sound(ad->type, ad->sh);
			return ECORE_CALLBACK_CANCEL;
		}
		if(!snd){
			_D("mute and volume up key pressed\n");
			vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, EINA_TRUE);
		}
		_set_sound_level(ad->type, val + 1);
		_play_sound(ad->type, ad->sh);
		DEL_TIMER(ad->sutimer)
		ADD_TIMER(ad->sutimer, 0.5, _su_timer_cb, ad)

		_D("set volume %d -> [%d]\n", val, val+1);

	} else if (!strcmp(ev->keyname, KEY_VOLUMEDOWN)) {
		if(!snd){
			/* Do nothing */
			return ECORE_CALLBACK_CANCEL;
		}
		_get_sound_level(ad->type, &val);
		if (val == 0) {
			_play_vib(ad->sh);
			return ECORE_CALLBACK_CANCEL;
		}
		_set_sound_level(ad->type, val - 1);
		_play_sound(ad->type, ad->sh);
		DEL_TIMER(ad->sdtimer)
		ADD_TIMER(ad->sdtimer, 0.5, _sd_timer_cb, ad)

		_D("type (%d) set volume %d -> [%d]\n", ad->type, val, val-1);

	}
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _key_release_cb(void *data, int type, void *event)
{
	Ecore_Event_Key *ev = event;
	struct appdata *ad = (struct appdata *)data;

	retvm_if(ev == NULL, ECORE_CALLBACK_CANCEL, "Invalid arguemnt: event is NULL\n");
	retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument:appdata is NULL\n");
	retvm_if(ad->win == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: window is NULL\n");

	if (ad->flag_touching == EINA_TRUE) {
		return ECORE_CALLBACK_CANCEL;
	}

	if (!strcmp(ev->keyname, KEY_VOLUMEUP)) {
		_D("up key released and del timer\n");
		DEL_TIMER(ad->sutimer)
		DEL_TIMER(ad->lutimer)

	} else if (!strcmp(ev->keyname, KEY_VOLUMEDOWN)) {
		_D("down key released and del timer\n");
		DEL_TIMER(ad->sdtimer)
		DEL_TIMER(ad->ldtimer)
	}

	ad->flag_pressing = EINA_FALSE;

	DEL_TIMER(ad->ptimer)

	/* In UG, This Callback should not be called. */
	if ( ad->ug == NULL )
		ADD_TIMER(ad->ptimer, 3.0, popup_timer_cb, ad)

	return ECORE_CALLBACK_CANCEL;
}

int _grab_key(struct appdata *ad)
{
	Ecore_X_Window xwin;
	Ecore_X_Display *disp;
	int ret;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	disp = ecore_x_display_get();
	retvm_if(disp == NULL, -1, "Failed to get display\n");

	retvm_if(ad->win == NULL, -1, "Invalid argument: ad->win is NULL\n");
	xwin = elm_win_xwindow_get(ad->win);
	retvm_if(xwin == 0, -1, "Failed to get xwindow\n");

	ret = utilx_grab_key(disp, xwin, KEY_VOLUMEDOWN, TOP_POSITION_GRAB);
	retvm_if(ret < 0, -1, "Failed to grab key down\n");

	ret = utilx_grab_key(disp, xwin, KEY_VOLUMEUP, TOP_POSITION_GRAB);
	retvm_if(ret < 0, -1, "Failed to grab key up\n");

	ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_press_cb, ad);
	ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_release_cb, ad);
	_D("key grabed\n");
	return 0;
}

void _ungrab_key(struct appdata *ad)
{
	Ecore_X_Window xwin;
	Ecore_X_Display *disp;

	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	retm_if(ad->win == NULL, "Invalid argument: ad->win is NULL\n");

	xwin = elm_win_xwindow_get(ad->win);
	retm_if(xwin == 0, "Failed to get xwindow\n");

	disp = ecore_x_display_get();
	retm_if(disp == NULL, "Failed to get display\n");

	if (disp && xwin) {
		utilx_ungrab_key(disp, xwin, KEY_VOLUMEUP);
		utilx_ungrab_key(disp, xwin, KEY_VOLUMEDOWN);
	}
}

int _get_vconf_idlelock(void)
{
	int ret, lock;

	lock = IDLELOCK_OFF;
	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock);
	retvm_if(ret < 0, -1, "Failed to get vconf %s\n",
		 VCONFKEY_IDLE_LOCK_STATE);
	_D("idlelock vconf:%d\n", lock);

	return lock == VCONFKEY_IDLE_LOCK ? IDLELOCK_ON : IDLELOCK_OFF;
}

volume_type_t _get_volume_type(void)
{
	int ret;
	volume_type_t type = -1;

	ret = mm_sound_volume_get_current_playing_type(&type);
	switch (ret) {
		case MM_ERROR_NONE:
			break;
		case MM_ERROR_SOUND_VOLUME_NO_INSTANCE:
		case MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY:
			type = VOLUME_TYPE_SYSTEM;
			break;
		default:
			fprintf(stderr, "Failed to get sound type(errno:%x)\n", ret);
			return -1;
	}
	if(type == VOLUME_TYPE_SYSTEM)
		type = VOLUME_TYPE_RINGTONE;
	return type;
}

int _check_status(int *lock, int *type)
{
	*lock = _get_vconf_idlelock();
	*type = _get_volume_type();
	_D("lock(%d) type(%d)\n", *lock, *type);

	if (*type == MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY
			|| *type == MM_ERROR_SOUND_INTERNAL) {
		_D("Do not show by type\n");
		return -1;
	}

	if (*lock == IDLELOCK_ON && *type != VOLUME_TYPE_MEDIA) {
		_D("lock is set, not in media\n");
		return -1;
	}
	return 0;
}

int _app_create(struct appdata *ad)
{
	_init_vconf(ad);
	_init_svi(ad);

	return 0;
}

int myterm(bundle *b, void *data)
{
	return 0;
}

int mytimeout(bundle *b, void *data)
{
	return 0;
}

syspopup_handler handler = {
	.def_term_fn = myterm,
	.def_timeout_fn = mytimeout
};

void _set_level(int type)
{
	int val;
	_get_sound_level(type, &val);
	_set_sound_level(type, val);
	_D("type(%d) val(%d)\n", type, val);
}

static void _block_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	_close_volume(data);
}

Eina_Bool _slider_timer_cb(void *data)
{
	double val;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	if (ad->sl) {
		val = elm_slider_value_get(ad->sl);
		val += 0.5;
		if ((int)val != 0) {	/* 0 value could be dealed with in changed callback */
			_set_sound_level(ad->type, (int)val);
		}
		return ECORE_CALLBACK_RENEW;
	}
	return ECORE_CALLBACK_CANCEL;
}

static void _slider_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("%s\n", __func__);
	int snd=0;
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	if (ad->flag_pressing == EINA_TRUE) {
		return;
	}
	ad->flag_touching = EINA_FALSE;

	DEL_TIMER(ad->ptimer)

	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd);
	if(!snd){
		vconf_set_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, EINA_TRUE);
		_set_icon(data, -1);
	}

	if (ad->lutimer || ad->ldtimer) {
		_D("return when long press is working\n");
		return;
	}

	ADD_TIMER(ad->stimer, 0.2, _slider_timer_cb, data)
}

static void _slider_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	double val;
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	if (ad->flag_pressing == EINA_TRUE) {
		return;
	}
	if (ad->lutimer || ad->ldtimer) {
		_D("return when long press is working\n");
		return;
	}
	val = elm_slider_value_get(ad->sl);
	if (val <= 0.5) {
		elm_slider_value_set(ad->sl, 0);
		_set_sound_level(ad->type, 0);

	}
}
static void _slider_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("func\n");
	double val;
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	if (ad->flag_pressing == EINA_TRUE) {
		return;
	}
	if (ad->lutimer || ad->ldtimer) {
		_D("return when long press is working\n");
		return;
	}
	DEL_TIMER(ad->stimer)
	if (ad->sl) {
		val = elm_slider_value_get(ad->sl);
		val += 0.5;
		_set_sound_level(ad->type, (int)val);
	}
	_play_sound(ad->type, ad->sh);
	ad->flag_touching = EINA_FALSE;
	ADD_TIMER(ad->ptimer, 3.0, popup_timer_cb, ad)
}

Eina_Bool _lu_warmup_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: appdata is NULL\n");
	ADD_TIMER(ad->lutimer, 0.0, _lu_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool _ld_warmup_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: appdata is NULL\n");
	_D("add long down timer\n");
	ADD_TIMER(ad->ldtimer, 0.0, _ld_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}

int _handle_bundle(bundle *b, struct appdata *ad)
{
	const char *bval;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	bval = bundle_get_val(b, "LONG_PRESS");
	if (bval) {
		_D("val(%s)\n", bval);
		Ecore_X_Display* disp = ecore_x_display_get();
		Utilx_Key_Status upstat, downstat;

		if (!strncmp(bval, "VOLUME_UP", strlen("LONG_PRESS"))) {
			upstat = utilx_get_key_status(disp, KEY_VOLUMEUP);
			if (upstat == UTILX_KEY_STATUS_PRESSED ) {
				_D("volume up long press\n");
				DEL_TIMER(ad->ptimer)
					ecore_timer_add(0.5, _lu_warmup_timer_cb, ad);
			}
		} else if (!strncmp(bval, "VOLUME_DOWN", strlen("LONG_PRESS"))) {
			downstat = utilx_get_key_status(disp, KEY_VOLUMEDOWN);
			if (downstat == UTILX_KEY_STATUS_PRESSED ) {
				_D("volume down long press\n");
				DEL_TIMER(ad->ptimer)
				ecore_timer_add(0.5, _ld_warmup_timer_cb, ad);
			}
		} else {
			ADD_TIMER(ad->ptimer, 3.0, popup_timer_cb, ad)
		}
	} else {
		ADD_TIMER(ad->ptimer, 3.0, popup_timer_cb, ad)
	}
	return 0;
}

static void _button_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("%s\n", __func__);
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	DEL_TIMER(ad->ptimer)
	_open_ug(ad);
}

Eina_Bool _unset_layout(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, EINA_FALSE,"Invalid argument: appdata is NULL\n");

	DEL_TIMER(ad->warntimer);
	if(elm_object_content_get(ad->pu)==ad->warn_ly){
		elm_object_content_unset(ad->pu);
		elm_object_content_set(ad->pu, ad->sl);
		evas_object_hide(ad->warn_ly);
	}
	return ECORE_CALLBACK_CANCEL;
}

int _lang_changed(void *data){
	_D("%s\n", __func__);
	struct appdata *ad = (struct appdata *)data;
	char buf[STRBUF_SIZE] = {0, };

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->win == NULL, -1, "Invalid argument: window is NULL\n");

	elm_object_text_set(ad->bt, S_("IDS_COM_BODY_SETTINGS"));
	snprintf(buf, sizeof(buf), "<font_size=32><b>%s</b></font_size>", 
			T_("IDS_COM_BODY_HIGH_VOLUMES_MAY_HARM_YOUR_HEARING_IF_YOU_LISTEN_FOR_A_LONG_TIME"));
	elm_object_text_set(ad->warn_lb, buf);
	return 0;
}

int _app_reset(bundle *b, void *data)
{
	int ret, status;
	int lock, type, val;
	Evas_Object *win, *pu, *ic, *sl, *bt;
	Evas_Object *label, *warn_ly;
	char buf[STRBUF_SIZE] = {0, };
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	ad->flag_touching = EINA_FALSE;

	_init_mm_sound(ad);
	status = _check_status(&lock, &type);
	mm_sound_volume_get_value(type, (unsigned int*)(&val));
	ad->type = type;

	if (status == 0) {
		if(ad->win){
			_grab_key(ad);
			_handle_bundle(b, ad);
			_unset_layout(data);
			evas_object_show(ad->pu);
			evas_object_show(ad->win);
			_mm_func(data);
			return 0;
		}
		ad->step = _get_step(type);

		_set_level(type);

		win = _add_window(PACKAGE);
		retvm_if(win == NULL, -1, "Failed add window\n");
		ad->win = win;

		_grab_key(ad);

		pu = _add_popup(win, "volumebarstyle");
		retvm_if(pu == NULL, -1, "Failed to add popup\n");
		evas_object_smart_callback_add(pu, "block,clicked", _block_clicked_cb, ad);
		_get_title(type, buf, sizeof(buf));
		elm_object_part_text_set(pu, "title,text", buf);
		ad->pu = pu;

		/* Make a Slider bar */
		sl = _add_slider(ad->pu, 0, ad->step, val);
		evas_object_smart_callback_add(sl, "slider,drag,start", _slider_start_cb, ad);
		evas_object_smart_callback_add(sl, "changed", _slider_changed_cb, ad);
		evas_object_smart_callback_add(sl, "slider,drag,stop", _slider_stop_cb, ad);
		ad->sl = sl;

		ic = elm_icon_add(ad->pu);
		evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		elm_icon_resizable_set(ic, EINA_FALSE, EINA_FALSE);
		elm_object_part_content_set(ad->sl, "icon", ic);
		ad->ic = ic;
		_set_icon(ad, val);

		elm_object_content_set(ad->pu, ad->sl);

		/* Make a Layout for volume slider with warning text. */
		snprintf(buf, sizeof(buf), "<font_size=32><b>%s</b></font_size>", 
			T_("IDS_COM_BODY_HIGH_VOLUMES_MAY_HARM_YOUR_HEARING_IF_YOU_LISTEN_FOR_A_LONG_TIME"));
		label = _add_label(ad->pu, "popup/default", buf);
		ad->warn_lb = label;
		evas_object_hide(ad->warn_lb);

		warn_ly = _add_layout(ad->pu, EDJ_THEME, GRP_VOLUME_SLIDER_WITH_WARNING);
		elm_object_part_content_set(warn_ly, "elm.swallow.warn_label", ad->warn_lb);
		ad->warn_ly = warn_ly;
		evas_object_hide(ad->warn_ly);

		/* Make a setting button */
		bt = _add_button(ad->pu, "popup_button/default", S_("IDS_COM_BODY_SETTINGS"));
		evas_object_smart_callback_add(bt, "clicked", _button_cb, ad);
		elm_object_part_content_set(ad->pu, "button1", bt);
		ad->bt = bt;

		ret = syspopup_create(b, &handler, ad->win, ad);
		retvm_if(ret < 0, -1, "Failed to create syspopup\n");

		_handle_bundle(b, ad);

		evas_object_show(ad->win);
	}

	return 0;
}

int _app_pause(struct appdata *ad)
{
	_D("%s\n", __func__);
	if(ad->ug){
		ug_destroy_all();
		ad->ug = NULL;
		ecore_x_netwm_window_type_set(elm_win_xwindow_get(ad->win), ECORE_X_WINDOW_TYPE_NOTIFICATION);
		utilx_set_window_opaque_state(ecore_x_display_get(), elm_win_xwindow_get(ad->win), UTILX_OPAQUE_STATE_OFF);
	}
	_close_volume(ad);
	return 0;
}
