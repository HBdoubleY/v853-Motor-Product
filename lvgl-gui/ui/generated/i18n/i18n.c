#include <stdio.h>
#include "i18n.h"


// 获取指定语言的字符串
const char *get_string_for_language(language_t lang, const char *key)
{
    const language_data_t *lang_data = languages[lang];
    if (lang_data == NULL)
    {
        return NULL; // 未找到对应语言或未初始化
    }
    if (key == NULL)
    {
        return NULL; // 未指定字符串键值
    }
    if (strcmp(key, "main_txt_carplay") == 0)
    {
        return lang_data->main_txt_carplay;
    }
    if (strcmp(key, "main_txt_AndroidAuto") == 0)
    {
        return lang_data->main_txt_AndroidAuto;
    }    
    if (strcmp(key, "main_txt_nConnect") == 0)
    {
        return lang_data->main_txt_nConnect;
    }
    if (strcmp(key, "main_txt_Connect") == 0)
    {
        return lang_data->main_txt_Connect;
    }
    if (strcmp(key, "main_txt_Monday") == 0)
    {
        return lang_data->main_txt_Monday;
    }
    if (strcmp(key, "main_txt_Tuesday") == 0)
    {
        return lang_data->main_txt_Tuesday;
    }
    if (strcmp(key, "main_txt_Wednesday") == 0)
    {
        return lang_data->main_txt_Wednesday;
    }
    if (strcmp(key, "main_txt_Thursday") == 0)
    {
        return lang_data->main_txt_Thursday;
    }
    if (strcmp(key, "main_txt_Friday") == 0)
    {
        return lang_data->main_txt_Friday;
    }
    if (strcmp(key, "main_txt_Saturday") == 0)
    {
        return lang_data->main_txt_Saturday;
    }
    if (strcmp(key, "main_txt_Sunday") == 0)
    {
        return lang_data->main_txt_Sunday;
    }
    else if (strcmp(key, "main_txt_DVR") == 0)
    {
        return lang_data->main_txt_DVR;
    }
    else if (strcmp(key, "main_txt_sysSet") == 0)
    {
        return lang_data->main_txt_sysSet;
    }
    else if (strcmp(key, "main_txt_fTyre") == 0)
    {
        return lang_data->main_txt_fTyre;
    }
    else if (strcmp(key, "main_txt_SlecetLanguage") == 0)
    {
        return lang_data->main_txt_SlecetLanguage;
    }
    else if (strcmp(key, "main_txt_bTyre") == 0)
    {
        return lang_data->main_txt_bTyre;
    }
    else if (strcmp(key, "sys_txt_invalid") == 0)
    {
        return lang_data->sys_txt_invalid;
    }   
    else if (strcmp(key, "sys_txt_generate") == 0)
    {
        return lang_data->sys_txt_generate;
    }
    else if (strcmp(key, "sys_txt_bt") == 0)
    {
        return lang_data->sys_txt_bt;
    }
    else if (strcmp(key, "sys_txt_sys") == 0)
    {
        return lang_data->sys_txt_sys;
    }
    else if (strcmp(key, "sys_txt_topic") == 0)
    {
        return lang_data->sys_txt_topic;
    }
    else if (strcmp(key, "sys_txt_auto") == 0)
    {
        return lang_data->sys_txt_auto;
    }
    else if (strcmp(key, "sys_txt_day") == 0)
    {
        return lang_data->sys_txt_day;
    }
    else if (strcmp(key, "sys_txt_night") == 0)
    {
        return lang_data->sys_txt_night;
    }
    else if (strcmp(key, "sys_txt_language") == 0)
    {
        return lang_data->sys_txt_language;
    }
    else if (strcmp(key, "sys_txt_zh") == 0)
    {
        return lang_data->sys_txt_zh;
    }
    else if (strcmp(key, "sys_txt_en") == 0)
    {
        return lang_data->sys_txt_en;
    }
    else if (strcmp(key, "sys_txt_zh_ft") == 0)
    {
        return lang_data->sys_txt_zh_ft;
    }
    else if (strcmp(key, "sys_txt_fr") == 0)
    {
        return lang_data->sys_txt_fr;
    }
    else if (strcmp(key, "sys_txt_ge") == 0)
    {
        return lang_data->sys_txt_ge;
    }
    else if (strcmp(key, "sys_txt_it") == 0)
    {
        return lang_data->sys_txt_it;
    }
    else if (strcmp(key, "sys_txt_jp") == 0)
    {
        return lang_data->sys_txt_jp;
    }
    else if (strcmp(key, "sys_txt_por") == 0)
    {
        return lang_data->sys_txt_por;
    }
    else if (strcmp(key, "sys_txt_ru") == 0)
    {
        return lang_data->sys_txt_ru;
    }
    else if (strcmp(key, "sys_txt_sp") == 0)
    {
        return lang_data->sys_txt_sp;
    }
    else if (strcmp(key, "sys_txt_pol") == 0)
    {
        return lang_data->sys_txt_pol;
    }
    else if (strcmp(key, "sys_txt_tu") == 0)
    {
        return lang_data->sys_txt_tu;
    }
    else if (strcmp(key, "sys_txt_ko") == 0)
    {
        return lang_data->sys_txt_ko;
    }
    else if (strcmp(key, "sys_txt_du") == 0)
    {
        return lang_data->sys_txt_du;
    }
    else if (strcmp(key, "sys_txt_setTime") == 0)
    {
        return lang_data->sys_txt_setTime;
    }
    else if (strcmp(key, "sys_txt_year") == 0)
    {
        return lang_data->sys_txt_year;
    }
    else if (strcmp(key, "sys_txt_mon") == 0)
    {
        return lang_data->sys_txt_mon;
    }
    else if (strcmp(key, "sys_txt_today") == 0)
    {
        return lang_data->sys_txt_today;
    }
    else if (strcmp(key, "sys_txt_hour") == 0)
    {
        return lang_data->sys_txt_hour;
    }
    else if (strcmp(key, "sys_txt_min") == 0)
    {
        return lang_data->sys_txt_min;
    }
    else if (strcmp(key, "sys_txt_backLight") == 0)
    {
        return lang_data->sys_txt_backLight;
    }
    else if (strcmp(key, "sys_txt_tireInfo") == 0)
    {
        return lang_data->sys_txt_tireInfo;
    }
    else if (strcmp(key, "sys_txt_open") == 0)
    {
        return lang_data->sys_txt_open;
    }
    else if (strcmp(key, "sys_txt_close") == 0)
    {
        return lang_data->sys_txt_close;
    }
    else if (strcmp(key, "sys_txt_btName") == 0)
    {
        return lang_data->sys_txt_btName;
    }
    else if (strcmp(key, "sys_txt_btPairDev") == 0)
    {
        return lang_data->sys_txt_btPairDev;
    }
    else if (strcmp(key, "sys_txt_about") == 0)
    {
        return lang_data->sys_txt_about;
    }
    else if (strcmp(key, "sys_txt_devModel") == 0)
    {
        return lang_data->sys_txt_devModel;
    }
    else if (strcmp(key, "sys_txt_devSerNum") == 0)
    {
        return lang_data->sys_txt_devSerNum;
    }
    else if (strcmp(key, "sys_txt_appVersion") == 0)
    {
        return lang_data->sys_txt_appVersion;
    }
    // else if (strcmp(key, "sys_txt_mcuVersion") == 0)
    // {
    //     return lang_data->sys_txt_mcuVersion;
    // }
    else if (strcmp(key, "sys_txt_hwVersion") == 0)
    {
        return lang_data->sys_txt_hwVersion;
    }
    else if (strcmp(key, "sys_txt_otaUpdata") == 0)
    {
        return lang_data->sys_txt_otaUpdata;
    }
    else if (strcmp(key, "sys_txt_logo") == 0)
    {
        return lang_data->sys_txt_logo;
    }
    else if (strcmp(key, "sys_txt_choose_path") == 0)
    {
        return lang_data->sys_txt_choose_path;
    }
    else if (strcmp(key, "sys_txt_extern_path") == 0)
    {
        return lang_data->sys_txt_extern_path;
    }
    else if (strcmp(key, "sys_txt_internal_path") == 0)
    {
        return lang_data->sys_txt_internal_path;
    }
    else if (strcmp(key, "sys_txt_choose_logo") == 0)
    {
        return lang_data->sys_txt_choose_logo;
    }
    else if (strcmp(key, "sys_txt_rebootSys") == 0)
    {
        return lang_data->sys_txt_rebootSys;
    }
    else if (strcmp(key, "sys_txt_confirm") == 0)
    {
        return lang_data->sys_txt_confirm;
    }
    else if (strcmp(key, "sys_txt_cancel") == 0)
    {
        return lang_data->sys_txt_cancel;
    }
    else if (strcmp(key, "sys_txt_rebootSys_label") == 0)
    {
        return lang_data->sys_txt_rebootSys_label;
    }
    else if (strcmp(key, "sys_txt_reFactory_label") == 0)
    {
        return lang_data->sys_txt_reFactory_label;
    }
    else if (strcmp(key, "sys_txt_reFactory") == 0)
    {
        return lang_data->sys_txt_reFactory;
    }
    else if (strcmp(key, "tire_txt_nPair") == 0)
    {
        return lang_data->tire_txt_nPair;
    }
    else if (strcmp(key, "tire_txt_tempMax") == 0)
    {
        return lang_data->tire_txt_tempMax;
    }
    else if (strcmp(key, "tire_txt_pressMax") == 0)
    {
        return lang_data->tire_txt_pressMax;
    }
    else if (strcmp(key, "tire_txt_pressMin") == 0)
    {
        return lang_data->tire_txt_pressMin;
    }    
    else if (strcmp(key, "tire_txt_pressUnit") == 0)
    {
        return lang_data->tire_txt_pressUnit;
    }
    else if (strcmp(key, "tire_txt_tempUnit") == 0)
    {
        return lang_data->tire_txt_tempUnit;
    }
    else if (strcmp(key, "tire_txt_paraReset") == 0)
    {
        return lang_data->tire_txt_paraReset;
    }
    else if (strcmp(key, "tire_txt_clearPair") == 0)
    {
        return lang_data->tire_txt_clearPair;
    }
    else if(strcmp(key, "tire_txt_clearPair1") == 0){
        return lang_data->tire_txt_clearPair1;
    }
    else if(strcmp(key, "tire_txt_paraReset1") == 0){
        return lang_data->tire_txt_paraReset1;
    }
    else if(strcmp(key, "carplay_txt_label1") == 0){
        return lang_data->carplay_txt_label1;
    }
    else if(strcmp(key, "carplay_txt_label2") == 0){
        return lang_data->carplay_txt_label2;
    }
    else if(strcmp(key, "carplay_txt_label3") == 0){
        return lang_data->carplay_txt_label3;
    }
    else if(strcmp(key, "carplay_txt_label4") == 0){
        return lang_data->carplay_txt_label4;
    }
    else if(strcmp(key, "carplay_txt_label5") == 0){
        return lang_data->carplay_txt_label5;
    }
    else if(strcmp(key, "carplay_txt_label6") == 0){
        return lang_data->carplay_txt_label6;
    }
    else if(strcmp(key, "android_txt_label1") == 0){
        return lang_data->android_txt_label1;
    }
    else if(strcmp(key, "android_txt_label2") == 0){
        return lang_data->android_txt_label2;
    }
    else if(strcmp(key, "android_txt_label3") == 0){
        return lang_data->android_txt_label3;
    }
    else if(strcmp(key, "android_txt_label4") == 0){
        return lang_data->android_txt_label4;
    }
    else if(strcmp(key, "android_txt_label5") == 0){
        return lang_data->android_txt_label5;
    }
    else if(strcmp(key, "android_txt_label6") == 0){
        return lang_data->android_txt_label6;
    }
    else if(strcmp(key, "dvr_set_txt_DVR") == 0){
        return lang_data->dvr_set_txt_DVR;
    }
    else if(strcmp(key, "dvr_set_txt_recorderSoundSet") == 0){
        return lang_data->dvr_set_txt_recorderSoundSet;
    }
    else if(strcmp(key, "dvr_set_txt_rearCameraImage") == 0){
        return lang_data->dvr_set_txt_rearCameraImage;
    }
    else if(strcmp(key, "dvr_set_txt_recorderTime") == 0){
        return lang_data->dvr_set_txt_recorderTime;
    }
    else if(strcmp(key, "dvr_set_txt_urgentRecorderTime") == 0){
        return lang_data->dvr_set_txt_urgentRecorderTime;
    }
    else if(strcmp(key, "dvr_set_txt_powerOnRecording") == 0){
        return lang_data->dvr_set_txt_powerOnRecording;
    }
    else if(strcmp(key, "dvr_set_txt_about") == 0){
        return lang_data->dvr_set_txt_about;
    }
    else if(strcmp(key, "dvr_set_txt_1min") == 0){
        return lang_data->dvr_set_txt_1min;
    }
    else if(strcmp(key, "dvr_set_txt_3min") == 0){
        return lang_data->dvr_set_txt_3min;
    }
    else if(strcmp(key, "dvr_set_txt_5min") == 0){
        return lang_data->dvr_set_txt_5min;
    }
    else if(strcmp(key, "dvr_set_txt_tfDetected") == 0){
        return lang_data->dvr_set_txt_tfDetected;
    }    
    else if(strcmp(key, "dvr_set_txt_recorderTimeMark") == 0){
        return lang_data->dvr_set_txt_recorderTimeMark;
    }  
    else if(strcmp(key, "dvr_set_txt_carSpeedMark") == 0){
        return lang_data->dvr_set_txt_carSpeedMark;
    }  
    if (strcmp(key, "Video") == 0)
        return lang_data->Video;
    if (strcmp(key, "Photo") == 0)
        return lang_data->Photo;
    if (strcmp(key, "Storage") == 0)
        return lang_data->Storage;
    if (strcmp(key, "Yes") == 0)
        return lang_data->Yes;
    if (strcmp(key, "No") == 0)
        return lang_data->No;
    if (strcmp(key, "Confirm") == 0)
        return lang_data->Confirm;
    if (strcmp(key, "Cancel") == 0)
        return lang_data->Cancel;
    if (strcmp(key, "OK") == 0)
        return lang_data->OK;
    if (strcmp(key, "Refresh") == 0)
        return lang_data->Refresh;
    if (strcmp(key, "Delete") == 0)
        return lang_data->Delete;
    if (strcmp(key, "Back") == 0)
        return lang_data->Back;
    if (strcmp(key, "Photo_Library") == 0)
        return lang_data->Photo_Library;
    if (strcmp(key, "Photo_scanning") == 0)
        return lang_data->Photo_scanning;
    if (strcmp(key, "Photo_reset") == 0)
        return lang_data->Photo_reset;
    if (strcmp(key, "No_photo_found") == 0)
        return lang_data->No_photo_found;
    if (strcmp(key, "check_contents") == 0)
        return lang_data->check_contents;
    if (strcmp(key, "Supported_formats") == 0)
        return lang_data->Supported_formats;
    if (strcmp(key, "Found") == 0)
        return lang_data->Found;
    if (strcmp(key, "photos") == 0)
        return lang_data->photos;
    if (strcmp(key, "Delete_photo_confirm") == 0)
        return lang_data->Delete_photo_confirm;
    if (strcmp(key, "Front") == 0)
        return lang_data->Front;
    if (strcmp(key, "Rear") == 0)
        return lang_data->Rear;
    if (strcmp(key, "Urgent") == 0)
        return lang_data->Urgent;
    if (strcmp(key, "Front_Videos") == 0)
        return lang_data->Front_Videos;
    if (strcmp(key, "Rear_Videos") == 0)
        return lang_data->Rear_Videos;
    if (strcmp(key, "Urgent_Videos") == 0)
        return lang_data->Urgent_Videos;
    if (strcmp(key, "Record") == 0)
        return lang_data->Record;
    if (strcmp(key, "No_video_found") == 0)
        return lang_data->No_video_found;
    if (strcmp(key, "Delete_video_confirm") == 0)
        return lang_data->Delete_video_confirm;
    if (strcmp(key, "SDCardTitle") == 0)
        return lang_data->SDCardTitle;
    if (strcmp(key, "SDCardUsed") == 0)
        return lang_data->SDCardUsed;
    if (strcmp(key, "SDCardFree") == 0)
        return lang_data->SDCardFree;
    if (strcmp(key, "SDCardFormat") == 0)
        return lang_data->SDCardFormat;
    if (strcmp(key, "SDCardNotFound") == 0)
        return lang_data->SDCardNotFound;
    if (strcmp(key, "SDCardInsertTip") == 0)
        return lang_data->SDCardInsertTip;
    if (strcmp(key, "Format_confirm") == 0)
        return lang_data->Format_confirm;
    if (strcmp(key, "Format_warning") == 0)
        return lang_data->Format_warning;
    if (strcmp(key, "Format_success") == 0)
        return lang_data->Format_success;
    if (strcmp(key, "Format_failed") == 0)
        return lang_data->Format_failed;
    if (strcmp(key, "ota_title") == 0)
        return lang_data->ota_title;
    if (strcmp(key, "ota_version_info") == 0)
        return lang_data->ota_version_info;
    if (strcmp(key, "ota_note") == 0)
        return lang_data->ota_note;
    if (strcmp(key, "ota_check_updates") == 0)
        return lang_data->ota_check_updates;
    if (strcmp(key, "ota_current_version") == 0)
        return lang_data->ota_current_version;
    if (strcmp(key, "ota_file_download_err") == 0)
        return lang_data->ota_file_download_err;
    if (strcmp(key, "ota_system_upgrade") == 0)
        return lang_data->ota_system_upgrade;
    if (strcmp(key, "ota_confirm_upgrade") == 0)
        return lang_data->ota_confirm_upgrade;
    if (strcmp(key, "ota_network_unavailable") == 0)
        return lang_data->ota_network_unavailable;
    if (strcmp(key, "ota_check_network") == 0)
        return lang_data->ota_check_network;
    if (strcmp(key, "ota_download_failed") == 0)
        return lang_data->ota_download_failed;
    if (strcmp(key, "ota_download_version_file_failed") == 0)
        return lang_data->ota_download_version_file_failed;
    if (strcmp(key, "ota_invalid_version") == 0)
        return lang_data->ota_invalid_version;
    if (strcmp(key, "ota_no_need_upgrade") == 0)
        return lang_data->ota_no_need_upgrade;
    if (strcmp(key, "ota_up_to_date") == 0)
        return lang_data->ota_up_to_date;
    if (strcmp(key, "ota_update_available") == 0)
        return lang_data->ota_update_available;
    if (strcmp(key, "ota_update_question") == 0)
        return lang_data->ota_update_question;
    if (strcmp(key, "ota_downloading") == 0)
        return lang_data->ota_downloading;
    if (strcmp(key, "ota_download_completed") == 0)
        return lang_data->ota_download_completed;
    if (strcmp(key, "ota_yes") == 0)
        return lang_data->ota_yes;
    if (strcmp(key, "ota_no") == 0)
        return lang_data->ota_no;
    if (strcmp(key, "wifi_title") == 0)
        return lang_data->wifi_title;
    if (strcmp(key, "wifi_connecting") == 0)
        return lang_data->wifi_connecting;
    if (strcmp(key, "wifi_scanning") == 0)
        return lang_data->wifi_scanning;
    if (strcmp(key, "wifi_status_scanning") == 0)
        return lang_data->wifi_status_scanning;
    if (strcmp(key, "wifi_status_connecting") == 0)
        return lang_data->wifi_status_connecting;
    if (strcmp(key, "wifi_obtaining_ip") == 0)
        return lang_data->wifi_obtaining_ip;
    if (strcmp(key, "wifi_connected") == 0)
        return lang_data->wifi_connected;
    if (strcmp(key, "wifi_failed") == 0)
        return lang_data->wifi_failed;
    if (strcmp(key, "wifi_connected_success") == 0)
        return lang_data->wifi_connected_success;
    if (strcmp(key, "wifi_check_password_or_network") == 0)
        return lang_data->wifi_check_password_or_network;
    if (strcmp(key, "wifi_connect") == 0)
        return lang_data->wifi_connect;
    if (strcmp(key, "wifi_cancel") == 0)
        return lang_data->wifi_cancel;
    if (strcmp(key, "wifi_ok") == 0)
        return lang_data->wifi_ok;
	if (strcmp(key, "dvr_txt_recording") == 0)
		return lang_data->dvr_txt_recording;
	if (strcmp(key, "dvr_txt_rear") == 0)
		return lang_data->dvr_txt_rear;
	if (strcmp(key, "dvr_txt_front") == 0)
		return lang_data->dvr_txt_front;
        const char *tmp_str = "unknown";
        return tmp_str; // 未找到对应字符串
}


