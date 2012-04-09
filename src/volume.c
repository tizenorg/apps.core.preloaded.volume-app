/*
 *  VOLUME
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * Contact: Jeesun Kim <iamjs.kim@samsung.com>
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS (Confidential Information). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with SAMSUNG ELECTRONICS.  SAMSUNG make no representations or warranties
 * about the suitability of the software, either express or implied,
 * including but not limited to the implied warranties of merchantability,
 * fitness for a particular purpose, or non-infringement. SAMSUNG shall
 * not be liable for any damages suffered by licensee as a result of
 * using, modifying or distributing this software or its derivatives.
 */

#include <stdio.h>
#include <appcore-efl.h>
#include <Ecore_X.h>

#include "volume.h"
#include "_util_log.h"
#include "_util_efl.h"
#include "_logic.h"

struct text_part {
	char *part;
	char *msgid;
};

static struct text_part main_txt[] = {
	{ "txt_title", N_("Application template"), },
	{ "txt_mesg", N_("Click to exit"), },
};


static void win_del(void *data, Evas_Object *obj, void *event)
{
	elm_exit();
}

static void main_quit_cb(void *data, Evas_Object *obj,
		const char *emission, const char *source)
{
	elm_exit();
}

static void update_ts(Evas_Object *eo, struct text_part *tp, int size)
{
	int i;

	if (eo == NULL || tp == NULL || size < 0)
		return;

	for (i = 0; i < size; i++) {
		if (tp[i].part && tp[i].msgid)
			edje_object_part_text_set(eo,
					tp[i].part, _(tp[i].msgid));
	}
}

static int lang_changed(void *data)
{
	return 0;
}

static int app_create(void *data)
{
	struct appdata *ad = data;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");

	_app_create(ad);

	lang_changed(ad);

	/* add system event callback */
	appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE,
			lang_changed, ad);

	/* appcore measure time example */
	printf("from AUL to %s(): %d msec\n", __func__,
			appcore_measure_time_from("APP_START_TIME"));

	appcore_measure_start();
	return 0;
}

static int app_terminate(void *data)
{
	struct appdata *ad = data;

	if (ad->win)
		evas_object_del(ad->win);

	return 0;
}

static int app_pause(void *data)
{
	struct appdata *ad = data;
	_app_pause(data);
	return 0;
}

static int app_resume(void *data)
{
	struct appdata *ad = data;

	return 0;
}

static int app_reset(bundle *b, void *data)
{
	struct appdata *ad = data;

	if (ad->flag_launching == EINA_TRUE) {
		return 0;
	}
	ad->flag_launching = EINA_TRUE;

	if (syspopup_has_popup(b)) {
		_D("has popup\n");

	} else {
		_D("has not popup\n");
		_app_reset(b, data);
	}
	/* appcore measure time example */
	printf("from AUL to %s(): %d msec\n", __func__,
			appcore_measure_time_from("APP_START_TIME"));
	printf("from create to %s(): %d msec\n", __func__,
			appcore_measure_time());

	if (ad->win)
		elm_win_activate(ad->win);

	ad->flag_launching = EINA_FALSE;

	return 0;
}

int main(int argc, char *argv[])
{
	struct appdata ad;
	struct appcore_ops ops = {
		.create = app_create,
		.terminate = app_terminate,
		.pause = app_pause,
		.resume = app_resume,
		.reset = app_reset,
	};

	/* appcore measure time example */
	printf("from AUL to %s(): %d msec\n", __func__,
			appcore_measure_time_from("APP_START_TIME"));

	memset(&ad, 0x0, sizeof(struct appdata));
	ops.data = &ad;

	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}

