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

