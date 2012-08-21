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


#ifndef __VOLUME_H__
#define __VOLUME_H__

#include <Elementary.h>
#include <mm_sound.h>
#include <ui-gadget.h>
#include <syspopup.h>
#include <appcore-efl.h>
#include <Ecore_X.h>

#if !defined(PACKAGE)
#  define PACKAGE "volume"
#endif

#if !defined(RESDIR)
#  define RESDIR "/opt/apps/org.tizen.volume/res"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR RESDIR"/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR RESDIR"/edje"
#endif

#define EDJ_FILE EDJDIR"/"PACKAGE".edj"
#define GRP_MAIN "main"
#define EDJ_THEME EDJDIR"/volume_popup.edj"
#define GRP_VOLUME_SLIDER "popup_slider_style"
#define GRP_VOLUME_SLIDER_WITH_WARNING "popup_slider_text_style_with_warn_label"

#define S_(str) dgettext("sys_string", str)
#define T_(str) dgettext(PACKAGE, str)

#define ADD_TIMER(x, time, _timer_cb, data) \
	x = ecore_timer_add(time, _timer_cb, data);\

#define DEL_TIMER(x) \
	if (x) {\
		ecore_timer_del(x);\
		x = NULL;\
	}

struct appdata
{
	Evas_Object *win, *pu, *bt, *sl, *ic, *warn_lb, *warn_ly;

	Ecore_Timer *ptimer;
	Ecore_Timer *stimer;	/* slider timer */
	Ecore_Timer *sutimer, *sdtimer, *lutimer, *ldtimer;	/* long press */
	Ecore_Timer *warntimer;	/* warning message timer */
	volume_type_t type;
	int step;
	int before_mute[32];

	/* ug handler */
	ui_gadget_h ug;

	/* add more variables here */
	int sh;	/* svi handle */
	int flag_pressing;	/* to set hard key press */
	int flag_touching;
	int flag_launching;	/* to block double launching by double click */
	int flag_deleting;
	int flag_warning;	/* set device warning flag */
};

#endif /* __VOLUME_H__ */

