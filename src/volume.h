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
#include <notification.h>

#if !defined(PACKAGE)
#  define PACKAGE "volume"
#endif

#if !defined(RESDIR)
#  define RESDIR "/usr/apps/org.samsung.volume/res"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR RESDIR"/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR RESDIR"/edje"
#endif

#if !defined(PKGNAME)
#  define PKGNAME "com.samsung.volume"
#endif

#define EDJ_APP EDJDIR"/volume_app.edj"
#define GRP_VOLUME_BLOCKEVENTS "block_events"
#define GRP_VOLUME_LAYOUT "volumeLayout"
#define GRP_VOLUME_CONTENT "volumeLayoutContent"
#define GRP_VOLUME_SLIDER_HORIZONTAL "volumeHorizontalSlider"

#define IMG_VOLUME_PACKAGE_ICON "/usr/share/icons/default/small/com.samsung.volume.png"
#define IMG_VOLUME_ICON "00_volume_icon.png"
#define IMG_VOLUME_ICON_CALL "00_volume_icon_Call.png"
#define IMG_VOLUME_ICON_MUTE "00_volume_icon_Mute.png"
#define IMG_VOLUME_ICON_VIB "00_volume_icon_Vibrat.png"
#define IMG_VOLUME_ICON_HEADPHONE "00_volume_icon_headphone.png"
#define IMG_VOLUME_ICON_MEDIA "00_volume_icon_media.png"
#define IMG_VOLUME_ICON_SETTINGS "00_volume_icon_settings.png"
#define IMG_VOLUME_ICON_SETTINGS_PRESSED "00_volume_icon_settings_pressed.png"

#define IDS_WARNING_MSG "IDS_COM_BODY_HIGH_VOLUMES_MAY_HARM_YOUR_HEARING_IF_YOU_LISTEN_FOR_A_LONG_TIME"
#define STR_WARNING_MSG "High volumes may harm your hearing if you listen for a long time"

#define IDS_MEDIA_MSG "IDS_COM_BODY_MEDIA"
#define STR_MEDIA_MSG "Media"

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
	Evas_Object *win, *block_events, *ly, *sl, *ic, *ic_settings, *warn_lb;

	Ecore_Timer *ptimer;
	Ecore_Timer *stimer;	/* slider timer */
	Ecore_Timer *sutimer, *sdtimer, *lutimer, *ldtimer;	/* long press */
	Ecore_Timer *warntimer;	/* warning message timer */
	Ecore_Timer *luwarmtimer, *ldwarmtimer;
	volume_type_t type;
	system_audio_route_device_t device;
	int step;
	int angle;

	/* ticker notification handler */
	int noti_id;
	bool noti_seen;

	/* ug handler */
	ui_gadget_h ug;

	/* add more variables here */
	int sh;	/* svi handle */
	int flag_pressing;	/* to set hard key press */
	int flag_touching;
	int flag_launching;
	int flag_deleting;
};

#endif /* __VOLUME_H__ */

