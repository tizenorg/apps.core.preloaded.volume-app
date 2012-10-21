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


#include <Ecore.h>
#include <Ecore_X.h>

#include "volume.h"
#include "_util_log.h"

Evas_Object *_add_window(const char *name)
{
	Evas_Object *eo = NULL;
	int w = -1, h = -1;
	eo = elm_win_add(NULL, name, ELM_WIN_NOTIFICATION);
	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		elm_win_alpha_set(eo, EINA_TRUE);
		ecore_x_window_size_get(
				ecore_x_window_root_first_get(),
				&w, &h);
		if(w == -1 || h == -1){
			_E("ecore_x_window_seiz_get() is failed\n");
			return NULL;
		}
		evas_object_resize(eo, w, h);
	}
	return eo;
}

Evas_Object *_add_slider(Evas_Object *parent, int min, int max, int val)
{
	Evas_Object *sl = NULL;
	sl = elm_slider_add(parent);
	retvm_if(sl == NULL, NULL, "Failed to add slider\n");
	elm_slider_horizontal_set(sl, EINA_TRUE);
	elm_slider_indicator_show_set(sl, EINA_TRUE);
	elm_slider_indicator_format_set(sl, "%.0f");
	evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
	elm_slider_min_max_set(sl, min, max);
	elm_slider_value_set(sl, val);
	return sl;
}

Evas_Object *_add_layout(Evas_Object *parent, const char *file,
			     const char *group)
{
	Evas_Object *eo = NULL;
	int r = -1;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");
	retvm_if(file == NULL, NULL, "Invalid argument: file is NULL\n");
	retvm_if(group == NULL, NULL, "Invalid argument: group is NULL\n");

	eo = elm_layout_add(parent);
	retvm_if(eo == NULL, NULL, "Failed to add layout\n");

	r = elm_layout_file_set(eo, file, group);
	if (!r) {
		_E("Failed to set file[%s]\n", file);
		evas_object_del(eo);
		return NULL;
	}

	evas_object_size_hint_weight_set(eo,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(eo);
	return eo;
}

Evas_Object *_add_button(Evas_Object *parent, const char *style, const char *text)
{
	Evas_Object *bt = NULL;
	bt = elm_button_add(parent);
	retvm_if(bt == NULL, NULL, "Failed to add button\n");
	if (style) elm_object_style_set(bt, style);
	elm_object_focus_set(bt, EINA_FALSE);
	elm_object_text_set(bt, text);
	evas_object_show(bt);
	return bt;
}

Evas_Object *_add_popup(Evas_Object *parent, const char *style)
{
	Evas_Object *pu = NULL;
	pu = elm_popup_add(parent);
	retvm_if(pu == NULL, NULL, "[Error] Failed to add popup\n");
	evas_object_size_hint_weight_set(pu, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	if (style) elm_object_style_set(pu, style);
	evas_object_show(pu);
	return pu;
}
Evas_Object *_add_label(Evas_Object *parent, const char *style, const char *text)
{
	Evas_Object *lb = NULL;
	lb = elm_label_add(parent);
	retvm_if(lb == NULL, NULL, "Failed to add label\n");
	elm_object_style_set(lb, style);
	evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_label_line_wrap_set(lb, ELM_WRAP_MIXED);
	elm_object_text_set(lb, text);
	return lb;
}

