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

//#include <X11/Xlib.h>
//#include <X11/extensions/XInput.h>
//#include <X11/extensions/XInput2.h>
//#include <X11/extensions/shape.h>

#include "main.h"
#include "view.h"
#include "control.h"
#include "_util_log.h"

/*struct _x_event_s_info {
	Ecore_Event_Handler *event_outer_touch_handler;
	Eina_Bool is_first_touch;
};
static struct _x_event_s_info x_event_info = {
	.event_outer_touch_handler = NULL,
	.is_first_touch = EINA_TRUE,
};

static Eina_Bool _event_handler_cb(void *data, int type, void *event_info);
static void _x_touch_check(int cur_x, int cur_y);

void volume_x_input_event_shape(Evas_Object *win, Eina_Bool is_warning_visible)
{
	_D("X input event shape");
	Evas_Object *ly = NULL;
	XRectangle rect;

	int x, y, w ,h;
	int tmp_x;
	int tmp_y;
	int tmp_w;
	int tmp_h;

	int current_angle = volume_control_get_current_angle();
	_D("Current angle : %d", current_angle);

	ly = volume_view_outer_layout_get();
	if (!ly) {
		_E("Failed to load edje");
		return;
	}

	edje_object_part_geometry_get(_EDJ(ly), "bg", &x, &y, &w, &h);
	_D("The position of bg x: %d, y: %d, w: %d, h: %d", x, y, w, h);

	if (current_angle == 90) {
		tmp_x = x;
		tmp_y = y;
		tmp_w = w;
		tmp_h = h;

		x = tmp_y;
		y = tmp_x;
		w = tmp_h;
		h = tmp_w;
	}
	else if (current_angle == 270) {
		tmp_x = x;
		tmp_y = y;
		tmp_w = w;
		tmp_h = h;

		x = volume_control_get_viewport_width()-tmp_y-tmp_h;
		y = tmp_x;
		w = tmp_h;
		h = tmp_w;
	}

	rect.x = x;
	rect.y = y;
	rect.height = h;
	rect.width  = w;
	_D("shape x: %d, y: %d, w: %d, h: %d", x, y, w, h);
	XShapeCombineRectangles(ecore_x_display_get(), elm_win_xwindow_get(win), ShapeInput, 0, 0, &rect, 1, ShapeSet, Unsorted);
}

volume_error_e volume_x_input_event_register(void)
{
	if (x_event_info.event_outer_touch_handler == NULL) {
		XIEventMask event_mask;

		event_mask.deviceid = XIAllMasterDevices;
		event_mask.mask_len = XIMaskLen(XI_RawMotion);
		event_mask.mask = calloc(event_mask.mask_len, sizeof(char));
		retvm_if(!event_mask.mask, VOLUME_ERROR_FAIL, "Failed to allocate memory");

		XISetMask(event_mask.mask, XI_RawMotion);
		XISelectEvents(ecore_x_display_get(), ecore_x_window_root_first_get(), &event_mask, 1);

		x_event_info.event_outer_touch_handler = ecore_event_handler_add(ECORE_X_EVENT_GENERIC, (Ecore_Event_Handler_Cb)_event_handler_cb, NULL);

		x_event_info.is_first_touch = EINA_TRUE;
		free(event_mask.mask);
	}

	return VOLUME_ERROR_OK;
}

volume_error_e volume_x_input_event_unregister(void)
{
	retv_if(x_event_info.event_outer_touch_handler == NULL, VOLUME_ERROR_FAIL);

	ecore_event_handler_del(x_event_info.event_outer_touch_handler);
	x_event_info.event_outer_touch_handler = NULL;

	return VOLUME_ERROR_OK;
}

static void _x_touch_check(int cur_x, int cur_y)
{
	int current_angle = volume_control_get_current_angle();

	int x = 0, y = 0;
	int w = 0, h = 0;

	Evas_Object *ly = volume_view_outer_layout_get();

	if (!ly) {
		_E("Failed to load edje");
		return;
	}

	edje_object_part_geometry_get(_EDJ(ly), "bg", &x, &y, &w, &h);
	_D("control view x: %d, y: %d, w: %d, h: %d", x, y, w, h);

	w = x + w; //WIDTH
	h = y + h; //HEIGHT

	if (current_angle == 90) {
		if (cur_x > y && cur_x < h && cur_y > x && cur_y < w)
			_D("touched inside");
		else {
			_D("current angle : %d / touched outside ", current_angle);
			if (VOLUME_ERROR_OK != volume_control_hide_view())
				_E("Failed to close volume");

			if (VOLUME_ERROR_OK != volume_control_cache_flush())
				_E("Failed to flush cache");
		}
	} else if (current_angle == 270) {
		if (cur_x > h && cur_x < (volume_control_get_viewport_width() - y) && cur_y > x && cur_y < w)
			_D("touched inside");
		else {
			_D("current angle : %d / touched outside ", current_angle);
			if (VOLUME_ERROR_OK != volume_control_hide_view())
				_E("Failed to close volume");

			if (VOLUME_ERROR_OK != volume_control_cache_flush())
				_E("Failed to flush cache");
		}
	} else {
		if (x < cur_x && cur_x < w && y < cur_y && cur_y < h)
			_D("current angle : %d / touched inside", current_angle);
		else {
			_D("current angle : %d / touched outside ", current_angle);
			if (VOLUME_ERROR_OK != volume_control_hide_view())
				_E("Failed to close volume");

			if (VOLUME_ERROR_OK != volume_control_cache_flush())
				_E("Failed to flush cache");
		}
	}
}

static Eina_Bool _event_handler_cb(void *data, int type, void *event_info)
{
	_D("%s", __func__);

	Ecore_X_Event_Generic *e = (Ecore_X_Event_Generic *)event_info;
	retv_if(e->evtype != XI_RawMotion, ECORE_CALLBACK_DONE);

	if (x_event_info.is_first_touch == EINA_TRUE) {
		x_event_info.is_first_touch = EINA_FALSE;
		return ECORE_CALLBACK_DONE;
	}
	x_event_info.is_first_touch = EINA_TRUE;

	int cur_x = 0, cur_y = 0;
	ecore_x_pointer_xy_get(ecore_x_window_root_first_get(), &cur_x, &cur_y);
	_D("cur_x : %d / cur_y : %d", cur_x, cur_y);

	_x_touch_check(cur_x, cur_y);

	return ECORE_CALLBACK_DONE;
}
*/
