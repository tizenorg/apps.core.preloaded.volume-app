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
#include <bundle.h>
#include <syspopup.h>

#include "volume.h"
#include "_util_log.h"
#include "_util_efl.h"
#include "_sound.h"

#define _D_add_timer(x, time, _timer_cb, data) \
	x = ecore_timer_add(time, _timer_cb, data);\

#define _D_del_timer(x) \
	if (x) {\
		ecore_timer_del(x);\
		x = NULL;\
	}

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

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL,
				     _sound_cb, ad);
	retvm_if(ret < 0, -1, "Failed to notify sound status\n");

	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL,
				     _vibration_cb, ad);
	retvm_if(ret < 0, -1, "Failed to notifi vibration status\n");


	return 0;
}

volume_type_t _get_type(void)
{
	int r;
	volume_type_t type = -1;

	r = mm_sound_volume_get_current_playing_type(&type);
	switch (r) {
	case MM_ERROR_NONE:
		break;

	case MM_ERROR_SOUND_VOLUME_NO_INSTANCE:
	case MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY:
		type = VOLUME_TYPE_SYSTEM;
		break;

	default:
		fprintf(stderr, "Failed to get sound type(errno:%x)\n", r);
		return -1;
	}

	return type;
}

int _close_volume(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	if (ad->flag_deleting == EINA_TRUE) {
		return -1;
	}

	_D("start closing volume\n");
	ad->flag_deleting = EINA_TRUE;

	_ungrab_key(ad);

	_D_del_timer(ad->sutimer)
	_D_del_timer(ad->lutimer)
	_D_del_timer(ad->sdtimer)
	_D("del long down timer\n");
	_D_del_timer(ad->ldtimer)
	_D_del_timer(ad->ptimer)

	if (ad->pu) {
		evas_object_del(ad->pu);
		ad->pu = NULL;
	}

	if (ad->win) {
		evas_object_del(ad->win);
		ad->win = NULL;
	}
	appcore_flush_memory();

	ad->flag_deleting = EINA_FALSE;
	_D("end closing volume\n");
}

Eina_Bool _popup_timer_cb(void *data)
{
_D("func\n");
	_close_volume(data);
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool _lu_timer_cb(void *data)
{
	int val;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	if (ad->win == NULL) {
		_D("win is NULL, so long press pass\n");
		return ECORE_CALLBACK_CANCEL;
	}
	_D_del_timer(ad->stimer)

	_get_sound_level(ad->type, &val);
	_set_sound_level(ad->type, val +1 > ad->step ? ad->step : val + 1);
	_D("down, type(%d), step(%d) val[%d]\n", ad->type, ad->step, val+1);

	return ECORE_CALLBACK_RENEW;
}

Eina_Bool _su_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	_D_add_timer(ad->lutimer, 0.0, _lu_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool _ld_timer_cb(void *data)
{
	int val;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");
	if (ad->win == NULL) {
		_D("win is NULL, so long press pass\n");
		return ECORE_CALLBACK_CANCEL;
	}
	_D_del_timer(ad->stimer)

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
	_D_add_timer(ad->ldtimer, 0.0, _ld_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	int val;
	Ecore_Event_Key *ev = event;
	struct appdata *ad = (struct appdata *)data;

	retvm_if(ev == NULL, 0, "Invalid arguemnt: event is NULL\n");
	retvm_if(ad == NULL, 0, "Invalid argument: appdata is NULL\n");

	if (ad->flag_touching == EINA_TRUE) {
		return ECORE_CALLBACK_CANCEL;
	}

	ad->flag_pressing = EINA_TRUE;

	_D_del_timer(ad->ptimer)

	if (!strcmp(ev->keyname, KEY_VOLUMEUP)) {
		_get_sound_level(ad->type, &val);
		if (val == ad->step) {
			_set_sound_level(ad->type, ad->step);
			_play_sound(ad->type, ad->sh);
			return ECORE_CALLBACK_CANCEL;
		}
		_set_sound_level(ad->type, val + 1);
		_play_sound(ad->type, ad->sh);
		_D_del_timer(ad->sutimer)
		_D_add_timer(ad->sutimer, 0.5, _su_timer_cb, ad)

		_D("set volume %d -> [%d]\n", val, val+1);

	} else if (!strcmp(ev->keyname, KEY_VOLUMEDOWN)) {
		_get_sound_level(ad->type, &val);
		if (val == 0) {
			_play_vib(ad->sh);
			return ECORE_CALLBACK_CANCEL;
		}
		_set_sound_level(ad->type, val - 1);
		_play_sound(ad->type, ad->sh);
		_D_del_timer(ad->sdtimer)
		_D_add_timer(ad->sdtimer, 0.5, _sd_timer_cb, ad)

		_D("type (%d) set volume %d -> [%d]\n", ad->type, val, val-1);

	}
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _key_release_cb(void *data, int type, void *event)
{
	Ecore_Event_Key *ev = event;
	struct appdata *ad = (struct appdata *)data;

	retvm_if(ev == NULL, 0, "Invalid arguemnt: event is NULL\n");
	retvm_if(ad == NULL, 0, "Invalid argument:appdata is NULL\n");
	if (ad->win == NULL) {
		_D("There is no window in volume, so pass\n");
	}

	if (ad->flag_touching == EINA_TRUE) {
		return ECORE_CALLBACK_CANCEL;
	}

	if (!strcmp(ev->keyname, KEY_VOLUMEUP)) {
		_D("up key released and del timer\n");
		_D_del_timer(ad->sutimer)
		_D_del_timer(ad->lutimer)

	} else if (!strcmp(ev->keyname, KEY_VOLUMEDOWN)) {
		_D("down key released and del timer\n");
		_D_del_timer(ad->sdtimer)
		_D("del long down timer\n");
		_D_del_timer(ad->ldtimer)
	}

	ad->flag_pressing = EINA_FALSE;

	_D_del_timer(ad->ptimer)
	_D_add_timer(ad->ptimer, 3.0, _popup_timer_cb, ad)

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

Eina_Bool _test_timer_cb(void *data)
{
	_D("func\n");
	Ecore_X_Display* disp = ecore_x_display_get();
	Utilx_Key_Status upstat, downstat;
	upstat = utilx_get_key_status(disp, KEY_VOLUMEUP);
	downstat = utilx_get_key_status(disp, KEY_VOLUMEDOWN);
	_D("disp(%x) up(%s) down(%s)\n", disp,
			upstat == UTILX_KEY_STATUS_PRESSED ? "press" : "release",
			downstat == UTILX_KEY_STATUS_PRESSED ? "press" : "release");
	if (upstat == UTILX_KEY_STATUS_RELEASED) {

	} else {

	}
		return ECORE_CALLBACK_RENEW;

}

int _app_create(struct appdata *ad)
{

	_init_vconf(ad);
	_init_svi(ad);
	_init_mm_sound(ad);

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

/* to set icon, slider etc. via mm sound callback */
void _set_level(int type)
{
	int val;
	_get_sound_level(type, &val);
	_set_sound_level(type, val);
	_D("type(%d) val(%d)\n", type, val);
}

static void _block_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
_D("func\n");

	_close_volume(data);
	evas_object_del(obj);
}

static void _title_0_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	char buf[64] = {0, };
	char name[64] = {0, };
	int i, type;
	struct appdata *ad = (struct appdata *)data;

	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	_D_del_timer(ad->ptimer)

	if (ad->flag_titleopen == EINA_TRUE) {
		ad->flag_titleopen = EINA_FALSE;
		edje_object_signal_emit(obj, "hide", "title");
		_D_add_timer(ad->ptimer, 3.0, _popup_timer_cb, ad)

	} else {
		ad->flag_titleopen = EINA_TRUE;
		edje_object_signal_emit(obj, "show", "title");

		type = ad->type;
		for (i = 1; i < 7; i++) {
			type = (type + 1) % 7;
			snprintf(name, sizeof(name), "text/title/%d", i);
			_get_title(type, buf, sizeof(buf));
			_D("name(%s) %d type(%s)\n", name, type, buf);
			edje_object_part_text_set(obj, name, buf);
		}
	}
}

void _set_title_text(Evas_Object *obj, Evas_Object *pu, int type)
{
	char buf[64] = {0, };

	retm_if(obj == NULL, "Invalid argument: object is NULL\n");
	_get_title(type, buf, sizeof(buf));
	elm_object_part_text_set(pu, "title,text", buf);
}

void _title_func(Evas_Object *obj, void *data, int pivot)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	edje_object_signal_emit(obj, "hide", "title");
	ad->type = (ad->type + pivot) % 7;
	ad->flag_titleopen = EINA_FALSE;
	_set_title_text(obj, ad->pu, ad->type);
	_set_level(ad->type);
	_D_add_timer(ad->ptimer, 3.0, _popup_timer_cb, ad)
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
		} else {

		}

		return ECORE_CALLBACK_RENEW;
	}
	return ECORE_CALLBACK_CANCEL;
}

static void _slider_start_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("func\n");
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	if (ad->flag_pressing == EINA_TRUE) {
		return;
	}
	ad->flag_touching = EINA_FALSE;

	_D_del_timer(ad->ptimer)

	if (ad->lutimer || ad->ldtimer) {
		_D("return when long press is working\n");
		return;
	}

	_D_add_timer(ad->stimer, 0.2, _slider_timer_cb, data)
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

	_D_del_timer(ad->stimer)
	if (ad->sl) {
		val = elm_slider_value_get(ad->sl);
		val += 0.5;
		_set_sound_level(ad->type, (int)val);
	}
	_play_sound(ad->type, ad->sh);

	ad->flag_touching = EINA_FALSE;
	_D_add_timer(ad->ptimer, 3.0, _popup_timer_cb, ad)
}

Eina_Bool _lu_warmup_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	_D_add_timer(ad->lutimer, 0.0, _lu_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}

Eina_Bool _ld_warmup_timer_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	_D("add long down timer\n");
	_D_add_timer(ad->ldtimer, 0.0, _ld_timer_cb, ad)
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
				_D_del_timer(ad->ptimer)
					ecore_timer_add(0.5, _lu_warmup_timer_cb, ad);
			}

		} else if (!strncmp(bval, "VOLUME_DOWN", strlen("LONG_PRESS"))) {
			downstat = utilx_get_key_status(disp, KEY_VOLUMEDOWN);
			if (downstat == UTILX_KEY_STATUS_PRESSED ) {
				_D("volume down long press\n");
				_D_del_timer(ad->ptimer)
					ecore_timer_add(0.5, _ld_warmup_timer_cb, ad);
			}

		} else {
			_D_add_timer(ad->ptimer, 3.0, _popup_timer_cb, ad)

		}

	} else {
		_D_add_timer(ad->ptimer, 3.0, _popup_timer_cb, ad)

	}
	return 0;
}

int _app_reset(bundle *b, void *data)
{
	int ret, status;
	int lock, type, val;
	double scale;
	Evas_Object *win, *pu, *ly, *ic, *sl;
	char buf[256] = {0, };
	struct appdata *ad = (struct appdata *)data;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	ad->flag_touching = EINA_FALSE;

	scale = elm_config_scale_get();
	_D("scale(%lf)\n", scale);
	status = _check_status(&lock, &type);
	mm_sound_volume_get_value(type, &val);
	ad->type = type;

	if (status == 0) {
		ad->step = _get_step(type);
		_set_level(type);

		if (win) {
			return 0;
		}

		win = _add_window(PACKAGE);
		elm_win_alpha_set(win, EINA_TRUE);
		retvm_if(win == NULL, -1, "Failed add window\n");
		ad->win = win;

		_grab_key(ad);

		pu = _add_popup(win);
		retvm_if(pu == NULL, -1, "Failed to add popup\n");
		evas_object_smart_callback_add(pu, "block,clicked", _block_clicked_cb, ad);
		_get_title(type, buf, sizeof(buf));
		elm_object_part_text_set(pu, "title,text", buf);
		evas_object_color_set(pu, 0, 0, 0, 0);
		ad->pu = pu;

		ly = _add_layout(pu, EDJ_FILE, "volume");
		elm_object_content_set(pu, ly);
		ad->ly = ly;

		sl = elm_slider_add(pu);
		elm_slider_indicator_show_set(sl, EINA_TRUE);
		elm_slider_indicator_format_set(sl, "%.0f");
		evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, 0.0);
		evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
		elm_slider_min_max_set(sl, 0, ad->step);
		elm_object_part_content_set(ly, "swallow/slider", sl);
		evas_object_smart_callback_add(sl, "slider,drag,start", _slider_start_cb, ad);
		evas_object_smart_callback_add(sl, "changed", _slider_changed_cb, ad);
		evas_object_smart_callback_add(sl, "slider,drag,stop", _slider_stop_cb, ad);
		elm_slider_min_max_set(sl, 0, ad->step);
		elm_slider_value_set(sl, val);
		ad->sl = sl;

		ret = syspopup_create(b, &handler, win, ad);
		retvm_if(ret < 0, -1, "Failed to create syspopup\n");

		_handle_bundle(b, ad);

		evas_object_show(win);
	}

	return 0;
}

int _app_pause(struct appdata *ad)
{
	_D("func\n");
	_close_volume(ad);
	return 0;
}


