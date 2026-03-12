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

static void screen_Tire_btn_return_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_Tire_del, setup_scr_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true, true);
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