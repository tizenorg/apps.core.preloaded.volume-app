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

#ifndef __VOLUME_LOGIC_H__
#define __VOLUME_LOGIC_H__

#include "volume.h"
int _app_create(struct appdata *ad);
int _app_pause(struct appdata *ad);
int _rotate_knob(Evas_Object *kn, Evas_Coord cx, Evas_Coord cy, double angle);

#endif
/* __VOLUME_LOGIC_H__ */
