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


#include <ui-gadget.h>

#include "_util_log.h"
#include "volume.h"
#include "_sound.h"
#include "_logic.h"

static void button_ug_layout_cb(ui_gadget_h ug,
		enum ug_mode mode, void *priv)
{
	Evas_Object *base = NULL;
	Evas_Object *win = NULL;

	base = ug_get_layout(ug);
	win = ug_get_window();

	retm_if(ug == NULL, "ug_get_layout API is failed\n");
	retm_if(ug == NULL, "ug_get_window API is failed\n");

	switch (mode) {
		case UG_MODE_FULLVIEW:
			evas_object_size_hint_weight_set(base,
					EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_win_resize_object_add(win, base);
			evas_object_show(base);
			break;
		default:
			break;
	}
}

static void button_ug_destroy_cb(ui_gadget_h ug, void *priv)
{
	_D("%s\n", __func__);
	struct appdata *ad = (struct appdata *)priv;
	retm_if(ug == NULL, "Invalid argument: ug is NULL\n");

	ug_destroy(ug);
	ad->ug = NULL;

	ecore_x_netwm_window_type_set(elm_win_xwindow_get(ad->win), ECORE_X_WINDOW_TYPE_NOTIFICATION);
	utilx_set_window_opaque_state(ecore_x_display_get(), elm_win_xwindow_get(ad->win), UTILX_OPAQUE_STATE_OFF);
	_close_volume(ad);
}

ui_gadget_h create_button_ug(void *data)
{
	ui_gadget_h ug = NULL;
	struct ug_cbs cbs = {0};
	struct appdata *ad = (struct appdata *)data;
	retvm_if(ad == NULL, 0, "Invalid argument:appdata is NULL\n");

	cbs.layout_cb = button_ug_layout_cb;
	cbs.destroy_cb = button_ug_destroy_cb;
	cbs.priv = (void *)data;

	ecore_x_netwm_window_type_set(elm_win_xwindow_get(ad->win), ECORE_X_WINDOW_TYPE_NORMAL);
	utilx_set_window_opaque_state(ecore_x_display_get(), elm_win_xwindow_get(ad->win), UTILX_OPAQUE_STATE_ON);
	ug = ug_create(NULL, "setting-profile-efl", UG_MODE_FULLVIEW, NULL, &cbs);

	return ug;
}

int _open_ug(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	ui_gadget_h ug= NULL;

	retvm_if(ad == NULL, -1, "Invalid argument: appdata is NULL\n");
	retvm_if(ad->win == NULL, -1, "Invalid argument: window is NULL\n");

	UG_INIT_EFL(ad->win, UG_OPT_INDICATOR_ENABLE);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	ug = create_button_ug(ad);

	retvm_if(ug == NULL, -1, "Failed to Create ug!\n");

	ad->ug = ug;

	return 0;
}

