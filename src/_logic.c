/*
 * org.tizen.volume
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
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

#define STRBUF_SIZE 128
#define PATHBUF_SIZE 256
#define EMUL_STR	"Emulator"

enum {
	IDLELOCK_OFF = 0x0,
	IDLELOCK_ON,
	IDLELOCK_MAX,
};

/* _check_status() return value */
enum{
	LOCK_AND_NOT_MEDIA = -0x1,
	UNLOCK_STATUS,
	LOCK_AND_MEIDA,
};

#ifndef VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS
#define VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS              VCONFKEY_SETAPPL_PREFIX"/accessibility/turn_off_all_sounds"
#endif

int _close_volume(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->flag_deleting == EINA_TRUE, -1, "Closing volume\n");

	_D("start closing volume\n");
	ad->flag_deleting = EINA_TRUE;

	_ungrab_key_new(ad);
	_grab_key_new(ad, ad->input_win, SHARED_GRAB);

	DEL_TIMER(ad->sutimer)
	DEL_TIMER(ad->lutimer)
	DEL_TIMER(ad->sdtimer)
	DEL_TIMER(ad->ldtimer)
	DEL_TIMER(ad->ptimer)

	if (evas_object_visible_get(ad->win) == EINA_TRUE){
		_D("hide window\n");
		evas_object_hide(ad->win);
	}
	appcore_flush_memory();

	ad->flag_deleting = EINA_FALSE;
	ad->flag_launching = EINA_FALSE;
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

Eina_Bool _volume_show(void *data)
{
	_D("%s\n", __func__);
	int status = -1;
	int type = MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY;
	int lock = IDLELOCK_ON;
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, EINA_FALSE, "Invalid argument: appdata is NULL\n");

	status = _check_status(&lock, &type);
	if(status != LOCK_AND_NOT_MEDIA && ad->win)
	{
		_init_mm_sound(ad);
		/* ungrab SHARED_GRAB */
		_ungrab_key_new(ad);

		if(status == UNLOCK_STATUS)
		{
			_grab_key_new(ad, -1, TOP_POSITION_GRAB);

			_rotate_func(ad);
			elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_HIDE);
			evas_object_show(ad->win);
			if(syspopup_has_popup(ad->volume_bundle))
				syspopup_reset(ad->volume_bundle);
		}
		else if(status == LOCK_AND_MEIDA)
		{
			_grab_key_new(ad, ad->input_win, EXCLUSIVE_GRAB);
		}
		ad->flag_launching = EINA_TRUE;
		_mm_func(ad);
		return EINA_TRUE;
	}
	else if(!ad->win)
	{
		/* recreate window */
	}

	_D("status == LOCK_AND_NOT_MEDIA\n");
	return EINA_FALSE;
}

static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	_D("%s\n", __func__);
	int val=0, snd=0, vib=0;
	Ecore_Event_Key *ev = event;
	int status = -1;
	int mtype = MM_ERROR_SOUND_VOLUME_CAPTURE_ONLY;
	int lock = IDLELOCK_ON;
	int is_turn_off_all_sounds = 0;
	int ret = -1;
	struct appdata *ad = (struct appdata *)data;

	status = _check_status(&lock, &mtype);

	retvm_if(ev == NULL, ECORE_CALLBACK_CANCEL, "Invalid arguemnt: event is NULL\n");
	retvm_if(ad == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->win == NULL, ECORE_CALLBACK_CANCEL, "Invalid argument: window is NULL\n");

	ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS, &is_turn_off_all_sounds);
	if(ret == 0)
	{
		retvm_if(is_turn_off_all_sounds == EINA_TRUE, ECORE_CALLBACK_CANCEL,
			"VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS is set, all sound is mute\n");
	}

	if(!ad->flag_launching)
	{
		if(_volume_show(data) != EINA_TRUE)
		{
			return ECORE_CALLBACK_CANCEL;
		}
	}

	if(ad->flag_touching == EINA_TRUE) {
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

	ADD_TIMER(ad->ptimer, 3.0, popup_timer_cb, ad)

	return ECORE_CALLBACK_CANCEL;
}

int _grab_key_new(struct appdata *ad, Ecore_X_Window _xwin, int grab_mode)
{
	_D("%s\n", __func__);
	Ecore_X_Display *disp = NULL;
	Ecore_X_Window xwin = 0;
	int ret = -1;

	/* ALREADY GRAB check */
	switch(grab_mode)
	{
		case SHARED_GRAB : if(ad->flag_shared_grabed)return -1;
			break;
		case EXCLUSIVE_GRAB : if(ad->flag_exclusive_grabed)return -1;
			break;
		case TOP_POSITION_GRAB : if(ad->flag_top_positioni_grabed)return -1;
			break;
	}

	disp = ecore_x_display_get();
	retvm_if(disp == NULL, -1, "Failed to get display\n");

	if(_xwin == -1)
	{
		/* TOP_POSITION_GRAB */
		xwin = elm_win_xwindow_get(ad->win);
		retvm_if(xwin == 0, -1, "elm_win_xwindow_get() failed\n");
	}
	else
		xwin = _xwin;

	ret = utilx_grab_key(disp, xwin, KEY_VOLUMEDOWN, grab_mode);
	retvm_if(ret < 0, -1, "Failed to grab key down\n");
	retvm_if(ret == 1, -1, "Already grab\n");

	ret = utilx_grab_key(disp, xwin, KEY_VOLUMEUP, grab_mode);
	retvm_if(ret < 0, -1, "Failed to grab key up\n");
	retvm_if(ret == 1, -1, "Already grab\n");

	switch(grab_mode)
	{
		case SHARED_GRAB :
			ad->flag_shared_grabed = EINA_TRUE;
			break;
		case EXCLUSIVE_GRAB :
			ad->flag_exclusive_grabed = EINA_TRUE;
			break;
		case TOP_POSITION_GRAB :
			ad->flag_top_positioni_grabed = EINA_TRUE;
			break;
	}

	return 0;
}

int _ungrab_key_new(struct appdata *ad)
{
	Ecore_X_Window xwin = 0;
	Ecore_X_Display *disp = NULL;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->input_win == 0, -1, "Invalid argument: ad->win is NULL\n");

	xwin = elm_win_xwindow_get(ad->win);
	retvm_if(xwin == 0, -1, "Failed to get xwindow\n");

	disp = ecore_x_display_get();
	retvm_if(disp == NULL, -1, "Failed to get display\n");

	utilx_ungrab_key(disp, ad->input_win, KEY_VOLUMEUP);
	utilx_ungrab_key(disp, ad->input_win, KEY_VOLUMEDOWN);
	_D("key ungrabed\n");

	if(ad->flag_exclusive_grabed)
		ad->flag_exclusive_grabed = EINA_FALSE;
	else if(ad->flag_top_positioni_grabed)
		ad->flag_top_positioni_grabed = EINA_FALSE;
	else if(ad->flag_shared_grabed)
		ad->flag_shared_grabed = EINA_FALSE;

	return 0;
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
	if(type == VOLUME_TYPE_NOTIFICATION || type == VOLUME_TYPE_SYSTEM)
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

	if (*lock == IDLELOCK_ON && *type == VOLUME_TYPE_MEDIA) {
		_D("lock is set, in media\n");
		return 1;
	}
	_D("unlock status, normal case\n");
	return 0;
}

void _starter_user_volume_key_vconf_changed_cb(keynode_t *key, void *data){
	_D("%s\n", __func__);
	int ret = EINA_FALSE;
	vconf_get_int(VCONFKEY_STARTER_USE_VOLUME_KEY, &ret);
	if(ret != 0)
	{
		_D("any other App grab volume hard key\n", __func__);
		_close_volume(data);
		vconf_set_int(VCONFKEY_STARTER_USE_VOLUME_KEY, 0);
	}
}

void _idle_lock_state_vconf_chnaged_cb(keynode_t *key, void *data){
	_close_volume(data);
}

int _app_create(struct appdata *ad)
{
	_D("%s\n", __func__);
	int ret = 0;
	_init_svi(ad);

	/* create input_window */
	ad->input_win = _add_input_window();
	retvm_if(ad->input_win == 0, -1, "Failed to create input window\n");

	/* vconf changed callback */
	vconf_notify_key_changed(VCONFKEY_STARTER_USE_VOLUME_KEY,
				 _starter_user_volume_key_vconf_changed_cb, ad);

	/* Lock screen status vconf changed callback */
	vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,
				_idle_lock_state_vconf_chnaged_cb, ad);

	/* grab volume shared grab */
	ret = _grab_key_new(ad, ad->input_win, SHARED_GRAB);

	/* ecore event handler add once */
	if(ad->event_volume_down == NULL)
		ad->event_volume_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_press_cb, ad);
	if(ad->event_volume_up == NULL)
		ad->event_volume_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_release_cb, ad);

	_init_mm_sound(ad);

	return ret;
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

	retvm_if(dpy == NULL, -1, "dpy is NULL\n");
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
		if ((int)val != 0) {
			_set_sound_level(ad->type, (int)val);
		}
		if (val <= 0.5) {
			elm_slider_value_set(ad->sl, 0);
			_set_sound_level(ad->type, 0);
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
	ad->flag_touching = EINA_TRUE;

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
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	if (ad->flag_pressing == EINA_TRUE) {
		return;
	}
	if (ad->lutimer || ad->ldtimer) {
		_D("return when long press is working\n");
		return;
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
	elm_image_file_set(obj, buf, NULL);
}

static void _button_cb(void *data, Evas_Object *obj, void *event_info)
{
	_D("%s\n", __func__);
	struct appdata *ad = (struct appdata *)data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	char buf[PATHBUF_SIZE] = {0, };
	snprintf(buf, sizeof(buf), "%s/%s", IMAGEDIR, IMG_VOLUME_ICON_SETTINGS);
	elm_image_file_set(obj, buf, NULL);
	if(evas_object_visible_get(ad->win)){
		DEL_TIMER(ad->ptimer)
		if(ecore_x_e_illume_quickpanel_state_get(
			ecore_x_e_illume_zone_get(elm_win_xwindow_get(ad->win))) != ECORE_X_ILLUME_QUICKPANEL_STATE_OFF)
		{
			_D("Quickpanel is hide\n");
			ecore_x_e_illume_quickpanel_state_send(
				ecore_x_e_illume_zone_get(elm_win_xwindow_get(ad->win)), ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
		}
		service_h svc;
		service_create(&svc);
		service_set_package(svc, "setting-profile-efl");
		service_send_launch_request(svc, NULL, NULL);
		_app_pause(ad);

		service_destroy(svc);
	}
}

static int _check_emul(void)
{
	int is_emul = 0;
	char *info = NULL;

	if (system_info_get_value_string(SYSTEM_INFO_KEY_MODEL, &info) == 0) {
		if (info == NULL) return 0;
		if (!strncmp(EMUL_STR, info, strlen(info))) {
			is_emul = 1;
		}
	}

	if (info != NULL) free(info);

	return is_emul;
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

	ad->noti_seen = EINA_FALSE;
	ad->flag_launching = EINA_FALSE;
	ad->flag_pressing = EINA_FALSE;
	ad->flag_touching = EINA_FALSE;

	status = _check_status(&lock, &type);
	ad->type = type;
	mm_sound_volume_get_value(type, (unsigned int*)(&val));

	if(ad->win == NULL) {
		win = _add_window(PACKAGE);
		retvm_if(win == NULL, -1, "Failed add window\n");
		_D("create window\n");
		ad->win = win;
	}
	else {
		_E("window already exist\n");
		return -1;
	}

	th = elm_theme_new();
	elm_theme_ref_set(th, NULL);
	elm_theme_extension_add(th, EDJ_APP);

	ad->flag_emul = _check_emul();

	if(!ad->flag_emul)
	{
		block = _add_layout(win, EDJ_APP, GRP_VOLUME_BLOCKEVENTS);
		retvm_if(block == NULL, -1, "Failed to add block layout\n");

		edje_object_signal_callback_add(elm_layout_edje_get(block), "clicked", "*", _block_clicked_cb, ad);

		outer = _add_layout(win, EDJ_APP, GRP_VOLUME_LAYOUT);
		retvm_if(outer== NULL, -1, "Failed to add outer layout\n");

		inner = _add_layout(win, EDJ_APP, "popup_volumebar");
		retvm_if(inner == NULL, -1, "Failed to add inner layout\n");

		ad->block_events = block;
		ad->ly = outer;

		elm_object_part_content_set(outer, "elm.swallow.content", inner);

		sl = _add_slider(win, 0, ad->step, val);
		elm_object_theme_set(sl, th);
		evas_object_smart_callback_add(sl, "slider,drag,start", _slider_start_cb, ad);
		evas_object_smart_callback_add(sl, "changed", _slider_changed_cb, ad);
		evas_object_smart_callback_add(sl, "slider,drag,stop", _slider_stop_cb, ad);

		ic_settings = elm_icon_add(win);
		evas_object_size_hint_aspect_set(ic_settings, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		elm_image_resizable_set(ic_settings, EINA_FALSE, EINA_FALSE);
		snprintf(buf, sizeof(buf), "%s/%s", IMAGEDIR, IMG_VOLUME_ICON_SETTINGS);
		_D("%s\n", buf);
		elm_image_file_set(ic_settings, buf, NULL);
		elm_object_part_content_set(inner, "elm.swallow.icon", ic_settings);
		evas_object_event_callback_add(ic_settings, EVAS_CALLBACK_MOUSE_DOWN, _button_mouse_down_cb, ad);
		evas_object_smart_callback_add(ic_settings, "clicked", _button_cb, ad);
		evas_object_show(ic_settings);
		ad->ic_settings = ic_settings;

		ad->sl = sl;
		elm_object_part_content_set(inner, "elm.swallow.content", sl);

		ic = elm_icon_add(win);
		evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
		elm_object_part_content_set(ad->sl, "icon", ic);
		ad->ic = ic;
		_set_icon(ad, val);
	}
	else
	{
		block = _add_layout(win, EDJ_APP, GRP_VOLUME_BLOCKEVENTS);
		edje_object_signal_callback_add(elm_layout_edje_get(block), "clicked", "*", _block_clicked_cb, ad);
		outer = _add_layout(win, EDJ_APP, GRP_VOLUME_LAYOUT);
		inner = _add_layout(win, EDJ_APP, GRP_VOLUME_CONTENT);
		ad->block_events = block;
		ad->ly = outer;

		elm_object_part_content_set(outer, "elm.swallow.content", inner);

		sl = _add_slider(win, 0, ad->step, val);
		evas_object_smart_callback_add(sl, "slider,drag,start", _slider_start_cb, ad);
		evas_object_smart_callback_add(sl, "changed", _slider_changed_cb, ad);
		evas_object_smart_callback_add(sl, "slider,drag,stop", _slider_stop_cb, ad);

		ad->sl = sl;
		elm_object_part_content_set(inner, "elm.swallow.content", sl);
		evas_object_show(sl);

		ic = elm_icon_add(win);
		evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
		elm_object_part_content_set(ad->sl, "icon", ic);
		ad->ic = ic;
		_set_icon(ad, val);
	}


	ret = syspopup_create(b, &handler, ad->win, ad);
	retvm_if(ret < 0, -1, "Failed to create syspopup\n");
	ad->volume_bundle = bundle_dup(b);

	return 0;
}

int _app_pause(struct appdata *ad)
{
	_D("%s\n", __func__);
	_close_volume(ad);
	return 0;
}
