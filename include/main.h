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


#ifndef __VOLUME_H__
#define __VOLUME_H__

#include <Elementary.h>
#include <sound_manager.h>
//#include <syspopup.h>
#include <appcore-efl.h>
//#include <Ecore_X.h>
#include <notification.h>
#include <syspopup_caller.h>

#if !defined(PACKAGE)
#  define PACKAGE "volume"
#endif

#if !defined(RESDIR)
#  define RESDIR "/usr/apps/org.tizen.volume/res"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR RESDIR"/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR RESDIR"/edje"
#endif

#if !defined(PKGNAME)
#  define PKGNAME "org.tizen.volume"
#endif

#define EDJ_APP EDJDIR"/volume_app.edj"

#define IMG_VOLUME_ICON "00_volume_icon.png"
#define IMG_VOLUME_ICON_CALL "00_volume_icon_call.png"
#define IMG_VOLUME_ICON_MUTE "00_volume_icon_mute.png"
#define IMG_VOLUME_ICON_VIB "00_volume_icon_vibrat.png"
#define IMG_VOLUME_ICON_NOTI "00_volume_icon_notification.png"
#define IMG_VOLUME_ICON_NOTI_VIB "00_volume_icon_notification_vibrate.png"
#define IMG_VOLUME_ICON_NOTI_MUTE "00_volume_icon_notification_mute.png"
#define IMG_VOLUME_ICON_HEADPHONE "00_volume_icon_headphone.png"
#define IMG_VOLUME_ICON_MEDIA "00_volume_icon_media.png"
#define IMG_VOLUME_ICON_SETTINGS "00_volume_icon_settings.png"
#define IMG_VOLUME_ICON_SETTINGS_PRESSED "00_volume_icon_settings_pressed.png"
#define IMG_VOLUME_ICON_SETTINGS_DISABLED "00_volume_icon_settings_disabled.png"

#define _EDJ(x) elm_layout_edje_get(x)
#define _X(x) (x) / elm_app_base_scale_get() * elm_config_scale_get()

#define S_(str) dgettext("sys_string", str)
#define T_(str) dgettext(PACKAGE, str)
#undef _
#define _(str) gettext(str)

#ifndef VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS
#define VCONFKEY_SETAPPL_ACCESSIBILITY_TURN_OFF_ALL_SOUNDS VCONFKEY_SETAPPL_PREFIX"/accessibility/turn_off_all_sounds"
#endif

/* Fake-BG : bundle */
#define SHOWLOCK "show_lock"
#define ISTRUE "TRUE"

/* For voice control */
#define SHOWVOLUME "show_volume"

#define LOCKSCREEN_PKG "org.tizen.lockscreen"

#define SCREEN_W_HD 720
#define SCREEN_H_HD 1280

#define SCREEN_W_WVGA 480
#define SCREEN_H_WVGA 800

#define SCREEN_W_FHD 1080
#define SCREEN_H_FHD 1920

#define WIN_WIDTH 456
#define WIN_HEIGHT_WITH_WARNING 222
#define WIN_HEIGHT 102

#define WIN_LANDSCAPE_X 79
#define WIN_LANDSCAPE_INVERTED_X volume_control_get_viewport_width()-WIN_HEIGHT_WITH_WARNING-WIN_LANDSCAPE_X
#define WIN_LANDSCAPE_Y (volume_control_get_viewport_height()-WIN_WIDTH)/2
#define WIN_PORTRAIT_X 12
#define WIN_PORTRAIT_Y 113
#define WIN_SLIDER_TOUCHING_OFFSET 55

#define SOUND_TYPE_SYSTEM 0
#define SOUND_TYPE_NOTIFICATION 1
#define SOUND_TYPE_ALARM 2
#define SOUND_TYPE_RINGTONE 3
#define SOUND_TYPE_MEDIA 4
#define SOUND_TYPE_CALL 5

#define VOLUME_MAX_SAFETY_VOLUME_LEVEL 10

typedef enum {
	TYPE_VCONF_SOUND_STATUS = 0,
	TYPE_VCONF_VIBRATION_STATUS
} volume_vconf_type_e;

/* Volume : return values */
typedef enum {
	VOLUME_ERROR_OK = 0,
	VOLUME_ERROR_FAIL = -1,
	VOLUME_ERROR_INVALID_PARAMETER = -2,
	VOLUME_ERROR_NO_DATA = -3
} volume_error_e;

#endif /* __VOLUME_H__ */

