#ifndef MYTIMER_H
#define MYTIMER_H

#include "lvgl.h"

typedef void (* fun_timer)(lv_timer_t *);
typedef struct LABEL_TIMER{
    lv_obj_t * obj;
	lv_timer_t *timer;
	int interval;
	bool timerStatus;
	fun_timer fun;
}LABEL_TIMER;

void labelTimer_once(LABEL_TIMER *timerLabel, char *str);

#endif