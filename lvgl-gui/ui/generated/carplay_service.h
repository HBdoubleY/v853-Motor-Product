#ifndef CARPLAY_SERVICE_H
#define CARPLAY_SERVICE_H

#if defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif
#include "common.h"
#include "hwdisplay.h"

typedef struct view_info VIEW_INFO;
typedef struct src_info SRC_INFO;
//lvgl获取carplay H264数据
int lvgl_carplay_service();
int setCarPlay2De();
void freeCarPlay2De();

#if defined(__cplusplus)||defined(c_plusplus)
}
#endif 

#endif
