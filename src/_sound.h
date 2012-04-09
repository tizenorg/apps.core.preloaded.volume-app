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

#ifndef __VOLUME_SOUND_H__
#define __VOLUME_SOUND_H__

#include <mm_sound.h>

int _init_svi();
int _init_mm_sound(void *data);
int _get_volume_type_max(void);
int _get_title(volume_type_t type, char *label, int size);
int _get_sound_level(volume_type_t type, int *val);
double _get_angle_with_value(int val, int step);
int _get_step(int type);
void _play_vib(int handle);
void _play_sound(int type, int handle);

#endif
/* __VOLUME_SOUND_H__ */
