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


#include <stdio.h>

#include "volume.h"
#include "_util_log.h"
#include "_util_efl.h"
#include "_logic.h"

struct text_part {
	char *part;
	char *msgid;
};

static int lang_changed(void *data)
{
	return _lang_changed(data);
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
	_app_pause(data);
	return 0;
}

static int app_resume(void *data)
{
	return 0;
}

static int app_reset(bundle *b, void *data)
{
	struct appdata *ad = data;

	if (ad->flag_launching == EINA_TRUE) {
		return 0;
	}
	ad->flag_launching = EINA_TRUE;

	if (syspopup_has_popup(b))
		syspopup_reset(b);
	_app_reset(b, data);

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

