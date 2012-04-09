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

#ifndef __VOLUME_H__
#define __VOLUME_H__

#include <Elementary.h>
#include <mm_sound.h>

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

#define D_(str) dgettext("sys_string", str)
#define T_(str) dgettext(PACKAGE, str)

struct appdata
{
	Evas_Object *win, *pu, *ly, *sl;

	Ecore_Timer *ptimer;
	Ecore_Timer *stimer;	/* slider timer */
	Ecore_Timer *sutimer, *sdtimer, *lutimer, *ldtimer;	/* long press */
	volume_type_t type;
	int step;
	int before_mute[32];

	/* add more variables here */
	int sh;	/* svi handle */
	int flag_pressing;	/* to set hard key press */
	int flag_touching;
	int flag_launching;	/* to block double launching by double click */
	int flag_deleting;
	int flag_titleopen;
};

#endif /* __VOLUME_H__ */

