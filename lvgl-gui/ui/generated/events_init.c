/*
* Copyright 2025 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include "pthread.h"
#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif
#include "hwdisplay.h"
#include "lv_obj_draw.h"
#include "Rtc2C.h"
#include "media_main_interface.h"
#include "ota_main_interface.h"
#include "storageDataApi.h"

extern void listStatuSwitch(language_t language);
extern void mianScreenLanguageUpdata(language_t language);
extern void languageItemStatu(language_t language);
typedef void (* fun_timer)(lv_timer_t *);
typedef struct LABEL_TIMER{
	lv_timer_t *timer;
	int interval;
	bool timerStatus;
	fun_timer fun;
}LABEL_TIMER;
// typedef void (* fun_timer)(lv_timer_t *);

LABEL_TIMER *screen_DVR_timer_Label;
LABEL_TIMER *screen_timer_popupLabel;
LABEL_TIMER *screen_timer_lockScreenBtn;
LABEL_TIMER *screen_SET_timer_popupLabel;

static void resetOrSetupTimer(LABEL_TIMER *timerLabel){
	printf("%s:%d\n",__func__,__LINE__);
	if(timerLabel->timerStatus){
		lv_timer_del(timerLabel->timer);
		timerLabel->timerStatus = 0;
		printf("has the label timer,delete it!!\n");
	}

	timerLabel->timer = lv_timer_create(timerLabel->fun, timerLabel->interval, timerLabel);
	if (timerLabel->timer) {
        timerLabel->timerStatus = 1;
        lv_timer_set_repeat_count(timerLabel->timer, 1);
		printf("register the label timer!!\n");
    }
}

typedef enum {
	CONT_TONGYONG = 0x0,
	CONT_LANYA,
	CONT_SYS,
	CONT_LANGUAGE,
	CONT_SETTIME,
	CONT_ABOUT,
	CONT_BOOTLOGO,
	CONT_REBOOT,
	CONT_RESETFACTORY,
} SYS_PAGE_MANAGE;

static SYS_PAGE_MANAGE sysPageManage = CONT_TONGYONG; 
static void sysPageManageFun(SYS_PAGE_MANAGE page) {
	switch(page) {
		case CONT_TONGYONG:
			lv_obj_add_flag(guider_ui.screen_SET_cont_tongyong, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_LANYA:
			lv_obj_add_flag(guider_ui.screen_SET_cont_lanya, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_SYS:
			lv_obj_add_flag(guider_ui.screen_SET_cont_sys, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_LANGUAGE:
			lv_obj_add_flag(guider_ui.screen_SET_cont_setlanguage, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_SETTIME:
			lv_obj_add_flag(guider_ui.screen_SET_cont_settime, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_ABOUT:
			lv_obj_add_flag(guider_ui.screen_SET_cont_guanyu, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_BOOTLOGO:
			lv_obj_add_flag(guider_ui.screen_SET_cont_bootlogo, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_REBOOT:
			lv_obj_add_flag(guider_ui.screen_SET_cont_sys, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(guider_ui.screen_SET_cont_reboot, LV_OBJ_FLAG_HIDDEN);
		break;
		case CONT_RESETFACTORY:
			lv_obj_add_flag(guider_ui.screen_SET_cont_sys, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(guider_ui.screen_SET_cont_resetFactory, LV_OBJ_FLAG_HIDDEN);
		break;
	}
}

static void screen_btn_cartrip_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
	lv_area_t coords;
	lv_point_t point;	
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(!g_sys_Data.carTripSwitch)
			return;
		lv_obj_get_coords(guider_ui.screen_btn_cartrip, &coords);
		lv_indev_get_point(lv_indev_get_act(), &point);
		if(point.x < coords.x1 || point.y < coords.y1 || point.x > coords.x2 || point.y > coords.y2){
			return;
		}

		if(g_sys_Data.lockScreenFlag){
			resetOrSetupTimer(screen_timer_popupLabel);
			break;
		}
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_Tire, guider_ui.screen_Tire_del, &guider_ui.screen_del, setup_scr_screen_Tire, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		break;
	}
    default:
        break;
    }
}

static void screen_btn_DVR_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
	lv_area_t coords;
	lv_point_t point;
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		lv_obj_get_coords(guider_ui.screen_btn_DVR, &coords);
		lv_indev_get_point(lv_indev_get_act(), &point);
		if(point.x < coords.x1 || point.y < coords.y1 || point.x > coords.x2 || point.y > coords.y2){
			return;
		}
		if(g_sys_Data.lockScreenFlag){
			resetOrSetupTimer(screen_timer_popupLabel);
			break;
		}	
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_DVR, guider_ui.screen_DVR_del, &guider_ui.screen_del, setup_scr_screen_DVR, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		break;
	}
    default:
        break;
    }
}

static void screen_btn_set_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
	lv_area_t coords;
	lv_point_t point;
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		lv_obj_get_coords(guider_ui.screen_btn_set, &coords);
		lv_indev_get_point(lv_indev_get_act(), &point);
		if(point.x < coords.x1 || point.y < coords.y1 || point.x > coords.x2 || point.y > coords.y2){
			return;
		}
		printf("-----LV_EVENT_RELEASED---\n");
		if(g_sys_Data.lockScreenFlag){
			resetOrSetupTimer(screen_timer_popupLabel);
			break;
		}
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_SET, guider_ui.screen_SET_del, &guider_ui.screen_del, setup_scr_screen_SET, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		lv_obj_add_state(guider_ui.screen_SET_list_set_item0,LV_STATE_PRESSED);
		lv_obj_add_state(guider_ui.screen_SET_list_set_item1,LV_STATE_DEFAULT);
		lv_obj_add_state(guider_ui.screen_SET_list_set_item2,LV_STATE_DEFAULT);
		lv_obj_clear_flag(guider_ui.screen_SET_cont_tongyong,LV_OBJ_FLAG_HIDDEN);
		break;
	}
    default:
        break;
    }
}


#if 0
static void screen_btn_carplay_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
	lv_area_t coords;
	lv_point_t point;
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		lv_obj_get_coords(guider_ui.screen_btn_carplay, &coords);
		lv_indev_get_point(lv_indev_get_act(), &point);
		if(point.x < coords.x1 || point.y < coords.y1 || point.x > coords.x2 || point.y > coords.y2){
			return;
		}
		printf("-----LV_EVENT_RELEASED---\n");
		if(g_sys_Data.lockScreenFlag){
			resetOrSetupTimer(screen_timer_popupLabel);
			break;
		}
		if(g_sys_Data.linktype == LINK_TYPE_ANDROIDAUTO){
			lv_label_set_text(guider_ui.screen_label_Popup, get_string_for_language(g_sys_Data.current_language,"sys_txt_invalid"));
			lv_obj_clear_flag(guider_ui.screen_label_Popup, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_timer_popupLabel);
			return;
		}	
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_carPlay, guider_ui.screen_carPlay_del, &guider_ui.screen_del, setup_scr_screen_carPlay, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		if(g_sys_Data.linktype == LINK_TYPE_CARPLAY){
			setCarPlay2De();
			request_link_action(LINK_TYPE_CARPLAY, LINK_ACTION_VIDEO_CTRL, 1, NULL);
		}
		break;
	}
    default:
        break;
    }
}

static void screen_btn_androidauto_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	lv_area_t coords;
	lv_point_t point;	
	switch (code)
	{
	case LV_EVENT_CLICKED:
		lv_obj_get_coords(guider_ui.screen_btn_androidauto, &coords);
		lv_indev_get_point(lv_indev_get_act(), &point);
		if(point.x < coords.x1 || point.y < coords.y1 || point.x > coords.x2 || point.y > coords.y2){
			return;
		}
		if(g_sys_Data.lockScreenFlag){
			resetOrSetupTimer(screen_timer_popupLabel);
			break;
		}
		if(g_sys_Data.linktype == LINK_TYPE_CARPLAY){
			lv_label_set_text(guider_ui.screen_label_Popup, get_string_for_language(g_sys_Data.current_language,"sys_txt_invalid"));
			lv_obj_clear_flag(guider_ui.screen_label_Popup, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_timer_popupLabel);
			return;
		}	
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_androidAuto, guider_ui.screen_androidAuto_del, &guider_ui.screen_del, setup_scr_screen_androidAuto, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		if(g_sys_Data.linktype == LINK_TYPE_ANDROIDAUTO){
			setCarPlay2De();
			request_link_action(LINK_TYPE_ANDROIDAUTO, LINK_ACTION_VIDEO_CTRL, 1, NULL);
		}
		/* code */
		break;
	
	default:
		break;
	}
}
#endif
static int language = LANG_ENGLISH;
static void screen_list_language_itemEN_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_ENGLISH)
			break;
		language = LANG_ENGLISH;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemCN_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_CHINESE)
			break;
		language = LANG_CHINESE;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemCN_FT_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_CHINESE_FT)
			break;
		language = LANG_CHINESE_FT;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemFR_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_FRANCE)
			break;
		language = LANG_FRANCE;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemGE_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_GERMANY)
			break;
		language = LANG_GERMANY;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemIT_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_ITALY)
			break;
		language = LANG_ITALY;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemJA_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_JAPAN)
			break;
		language = LANG_JAPAN;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemPOR_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_PORTUGUESE)
			break;
		language = LANG_PORTUGUESE;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemRU_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_RUSSIAN)
			break;
		language = LANG_RUSSIAN;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemSP_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_SPANISH)
			break;
		language = LANG_SPANISH;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemPOL_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_POLAND)
			break;
		language = LANG_POLAND;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemTU_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_TURKISH)
			break;
		language = LANG_TURKISH;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemKO_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_KOREAN)
			break;
		language = LANG_KOREAN;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_list_language_itemDU_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(language == LANG_DUTCH)
			break;
		language = LANG_DUTCH;
		listStatuSwitch(language);
		mianScreenLanguageUpdata(language);
		break;
	}
    default:
        break;
    }
}

static void screen_btn_confirm_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		g_sys_Data.current_language = language;
		storageData_setInt("current_language", g_sys_Data.current_language);
		g_sys_Data.resetFactory = false;
		storageData_setBool("resetFactory", g_sys_Data.resetFactory);
		lv_obj_add_flag(guider_ui.screen_cont_languageChoose, LV_OBJ_FLAG_HIDDEN);
		break;
	}
    default:
        break;
    }
}

static void screen_btn_lockScreen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		lv_obj_clear_flag(guider_ui.screen_label_Popup, LV_OBJ_FLAG_HIDDEN);
		if(g_sys_Data.lockScreenFlag){
			g_sys_Data.lockScreenFlag = false;
			lv_obj_set_style_bg_img_src(guider_ui.screen_btn_lockScreen, &_unlock_select_80x80, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_label_set_text(guider_ui.screen_label_Popup, "Unlock");
			resetOrSetupTimer(screen_timer_popupLabel);
		}else{
			g_sys_Data.lockScreenFlag = true;
			lv_obj_set_style_bg_img_src(guider_ui.screen_btn_lockScreen, &_lock_select_80x80, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_label_set_text(guider_ui.screen_label_Popup, "Locked on screen");
			resetOrSetupTimer(screen_timer_popupLabel);
		}
		break;
	}
    default:
        break;
    }
}

static void btn_anim(lv_obj_t *btn, int endPoint){
	lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn);
    lv_anim_set_exec_cb(&a, lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(btn), endPoint);      // 从y=10到y=200
    lv_anim_set_time(&a, 1000);           // 1秒
    lv_anim_set_delay(&a, 100);           // 延迟500ms
    lv_anim_set_playback_time(&a, 0);   // 回放时间
    lv_anim_set_repeat_count(&a, 0);  // 无限循环	
	lv_anim_start(&a);
}

static void screen_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
	lv_coord_t coord;
    lv_point_t point;
    switch (code) {
	case LV_EVENT_PRESSED:
	{
		lv_indev_get_point(lv_indev_get_act(), &point);
		if(point.y < 80){
			coord = lv_obj_get_y(guider_ui.screen_btn_lockScreen);
			if(coord == -80){
				btn_anim(guider_ui.screen_btn_lockScreen, 0);
				resetOrSetupTimer(screen_timer_lockScreenBtn);
			}else{
				btn_anim(guider_ui.screen_btn_lockScreen, -80);
			}
		}
		break;
	}
	case LV_EVENT_RELEASED:
		

		break;
    default:
        break;
    }
}

static void popup_label(lv_timer_t *timer){
	printf("%s:%d\n",__func__,__LINE__);
	if(lv_obj_is_valid(guider_ui.screen_label_Popup)){
	lv_obj_add_flag(guider_ui.screen_label_Popup, LV_OBJ_FLAG_HIDDEN);
	screen_timer_popupLabel->timerStatus = false;
	}
	
}

static void lockScreen_Btn(lv_timer_t *timer){
	printf("%s:%d\n",__func__,__LINE__);
	if(lv_obj_is_valid(guider_ui.screen_btn_lockScreen)){
		btn_anim(guider_ui.screen_btn_lockScreen, -80);
	}
	screen_timer_lockScreenBtn->timerStatus = false;
}

void events_init_screen (lv_ui *ui)
{
	if(screen_timer_popupLabel == NULL){
		screen_timer_popupLabel = (LABEL_TIMER *)malloc(sizeof(LABEL_TIMER));
		screen_timer_popupLabel->interval = 1000;
		screen_timer_popupLabel->timerStatus = false;
		screen_timer_popupLabel->fun = popup_label;

	}
	if(screen_timer_lockScreenBtn == NULL){
		screen_timer_lockScreenBtn = (LABEL_TIMER *)malloc(sizeof(LABEL_TIMER));
		screen_timer_lockScreenBtn->interval = 5000;
		screen_timer_lockScreenBtn->timerStatus = false;
		screen_timer_lockScreenBtn->fun = lockScreen_Btn;

	}
	lv_obj_add_event_cb(ui->screen, screen_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_btn_cartrip, screen_btn_cartrip_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_btn_DVR, screen_btn_DVR_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_btn_set, screen_btn_set_event_handler, LV_EVENT_ALL, ui);
	// lv_obj_add_event_cb(ui->screen_btn_carplay, screen_btn_carplay_event_handler, LV_EVENT_ALL, ui);
	// lv_obj_add_event_cb(ui->screen_btn_androidauto, screen_btn_androidauto_event_handler,LV_EVENT_ALL,ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemEN, screen_list_language_itemEN_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemCN, screen_list_language_itemCN_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemCN_FT, screen_list_language_itemCN_FT_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemFR, screen_list_language_itemFR_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemGE, screen_list_language_itemGE_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemIT, screen_list_language_itemIT_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemJA, screen_list_language_itemJA_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemPOR, screen_list_language_itemPOR_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemRU, screen_list_language_itemRU_event_handler, LV_EVENT_ALL, ui);	
	lv_obj_add_event_cb(ui->screen_list_language_itemSP, screen_list_language_itemSP_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemPOL, screen_list_language_itemPOL_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemTU, screen_list_language_itemTU_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemKO, screen_list_language_itemKO_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_list_language_itemDU, screen_list_language_itemDU_event_handler, LV_EVENT_ALL, ui);	
	lv_obj_add_event_cb(ui->screen_btn_confirm, screen_btn_confirm_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_btn_lockScreen, screen_btn_lockScreen_event_handler, LV_EVENT_ALL, ui);
}

extern void recorder_status_timer(lv_timer_t *timer);
extern lv_timer_t *staTimer;
extern void clearRecorderStatu(void);
extern void prviewModeLabelDash(preview_mode mode);
static void screen_DVR_btn_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_DVR_del, setup_scr_screen, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		switch (g_sys_Data.previewMode)
		{
		case PREVIEW_NONE:
			break;
		case PREVIEW_FRONT:
			stopPreview(&g_sys_Data.vipp0_config);
			g_sys_Data.previewMode = PREVIEW_NONE;
			break;
		case PREVIEW_REAR:
			stopPreview(&g_sys_Data.vipp8_config);
			g_sys_Data.previewMode = PREVIEW_NONE;
			break;
		case PREVIEW_FRONT_REAR:
			stopPreview(&g_sys_Data.vipp0_config);
			stopPreview(&g_sys_Data.vipp8_config);		
			g_sys_Data.previewMode = PREVIEW_NONE;
			break;	
		default:
			break;
		}
		// free(screen_DVR_timer_Label);
		// screen_DVR_timer_Label = NULL;
		break;
	}
    default:
        break;
    }
}

static void screen_DVR_btn_preview_mode_event_handler (lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
	m_para_conf para;
    memset(&para, 0 , sizeof(m_para_conf));
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		switch (g_sys_Data.previewMode)
		{
		case PREVIEW_NONE:
			setDstSizeAndDispPos(&g_sys_Data.vipp0_config,720,1440,0,0);
			createVoChn(&g_sys_Data.vipp0_config, 0, 0);
			startPreview(&g_sys_Data.vipp0_config);
			g_sys_Data.previewMode = PREVIEW_FRONT;
			break;
		case PREVIEW_FRONT:
			stopPreview(&g_sys_Data.vipp0_config);
			setDstSizeAndDispPos(&g_sys_Data.vipp8_config,720,1440,0,0);
			createVoChn(&g_sys_Data.vipp8_config, 0, 0);
			startPreview(&g_sys_Data.vipp8_config);	
			g_sys_Data.previewMode = PREVIEW_REAR;
			break;
		case PREVIEW_REAR:
			stopPreview(&g_sys_Data.vipp8_config);
			setDstSizeAndDispPos(&g_sys_Data.vipp0_config,720,720,0,720);
			createVoChn(&g_sys_Data.vipp0_config, 0, 0);
			startPreview(&g_sys_Data.vipp0_config);

			setDstSizeAndDispPos(&g_sys_Data.vipp8_config,720,720,0,0);
			createVoChn(&g_sys_Data.vipp8_config, 0, 0);
			startPreview(&g_sys_Data.vipp8_config);
			g_sys_Data.previewMode = PREVIEW_FRONT_REAR;
			break;
		case PREVIEW_FRONT_REAR:
			stopPreview(&g_sys_Data.vipp0_config);
			stopPreview(&g_sys_Data.vipp8_config);		
			setDstSizeAndDispPos(&g_sys_Data.vipp0_config,720,1440,0,0);
			createVoChn(&g_sys_Data.vipp0_config, 0, 0);
			startPreview(&g_sys_Data.vipp0_config);
			g_sys_Data.previewMode = PREVIEW_FRONT;
			break;	
		default:
			break;
		}
		prviewModeLabelDash(g_sys_Data.previewMode);
		break;
	}
    default:
        break;
    }	
}


static void screen_DVR_btn_recorder_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		/* code */
		if(!checkTFCardMountProc()){
			lv_obj_clear_flag(guider_ui.screen_DVR_label_NotTF, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_DVR_timer_Label);
			break;
		}

		switch (g_sys_Data.recorderMode)
		{
		case RECORDER_NONE:
			g_sys_Data.recorderMode = RECORDER_NORMAL;
			printf("start to recording!!!!!\n");
			CreateMsgQueueThread(&g_sys_Data.vipp0_config);
			createVencChn(&g_sys_Data.vipp0_config);
			createMuxChn(&g_sys_Data.vipp0_config);
			prepare(&g_sys_Data.vipp0_config);
			startVideoRecording(&g_sys_Data.vipp0_config);

// -------------
			CreateMsgQueueThread(&g_sys_Data.vipp8_config);
			createVencChn(&g_sys_Data.vipp8_config);
			createMuxChn(&g_sys_Data.vipp8_config);
			prepare(&g_sys_Data.vipp8_config);
			startVideoRecording(&g_sys_Data.vipp8_config);
//--------------
			staTimer = lv_timer_create(recorder_status_timer, 1000, NULL);
			break;
		case RECORDER_NORMAL:
		case RECORDER_URGENT:
			g_sys_Data.recorderMode = RECORDER_NONE;
			printf("stop to recording!!!!!\n");
			stopVideoRecording(&g_sys_Data.vipp0_config);
//----------------
			stopVideoRecording(&g_sys_Data.vipp8_config);
			lv_timer_del(staTimer);			
			clearRecorderStatu();
			break;
			/* code */
			break;		
		default:
			break;
		}
		break;
	
	default:
		break;
	}
}

static void screen_DVR_btn_playback_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		/* code */
		if(!checkTFCardMountProc()){
			lv_obj_clear_flag(guider_ui.screen_DVR_label_NotTF, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_DVR_timer_Label);
			break;
		} else {
			stopPreview(&g_sys_Data.vipp0_config);
			stopPreview(&g_sys_Data.vipp8_config);
			g_sys_Data.previewMode = PREVIEW_NONE;
			ui_load_scr_animation(&guider_ui, &guider_ui.screen_MediaLibrary,
						guider_ui.screen_MediaLibrary_del, &guider_ui.screen_DVR_del,
						setup_scr_media_library, LV_SCR_LOAD_ANIM_NONE,
						200, 200, true, true);
		}
		break;
	
	default:
		break;
	}
}

static void screen_DVR_btn_set_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_DVR_SET, guider_ui.screen_DVR_SET_del, &guider_ui.screen_DVR_del, setup_scr_screen_DVR_SET, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		break;
	}
    default:
        break;
    }
}

static void screen_DVR_btn_takepic_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		// return;
		if(!checkTFCardMountProc()){
			lv_obj_clear_flag(guider_ui.screen_DVR_label_NotTF, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_DVR_timer_Label);
			break;
		}
		startTakePic(&g_sys_Data.vipp0_config);
		startTakePic(&g_sys_Data.vipp8_config);
		break;
	}
    default:
        break;
    }
}


static void screen_DVR_btn_lock_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(!checkTFCardMountProc()){
			lv_obj_clear_flag(guider_ui.screen_DVR_label_NotTF, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_DVR_timer_Label);
			break;
		}
		switch (g_sys_Data.recorderMode)
		{
		case RECORDER_NONE:
			g_sys_Data.recorderMode = RECORDER_URGENT;
			printf("start to recording!!!!!\n");
			CreateMsgQueueThread(&g_sys_Data.vipp0_config);
			createVencChn(&g_sys_Data.vipp0_config);
			createMuxChn(&g_sys_Data.vipp0_config);
			prepare(&g_sys_Data.vipp0_config);
			startVideoRecording(&g_sys_Data.vipp0_config);
// -------------
			CreateMsgQueueThread(&g_sys_Data.vipp8_config);
			createVencChn(&g_sys_Data.vipp8_config);
			createMuxChn(&g_sys_Data.vipp8_config);
			prepare(&g_sys_Data.vipp8_config);
			startVideoRecording(&g_sys_Data.vipp8_config);
//--------------
			staTimer = lv_timer_create(recorder_status_timer, 1000, NULL);
			break;
		case RECORDER_NORMAL:
			g_sys_Data.recorderMode = RECORDER_URGENT;
			changeVideoRecordingMode(&g_sys_Data.vipp0_config);
			break;
		case RECORDER_URGENT:
			g_sys_Data.recorderMode = RECORDER_NORMAL;
			changeVideoRecordingMode(&g_sys_Data.vipp0_config);
			/* code */
			lv_timer_del(staTimer);
			clearRecorderStatu();
			break;		
		default:
			break;
		}
		break;
	}
    default:
        break;
    }
}

static void TF_card_umount_label(lv_timer_t *timer){
	printf("%s:%d\n",__func__,__LINE__);
	if(lv_obj_is_valid(guider_ui.screen_DVR_label_NotTF)){	
	lv_obj_add_flag(guider_ui.screen_DVR_label_NotTF, LV_OBJ_FLAG_HIDDEN);
	screen_DVR_timer_Label->timerStatus = false;
	}
}
extern lv_timer_t *HideBtnContTimer;
extern void HideBtnCont(void);
static void screen_DVR_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(lv_obj_has_flag(guider_ui.screen_DVR_cont_bottom, LV_OBJ_FLAG_HIDDEN)){
			lv_obj_clear_flag(guider_ui.screen_DVR_cont_bottom, LV_OBJ_FLAG_HIDDEN);
		}
		if(HideBtnContTimer){
			lv_timer_reset(HideBtnContTimer);
		}else{
			HideBtnContTimer = lv_timer_create(HideBtnCont, 5000, NULL);
			lv_timer_set_repeat_count(HideBtnContTimer, 1);
		}
		break;
	}
    default:
        break;
    }
}

void events_init_screen_DVR (lv_ui *ui)
{
	if(screen_DVR_timer_Label == NULL){
		screen_DVR_timer_Label = (LABEL_TIMER *)malloc(sizeof(LABEL_TIMER));
		screen_DVR_timer_Label->interval = 1000;
		screen_DVR_timer_Label->timerStatus = false;
		screen_DVR_timer_Label->fun = TF_card_umount_label;

	}
	lv_obj_add_event_cb(ui->screen_DVR, screen_DVR_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_btn_return, screen_DVR_btn_return_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_btn_preview_mode, screen_DVR_btn_preview_mode_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_btn_recorder, screen_DVR_btn_recorder_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_btn_playback, screen_DVR_btn_playback_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_btn_set, screen_DVR_btn_set_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_btn_takepic, screen_DVR_btn_takepic_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_btn_lock, screen_DVR_btn_lock_event_handler, LV_EVENT_ALL, ui);
}

static void screen_DVR_SET_btn_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_DVR, guider_ui.screen_DVR_del, &guider_ui.screen_DVR_SET_del, setup_scr_screen_DVR, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		break;
	}
    default:
        break;
    }
}

static void screen_DVR_SET_btn_rearCameraImage_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(g_sys_Data.rearCameraImage){
			g_sys_Data.rearCameraImage = false;
		}else{
			g_sys_Data.rearCameraImage = true;
		}
		break;
	}
    default:
        break;
    }
}
extern void RecorderTimeBtnSta();
static void screen_DVR_SET_btn_recorder_1min_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		g_sys_Data.recorderTime = RECORDER_1;
		RecorderTimeBtnSta();
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_recorder_3min_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		g_sys_Data.recorderTime = RECORDER_3;
		RecorderTimeBtnSta();
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_recorder_5min_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		g_sys_Data.recorderTime = RECORDER_5;
		RecorderTimeBtnSta();
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_urgent_1min_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		g_sys_Data.recorderUrgentTime = RECORDER_1;
		RecorderTimeBtnSta();
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_urgent_3min_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		g_sys_Data.recorderUrgentTime = RECORDER_3;
		RecorderTimeBtnSta();
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_Urgent_5min_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		g_sys_Data.recorderUrgentTime = RECORDER_5;
		RecorderTimeBtnSta();
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_powerOnRecording_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(g_sys_Data.powerOnRecorder){
			g_sys_Data.powerOnRecorder = false;
		}else{
			g_sys_Data.powerOnRecorder = true;
		}
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_recorderSoundSet_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(g_sys_Data.SoundRecorder){
			g_sys_Data.SoundRecorder = false;
		}else{
			g_sys_Data.SoundRecorder = true;
		}
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_recorderTimeMark_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(g_sys_Data.TimeMark){
			g_sys_Data.TimeMark = false;
		}else{
			g_sys_Data.TimeMark = true;
		}		
		break;
	}
    default:
        break;
    }
}
static void screen_DVR_SET_btn_carSpeedMark_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		if(g_sys_Data.SpeedMark){
			g_sys_Data.SpeedMark = false;
		}else{
			g_sys_Data.SpeedMark = true;
		}	
		break;
	}
    default:
        break;
    }
}
void events_init_screen_DVR_SET (lv_ui *ui)
{
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_return, screen_DVR_SET_btn_return_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_rearCameraImage, screen_DVR_SET_btn_rearCameraImage_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_recorder_1min, screen_DVR_SET_btn_recorder_1min_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_recorder_3min, screen_DVR_SET_btn_recorder_3min_event_handler,LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_recorder_5min , screen_DVR_SET_btn_recorder_5min_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_urgent_1min, screen_DVR_SET_btn_urgent_1min_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_urgent_3min, screen_DVR_SET_btn_urgent_3min_event_handler,LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_urgent_5min , screen_DVR_SET_btn_Urgent_5min_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_powerOnRecording , screen_DVR_SET_btn_powerOnRecording_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_recorderSoundSet , screen_DVR_SET_btn_recorderSoundSet_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_recorderTimeMark , screen_DVR_SET_btn_recorderTimeMark_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_DVR_SET_btn_carSpeedMark, screen_DVR_SET_btn_carSpeedMark_event_handler, LV_EVENT_ALL, ui);
}

static void screen_SET_btn_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		sysPageManageFun(sysPageManage);
		switch (sysPageManage)
		{
		case CONT_TONGYONG:
		case CONT_LANYA:
		case CONT_SYS:
			ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_SET_del, setup_scr_screen, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
			sysPageManage = CONT_TONGYONG;
			/* code */
			break;	
		case CONT_LANGUAGE:
		case CONT_SETTIME:
			lv_obj_clear_flag(guider_ui.screen_SET_cont_tongyong, LV_OBJ_FLAG_HIDDEN);
			sysPageManage = CONT_TONGYONG;
		break;
		case CONT_ABOUT:
		case CONT_BOOTLOGO:
		case CONT_REBOOT:
		case CONT_RESETFACTORY:
			lv_obj_clear_flag(guider_ui.screen_SET_cont_sys, LV_OBJ_FLAG_HIDDEN);
			sysPageManage = CONT_SYS;
		break;	
		default:	
			break;
		}
		break;
	}
    default:
        break;
    }
}

static void screen_SET_list_set_item0_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:genaral click event
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_TONGYONG;
		lv_obj_add_state(guider_ui.screen_SET_list_set_item0,LV_STATE_PRESSED);
		lv_obj_clear_state(guider_ui.screen_SET_list_set_item1,LV_STATE_PRESSED);
		lv_obj_clear_state(guider_ui.screen_SET_list_set_item2,LV_STATE_PRESSED);
		lv_obj_clear_flag(guider_ui.screen_SET_cont_tongyong, LV_OBJ_FLAG_HIDDEN);
		break;
	}
    default:
        break;
    }
}

static void screen_SET_list_set_item1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:bt click enevnt
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_LANYA;
		lv_obj_clear_state(guider_ui.screen_SET_list_set_item0,LV_STATE_PRESSED);
		lv_obj_add_state(guider_ui.screen_SET_list_set_item1,LV_STATE_PRESSED);
		lv_obj_clear_state(guider_ui.screen_SET_list_set_item2,LV_STATE_PRESSED);
		lv_obj_clear_flag(guider_ui.screen_SET_cont_lanya, LV_OBJ_FLAG_HIDDEN);
		break;
	}
    default:
        break;
    }
}

static void screen_SET_list_set_item2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:sys click event
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_SYS;
		lv_obj_clear_state(guider_ui.screen_SET_list_set_item0,LV_STATE_PRESSED);
		lv_obj_clear_state(guider_ui.screen_SET_list_set_item1,LV_STATE_PRESSED);
		lv_obj_add_state(guider_ui.screen_SET_list_set_item2,LV_STATE_PRESSED);
		lv_obj_clear_flag(guider_ui.screen_SET_cont_sys, LV_OBJ_FLAG_HIDDEN);
		break;
	}
    default:
        break;
    }
}
static const char* themeMode[] = {"sys_txt_day", "sys_txt_night", "sys_txt_auto"};
extern void ScreenSetBlackNightMode(lv_ui *ui, THEME_MODE mode); 
void screen_SET_btn_zhuti_left_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.themeMode = g_sys_Data.themeMode==1?THEME_DAY:(g_sys_Data.themeMode > 1 ? THEME_DARK : THEME_AUTO);
		lv_label_set_text(guider_ui.screen_SET_label_zhuti1,get_string_for_language(g_sys_Data.current_language,themeMode[g_sys_Data.themeMode]));
		// ScreenSetBlackNightMode(&guider_ui, g_sys_Data.themeMode);
		break;
	default:
		break;
	}
}
void screen_SET_btn_zhuti_right_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.themeMode = g_sys_Data.themeMode==1?THEME_AUTO:(g_sys_Data.themeMode > 1 ? THEME_DAY : THEME_DARK);
		lv_label_set_text(guider_ui.screen_SET_label_zhuti1,get_string_for_language(g_sys_Data.current_language,themeMode[g_sys_Data.themeMode]));
		// ScreenSetBlackNightMode(&guider_ui, g_sys_Data.themeMode);
		break;
	default:
		break;
	}
}

void screen_SET_btn_yuyan1_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		/* code */
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_LANGUAGE;
		lv_obj_clear_flag(guider_ui.screen_SET_cont_setlanguage, LV_OBJ_FLAG_HIDDEN);
		break;
	
	default:
		break;
	}
}

void screen_SET_btn_shijian_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		if(g_sys_Data.linktype == LINK_TYPE_ANDROIDAUTO || g_sys_Data.linktype == LINK_TYPE_CARPLAY){
			lv_obj_clear_flag(guider_ui.screen_SET_label_popup, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_SET_timer_popupLabel);
			return;
		}
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_SETTIME;
		lv_obj_clear_flag(guider_ui.screen_SET_cont_settime, LV_OBJ_FLAG_HIDDEN);
		break;
	
	default:
		break;
	}
}

void screen_SET_btn_about_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		/* code */
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_ABOUT;
		lv_obj_clear_flag(guider_ui.screen_SET_cont_guanyu, LV_OBJ_FLAG_HIDDEN);
		break;
	
	default:
		break;
	}
}

void screen_SET_btn_ZH_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_CHINESE;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_EN_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_ENGLISH;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_ZH_FT_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_CHINESE_FT;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_FR_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_FRANCE;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_GE_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_GERMANY;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_IT_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_ITALY;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_JP_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_JAPAN;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_POR_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_PORTUGUESE;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_RU_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_RUSSIAN;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_SP_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_SPANISH;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_POL_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_POLAND;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_TU_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_TURKISH;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_KO_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_KOREAN;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}
void screen_SET_btn_DU_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		g_sys_Data.current_language = LANG_DUTCH;
		languageItemStatu(g_sys_Data.current_language);
		if(!storageData_setInt("current_language", g_sys_Data.current_language)){
			printf("storage data language[%d] fail!\n", g_sys_Data.current_language);
		}
		break;
	default:
		break;
	}
}


void screen_SET_slider_backlight_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
	g_sys_Data.backlight = lv_slider_get_value(obj);
     if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
         lv_coord_t * s = lv_event_get_param(e);
         *s = LV_MAX(*s, 60);
     }
    else if(code == LV_EVENT_VALUE_CHANGED)
    {	
		SetBackLight(g_sys_Data.backlight);
		char str[5] = {0};
		sprintf(str,"%d",g_sys_Data.backlight);
		lv_label_set_text(guider_ui.screen_SET_label_beiguang1, str);
		storageData_setInt("backlight", g_sys_Data.backlight);
    }
     else if(code == LV_EVENT_DRAW_PART_END) {
         lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
         if(dsc->part == LV_PART_KNOB && lv_obj_has_state(obj, LV_STATE_PRESSED)) {
             char buf[8];
		}
	}
}

//--------------------data set ----------------------
static void screen_SET_roller_year_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_roller_get_selected_str(obj, buf, sizeof(buf));
        printf("Selected years: %s\n", buf);
		TTime time;
		memset(&time, 0 ,sizeof(TTime));
		RtcGetTime2C(&time);
		time.nYear = atoi(buf);
		RtcSetTime2C(&time);
		printf("Selected years: %d\n", time.nYear);
    }
}
extern char* CreateDayOptions(void);
static void screen_SET_roller_month_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_roller_get_selected_str(obj, buf, sizeof(buf));
        printf("Selected month: %s\n", buf);
		TTime time;
		memset(&time, 0 ,sizeof(TTime));
		RtcGetTime2C(&time);
		time.nMonth = atoi(buf);
		time.nDay = 1;
		RtcSetTime2C(&time);
		lv_roller_set_options(guider_ui.screen_SET_roller_day, CreateDayOptions(), LV_ROLLER_MODE_INFINITE);
		lv_roller_set_selected(guider_ui.screen_SET_roller_day,0,LV_ANIM_OFF);

		printf("Selected nMonth: %d\n", time.nMonth);
    }
}

static void screen_SET_roller_day_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_roller_get_selected_str(obj, buf, sizeof(buf));
        printf("Selected day: %s\n", buf);
		TTime time;
		memset(&time, 0 ,sizeof(TTime));
		RtcGetTime2C(&time);
		time.nDay = atoi(buf);
		RtcSetTime2C(&time);
		printf("Selected day: %d\n", time.nDay);
    }
}

static void screen_SET_roller_hour_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_roller_get_selected_str(obj, buf, sizeof(buf));
        printf("Selected hour: %s\n", buf);
		TTime time;
		memset(&time, 0 ,sizeof(TTime));
		RtcGetTime2C(&time);
		time.nHour = atoi(buf);
		RtcSetTime2C(&time);
		printf("Selected hour: %d\n", time.nHour);
    }
}

static void screen_SET_roller_minute_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_roller_get_selected_str(obj, buf, sizeof(buf));
        printf("Selected minute: %s\n", buf);
		TTime time;
		memset(&time, 0 ,sizeof(TTime));
		RtcGetTime2C(&time);
		time.nMinute = atoi(buf);
		RtcSetTime2C(&time);
		printf("Selected minute: %d\n", time.nMinute);
    }
}
//----
static void screen_SET_screen_SET_btn_taiya_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_PRESSED:
		printf("---LV_EVENT_PRESSED---\n");
		lv_img_set_src(guider_ui.screen_SET_btn_taiya_img, &_right_but_down_60x75);
		break;
	case LV_EVENT_RELEASED:
		printf("---LV_EVENT_RELEASED---\n");
		lv_img_set_src(guider_ui.screen_SET_btn_taiya_img, &_right_but_normal_day_60x75);
		if(g_sys_Data.carTripSwitch){
			g_sys_Data.carTripSwitch = false;
			lv_label_set_text(guider_ui.screen_SET_btn_taiya_sw_label, get_string_for_language(g_sys_Data.current_language,"sys_txt_close"));
		}else{
			g_sys_Data.carTripSwitch = true;
			lv_label_set_text(guider_ui.screen_SET_btn_taiya_sw_label, get_string_for_language(g_sys_Data.current_language,"sys_txt_open"));
		}
		storageData_setBool("carTripSwitch", g_sys_Data.carTripSwitch);
		break;
	default:
		break;
	}
}

void screen_SET_btn_logo_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_BOOTLOGO;
		lv_obj_clear_flag(guider_ui.screen_SET_cont_bootlogo, LV_OBJ_FLAG_HIDDEN);
		break;
	default:
		break;
	}
}

void screen_SET_btn_reboot_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_REBOOT;
		lv_obj_clear_flag(guider_ui.screen_SET_cont_sys, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(guider_ui.screen_SET_cont_reboot, LV_OBJ_FLAG_HIDDEN);
		break;
	default:
		break;
	}
}

void screen_SET_btn_refactory_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		sysPageManageFun(sysPageManage);
		sysPageManage = CONT_RESETFACTORY;
		lv_obj_clear_flag(guider_ui.screen_SET_cont_sys, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(guider_ui.screen_SET_cont_resetFactory, LV_OBJ_FLAG_HIDDEN);
		break;
	default:
		break;
	}
}

void screen_SET_btn_reboot_confirm_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		lv_obj_add_flag(guider_ui.screen_SET_cont_reboot, LV_OBJ_FLAG_HIDDEN);
		sysPageManage = CONT_SYS;
		system("sync");
		system("reboot");
		break;
	default:
		break;
	}
}

void screen_SET_btn_reboot_cancel_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		lv_obj_add_flag(guider_ui.screen_SET_cont_reboot, LV_OBJ_FLAG_HIDDEN);
		sysPageManage = CONT_SYS;
		break;
	default:
		break;
	}
}

void screen_SET_btn_resetFactory_confirm_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		lv_obj_add_flag(guider_ui.screen_SET_cont_resetFactory, LV_OBJ_FLAG_HIDDEN);
		sysPageManage = CONT_SYS;
		RestorefactorySettings();
		break;
	default:
		break;
	}
}

void screen_SET_btn_resetFactory_cancel_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		lv_obj_add_flag(guider_ui.screen_SET_cont_resetFactory, LV_OBJ_FLAG_HIDDEN);
		sysPageManage = CONT_SYS;
		break;
	default:
		break;
	}
}

void screen_SET_btn_otaUpdata_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		if(g_sys_Data.linktype == LINK_TYPE_ANDROIDAUTO || g_sys_Data.linktype == LINK_TYPE_CARPLAY){
			lv_obj_clear_flag(guider_ui.screen_SET_label_popup, LV_OBJ_FLAG_HIDDEN);
			resetOrSetupTimer(screen_SET_timer_popupLabel);
			return;
		}
		if(screen_SET_timer_popupLabel && screen_SET_timer_popupLabel->timerStatus && screen_SET_timer_popupLabel->timer){
			lv_timer_del(screen_SET_timer_popupLabel->timer);
			screen_SET_timer_popupLabel->timer = NULL;
			screen_SET_timer_popupLabel->timerStatus = false;
		}
		ui_load_scr_animation(&guider_ui, &guider_ui.screen_OTA,
					guider_ui.screen_OTA_del, &guider_ui.screen_SET_del,
					setup_scr_ota, LV_SCR_LOAD_ANIM_NONE,
					200, 200, true, true);
		break;
	default:
		break;
	}
}


static void screen_SET_timer_popupLabel_dash(lv_timer_t *timer){
printf("%s:%d\n",__func__,__LINE__);
	lv_obj_t *scr = lv_scr_act();
	if(scr == guider_ui.screen_SET && lv_obj_is_valid(guider_ui.screen_SET_label_popup)){
	lv_obj_add_flag(guider_ui.screen_SET_label_popup, LV_OBJ_FLAG_HIDDEN);
	}
	if(screen_SET_timer_popupLabel) screen_SET_timer_popupLabel->timerStatus = false;
}

void events_init_screen_SET (lv_ui *ui)
{
	if(screen_SET_timer_popupLabel == NULL){
		screen_SET_timer_popupLabel = (LABEL_TIMER *)malloc(sizeof(LABEL_TIMER));
		screen_SET_timer_popupLabel->interval = 1000;
		screen_SET_timer_popupLabel->timerStatus = false;
		screen_SET_timer_popupLabel->fun = screen_SET_timer_popupLabel_dash;
	}
	lv_obj_add_event_cb(ui->screen_SET_btn_return, screen_SET_btn_return_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_list_set_item0, screen_SET_list_set_item0_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_list_set_item1, screen_SET_list_set_item1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_list_set_item2, screen_SET_list_set_item2_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_taiya, screen_SET_screen_SET_btn_taiya_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_zhuti_left, screen_SET_btn_zhuti_left_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_zhuti_right, screen_SET_btn_zhuti_right_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_yuyan, screen_SET_btn_yuyan1_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_shijian, screen_SET_btn_shijian_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_about, screen_SET_btn_about_event_handler, LV_EVENT_ALL, ui);
	
	lv_obj_add_event_cb(ui->screen_SET_btn_ZH, screen_SET_btn_ZH_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_EN, screen_SET_btn_EN_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_ZH_FT, screen_SET_btn_ZH_FT_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_FR, screen_SET_btn_FR_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_GE, screen_SET_btn_GE_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_IT, screen_SET_btn_IT_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_JP, screen_SET_btn_JP_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_POR, screen_SET_btn_POR_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_RU, screen_SET_btn_RU_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_SP, screen_SET_btn_SP_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_POL, screen_SET_btn_POL_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_TU, screen_SET_btn_TU_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_KO, screen_SET_btn_KO_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_DU, screen_SET_btn_DU_event_handler, LV_EVENT_ALL, ui);


	lv_obj_add_event_cb(ui->screen_SET_slider_backlight, screen_SET_slider_backlight_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_roller_year, screen_SET_roller_year_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_roller_month, screen_SET_roller_month_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_roller_day, screen_SET_roller_day_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_roller_hour, screen_SET_roller_hour_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_roller_minute, screen_SET_roller_minute_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_logo, screen_SET_btn_logo_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_reboot, screen_SET_btn_reboot_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_refactory, screen_SET_btn_refactory_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_reboot_confirm, screen_SET_btn_reboot_confirm_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_reboot_cancel, screen_SET_btn_reboot_cancel_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_resetFactory_confirm, screen_SET_btn_resetFactory_confirm_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_resetFactory_cancel, screen_SET_btn_resetFactory_cancel_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_SET_btn_otaUpdata, screen_SET_btn_otaUpdata_event_handler, LV_EVENT_ALL, ui);	

}

static void screen_Tire_btn_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_Tire_del, setup_scr_screen, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_lPressMax_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:pressure min regulate down
		if(g_sys_Data.pressureMax <= 0)
			return;
		if(!g_sys_Data.pressureUnit){
			g_sys_Data.pressureMax -= 0.1;
			char str[20];
			sprintf(str,"%.1f",g_sys_Data.pressureMax);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMax1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMax1); 			
		}else{
			g_sys_Data.pressureMax -= 1;
			char str[20];
			sprintf(str,"%.0f",g_sys_Data.pressureMax);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMax1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMax1); 		
		}		
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_rPressMin_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:pressure min regulate up
		if(!g_sys_Data.pressureUnit){
			g_sys_Data.pressureMin += 0.1;
			char str[20];
			sprintf(str,"%.1f",g_sys_Data.pressureMin);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMin1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMin1); 			
		}else{
			g_sys_Data.pressureMin += 1;
			char str[20];
			sprintf(str,"%.0f",g_sys_Data.pressureMin);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMin1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMin1); 			
		}		
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_lPressMin_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:pressure max regulat down
		if(g_sys_Data.pressureMin<=0)
			return;
		if(!g_sys_Data.pressureUnit){
			g_sys_Data.pressureMin -= 0.1;
			char str[20];
			sprintf(str,"%.1f",g_sys_Data.pressureMin);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMin1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMin1); 
		}else{
			g_sys_Data.pressureMin -= 1;
			char str[20];
			sprintf(str,"%.0f",g_sys_Data.pressureMin);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMin1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMin1); 			
		}
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_rPressMax_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:pressure max regulate up
		if(!g_sys_Data.pressureUnit){
			g_sys_Data.pressureMax += 0.1;
			char str[20];
			sprintf(str,"%.1f",g_sys_Data.pressureMax);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMax1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMax1); 			
		}else{
			g_sys_Data.pressureMax += 1;
			char str[20];
			sprintf(str,"%.0f",g_sys_Data.pressureMax);
			lv_label_set_text(guider_ui.screen_Tire_label_pressMax1,str);
			lv_obj_invalidate(guider_ui.screen_Tire_label_pressMax1); 			
		}

		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_lTemp_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:temp max regulate down
		if(g_sys_Data.tempMax == 0)
			return;
		g_sys_Data.tempMax -= 1;
		char str[20];
		sprintf(str,"%d",g_sys_Data.tempMax);
		lv_label_set_text(guider_ui.screen_Tire_label_tempMax1,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_tempMax1); 
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_rTemp_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:temp max regulate up
		g_sys_Data.tempMax += 1;
		char str[20];
		sprintf(str,"%d",g_sys_Data.tempMax);
		lv_label_set_text(guider_ui.screen_Tire_label_tempMax1,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_tempMax1); 
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_pressBar_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:change the pressure unit
		if(!g_sys_Data.pressureUnit)
			return;
		g_sys_Data.bPressure = PressureUnitConversion(g_sys_Data.bPressure);
		g_sys_Data.fPressure = PressureUnitConversion(g_sys_Data.fPressure);		
		g_sys_Data.pressureMax = PressureUnitConversion(g_sys_Data.pressureMax);
		g_sys_Data.pressureMin = PressureUnitConversion(g_sys_Data.pressureMin);
		g_sys_Data.pressureUnit =  false;
		lv_img_set_src(guider_ui.screen_Tire_img_pressBar,&_selected_40x40);
		lv_img_set_src(guider_ui.screen_Tire_img_pressPsj,&_not_select_40x40);
		lv_obj_invalidate(guider_ui.screen_Tire_img_pressBar);
		lv_obj_invalidate(guider_ui.screen_Tire_img_pressPsj);	
		char str[20];
		sprintf(str,"%.1f Bar",g_sys_Data.fPressure);
		lv_label_set_text(guider_ui.screen_Tire_label_fPressure,str);
		sprintf(str,"%.1f Bar",g_sys_Data.bPressure);
		lv_label_set_text(guider_ui.screen_Tire_label_bPressure,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_bPressure);
		lv_obj_invalidate(guider_ui.screen_Tire_label_fPressure);
		sprintf(str,"%.1f",g_sys_Data.pressureMin);
		lv_label_set_text(guider_ui.screen_Tire_label_pressMin1,str);
		sprintf(str,"%.1f",g_sys_Data.pressureMax);
		lv_label_set_text(guider_ui.screen_Tire_label_pressMax1,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_pressMin1); 
		lv_obj_invalidate(guider_ui.screen_Tire_label_pressMax1); 		
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_pressPsj_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:change the pressure unit
		if(g_sys_Data.pressureUnit)
			return;
		g_sys_Data.bPressure = PressureUnitConversion(g_sys_Data.bPressure);
		g_sys_Data.fPressure = PressureUnitConversion(g_sys_Data.fPressure);
		g_sys_Data.pressureMax = PressureUnitConversion(g_sys_Data.pressureMax);
		g_sys_Data.pressureMin = PressureUnitConversion(g_sys_Data.pressureMin);
		g_sys_Data.pressureUnit =  true;
		lv_img_set_src(guider_ui.screen_Tire_img_pressBar,&_not_select_40x40);
		lv_img_set_src(guider_ui.screen_Tire_img_pressPsj,&_selected_40x40);
		lv_obj_invalidate(guider_ui.screen_Tire_img_pressBar);
		lv_obj_invalidate(guider_ui.screen_Tire_img_pressPsj);
		char str[20];
		sprintf(str,"%.0f Psi",g_sys_Data.fPressure);
		lv_label_set_text(guider_ui.screen_Tire_label_fPressure,str);
		sprintf(str,"%.0f Psi",g_sys_Data.bPressure);
		lv_label_set_text(guider_ui.screen_Tire_label_bPressure,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_bPressure);
		lv_obj_invalidate(guider_ui.screen_Tire_label_fPressure);
		sprintf(str,"%.0f",g_sys_Data.pressureMin);
		lv_label_set_text(guider_ui.screen_Tire_label_pressMin1,str);
		sprintf(str,"%.0f",g_sys_Data.pressureMax);
		lv_label_set_text(guider_ui.screen_Tire_label_pressMax1,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_pressMin1); 
		lv_obj_invalidate(guider_ui.screen_Tire_label_pressMax1); 						
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_tempC_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:change the temp unit
		if(!g_sys_Data.tempUnit)
			return;
		g_sys_Data.bTemp = TempUnitConversion(g_sys_Data.bTemp);
		g_sys_Data.fTemp = TempUnitConversion(g_sys_Data.fTemp);
		g_sys_Data.tempMax = TempUnitConversion(g_sys_Data.tempMax);
		g_sys_Data.tempUnit = false;
		lv_img_set_src(guider_ui.screen_Tire_img_tempC,&_selected_40x40);
		lv_img_set_src(guider_ui.screen_Tire_img_tempF,&_not_select_40x40);
		lv_obj_invalidate(guider_ui.screen_Tire_img_tempC);
		lv_obj_invalidate(guider_ui.screen_Tire_img_tempF);
		char str[20];
		sprintf(str,"%d ℃",g_sys_Data.fTemp);
		lv_label_set_text(guider_ui.screen_Tire_label_fTemp,str);
		sprintf(str,"%d ℃",g_sys_Data.bTemp);
		lv_label_set_text(guider_ui.screen_Tire_label_bTemp,str);
		sprintf(str,"%d",g_sys_Data.tempMax);
		lv_label_set_text(guider_ui.screen_Tire_label_tempMax1,str);			
		lv_obj_invalidate(guider_ui.screen_Tire_label_fTemp);
		lv_obj_invalidate(guider_ui.screen_Tire_label_bTemp);
		lv_obj_invalidate(guider_ui.screen_Tire_label_tempMax1);
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_tempF_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:change the temp unit
		if(g_sys_Data.tempUnit)
			return;
		g_sys_Data.bTemp = TempUnitConversion(g_sys_Data.bTemp);
		g_sys_Data.fTemp = TempUnitConversion(g_sys_Data.fTemp);	
		g_sys_Data.tempMax = TempUnitConversion(g_sys_Data.tempMax);		
		g_sys_Data.tempUnit = true;
		lv_img_set_src(guider_ui.screen_Tire_img_tempC,&_not_select_40x40);
		lv_img_set_src(guider_ui.screen_Tire_img_tempF,&_selected_40x40);
		lv_obj_invalidate(guider_ui.screen_Tire_img_tempC);
		lv_obj_invalidate(guider_ui.screen_Tire_img_tempF);
		char str[20];
		sprintf(str,"%d ℉",g_sys_Data.fTemp);
		lv_label_set_text(guider_ui.screen_Tire_label_fTemp,str);
		sprintf(str,"%d ℉",g_sys_Data.bTemp);
		lv_label_set_text(guider_ui.screen_Tire_label_bTemp,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_fTemp);
		lv_obj_invalidate(guider_ui.screen_Tire_label_bTemp);	
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_paraReset_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:reset the parameter 
		if(!g_sys_Data.pressureUnit){
			g_sys_Data.pressureMax = 4.0;
			g_sys_Data.pressureMin = 1.6;
		}else{
			g_sys_Data.pressureMax = 58;
			g_sys_Data.pressureMin = 23;
		}
		if(g_sys_Data.tempUnit){
			g_sys_Data.tempMax = 154;
		}else{
			g_sys_Data.tempMax = 68;
		}
		char str[20];
		sprintf(str,"%.1f",g_sys_Data.pressureMin);
		lv_label_set_text(guider_ui.screen_Tire_label_pressMin1,str);
		sprintf(str,"%d",g_sys_Data.tempMax);
		lv_label_set_text(guider_ui.screen_Tire_label_tempMax1,str);
		sprintf(str,"%.1f",g_sys_Data.pressureMax);
		lv_label_set_text(guider_ui.screen_Tire_label_pressMax1,str);
		lv_obj_invalidate(guider_ui.screen_Tire_label_pressMin1); 
		lv_obj_invalidate(guider_ui.screen_Tire_label_tempMax1); 
		lv_obj_invalidate(guider_ui.screen_Tire_label_pressMax1); 
		break;
	}
    default:
        break;
    }
}

static void screen_Tire_btn_clearPair_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		//by jy:chear the bind devices
		break;
	}
    default:
        break;
    }
}

void events_init_screen_Tire (lv_ui *ui)
{
	lv_obj_add_event_cb(ui->screen_Tire_btn_return, screen_Tire_btn_return_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_lPressMax, screen_Tire_btn_lPressMax_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_rPressMin, screen_Tire_btn_rPressMin_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_lPressMin, screen_Tire_btn_lPressMin_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_rPressMax, screen_Tire_btn_rPressMax_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_lTemp, screen_Tire_btn_lTemp_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_rTemp, screen_Tire_btn_rTemp_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_pressBar, screen_Tire_btn_pressBar_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_pressPsj, screen_Tire_btn_pressPsj_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_tempC, screen_Tire_btn_tempC_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_tempF, screen_Tire_btn_tempF_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_paraReset, screen_Tire_btn_paraReset_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->screen_Tire_btn_clearPair, screen_Tire_btn_clearPair_event_handler, LV_EVENT_ALL, ui);
}

void screen_carPlay_btn_return_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		/* code */
		ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_carPlay_del, setup_scr_screen, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
		freeCarPlay2De();
		break;
	
	default:
		break;
	}
}


void events_init_screen_carPlay(lv_ui *ui){
	lv_obj_add_event_cb(ui->screen_carPlay_btn_return,screen_carPlay_btn_return_event_handler,LV_EVENT_ALL,ui);
}

void screen_androidAuto_btn_return_event_handler(lv_event_t *e){
	lv_event_code_t code = lv_event_get_code(e);
	switch (code)
	{
	case LV_EVENT_CLICKED:
		/* code */
		ui_load_scr_animation(&guider_ui,&guider_ui.screen,guider_ui.screen_del,&guider_ui.screen_androidAuto_del,setup_scr_screen,LV_SCR_LOAD_ANIM_NONE,200,200,true,true);
		freeCarPlay2De();
		break;
	
	default:
		break;
	}
}

void event_init_screen_androidAuto(lv_ui *ui){
	lv_obj_add_event_cb(ui->screen_androidAuto_btn_return,screen_androidAuto_btn_return_event_handler,LV_EVENT_ALL,ui);
}

