/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __VOLUME_TIMER_H__
#define __VOLUME_TIMER_H__

#define DEL_TIMER(x) \
	if (x) {\
		_D("DELTIMER x : %p\n", x);\
		ecore_timer_del(x);\
		x = NULL;\
	}
#define ADD_TIMER(x, time, _timer_cb, data) \
	if(x != NULL) DEL_TIMER(x); \
	x = ecore_timer_add(time, _timer_cb, data);\
	_D("ADDTIMER x : %p\n", x);\

typedef enum {
	TYPE_TIMER_POPUP = 0,
	TYPE_TIMER_SLIDER,
	TYPE_TIMER_SD,
	TYPE_TIMER_SU,
	TYPE_TIMER_BT
} volume_timer_type;

extern Ecore_Timer *volume_timer_popup_timer_get(void);
extern Ecore_Timer *volume_timer_slider_timer_get(void);
extern Ecore_Timer *volume_timer_su_timer_get(void);
extern Ecore_Timer *volume_timer_sd_timer_get(void);

extern Eina_Bool volume_timer_popup_cb(void *data);
extern Eina_Bool volume_timer_slider_timer_cb(void *data);

extern void volume_timer_add(double time, volume_timer_type type);
extern void volume_timer_del(volume_timer_type);

#endif /* __VOLUME_TIMER_H__ */
