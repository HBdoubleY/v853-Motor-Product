#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/sunxifb.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include "lv_drv_conf.h"
#include "lvgl_main.h"
#include <pthread.h>
#include "ui/generated/gui_guider.h"
#include "ui/generated/events_init.h"
#include "lvgl_system.h"
#include "common.h"
#include "storageDataApi.h"

static pthread_t threadID;
int screanWidth = 1440;
int screanHeight = 720;
extern sys_data g_sys_Data;
lv_ui guider_ui;
#if 0
extern int get_BT_connect_state();
extern char *get_phone_name();


static void bt_status_check_timer(lv_timer_t *timer) { 
    lv_obj_t* current_screen = lv_scr_act();   
    if(current_screen == guider_ui.screen && lv_obj_is_valid(guider_ui.screen_img_carPLay)){
        switch (g_sys_Data.linktype)
        {
        case LINK_TYPE_CARPLAY:
            if(lv_obj_has_flag(guider_ui.screen_img_carPLay, LV_OBJ_FLAG_HIDDEN))
                lv_obj_clear_flag(guider_ui.screen_img_carPLay, LV_OBJ_FLAG_HIDDEN);
            break;
        case LINK_TYPE_ANDROIDAUTO:
            if(lv_obj_has_flag(guider_ui.screen_img_androiAuto, LV_OBJ_FLAG_HIDDEN))
                lv_obj_clear_flag(guider_ui.screen_img_androiAuto, LV_OBJ_FLAG_HIDDEN);
            break;        
        default:
            if(!lv_obj_has_flag(guider_ui.screen_img_carPLay, LV_OBJ_FLAG_HIDDEN))
                lv_obj_add_flag(guider_ui.screen_img_carPLay, LV_OBJ_FLAG_HIDDEN);
            if(!lv_obj_has_flag(guider_ui.screen_img_androiAuto, LV_OBJ_FLAG_HIDDEN))
                lv_obj_add_flag(guider_ui.screen_img_androiAuto, LV_OBJ_FLAG_HIDDEN);               
            break;
        }
        if(checkTFCardMountProc()){
            if(lv_obj_has_flag(guider_ui.screen_img_TF, LV_OBJ_FLAG_HIDDEN)){
                lv_obj_clear_flag(guider_ui.screen_img_TF, LV_OBJ_FLAG_HIDDEN);
            }
        }else{
            if(!lv_obj_has_flag(guider_ui.screen_img_TF, LV_OBJ_FLAG_HIDDEN)){
                lv_obj_add_flag(guider_ui.screen_img_TF, LV_OBJ_FLAG_HIDDEN);
            }        
        }
    }

    switch (g_sys_Data.linktype)
    {
    case LINK_TYPE_ANDROIDAUTO:
        if(lv_obj_is_valid(guider_ui.screen_btn_androidauto_label_statu))
            lv_label_set_text(guider_ui.screen_btn_androidauto_label_statu,get_string_for_language(g_sys_Data.current_language,"main_txt_Connect"));
        break;
    case LINK_TYPE_CARPLAY:
        if(lv_obj_is_valid(guider_ui.screen_btn_carplay_label_statu))
            lv_label_set_text(guider_ui.screen_btn_carplay_label_statu,get_string_for_language(g_sys_Data.current_language,"main_txt_Connect"));
        break;        
    default:
        if(lv_obj_is_valid(guider_ui.screen_btn_androidauto_label_statu))
            lv_label_set_text(guider_ui.screen_btn_androidauto_label_statu,get_string_for_language(g_sys_Data.current_language,"main_txt_nConnect"));
        if(lv_obj_is_valid(guider_ui.screen_btn_carplay_label_statu))
            lv_label_set_text(guider_ui.screen_btn_carplay_label_statu,get_string_for_language(g_sys_Data.current_language,"main_txt_nConnect"));
        break;
    }

    if(get_BT_connect_state()){
        if(lv_obj_is_valid(guider_ui.screen_img_bt)){
            lv_obj_clear_flag(guider_ui.screen_img_bt, LV_OBJ_FLAG_HIDDEN);
        }
        if(lv_obj_is_valid(guider_ui.screen_SET_label_connect)){
            lv_label_set_text(guider_ui.screen_SET_label_connect,get_phone_name());
            lv_obj_invalidate(guider_ui.screen_SET_label_connect);
        }           
    }else{
        if(lv_obj_is_valid(guider_ui.screen_img_bt) ){
            lv_obj_add_flag(guider_ui.screen_img_bt, LV_OBJ_FLAG_HIDDEN);
        }else if(lv_obj_is_valid(guider_ui.screen_SET_label_connect)){
            lv_label_set_text(guider_ui.screen_SET_label_connect,get_string_for_language(g_sys_Data.current_language,"main_txt_nConnect"));
            lv_obj_invalidate(guider_ui.screen_SET_label_connect);
        }  
    }
}

void lv_touch_feedback_cb(lv_indev_drv_t * drv, uint8_t event){
    // printf("-----%s--------%d-----\n",__func__,__LINE__);
    lv_point_t point;
    lv_obj_t* current_screen = lv_scr_act();
    LinkType type;
    if(current_screen == guider_ui.screen_androidAuto){
        type = LINK_TYPE_ANDROIDAUTO;
    }else if(current_screen == guider_ui.screen_carPlay){
        type = LINK_TYPE_CARPLAY;
    }else{
        return;
    }
    switch (event){
    case LV_EVENT_PRESSED:    
    case LV_EVENT_PRESSING:
        /* code */
	    lv_indev_get_point(lv_indev_get_act(), &point);
    	request_link_touchevent(type,true,point.x,point.y);
	    printf("###### down-------screen touch point---x:%d----y:%d\n",point.x,point.y);
        break;
    case LV_EVENT_RELEASED:
    	lv_indev_get_point(lv_indev_get_act(), &point);
        request_link_touchevent(type,false,point.x,point.y);
        printf("######  up-------screen touch point---x:%d----y:%d\n",point.x,point.y);	    
        break;
    default:
        break;
    }
}
#endif
int lvgl_main(int w, int h)
{
    printf("-------%s:%d-----------------\n",__func__,__LINE__);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    // uint32_t rotated = LV_DISP_ROT_NONE;
    uint32_t rotated = LV_DISP_ROT_90;


    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    sunxifb_init(rotated);

    /*A buffer for LittlevGL to draw the screen's content*/
    static uint32_t width, height;
    sunxifb_get_sizes(&width, &height);

    static lv_color_t *buf;
    buf = (lv_color_t*) sunxifb_alloc(width * height * sizeof(lv_color_t),
            "lv_examples");

    if (buf == NULL) {
        sunxifb_exit();
        printf("malloc draw buffer fail\n");
        return 0;
    }

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, width * height);

    /*Initialize and register a display driver*/
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = sunxifb_flush;
    disp_drv.hor_res    = width;
    disp_drv.ver_res    = height;
    disp_drv.rotated    = rotated;
#ifndef USE_SUNXIFB_G2D_ROTATE
    if (rotated != LV_DISP_ROT_NONE)
        disp_drv.sw_rotate = 1;
#endif
    lv_disp_drv_register(&disp_drv);

    evdev_init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);                /*Basic initialization*/
    indev_drv.type =LV_INDEV_TYPE_POINTER;        /*See below.*/
    indev_drv.read_cb = evdev_read;               /*See below.*/
    // indev_drv.feedback_cb = lv_touch_feedback_cb;
    /*Register the driver in LVGL and save the created input device object*/
    lv_indev_t * evdev_indev = lv_indev_drv_register(&indev_drv);

    // lv_demo_widgets();
//    lv_demo_music();
//    lv_demo_benchmark();
//    lv_demo_benchmark();
//    lv_demo_keypad_encoder();
//    lv_demo_keypad_encoder();
//    lv_demo_stress();
//------------------------------------------------
    setup_ui(&guider_ui);
    custom_init(&guider_ui);
    MPI_init();
    AW_MPI_VDEC_SetVEFreq(MM_INVALID_CHN, 0);
    creatVideoPathAndPicPath();


    InitMppCameraData(&g_sys_Data.vipp0_config);
    setConfigPara(&g_sys_Data.vipp0_config);
    createViChn(&g_sys_Data.vipp0_config, 0, 0);

    InitMppCameraData(&g_sys_Data.vipp8_config);
    setConfigPara(&g_sys_Data.vipp8_config);
    createViChn(&g_sys_Data.vipp8_config, 8, 0);
    printf("-------------------------------1212121313---------------------\n");
    // lv_timer_create(bt_status_check_timer, 500, NULL);
//--------------------------------------------------------------
    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_task_handler();
        usleep(5000);
    }

    sunxifb_free((void**) &buf, "lv_examples");
    sunxifb_exit();
    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
int the_tick_to_extern(void){
    return custom_tick_get();
}

void* LvglMain(void *arg) {
   
    lvgl_main(screanWidth, screanHeight);  //while(1)

    return NULL;
}

int LvglService(int w, int h) {
	screanWidth = w;
	screanHeight = h;

//---------------------------上电获取储存数据------------------
    storageData_init(STORAGE_DATA_PATH, true);
    g_sys_Data.backlight = storageData_getInt("backlight", 80);
    g_sys_Data.themeMode = storageData_getInt("themeMode", THEME_DAY);
    g_sys_Data.splitScreenDisp = storageData_getInt("splitScreenDisp", DISPLAY_NORMAL);
    g_sys_Data.current_language = storageData_getInt("current_language", LANG_ENGLISH);
    g_sys_Data.carTripSwitch = storageData_getBool("carTripSwitch", true);
    g_sys_Data.resetFactory = storageData_getBool("resetFactory", true);
    g_sys_Data.carTripSwitch = storageData_getBool("carTripSwitch", true);
    g_sys_Data.recorderTime = storageData_getInt("recorderTime", RECORDER_1);
    g_sys_Data.recorderUrgentTime = storageData_getInt("recorderUrgentTime", RECORDER_3);
    g_sys_Data.rearCameraImage = storageData_getBool("rearCameraImage", false);
    g_sys_Data.powerOnRecorder = storageData_getBool("powerOnRecorder", false);
    g_sys_Data.SoundRecorder = storageData_getBool("SoundRecorder", false);
    g_sys_Data.TimeMark = storageData_getBool("TimeMark", false);
    g_sys_Data.SpeedMark = storageData_getBool("SpeedMark", false);
    printf("storage data: backlight:%d,\n \
    splitScreenDisp:%d,\n,\
    language:%d,\n\
    recorderTime:%d\n,\
    recorderUrgentTime:%d\n,\
    carTripSwitch:%d\n"\
    , g_sys_Data.backlight,g_sys_Data.splitScreenDisp,g_sys_Data.current_language,g_sys_Data.recorderTime, g_sys_Data.recorderUrgentTime, g_sys_Data.carTripSwitch);

    SetBackLight(g_sys_Data.backlight);
//------------------------------------------------------------
	int err = pthread_create(&threadID, NULL, LvglMain, NULL);
	if(err != 0)
	{
	    printf("LvglService error!\n");
	    return 0;
	}
	return 0;
}


