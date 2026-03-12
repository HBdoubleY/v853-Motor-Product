#include "myTimer.h"
#include "lv_obj.h"
#include "i18n.h"
#include "lvgl_system.h"

void labelTimer_once(LABEL_TIMER *timerLabel, char *str){
	printf("%s:%d\n",__func__,__LINE__);

    if(lv_obj_is_valid(timerLabel->obj)){
		printf("----111111-----\n");
        lv_label_set_text(timerLabel->obj, get_string_for_language(g_sys_Data.current_language,str));
        lv_obj_clear_flag(timerLabel->obj, LV_OBJ_FLAG_HIDDEN);
    }

	if(timerLabel->timerStatus){
		lv_timer_del(timerLabel->timer);
		timerLabel->timerStatus = false;
		printf("has the timer,delete it!!\n");
	}

	timerLabel->timer = lv_timer_create(timerLabel->fun, timerLabel->interval, timerLabel);
	if (timerLabel->timer) {
        timerLabel->timerStatus = true;
        lv_timer_set_repeat_count(timerLabel->timer, 1);
		printf("register the timer!!\n");
    }
}