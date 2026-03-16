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
#include "ui/generated/OTA/WIFIConnect.h"
#include "lvgl_system.h"
#include "common.h"
#include "storageDataApi.h"
#ifdef ENABLE_CARPLAY
#include "carplay_display.h"
#include "zlink_client.h"
#endif
#include "myTimer.h"

static pthread_t threadID;
lv_ui guider_ui;
static int screanWidth = 1440;
static int screanHeight = 720;
static int i2c0_fd = -1;
static int i2c1_fd = -1;

lv_timer_t *DVRstaTimer = NULL;


extern sys_data g_sys_Data;
extern LABEL_TIMER *screen_DVR_timer_Label;

extern void recorder_status_timer(lv_timer_t *timer);
extern int get_BT_connect_state(void);

#if 1

static void detected_TF_FreeMem_Timer(lv_timer_t *timer){
    CheckDiskInfoDeleteVideoFile();
}

static bool carplay_connected = false;
static bool androidauto_connected = false;

static void bt_status_check_timer(lv_timer_t *timer) { 
    lv_obj_t* current_screen = lv_scr_act();
    static bool recorderFlag = false;
    g_sys_Data.frontCamera = tp2804_check_camera_connected(i2c0_fd);
    g_sys_Data.rearCamera = tp2804_check_camera_connected(i2c1_fd);

    if(g_sys_Data.TFmounted != checkTFCardMountProc()){
        g_sys_Data.TFmounted = checkTFCardMountProc();
        if(g_sys_Data.TFmounted){
            if(is_popup_visible_v8) close_popup_v8();
            show_popup_simple_v8(get_string_for_language(g_sys_Data.current_language,"dvr_txt_mountTF"));
        }else{
            if(is_popup_visible_v8) close_popup_v8();
            show_popup_simple_v8(get_string_for_language(g_sys_Data.current_language,"dvr_txt_moveTF"));
        }
        

        recorderFlag = true;
        if(current_screen == guider_ui.screen && lv_obj_is_valid(guider_ui.screen_img_TF)){
            g_sys_Data.TFmounted ? lv_obj_clear_flag(guider_ui.screen_img_TF, LV_OBJ_FLAG_HIDDEN) : lv_obj_add_flag(guider_ui.screen_img_TF, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 主界面 WiFi 图标：根据缓存状态显示/隐藏（仅读缓存，不阻塞；主界面重建后也会同步） */
    if (current_screen == guider_ui.screen && lv_obj_is_valid(guider_ui.screen_img_wifi)) {
        bool want_show = WIFIConnect_is_connected_cached();
        bool is_hidden = lv_obj_has_flag(guider_ui.screen_img_wifi, LV_OBJ_FLAG_HIDDEN);
        if (want_show && is_hidden) {
            lv_obj_clear_flag(guider_ui.screen_img_wifi, LV_OBJ_FLAG_HIDDEN);
        } else if (!want_show && !is_hidden) {
            lv_obj_add_flag(guider_ui.screen_img_wifi, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 主界面 BT 图标：根据蓝牙连接状态显示/隐藏（仅读 get_BT_connect_state，不阻塞；主界面重建后也会同步） */
    if (current_screen == guider_ui.screen && lv_obj_is_valid(guider_ui.screen_img_bt)) {
        bool want_show = (get_BT_connect_state() != 0);
        bool is_hidden = lv_obj_has_flag(guider_ui.screen_img_bt, LV_OBJ_FLAG_HIDDEN);
        if (want_show && is_hidden) {
            lv_obj_clear_flag(guider_ui.screen_img_bt, LV_OBJ_FLAG_HIDDEN);
        } else if (!want_show && !is_hidden) {
            lv_obj_add_flag(guider_ui.screen_img_bt, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 主界面 carplay 图标：根据蓝牙连接状态显示/隐藏（仅读 carplay_connected，不阻塞；主界面重建后也会同步） */
    if (current_screen == guider_ui.screen && lv_obj_is_valid(guider_ui.screen_img_carPLay)) {
        bool is_hidden = lv_obj_has_flag(guider_ui.screen_img_carPLay, LV_OBJ_FLAG_HIDDEN);
        if (carplay_connected && is_hidden) {
            lv_obj_clear_flag(guider_ui.screen_img_carPLay, LV_OBJ_FLAG_HIDDEN);
        } else if (!carplay_connected && !is_hidden) {
            lv_obj_add_flag(guider_ui.screen_img_carPLay, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 主界面 android auto 图标：根据蓝牙连接状态显示/隐藏（仅读 androidauto_connected，不阻塞；主界面重建后也会同步） */
    if (current_screen == guider_ui.screen && lv_obj_is_valid(guider_ui.screen_img_carPLay)) {
        bool is_hidden = lv_obj_has_flag(guider_ui.screen_img_androiAuto, LV_OBJ_FLAG_HIDDEN);
        if (androidauto_connected && is_hidden) {
            lv_obj_clear_flag(guider_ui.screen_img_androiAuto, LV_OBJ_FLAG_HIDDEN);
        } else if (!androidauto_connected && !is_hidden) {
            lv_obj_add_flag(guider_ui.screen_img_androiAuto, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if(!g_sys_Data.TFmounted){
        recorderFlag = true;
        if(g_sys_Data.recorderMode == RECORDER_NORMAL || g_sys_Data.recorderMode == RECORDER_URGENT){

            if(current_screen == guider_ui.screen_DVR && lv_obj_is_valid(guider_ui.screen_DVR_btn_recorder)){
                lv_obj_clear_state(guider_ui.screen_DVR_btn_recorder, LV_STATE_CHECKED);

                labelTimer_once(screen_DVR_timer_Label, "dvr_txt_stopRecorder");
            }
            stopVideoRecording(&g_sys_Data.vipp0_config);
            stopVideoRecording(&g_sys_Data.vipp8_config);

            g_sys_Data.recorderMode = RECORDER_NONE;
            if(DVRstaTimer != NULL){
                lv_timer_del(DVRstaTimer);
                DVRstaTimer = NULL;	
            }
		
            clearRecorderStatu();
        } 

    }else{
        if(g_sys_Data.frontCamera || g_sys_Data.rearCamera) {
            if(recorderFlag){
                recorderFlag = false;
                if(g_sys_Data.powerOnRecorder && g_sys_Data.recorderMode == RECORDER_NONE){
                    g_sys_Data.recorderMode = RECORDER_NORMAL;

                    if(lv_obj_is_valid(guider_ui.screen_DVR_btn_recorder)){
                        lv_obj_add_state(guider_ui.screen_DVR_btn_recorder, LV_STATE_CHECKED);

                        labelTimer_once(screen_DVR_timer_Label, "dvr_txt_startRecorder");
                        // resetOrSetupTimer(screen_DVR_timer_Label);
                    }

                    printf("start to recording!!!!!\n");
                    CreateMsgQueueThread(&g_sys_Data.vipp0_config);	
                    createAIChn(&g_sys_Data.vipp0_config, 0, 0);
                    createAencChn(&g_sys_Data.vipp0_config);		
                    createVencChn(&g_sys_Data.vipp0_config);
                    createMuxChn(&g_sys_Data.vipp0_config);
                    prepare(&g_sys_Data.vipp0_config);
                    startVideoRecording(&g_sys_Data.vipp0_config);

                    CreateMsgQueueThread(&g_sys_Data.vipp8_config);
                    createAIChn(&g_sys_Data.vipp8_config, 0, 0);
                    createAencChn(&g_sys_Data.vipp8_config);	
                    createVencChn(&g_sys_Data.vipp8_config);
                    createMuxChn(&g_sys_Data.vipp8_config);
                    prepare(&g_sys_Data.vipp8_config);
                    startVideoRecording(&g_sys_Data.vipp8_config);

                    dashSpeedMark(&g_sys_Data.vipp0_config, g_sys_Data.TimeMark);
                    dashSpeedMark(&g_sys_Data.vipp8_config, g_sys_Data.TimeMark);

            
                    SoundRecording(&g_sys_Data.vipp0_config, g_sys_Data.SoundRecorder);
                    SoundRecording(&g_sys_Data.vipp8_config, g_sys_Data.SoundRecorder);

                    if(DVRstaTimer == NULL) DVRstaTimer = lv_timer_create(recorder_status_timer, 1000, NULL);    
                }
            }       
        }else{
            recorderFlag = true;
            if(g_sys_Data.recorderMode == RECORDER_NORMAL || g_sys_Data.recorderMode == RECORDER_URGENT){

                if(current_screen == guider_ui.screen_DVR && lv_obj_is_valid(guider_ui.screen_DVR_btn_recorder)){
                    lv_obj_clear_state(guider_ui.screen_DVR_btn_recorder, LV_STATE_CHECKED);

                    labelTimer_once(screen_DVR_timer_Label, "dvr_txt_stopRecorder");
                    // resetOrSetupTimer(screen_DVR_timer_Label);
                }
                stopVideoRecording(&g_sys_Data.vipp0_config);
                stopVideoRecording(&g_sys_Data.vipp8_config);
                g_sys_Data.recorderMode = RECORDER_NONE;
                if(DVRstaTimer != NULL){
                    lv_timer_del(DVRstaTimer);
                    DVRstaTimer = NULL;	
                }			
                clearRecorderStatu();
            }
            if(current_screen == guider_ui.screen_DVR && lv_obj_is_valid(guider_ui.screen_DVR_img_rec)){
//                ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_DVR_del, setup_scr_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true);
                stopPreview(&g_sys_Data.vipp0_config);
                stopPreview(&g_sys_Data.vipp8_config);		
                g_sys_Data.previewMode = PREVIEW_NONE; 
            }             
        }
    }

    if(g_sys_Data.recorderMode == RECORDER_NONE){
        if(g_sys_Data.frontCamera || g_sys_Data.rearCamera){
            if(lv_obj_is_valid(guider_ui.screen_img_rec)){
                if(lv_obj_has_flag(guider_ui.screen_img_rec, LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(guider_ui.screen_img_rec, LV_OBJ_FLAG_HIDDEN);

            }
        }else{
            if(lv_obj_is_valid(guider_ui.screen_img_rec)){
                if(!lv_obj_has_flag(guider_ui.screen_img_rec, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(guider_ui.screen_img_rec, LV_OBJ_FLAG_HIDDEN);           
            }

        }
    }

    //home 显示dvr录像状态
    if(lv_obj_is_valid(guider_ui.screen_btn_DVR_label_status)){
        if(g_sys_Data.recorderMode == RECORDER_NORMAL || g_sys_Data.recorderMode == RECORDER_URGENT){
            if(lv_obj_has_flag(guider_ui.screen_btn_DVR_label_status, LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(guider_ui.screen_btn_DVR_label_status, LV_OBJ_FLAG_HIDDEN);
        }else{
            if(!lv_obj_has_flag(guider_ui.screen_btn_DVR_label_status, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(guider_ui.screen_btn_DVR_label_status, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
#endif

// 添加静态变量跟踪触摸状态
static bool touch_pressed = false;
static int last_x = -1, last_y = -1;

// 定义触摸阈值（可根据实际体验调整）
#define TOUCH_MOVE_THRESHOLD 7  // 移动阈值（像素）
#define TOUCH_MOVE_THRESHOLD_SQ (TOUCH_MOVE_THRESHOLD * TOUCH_MOVE_THRESHOLD)

void lv_touch_feedback_cb(lv_indev_drv_t * drv, uint8_t event){
    lv_point_t point;
    lv_obj_t* current_screen = lv_scr_act();
    LinkType type;
    
    // 获取当前屏幕类型
    if(current_screen == guider_ui.screen_androidAuto){
        type = LINK_TYPE_ANDROIDAUTO;
    }else if(current_screen == guider_ui.screen_carPlay){
        type = LINK_TYPE_CARPLAY;
    }else{
        return;  // 不是投屏界面，不处理触摸事件
    }
    
    // 获取当前触摸点坐标
    lv_indev_get_point(lv_indev_get_act(), &point);
    
    switch (event){
    case LV_EVENT_PRESSED:    
        /* 首次按下 - 发送按下事件 */
        if (!touch_pressed) {
            request_link_touchevent(type, true, point.x, point.y);
            touch_pressed = true;
            last_x = point.x;
            last_y = point.y;
            printf("###### DOWN - screen touch point: x=%d, y=%d\n", point.x, point.y);
        }
        break;
        
    case LV_EVENT_PRESSING:
        /* 滑动过程中 - 发送移动事件（带阈值优化） */
        if (touch_pressed) {
            int dx = point.x - last_x;
            int dy = point.y - last_y;
            
            // 使用平方距离判断是否超过移动阈值（避免频繁更新）
            int distance_sq = dx*dx + dy*dy;
            
            if (distance_sq > TOUCH_MOVE_THRESHOLD_SQ) {
                request_link_touchevent(type, true, point.x, point.y);
                last_x = point.x;
                last_y = point.y;
                printf("###### MOVE - screen touch point: x=%d, y=%d (dx=%d, dy=%d, dist=%.1f)\n", 
                    point.x, point.y, dx, dy, sqrt((float)distance_sq));
            }
        }
        break;
        
    case LV_EVENT_RELEASED:
        /* 手指抬起 - 发送抬起事件 */
        if (touch_pressed) {
            // request_link_touchevent(type, false, point.x, point.y);
            request_link_touchevent(type, false, last_x, last_y);
            touch_pressed = false;
            printf("###### UP - screen touch point: x=%d, y=%d\n", point.x, point.y);
        }
        break;
        
    default:
        break;
    }
}

#ifdef ENABLE_CARPLAY
int request_link_touchevent(LinkType type, bool isPressed, int x, int y)
{
	if (type == LINK_TYPE_CARPLAY || type == LINK_TYPE_ANDROIDAUTO)
		carplay_touch_send_xy(x, y, isPressed ? 1 : 0);
	return 0;
}
#else
int request_link_touchevent(LinkType type, bool isPressed, int x, int y)
{
	(void)type;
	(void)isPressed;
	(void)x;
	(void)y;
	return 0;
}
#endif

#ifdef ENABLE_CARPLAY
static void lvgl_refresh_main_link_labels(void)
{
    lv_obj_t *cur = lv_scr_act();
    if (cur != guider_ui.screen)
        return;
    if (!lv_obj_is_valid(guider_ui.screen_btn_carplay_label_statu) ||
        !lv_obj_is_valid(guider_ui.screen_btn_androidauto_label_statu))
        return;

    static int last_session = -1;
    static int last_linktype = -1;
    static language_t last_language = (language_t)-1;
    int session_started = zlink_client_is_session_started();
    int linktype = (int)g_sys_Data.linktype;
    language_t lang = g_sys_Data.current_language;

    if (last_session == session_started && last_linktype == linktype && last_language == lang)
        return;
    last_session = session_started;
    last_linktype = linktype;
    last_language = lang;

    if (session_started && linktype == LINK_TYPE_CARPLAY) {
        lv_label_set_text(guider_ui.screen_btn_carplay_label_statu,
            get_string_for_language(lang, "main_txt_Connect"));
        carplay_connected = true;
    } else {
        lv_label_set_text(guider_ui.screen_btn_carplay_label_statu,
            get_string_for_language(lang, "main_txt_nConnect"));
        carplay_connected = false;
    }

    if (session_started && linktype == LINK_TYPE_ANDROIDAUTO) {
        lv_label_set_text(guider_ui.screen_btn_androidauto_label_statu,
            get_string_for_language(lang, "main_txt_Connect"));
        androidauto_connected = true;
    } else {
        lv_label_set_text(guider_ui.screen_btn_androidauto_label_statu,
            get_string_for_language(lang, "main_txt_nConnect"));
        androidauto_connected = false;
    }
}

static void lvgl_handle_zlink_ui_requests(void)
{
    int link_type = zlink_client_take_pending_home_request();
    if (link_type != 0) {
        if (link_type == LINK_TYPE_CARPLAY) {
            if (lv_scr_act() == guider_ui.screen_carPlay) {
                request_link_action(LINK_TYPE_CARPLAY, LINK_ACTION_VIDEO_CTRL, 0, NULL);
                ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del,
                                      &guider_ui.screen_carPlay_del, setup_scr_screen,
                                      LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true);
                // zlink_client_set_video_dump(0);
            }
        } else if (link_type == LINK_TYPE_ANDROIDAUTO) {
            if (lv_scr_act() == guider_ui.screen_androidAuto) {
                request_link_action(LINK_TYPE_ANDROIDAUTO, LINK_ACTION_VIDEO_CTRL, 0, NULL);
                ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del,
                                      &guider_ui.screen_androidAuto_del, setup_scr_screen,
                                      LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true);
                // zlink_client_set_video_dump(0);
            }
        }
    }
    lvgl_refresh_main_link_labels();
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
    disp_drv.antialiasing =1;
    disp_drv.screen_transp = 1;
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
    indev_drv.feedback_cb = lv_touch_feedback_cb;
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
    tp2804_i2c_init(&i2c0_fd, I2C_BUS0, TP2804_I2C_ADDR);
    tp2804_i2c_init(&i2c1_fd, I2C_BUS1, TP2804_I2C_ADDR);

    setup_ui(&guider_ui);
    custom_init(&guider_ui);
    MPI_init();
    AW_MPI_VDEC_SetVEFreq(MM_INVALID_CHN, 0);

#if 0
    InitMppCameraData(&g_sys_Data.vipp0_config);
    setConfigPara(&g_sys_Data.vipp0_config);
    createViChn(&g_sys_Data.vipp0_config, 0, 0);
    

    InitMppCameraData(&g_sys_Data.vipp8_config);
    setConfigPara(&g_sys_Data.vipp8_config);
    createViChn(&g_sys_Data.vipp8_config, 8, 0);
 #endif   
    lv_timer_create(bt_status_check_timer, 500, NULL);
    lv_timer_create(detected_TF_FreeMem_Timer, 1000*30, NULL);
    WIFIConnect_start_status_poll();  /* 后台轮询 WiFi 状态，主界面图标据此显示/隐藏 */
//--------------------------------------------------------------
    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_task_handler();
#ifdef ENABLE_CARPLAY
        lvgl_handle_zlink_ui_requests();
#endif
        usleep(5000);
    }
    tp2804_i2c_deinit(&i2c0_fd);
    tp2804_i2c_deinit(&i2c1_fd);

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
    g_sys_Data.recorderTime = storageData_getInt("recorderTime", RECORDER_3);
    g_sys_Data.recorderUrgentTime = storageData_getInt("recorderUrgentTime", RECORDER_1);
    g_sys_Data.rearCameraImage = storageData_getBool("rearCameraImage", true);
    g_sys_Data.powerOnRecorder = storageData_getBool("powerOnRecorder", true);
    g_sys_Data.SoundRecorder = storageData_getBool("SoundRecorder", true);
    g_sys_Data.TimeMark = storageData_getBool("TimeMark", true);
    g_sys_Data.SpeedMark = storageData_getBool("SpeedMark", true);
    g_sys_Data.SpeedUnit = storageData_getBool("SpeedUnit", true);
    printf("storage data: backlight:%d,\n \
    splitScreenDisp:%d,\n,\
    language:%d,\n\
    recorderTime:%d\n,\
    recorderUrgentTime:%d\n,\
    carTripSwitch:%d\n"\
    , g_sys_Data.backlight,g_sys_Data.splitScreenDisp,g_sys_Data.current_language,g_sys_Data.recorderTime, g_sys_Data.recorderUrgentTime, g_sys_Data.carTripSwitch);

    SetBackLight(g_sys_Data.backlight);
    CheckDiskInfoDeleteVideoFile();
//------------------------------------------------------------
	int err = pthread_create(&threadID, NULL, LvglMain, NULL);
	if(err != 0)
	{
	    printf("LvglService error!\n");
	    return 0;
	}
	return 0;
}


