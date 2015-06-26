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

#include <feedback.h>
#include <vconf.h>
#include <app_control_internal.h>
#include <app_preference.h>
#include <Elementary.h>
#include <appcore-efl.h>
#include <app.h>

#include "main.h"
#include "_util_log.h"
#include "_util_efl.h"
#include "view.h"
#include "control.h"

static bool _create_app(void *user_data);
static void _terminate_app(void *user_data);
static void _pause_app(void *user_data);
static void _resume_app(void *user_data);
static void _control_app(app_control_h service, void *user_data);
static void _control_app(app_control_h service, void *user_data);
static void _changed_language(app_event_info_h event_info, void *user_data);

int main(int argc, char *argv[])
{
	_D(">>>>>>>>>>>>>>>>>>>>>>>>>> main >>>>>>>>>>>>>>>>");
	int ret = 0;

	ui_app_lifecycle_callback_s lifecycle_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	lifecycle_callback.create = _create_app;
	lifecycle_callback.terminate = _terminate_app;
	lifecycle_callback.pause = _pause_app;
	lifecycle_callback.resume = _resume_app;
	lifecycle_callback.app_control = _control_app;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, NULL, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, NULL, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, NULL, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, _changed_language, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, NULL, NULL);

	ret = ui_app_main(argc, argv, &lifecycle_callback, NULL);
	if (ret != APP_ERROR_NONE)
		_E("app_main() is failed. err = %d", ret);

	return ret;
}

static bool _create_app(void *user_data)
{
	_D(">>>>>>>>>>>>>>>>>>>>>>>>> create app >>>>>>>>>>>>>>>>>>>>");
	elm_app_base_scale_set(1.8);
	/* Initialize feedback */
	//feedback_initialize();

	/* Initialize volume */
	if (VOLUME_ERROR_OK != volume_control_initialize()) {
		_E("Failed to initialize volume");
		return false;
	}

	return true;
}

static void _terminate_app(void *user_data)
{
	/* Deinitialize feedback */
	feedback_deinitialize();

	/* Deinitialize volume */
	volume_control_deinitialize();
}

static void _pause_app(void *user_data)
{
	if(VOLUME_ERROR_OK != volume_control_pause())
		_E("Failed to pause volume");
}

static void _resume_app(void *user_data)
{
}

static void _control_app(app_control_h service, void *user_data)
{
	bundle *b = NULL;
	app_control_to_bundle(service, &b);

	if(VOLUME_ERROR_OK != volume_control_reset(b)) {
		_E("Failed to reset volume");
		return;
	}

	Evas_Object *win = volume_view_win_get();
	if(win)
		elm_win_activate(win);
}

static void _changed_language(app_event_info_h event_info, void *user_data)
{
	_D("language changed");
	char *locale = vconf_get_str(VCONFKEY_LANGSET);
	if (locale)
		elm_language_set(locale);
	free(locale);
}

