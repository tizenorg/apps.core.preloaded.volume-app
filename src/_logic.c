/* test
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
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <system_info.h>

#include"_logic.h"
#include "volume.h"
#include "_util_log.h"
#include "_util_efl.h"
#include "_sound.h"
#include "_button.h"

#define STRBUF_SIZE 128
#define PATHBUF_SIZE 256

enum {
	IDLELOCK_OFF = 0x0,
	IDLELOCK_ON,
	IDLELOCK_MAX,
};

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
	int val = -1;
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
	int val = 0;
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
	int val=0, snd=0, vib=0;
	Ecore_Event_Key *ev = event;
	struct appdata *ad = (struct appdata *)data;

	retvm_if(ev == NULL, ECORE_CALLBACK_CANCEL, "Invalid arguemnt: event is NULL\n");
	retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->win == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: window is NULL\n");

	if (ad->flag_touching == EINA_TRUE) {
		return ECORE_CALLBACK_CANCEL;
	}

	ad->flag_pressing = EINA_TRUE;

	DEL_TIMER(ad->ptimer)
	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &snd);
	vconf_get_bool(VCONFKEY_SETAPPL_VIBRATION_STATUS_BOOL, &vib);

	/* If sound is set off, only RINGTONE, MEDIA types are able to change. */
	if(!snd && ad->type != VOLUME_TYPE_MEDIA && ad->type != VOLUME_TYPE_RINGTONE){
		if(vib)_play_vib(ad->sh);
		return ECORE_CALLBACK_CANCEL;
	}

	if (!strcmp(ev->keyname, KEY_VOLUMEUP)) {
		_get_sound_level(ad->type, &val);
		if (val == ad->step) {
			_set_sound_level(ad->type, ad->step);
			if(snd)_play_sound(ad->type, ad->sh);
			return ECORE_CALLBACK_CANCEL;
		}
		_set_sound_level(ad->type, val + 1);
		if(snd)_play_sound(ad->type, ad->sh);
		DEL_TIMER(ad->sutimer)
		ADD_TIMER(ad->sutimer, 0.5, _su_timer_cb, ad)

		_D("set volume %d -> [%d]\n", val, val+1);

	} else if (!strcmp(ev->keyname, KEY_VOLUMEDOWN)) {
		_get_sound_level(ad->type, &val);
		if (val == 0) {
			if(vib)_play_vib(ad->sh);
			return ECORE_CALLBACK_CANCEL;
		}
		_set_sound_level(ad->type, val - 1);
		if(snd)_play_sound(ad->type, ad->sh);
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
		DEL_TIMER(ad->luwarmtimer)
	} else if (!strcmp(ev->keyname, KEY_VOLUMEDOWN)) {
		_D("down key released and del timer\n");
		DEL_TIMER(ad->sdtimer)
		DEL_TIMER(ad->ldtimer)
		DEL_TIMER(ad->ldwarmtimer)
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
	Ecore_X_Window xwin = 0;
	Ecore_X_Display *disp = NULL;
	int ret = -1;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	disp = ecore_x_display_get();
	retvm_if(disp == NULL, -1, "Failed to get display\n");

	retvm_if(ad->win == NULL, -1, "Invalid argument: ad->win is NULL\n");
	xwin = elm_win_xwindow_get(ad->win);
	retvm_if(xwin == 0, -1, "Failed to get xwindow\n");

	ret = utilx_grab_key(disp, xwin, KEY_VOLUMEDOWN, TOP_POSITION_GRAB);
	retvm_if(ret < 0, -1, "Failed to grab key down\n");
	retvm_if(ret == 1, -1, "Already grab\n");

	ret = utilx_grab_key(disp, xwin, KEY_VOLUMEUP, TOP_POSITION_GRAB);
	retvm_if(ret < 0, -1, "Failed to grab key up\n");
	retvm_if(ret == 1, -1, "Already grab\n");

	ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_press_cb, ad);
	ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_release_cb, ad);
	_D("key grabed\n");
	return 0;
}

void _ungrab_key(struct appdata *ad)
{
	Ecore_X_Window xwin = 0;
	Ecore_X_Display *disp = NULL;

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
	int ret = -1;
	int lock = IDLELOCK_OFF;

	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock);
	retvm_if(ret < 0, -1, "Failed to get vconf %s\n",
		 VCONFKEY_IDLE_LOCK_STATE);
	_D("idlelock vconf:%d\n", lock);

	return lock == VCONFKEY_IDLE_LOCK ? IDLELOCK_ON : IDLELOCK_OFF;
}

volume_type_t _get_volume_type(void)
{
	int ret = MM_ERROR_NONE;
	volume_type_t type = -1;

	ret = mm_sound_volume_get_current_playing_type(&type);
	switch (ret) {
		case MM_ERROR_NONE:
			break;
		case MM_ERROR_SOUND_VOLUME_NO_INSTANCE:
		case MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY:
			type = VOLUME_TYPE_RINGTONE;
			break;
		default:
			_D("Failed to get sound type(errno:%x)\n", ret);
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

int _app_create(struct appdata *ad)
{
	_init_svi(ad);

	return 0;
}

int __utilx_ss_get_window_property(Display *dpy, Window win, Atom atom,
					  Atom type, unsigned int *val,
					  unsigned int len)
{
	unsigned char *prop_ret = NULL;
	Atom type_ret = -1;
	unsigned long bytes_after = 0;
	unsigned long  num_ret = -1;
	int format_ret = -1;
	unsigned int i = 0;
	int num = 0;

	prop_ret = NULL;
	if (XGetWindowProperty(dpy, win, atom, 0, 0x7fffffff, False,
			       type, &type_ret, &format_ret, &num_ret,
			       &bytes_after, &prop_ret) != Success)
		return -1;

	if (type_ret != type || format_ret != 32)
		num = -1;
	else if (num_ret == 0 || !prop_ret)
		num = 0;
	else {
		if (num_ret < len)
			len = num_ret;
		for (i = 0; i < len; i++) {
			val[i] = ((unsigned long *)prop_ret)[i];
		}
		num = len;
	}

	if (prop_ret)
		XFree(prop_ret);

	return num;
}

int _x_rotation_get(Display *dpy, void *data)
{
	Window active_win = 0;
	Window root_win = 0;
	int rotation = -1;
	int ret = -1;

	Atom atom_active_win;
	Atom atom_win_rotate_angle;

	root_win = XDefaultRootWindow(dpy);

	atom_active_win = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	ret = __utilx_ss_get_window_property(dpy, root_win, atom_active_win,
					     XA_WINDOW,
					     (unsigned int *)&active_win, 1);

	if(ret < 0)
		return ret;

	atom_win_rotate_angle = XInternAtom(dpy, "_E_ILLUME_ROTATE_ROOT_ANGLE", False);
	ret = __utilx_ss_get_window_property(dpy, root_win,
					  atom_win_rotate_angle, XA_CARDINAL,
					  (unsigned int *)&rotation, 1);

	if(ret != -1)
		return rotation;

	return -1;
}

int _volume_popup_resize(void *data, int angle)
{
	int rotation = 0;
	int offx=0, offy=0;
	Evas_Coord minw = -1, minh = -1;
	Evas_Coord w = 0, h = 0;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad->win == NULL, -1, "Invalid argument: window is NULL\n");
	retvm_if(ad->block_events == NULL, -1, "Invalid argument: notify is NULL\n");

	if(angle == -1)
		rotation = ad->angle;
	else
		rotation = angle;

	system_info_get_value_int(SYSTEM_INFO_KEY_SCREEN_WIDTH, &w);
	system_info_get_value_int(SYSTEM_INFO_KEY_SCREEN_HEIGHT, &h);

	switch(rotation){
		case 90 :
		case 270 :
			evas_object_resize(ad->block_events, h, w);
			edje_object_size_min_get(ad->block_events, &minw, &minh);
			edje_object_size_min_restricted_calc(ad->block_events, &minw, &minh, minw, minh);
			offx = (h - minw) / 2;
			offy = (w - minh) / 5;
			break;
		case 0 :
		case 180 :
		default :
			evas_object_resize(ad->block_events, w, h);
			edje_object_size_min_get(ad->block_events, &minw, &minh);
			edje_object_size_min_restricted_calc(ad->block_events, &minw, &minh, minw, minh);
			offx = (w - minw) / 2;
			offy = (h - minh) / 5;
			break;
	}
	_D("w(%d) h(%d) offx(%d) offy(%d)\n", w, h, offx, offy);
	evas_object_move(ad->ly, offx, offy);
	return 1;
}

int _efl_rotate(Display *dpy, void *data)
{
	int rotation = -1;
	struct appdata *ad = (struct appdata *)data;

	rotation = _x_rotation_get(dpy, ad);

	if(rotation == -1)
		rotation = 0;

	if(rotation >= 0){
		elm_win_rotation_set(ad->win, rotation);
		ad->angle = rotation;
		return _volume_popup_resize(data, rotation);
	}

	return 0;
}

int _rotate_func(void *data)
{
	_D("%s\n", __func__);
	struct appdata *ad = (struct appdata *)data;
	Display *d = NULL;
	int ret = 0;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	d = XOpenDisplay(NULL);
	ret = _efl_rotate(d, ad);
	XCloseDisplay(d);

	return ret;
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
	int val = 0;
	_get_sound_level(type, &val);
	_set_sound_level(type, val);
	_D("type(%d) val(%d)\n", type, val);
}

static void _block_clicked_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
	_close_volume(data);
}

Eina_Bool _slider_timer_cb(void *data)
{
	double val = 0;
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
	int snd= 0 ;
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
	double val = 0;
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
	double val = 0;
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
	ADD_TIMER(ad->ldtimer, 0.0, _ld_timer_cb, ad)
	return ECORE_CALLBACK_CANCEL;
}

void _init_press_timers(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	DEL_TIMER(ad->sdtimer)
	DEL_TIMER(ad->ldtimer)
	DEL_TIMER(ad->ldwarmtimer)
	DEL_TIMER(ad->sutimer)
	DEL_TIMER(ad->lutimer)
	DEL_TIMER(ad->luwarmtimer)
}

int _handle_bundle(bundle *b, struct appdata *ad)
{
	const char *bval = NULL;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	bval = bundle_get_val(b, "LONG_PRESS");
	if (bval) {
		_D("val(%s)\n", bval);

		if (!strncmp(bval, "VOLUME_UP", strlen("LONG_PRESS"))) {
			_D("volume up long press\n");
			DEL_TIMER(ad->ptimer)
			ADD_TIMER(ad->luwarmtimer, 0.5, _lu_warmup_timer_cb, ad);
		} else if (!strncmp(bval, "VOLUME_DOWN", strlen("LONG_PRESS"))) {
			_D("volume down long press\n");
			DEL_TIMER(ad->ptimer)
			ADD_TIMER(ad->ldwarmtimer, 0.5, _ld_warmup_timer_cb, ad);
		} else {
			ADD_TIMER(ad->ptimer, 3.0, popup_timer_cb, ad)
		}
	} else {
		ADD_TIMER(ad->ptimer, 3.0, popup_timer_cb, ad)
		_init_press_timers(ad);
	}
	return 0;
}

static void _button_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	_D("%s\n", __func__);
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	char buf[PATHBUF_SIZE] = {0, };
	snprintf(buf, sizeof(buf), "%s/%s", IMAGEDIR, IMG_VOLUME_ICON_SETTINGS_PRESSED);
	elm_icon_file_set(obj, buf, NULL);
}

static void _button_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("%s\n", __func__);
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	char buf[PATHBUF_SIZE] = {0, };
	snprintf(buf, sizeof(buf), "%s/%s", IMAGEDIR, IMG_VOLUME_ICON_SETTINGS);
	elm_icon_file_set(obj, buf, NULL);
	if(evas_object_visible_get(ad->win)){
		DEL_TIMER(ad->ptimer)
		_open_ug(ad);
	}
}

int _app_reset(bundle *b, void *data)
{
	_D("%s\n", __func__);
	int ret = -1, status = -1, val = 0;
	int type = MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY;
	int lock = IDLELOCK_ON;
	Evas_Object *win, *sl, *ic, *ic_settings;
	/* volume-app layout */
	Elm_Theme *th;
	Evas_Object *outer, *inner, *block;
	char buf[PATHBUF_SIZE] = {0, };
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	ad->flag_touching = EINA_FALSE;
	ad->noti_seen = EINA_FALSE;

	_init_mm_sound(ad);
	status = _check_status(&lock, &type);
	ad->type = type;
	mm_sound_volume_get_value(type, (unsigned int*)(&val));

	if (status == 0) {
		if(ad->win){
			_D("window exists", __func__);
			if(_grab_key(ad)==-1)return -1;
			_handle_bundle(b, ad);
			_rotate_func(ad);
			elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_HIDE);
			evas_object_show(ad->win);
			_mm_func(ad);
			if (syspopup_has_popup(b))
				syspopup_reset(b);
			ad->flag_launching = EINA_FALSE;
			return 0;
		}
		ad->step = _get_step(type);

		_set_level(type);

		win = _add_window(PACKAGE);
		retvm_if(win == NULL, -1, "Failed add window\n");
		ad->win = win;

		_grab_key(ad);
		mm_sound_route_get_playing_device(&(ad->device));

		th = elm_theme_new();
		elm_theme_ref_set(th, NULL);
		elm_theme_extension_add(th, EDJ_APP);

		block = _add_layout(win, EDJ_APP, GRP_VOLUME_BLOCKEVENTS);
		edje_object_signal_callback_add(elm_layout_edje_get(block), "clicked", "*", _block_clicked_cb, ad);
		outer = _add_layout(win, EDJ_APP, GRP_VOLUME_LAYOUT);
		inner = _add_layout(win, EDJ_APP, GRP_VOLUME_CONTENT);
		ad->block_events = block;
		ad->ly = outer;

		elm_object_part_content_set(outer, "elm.swallow.content", inner);

		sl = _add_slider(win, 0, ad->step, val);
		elm_object_theme_set(sl, th);
		elm_object_style_set(sl, GRP_VOLUME_SLIDER_HORIZONTAL);
		evas_object_smart_callback_add(sl, "slider,drag,start", _slider_start_cb, ad);
		evas_object_smart_callback_add(sl, "changed", _slider_changed_cb, ad);
		evas_object_smart_callback_add(sl, "slider,drag,stop", _slider_stop_cb, ad);

		ic_settings = elm_icon_add(win);
		evas_object_size_hint_aspect_set(ic_settings, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		elm_icon_resizable_set(ic_settings, EINA_FALSE, EINA_FALSE);
		snprintf(buf, sizeof(buf), "%s/%s", IMAGEDIR, IMG_VOLUME_ICON_SETTINGS);
		_D("%s\n", buf);
		elm_icon_file_set(ic_settings, buf, NULL);
		elm_object_part_content_set(sl, "end", ic_settings);
		evas_object_event_callback_add(ic_settings, EVAS_CALLBACK_MOUSE_DOWN, _button_mouse_down_cb, ad);
		evas_object_smart_callback_add(ic_settings, "clicked", _button_cb, ad);
		evas_object_show(ic_settings);
		ad->ic_settings = ic_settings;

		ad->sl = sl;
		elm_object_part_content_set(inner, "elm.swallow.content", sl);

		ic = elm_icon_add(win);
		evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		elm_icon_resizable_set(ic, EINA_FALSE, EINA_FALSE);
		elm_object_part_content_set(ad->sl, "icon", ic);
		ad->ic = ic;
		_set_icon(ad, val);

		ret = syspopup_create(b, &handler, ad->win, ad);
		retvm_if(ret < 0, -1, "Failed to create syspopup\n");

		_handle_bundle(b, ad);

		_rotate_func(ad);
		evas_object_show(ad->win);
	}
	ad->flag_launching = EINA_FALSE;
	return 0;
}

int _app_pause(struct appdata *ad)
{
	_D("%s\n", __func__);
	if(ad->ug){
		_D("%d\n", ug_destroy(ad->ug));
		ad->ug = NULL;
		ecore_x_netwm_window_type_set(elm_win_xwindow_get(ad->win), ECORE_X_WINDOW_TYPE_NOTIFICATION);
		utilx_set_window_opaque_state(ecore_x_display_get(), elm_win_xwindow_get(ad->win), UTILX_OPAQUE_STATE_OFF);
		elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_HIDE);
	}
	_close_volume(ad);
	return 0;
}
