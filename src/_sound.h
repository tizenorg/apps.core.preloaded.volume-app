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


#ifndef __VOLUME_SOUND_H__
#define __VOLUME_SOUND_H__

#include <mm_sound.h>

int _init_svi();
int _init_mm_sound(void *data);
int _get_volume_type_max(void);
int _get_title(volume_type_t type, char *label, int size);
int _get_sound_level(volume_type_t type, int *val);
int _get_step(int type);
void _play_vib(int handle);
void _play_sound(int type, int handle);
void _mm_func(void *data);
int _set_icon(void * data, int val);
int _set_sound_level(volume_type_t type, int val);

#endif
/* __VOLUME_SOUND_H__ */
