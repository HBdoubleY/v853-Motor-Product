/**
 * @file WIFIConnect.c
 * @brief WiFi连接模块实现
 * 
 * 实现WiFi扫描、连接和UI显示功能
 */

#include "WIFIConnect.h"
#include "OTA.h"
#include "ota_theme.h"
#include "../i18n/i18n.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/*============================================================================
 * 常量与宏定义
 *============================================================================*/

/* 统一 WiFi 接口与配置路径，改为使用 wlan1 做 STA 连接 */
#define WIFI_IFACE          "wlan1"

/*============================================================================
 * 全局变量定义
 *============================================================================*/

/* WiFi UI全局实例 */
wifi_t g_wifi_ui = {0};

/* 主界面 WiFi 图标用：缓存连接状态，由后台线程更新，UI 仅读此变量（非阻塞） */
static volatile bool s_wifi_connected_cached = false;
static pthread_t s_wifi_poll_tid;
static volatile bool s_wifi_poll_running = false;

/*============================================================================
 * 私有函数声明
 *============================================================================*/

static char *run_cmd(const char *cmd);
static bool prepare_wifi_env(void);
static bool close_wifi_connection(void);
static char **scan_networks(int *count);
static void connect_sync(struct connect_arg *ca);
static bool wifi_query_status(char *ssid_buf, size_t len);
static void refresh_add_list_internal(void);

/* 事件回调函数 */
static void wifi_connect_event_cb(lv_event_t *e);
static void refresh_btn_event_cb(lv_event_t *e);
static void list_event_cb(lv_event_t *e);
static void ask_dialog_yes_event_cb(lv_event_t *e);
static void ask_dialog_no_event_cb(lv_event_t *e);
static void password_event_cb(lv_event_t *e);
static void mask_event_cb(lv_event_t *e);
static void result_popup_btn_cb(lv_event_t *e);
static void result_popup_mask_cb(lv_event_t *e);

/* UI辅助函数 */
static void create_ask_dialog(lv_obj_t *parent, const char *txt);
static void show_format_sd_dialog(lv_obj_t *parent, const char *txt);
static lv_obj_t* create_styled_status_popup(lv_obj_t **out_label, lv_obj_t **out_spinner);
static void show_styled_result_popup(bool success);
static void apply_keyboard_daytime_style(lv_obj_t *kb);

/*============================================================================
 * 系统命令执行函数
 *============================================================================*/

/**
 * @brief 执行系统命令并获取输出
 * @param cmd 命令字符串
 * @return 命令输出（需要调用者释放），失败返回NULL
 */
static char *run_cmd(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    char *out = NULL;
    size_t len = 0;
    char buf[256];
    
    while (fgets(buf, sizeof(buf), fp)) {
        size_t l = strlen(buf);
        char *n = realloc(out, len + l + 1);
        if (!n) { 
            free(out); 
            pclose(fp); 
            return NULL; 
        }
        out = n;
        memcpy(out + len, buf, l);
        len += l;
        out[len] = '\0';
    }
    pclose(fp);
    return out;
}

/*============================================================================
 * WiFi系统控制函数
 *============================================================================*/

/**
 * @brief 准备WiFi环境
 * @return 成功返回true
 */
static bool prepare_wifi_env(void) {
    printf("[WiFi] Preparing WiFi environment...\n");

    /* 不再干预全局 wpa_supplicant，仅确保 wlan1 处于 up 状态并触发一次扫描 */
    system("ifconfig " WIFI_IFACE " up 2>/dev/null || true");
    system("wpa_cli -i " WIFI_IFACE " scan >/dev/null 2>&1");

    printf("[WiFi] WiFi environment prepared for " WIFI_IFACE "\n");
    return true;
}

/**
 * @brief 关闭WiFi连接
 * @return 成功返回true
 */
static bool close_wifi_connection(void) {
    printf("[WiFi] Closing WiFi connection...\n");

    /* 仅断开当前 wlan1 连接，不停止全局 wpa_supplicant 服务 */
    system("wpa_cli -i " WIFI_IFACE " disconnect >/dev/null 2>&1");
    system("wpa_cli -i " WIFI_IFACE " remove_network all >/dev/null 2>&1");

    s_wifi_connected_cached = false;

    printf("[WiFi] WiFi connection on " WIFI_IFACE " closed\n");
    return true;
}

/* CarPlay AP 使用开发板配置文件：/etc/hostapd.conf、/etc/udhcpd.conf（与 libzlink 文档一致）；
 * udhcpd.conf 中 opt router 为 192.168.1.2，故 wlan0 设为 192.168.1.2。 */
#define CARPLAY_AP_IP          "192.168.1.2"
#define HOSTAPD_CONF_PATH     "/etc/hostapd.conf"
#define UDHCPD_CONF_PATH      "/etc/udhcpd.conf"
#define UDHCPD_LEASES_PATH    "/var/lib/misc/udhcpd.leases"

/**
 * @brief 将 wlan0 切到 CarPlay 热点模式（AP）
 * 说明：CarPlay AP 由启动脚本负责配置；WiFi 页面不再切换 wlan0 模式，保留此函数避免外部链接错误。
 */
void wlan0_switch_to_carplay_ap(void) {
    (void)CARPLAY_AP_IP;
    (void)HOSTAPD_CONF_PATH;
    (void)UDHCPD_CONF_PATH;
    (void)UDHCPD_LEASES_PATH;
    printf("[WiFi] wlan0_switch_to_carplay_ap() is deprecated in WiFi UI, handled by run.config\n");
}

/**
 * @brief 将 wlan0 切到 STA 模式供 OTA WiFi 使用
 * 说明：OTA WiFi 已改为使用 wlan1，保留空实现以兼容旧接口。
 */
void wlan0_ensure_sta_for_ota(void) {
    printf("[WiFi] wlan0_ensure_sta_for_ota() is deprecated, OTA WiFi now uses " WIFI_IFACE "\n");
}

/**
 * @brief 扫描可用的WiFi网络
 * @param count 输出参数，返回网络数量
 * @return 网络SSID数组（需要调用者释放）
 */
static char **scan_networks(int *count) {
    *count = 0;

    system("wpa_cli -i " WIFI_IFACE " scan >/dev/null 2>&1");
    char *out = run_cmd("wpa_cli -i " WIFI_IFACE " scan_results 2>/dev/null");
    if (!out) return NULL;
    
    char *save = NULL;
    char *line = strtok_r(out, "\n", &save);
    int idx = 0;
    char **list = NULL;
    
    while (line) {
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0') { 
            line = strtok_r(NULL, "\n", &save); 
            continue; 
        }
        
        char bssid[64] = {0}; 
        int freq = 0, signal = 0; 
        char flags[256] = {0}; 
        int offset = 0;
        
        int matched = sscanf(line, "%63s %d %d %255s %n", bssid, &freq, &signal, flags, &offset);
        
        if (matched >= 4 && strchr(bssid, ':')) {
            const char *ssid = line + offset; 
            while (*ssid == ' ' || *ssid == '\t') ssid++;
            
            char *ss = NULL;
            if (ssid && *ssid) { 
                ss = strdup(ssid); 
                char *end = ss + strlen(ss) - 1; 
                while (end > ss && (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t')) 
                    *end-- = '\0'; 
            } else {
                ss = strdup("[Hidden Network]");
            }
            
            if (ss) { 
                char **n = realloc(list, sizeof(char*) * (idx + 1)); 
                if (!n) { 
                    free(ss); 
                    break; 
                } 
                list = n; 
                list[idx++] = ss; 
            }
        }
        line = strtok_r(NULL, "\n", &save);
    }
    
    free(out);
    *count = idx;
    
    printf("[WiFi] Scanned %d networks\n", idx);
    return list;
}

/*============================================================================
 * 弹窗UI函数
 *============================================================================*/

/**
 * @brief 结果弹窗按钮回调
 */
static void result_popup_btn_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *popup = lv_obj_get_parent(target);
    lv_obj_t *mask = lv_obj_get_parent(popup);
    lv_obj_del(mask);
}

/**
 * @brief 结果弹窗遮罩回调
 */
static void result_popup_mask_cb(lv_event_t *e) {
    lv_obj_t *mask = lv_event_get_target(e);
    lv_obj_t *indev_obj = lv_indev_get_obj_act();
    if (indev_obj == mask) {
        lv_obj_del(mask);
    }
}

/**
 * @brief 创建连接状态弹窗（带加载动画）
 * @param out_label 输出状态标签指针
 * @param out_spinner 输出加载动画指针
 * @return 遮罩层对象
 */
static lv_obj_t* create_styled_status_popup(lv_obj_t **out_label, lv_obj_t **out_spinner) {
    /* 创建半透明遮罩层 */
    lv_obj_t *mask = lv_obj_create(lv_scr_act());
    lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(mask, TH_BG_OVERLAY, 0);
    lv_obj_set_style_bg_opa(mask, TH_OVERLAY_OPA, 0);
    lv_obj_set_style_border_width(mask, 0, 0);
    lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(mask);

    /* 创建弹窗主体 */
    lv_obj_t *popup = lv_obj_create(mask);
    lv_obj_set_size(popup, MODAL_WIDTH_MD, MODAL_HEIGHT_MD);
    lv_obj_center(popup);
    lv_obj_set_style_bg_color(popup, TH_BG_MODAL, 0);
    lv_obj_set_style_bg_opa(popup, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(popup, RADIUS_XL, 0);
    lv_obj_set_style_border_width(popup, 0, 0);
    lv_obj_set_style_shadow_width(popup, SHADOW_WIDTH_LG, 0);
    lv_obj_set_style_shadow_color(popup, TH_SHADOW, 0);
    lv_obj_set_style_shadow_opa(popup, TH_SHADOW_OPA, 0);
    lv_obj_set_style_shadow_spread(popup, 2, 0);
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

    /* 创建加载动画 */
    lv_obj_t *spinner = lv_spinner_create(popup, 1000, 60);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_arc_color(spinner, TH_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, TH_PROGRESS_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 6, LV_PART_MAIN);

    /* 标题标签 */
    lv_obj_t *title = lv_label_create(popup);
    lv_label_set_text(title, get_string_for_language(ota_language_setting, "wifi_connecting"));
    lv_obj_set_style_text_font(title, &v853Font_WIFI_24, 0);
    lv_obj_set_style_text_color(title, TH_TEXT_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 115);

    /* 状态标签 */
    lv_obj_t *status_lbl = lv_label_create(popup);
    lv_label_set_text(status_lbl, get_string_for_language(ota_language_setting, "wifi_connecting"));
    lv_obj_set_style_text_font(status_lbl, &v853Font_WIFI_18, 0);
    lv_obj_set_style_text_color(status_lbl, TH_TEXT_SECONDARY, 0);
    lv_obj_align(status_lbl, LV_ALIGN_CENTER, 0, 50);

    /* 进度条 */
    lv_obj_t *bar = lv_bar_create(popup);
    lv_obj_set_size(bar, 360, 8);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(bar, TH_PROGRESS_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, TH_PROGRESS_FILL, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, RADIUS_SM, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, RADIUS_SM, LV_PART_INDICATOR);
    lv_bar_set_range(bar, 0, 20);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_user_data(popup, bar);

    *out_label = status_lbl;
    *out_spinner = spinner;
    return mask;
}

/**
 * @brief 显示连接结果弹窗
 * @param success 是否成功
 */
static void show_styled_result_popup(bool success) {
    /* 创建半透明遮罩层 */
    lv_obj_t *mask = lv_obj_create(lv_scr_act());
    lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(mask, TH_BG_OVERLAY, 0);
    lv_obj_set_style_bg_opa(mask, TH_OVERLAY_OPA, 0);
    lv_obj_set_style_border_width(mask, 0, 0);
    lv_obj_clear_flag(mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(mask);

    /* 创建弹窗主体 */
    lv_obj_t *popup = lv_obj_create(mask);
    lv_obj_set_size(popup, MODAL_WIDTH_SM, MODAL_HEIGHT_LG);
    lv_obj_center(popup);
    lv_obj_set_style_bg_color(popup, TH_BG_MODAL, 0);
    lv_obj_set_style_bg_opa(popup, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(popup, RADIUS_XL, 0);
    lv_obj_set_style_border_width(popup, 0, 0);
    lv_obj_set_style_shadow_width(popup, SHADOW_WIDTH_LG, 0);
    lv_obj_set_style_shadow_color(popup, TH_SHADOW, 0);
    lv_obj_set_style_shadow_opa(popup, TH_SHADOW_OPA, 0);
    lv_obj_set_style_shadow_spread(popup, 2, 0);
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);

    /* 状态图标背景圆 */
    lv_obj_t *icon_bg = lv_obj_create(popup);
    lv_obj_set_size(icon_bg, 80, 80);
    lv_obj_align(icon_bg, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_radius(icon_bg, 40, 0);
    lv_obj_set_style_border_width(icon_bg, 0, 0);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(icon_bg, success ? TH_SUCCESS_BG : TH_ERROR_BG, 0);

    /* 状态图标 */
    lv_obj_t *icon = lv_label_create(icon_bg);
    lv_obj_set_style_text_font(icon, &v853Font_WIFI_42, 0);
    lv_label_set_text(icon, success ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(icon, success ? TH_SUCCESS : TH_ERROR, 0);
    lv_obj_center(icon);

    /* 结果标题 */
    lv_obj_t *title = lv_label_create(popup);
    lv_obj_set_style_text_font(title, &v853Font_WIFI_28, 0);
    lv_label_set_text(title, success ? get_string_for_language(ota_language_setting, "wifi_connected") : get_string_for_language(ota_language_setting, "wifi_failed"));
    lv_obj_set_style_text_color(title, success ? TH_SUCCESS : TH_ERROR, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -10);

    /* 描述文字 */
    lv_obj_t *desc = lv_label_create(popup);
    lv_obj_set_style_text_font(desc, &v853Font_WIFI_18, 0);
    lv_obj_set_style_text_color(desc, TH_TEXT_SECONDARY, 0);
    lv_label_set_text(desc, success ? get_string_for_language(ota_language_setting, "wifi_connected_success") : get_string_for_language(ota_language_setting, "wifi_check_password_or_network"));
    lv_obj_align(desc, LV_ALIGN_CENTER, 0, 30);

    /* 确认按钮 */
    lv_obj_t *btn = lv_btn_create(popup);
    lv_obj_set_size(btn, BTN_WIDTH_MD, BTN_HEIGHT_MD);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_style_radius(btn, RADIUS_ROUND, 0);
    lv_obj_set_style_bg_color(btn, success ? TH_SUCCESS : TH_PRIMARY, 0);
    lv_obj_set_style_shadow_width(btn, SHADOW_WIDTH_SM, 0);
    lv_obj_set_style_shadow_color(btn, success ? TH_SUCCESS : TH_PRIMARY, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
    
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, get_string_for_language(ota_language_setting, "wifi_ok"));
    lv_obj_set_style_text_font(btn_label, &v853Font_WIFI_20, 0);
    lv_obj_set_style_text_color(btn_label, TH_TEXT_INVERSE, 0);
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(btn, result_popup_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(mask, result_popup_mask_cb, LV_EVENT_CLICKED, NULL);
}

/*============================================================================
 * WiFi连接流程
 *============================================================================*/

/**
 * @brief 同步连接WiFi（阻塞式）
 * @param ca 连接参数（函数会释放）
 */
static void connect_sync(struct connect_arg *ca) {
    printf("[WiFi] Connecting to SSID: %s\n", ca->ssid);
    
    /* 移除已有网络配置 */
    system("wpa_cli -i " WIFI_IFACE " remove_network all >/dev/null 2>&1");
    
    /* 添加新网络 */
    char cmd[512];
    char *id_out = run_cmd("wpa_cli -i " WIFI_IFACE " add_network 2>/dev/null | tail -n1");
    int netid = -1;
    if (id_out) { 
        netid = atoi(id_out); 
        free(id_out); 
    }
    if (netid < 0) netid = 0;
    
    snprintf(cmd, sizeof(cmd), "wpa_cli -i " WIFI_IFACE " set_network %d ssid '\"%s\"' >/dev/null 2>&1", netid, ca->ssid);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "wpa_cli -i " WIFI_IFACE " set_network %d psk '\"%s\"' >/dev/null 2>&1", netid, ca->psk);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "wpa_cli -i " WIFI_IFACE " set_network %d key_mgmt WPA-PSK >/dev/null 2>&1", netid);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "wpa_cli -i " WIFI_IFACE " enable_network %d >/dev/null 2>&1", netid);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "wpa_cli -i " WIFI_IFACE " select_network %d >/dev/null 2>&1", netid);
    system(cmd);

    /* 创建状态弹窗 */
    lv_obj_t *status_lbl = NULL;
    lv_obj_t *spinner = NULL;
    lv_obj_t *status_mask = create_styled_status_popup(&status_lbl, &spinner);
    lv_obj_t *status_popup = lv_obj_get_child(status_mask, 0);
    lv_obj_t *progress_bar = (lv_obj_t *)lv_obj_get_user_data(status_popup);

    /* 轮询连接状态 */
    int connected = 0;
    for (int i = 0; i < 20; i++) {
        char *st = run_cmd("wpa_cli -i " WIFI_IFACE " status 2>/dev/null | grep \"wpa_state\" | cut -d= -f2");
        if (st) {
            /* 去除换行符 */
            char *s = st;
            while (*s && (*s == '\r' || *s == '\n' || *s == ' ' || *s == '\t')) s++;
            char *end = st + strlen(st) - 1;
            while (end >= st && (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t')) 
                *end-- = '\0';
            
            if (strstr(st, "COMPLETED")) { 
                connected = 1;
                s_wifi_connected_cached = true;
                free(st); 
                break; 
            }
            
            lv_label_set_text_fmt(status_lbl, get_string_for_language(ota_language_setting, "wifi_status_connecting"), st, i + 1);
            printf("[WiFi] Poll %d: wpa_state='%s'\n", i + 1, st);
            free(st);
        } else {
            lv_label_set_text_fmt(status_lbl, get_string_for_language(ota_language_setting, "wifi_status_scanning"), i + 1);
        }
        
        lv_bar_set_value(progress_bar, i + 1, LV_ANIM_ON);
        lv_task_handler();
        usleep(500000);
    }

    if (connected) {
        /* 隐藏加载动画，更新状态 */
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(status_lbl, get_string_for_language(ota_language_setting, "wifi_obtaining_ip"));
        lv_bar_set_value(progress_bar, 20, LV_ANIM_ON);
        lv_task_handler();
        
        system("udhcpc -i " WIFI_IFACE " -n -q -t 5 >/dev/null 2>&1 || true");
        printf("[WiFi] DHCP completed on " WIFI_IFACE "\n");
    }

    /* 删除状态弹窗 */
    if (status_mask) lv_obj_del(status_mask);

    /* 显示结果弹窗 */
    show_styled_result_popup(connected);

    /* 清理资源 */
    free(ca->ssid);
    free(ca->psk);
    free(ca);
}

/**
 * @brief 查询当前 WiFi 连接状态
 * @param ssid_buf 输出 SSID 缓冲区（可为 NULL）
 * @param len 缓冲区长度
 * @return 已连接返回 true，否则返回 false
 */
static bool wifi_query_status(char *ssid_buf, size_t len) {
    bool connected = false;

    if (ssid_buf && len > 0) {
        ssid_buf[0] = '\0';
    }

    char *status = run_cmd("wpa_cli -i " WIFI_IFACE " status 2>/dev/null");
    if (!status) {
        return false;
    }

    char *save = NULL;
    char *line = strtok_r(status, "\n", &save);
    while (line) {
        while (*line == ' ' || *line == '\t') line++;

        if (strncmp(line, "wpa_state=", 10) == 0) {
            if (strstr(line + 10, "COMPLETED") != NULL) {
                connected = true;
            }
        } else if (strncmp(line, "ssid=", 5) == 0) {
            if (ssid_buf && len > 0) {
                strncpy(ssid_buf, line + 5, len - 1);
                ssid_buf[len - 1] = '\0';
            }
        }

        line = strtok_r(NULL, "\n", &save);
    }

    free(status);
    return connected;
}

/*============================================================================
 * 对话框相关回调
 *============================================================================*/

/**
 * @brief 取消按钮回调
 */
static void ask_dialog_no_event_cb(lv_event_t *e) {
    if (g_wifi_ui.mask_layer_dialog) {
        lv_obj_del(g_wifi_ui.mask_layer_dialog);
        g_wifi_ui.mask_layer_dialog = NULL;
    }
    if (g_wifi_ui.kb) {
        lv_obj_del(g_wifi_ui.kb);
        g_wifi_ui.kb = NULL;
    }
}

/**
 * @brief 确认按钮回调
 */
static void ask_dialog_yes_event_cb(lv_event_t *e) {
    const char *ssid = (const char *)lv_obj_get_user_data(g_wifi_ui.ssid_label);
    if (!ssid) return;
    
    const char *pw = lv_textarea_get_text(g_wifi_ui.password_ta);
    printf("[WiFi] SSID: %s, Password: %s\n", ssid, pw);

    struct connect_arg *ca = calloc(1, sizeof(*ca));
    ca->ssid = strdup(ssid);
    ca->psk = strdup(pw);

    if (g_wifi_ui.mask_layer_dialog) {
        lv_obj_del(g_wifi_ui.mask_layer_dialog);
        g_wifi_ui.mask_layer_dialog = NULL;
    }
    if (g_wifi_ui.kb) {
        lv_obj_del(g_wifi_ui.kb);
        g_wifi_ui.kb = NULL;
    }

    connect_sync(ca);
}

/**
 * @brief 密码输入框点击回调
 */
static void password_event_cb(lv_event_t *e) {
    if (g_wifi_ui.kb) {
        lv_obj_clear_flag(g_wifi_ui.kb, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_align(g_wifi_ui.dialog_bg, LV_ALIGN_CENTER, 0, -100);
}

/**
 * @brief 遮罩层点击回调
 */
static void mask_event_cb(lv_event_t *e) {
    if (g_wifi_ui.kb && !lv_obj_has_flag(g_wifi_ui.kb, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(g_wifi_ui.kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_center(g_wifi_ui.dialog_bg);
    }
}

/**
 * @brief 应用键盘白天风格样式（用户偏好）
 * @param kb 键盘对象
 * 
 * 无论当前是白天还是黑夜主题，键盘始终使用白天风格
 * 使用较大的按键便于触摸交互
 */
static void apply_keyboard_daytime_style(lv_obj_t *kb) {
    if (!kb) return;
    
    /* 键盘整体样式 - 浅灰背景 */
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xF5F5F5), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(kb, RADIUS_LG, LV_PART_MAIN);
    lv_obj_set_style_border_width(kb, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(kb, lv_color_hex(0xE0E0E0), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(kb, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(kb, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(kb, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(kb, PADDING_SM, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(kb, RADIUS_SM, LV_PART_MAIN);
    
    /* 按键样式 - 白色按键带微阴影 */
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(kb, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_radius(kb, RADIUS_MD, LV_PART_ITEMS);
    lv_obj_set_style_border_width(kb, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(kb, lv_color_hex(0xDDDDDD), LV_PART_ITEMS);
    lv_obj_set_style_shadow_width(kb, 4, LV_PART_ITEMS);
    lv_obj_set_style_shadow_color(kb, lv_color_hex(0x000000), LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(kb, LV_OPA_10, LV_PART_ITEMS);
    lv_obj_set_style_text_color(kb, lv_color_hex(0x333333), LV_PART_ITEMS);
    lv_obj_set_style_text_font(kb, &v853Font_WIFI_20, LV_PART_ITEMS);
    
    /* 按下状态 - 浅蓝色 */
    lv_obj_set_style_bg_color(kb, lv_color_hex(0xE3F2FD), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(kb, 0, LV_PART_ITEMS | LV_STATE_PRESSED);
}

/**
 * @brief 创建密码输入对话框
 * @param parent 父对象
 * @param txt SSID名称
 */
static void create_ask_dialog(lv_obj_t *parent, const char *txt) {
    g_wifi_ui.dialog_bg = lv_obj_create(parent);
    lv_obj_set_size(g_wifi_ui.dialog_bg, LV_PCT(40), LV_PCT(40));
    lv_obj_center(g_wifi_ui.dialog_bg);
    lv_obj_set_style_bg_color(g_wifi_ui.dialog_bg, TH_BG_MODAL, 0);
    lv_obj_set_style_radius(g_wifi_ui.dialog_bg, RADIUS_MD, 0);
    lv_obj_set_style_pad_all(g_wifi_ui.dialog_bg, PADDING_XL, 0);
    lv_obj_set_style_border_width(g_wifi_ui.dialog_bg, BORDER_WIDTH_NORMAL, 0);
    lv_obj_set_style_border_color(g_wifi_ui.dialog_bg, TH_BORDER, 0);
    lv_obj_clear_flag(g_wifi_ui.dialog_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* SSID标签 */
    g_wifi_ui.ssid_label = lv_label_create(g_wifi_ui.dialog_bg);
    lv_label_set_text(g_wifi_ui.ssid_label, txt);
    lv_obj_set_user_data(g_wifi_ui.ssid_label, (void*)txt);
    lv_obj_set_style_text_font(g_wifi_ui.ssid_label, &lv_font_montserrat_30, 0);
    lv_obj_set_width(g_wifi_ui.ssid_label, LV_PCT(100));
    lv_obj_set_style_text_align(g_wifi_ui.ssid_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(g_wifi_ui.ssid_label, TH_TEXT_PRIMARY, 0);
    lv_obj_align(g_wifi_ui.ssid_label, LV_ALIGN_TOP_MID, 0, LV_PCT(10));

    /* 密码输入框 */
    g_wifi_ui.password_ta = lv_textarea_create(g_wifi_ui.dialog_bg);
    lv_obj_set_width(g_wifi_ui.password_ta, 460);
    lv_obj_set_height(g_wifi_ui.password_ta, 50);
    lv_textarea_set_password_mode(g_wifi_ui.password_ta, false);
    lv_obj_clear_flag(g_wifi_ui.password_ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(g_wifi_ui.password_ta, TH_BG_INPUT, 0);
    lv_obj_set_style_text_color(g_wifi_ui.password_ta, TH_TEXT_PRIMARY, 0);
    lv_obj_align(g_wifi_ui.password_ta, LV_ALIGN_CENTER, 0, -10);
    lv_obj_add_event_cb(g_wifi_ui.password_ta, password_event_cb, LV_EVENT_CLICKED, NULL);

    /* 确认按钮 */
    lv_obj_t *yes_btn = lv_btn_create(g_wifi_ui.dialog_bg);
    lv_obj_set_size(yes_btn, BTN_WIDTH_SM, BTN_HEIGHT_SM);
    lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(yes_btn, TH_SUCCESS, 0);
    lv_obj_set_style_radius(yes_btn, RADIUS_SM, 0);
    lv_obj_add_event_cb(yes_btn, ask_dialog_yes_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *yes_label = lv_label_create(yes_btn);
    lv_label_set_text(yes_label, get_string_for_language(ota_language_setting, "wifi_connect"));
    lv_obj_set_style_text_font(yes_label, &v853Font_WIFI_16, 0);
    lv_obj_set_style_text_color(yes_label, TH_TEXT_INVERSE, 0);
    lv_obj_center(yes_label);

    /* 取消按钮 */
    lv_obj_t *no_btn = lv_btn_create(g_wifi_ui.dialog_bg);
    lv_obj_set_size(no_btn, BTN_WIDTH_SM, BTN_HEIGHT_SM);
    lv_obj_align(no_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(no_btn, TH_ERROR, 0);
    lv_obj_set_style_radius(no_btn, RADIUS_SM, 0);
    lv_obj_add_event_cb(no_btn, ask_dialog_no_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *no_label = lv_label_create(no_btn);
    lv_label_set_text(no_label, get_string_for_language(ota_language_setting, "wifi_cancel"));
    lv_obj_set_style_text_font(no_label, &v853Font_WIFI_16, 0);
    lv_obj_set_style_text_color(no_label, TH_TEXT_INVERSE, 0);
    lv_obj_center(no_label);

    /* 创建键盘 - 始终使用白天风格（用户偏好） */
    g_wifi_ui.kb = lv_keyboard_create(lv_scr_act());
    lv_obj_set_size(g_wifi_ui.kb, LV_PCT(KEYBOARD_WIDTH_PCT), LV_PCT(KEYBOARD_HEIGHT_PCT));
    lv_obj_align(g_wifi_ui.kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(g_wifi_ui.kb, g_wifi_ui.password_ta);
    lv_obj_add_flag(g_wifi_ui.kb, LV_OBJ_FLAG_HIDDEN);
    
    /* 应用白天风格 */
    apply_keyboard_daytime_style(g_wifi_ui.kb);
}

/**
 * @brief 显示密码输入对话框
 * @param parent 父对象
 * @param txt SSID名称
 */
static void show_format_sd_dialog(lv_obj_t *parent, const char *txt) {
    /* 如果已存在对话框，先删除 */
    if (g_wifi_ui.mask_layer_dialog) {
        lv_obj_del(g_wifi_ui.mask_layer_dialog);
        g_wifi_ui.mask_layer_dialog = NULL;
    }
    
    g_wifi_ui.mask_layer_dialog = lv_obj_create(parent);
    lv_obj_set_size(g_wifi_ui.mask_layer_dialog, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_wifi_ui.mask_layer_dialog, TH_BG_OVERLAY, 0);
    lv_obj_set_style_bg_opa(g_wifi_ui.mask_layer_dialog, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(g_wifi_ui.mask_layer_dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(g_wifi_ui.mask_layer_dialog);
    lv_obj_add_event_cb(g_wifi_ui.mask_layer_dialog, mask_event_cb, LV_EVENT_CLICKED, NULL);

    create_ask_dialog(g_wifi_ui.mask_layer_dialog, txt);
}

/*============================================================================
 * 列表相关回调
 *============================================================================*/

/**
 * @brief WiFi列表项点击回调
 */
static void list_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_list_get_btn_text(g_wifi_ui.list, btn);
    if (!txt) return;
    
    printf("[WiFi] Selected network: %s\n", txt);
    show_format_sd_dialog(lv_scr_act(), txt);
}

/**
 * @brief 刷新WiFi列表（内部函数）
 */
static void refresh_add_list_internal(void) {
    int cnt = 0;
    char **nets = scan_networks(&cnt);
    
    if (nets && cnt > 0) {
        for (int i = 0; i < cnt; i++) {
            /* 跳过隐藏网络 */
            if (nets[i] && strcmp(nets[i], "[Hidden Network]") == 0) { 
                free(nets[i]); 
                continue; 
            }
            
            lv_obj_t *list_btn = lv_list_add_btn(g_wifi_ui.list, NULL, nets[i]);
            if (list_btn) {
                lv_obj_set_style_bg_color(list_btn, TH_BG_LIST, 0);
                lv_obj_set_style_bg_opa(list_btn, LV_OPA_100, 0);
                lv_obj_set_style_text_color(list_btn, TH_TEXT_PRIMARY, 0);
                lv_obj_set_style_text_font(list_btn, &lv_font_montserrat_30, 0);
                lv_obj_set_style_radius(list_btn, 0, 0);
                lv_obj_set_style_border_width(list_btn, 1, 0);
                lv_obj_set_style_border_color(list_btn, TH_DIVIDER, 0);
                lv_obj_set_style_pad_all(list_btn, PADDING_SM, 0);
                lv_obj_set_style_bg_color(list_btn, TH_BTN_PRESSED, LV_STATE_PRESSED);
                lv_obj_set_style_bg_opa(list_btn, LV_OPA_40, LV_STATE_PRESSED);
                lv_obj_add_event_cb(list_btn, list_event_cb, LV_EVENT_CLICKED, NULL);
            }
            free(nets[i]);
        }
        free(nets);
    }
}

/*============================================================================
 * WiFi开关和刷新按钮回调
 *============================================================================*/

/**
 * @brief WiFi开关按钮回调
 */
static void wifi_connect_event_cb(lv_event_t *e) {
    if (!g_wifi_ui.currently_connected) {
        printf("[WiFi] Enabling WiFi list on " WIFI_IFACE "...\n");

        /* 开关从关 → 开：仅控制列表显示与扫描，不关闭底层 wpa_supplicant */
        lv_obj_clear_flag(g_wifi_ui.connect_btn_yes, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_wifi_ui.connect_btn_no, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(g_wifi_ui.list_cont, LV_OBJ_FLAG_HIDDEN);

        if (g_wifi_ui.list) lv_obj_clean(g_wifi_ui.list);

        prepare_wifi_env();
        refresh_add_list_internal();

        char ssid[128] = {0};
        bool linked = wifi_query_status(ssid, sizeof(ssid));
        if (g_wifi_ui.current_ssid_label) {
            if (linked && ssid[0] != '\0') {
                lv_label_set_text(g_wifi_ui.current_ssid_label, ssid);
            } else {
                lv_label_set_text(g_wifi_ui.current_ssid_label, "未连接");
            }
        }

        g_wifi_ui.currently_connected = true;
    } else {
        printf("[WiFi] Disabling WiFi list and disconnecting " WIFI_IFACE "...\n");

        lv_obj_clear_flag(g_wifi_ui.connect_btn_no, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_wifi_ui.connect_btn_yes, LV_OBJ_FLAG_HIDDEN);

        if (g_wifi_ui.list) lv_obj_clean(g_wifi_ui.list);
        lv_obj_add_flag(g_wifi_ui.list_cont, LV_OBJ_FLAG_HIDDEN);

        close_wifi_connection();

        if (g_wifi_ui.current_ssid_label) {
            lv_label_set_text(g_wifi_ui.current_ssid_label, "未连接");
        }

        g_wifi_ui.currently_connected = false;
    }
}

/**
 * @brief 刷新按钮回调
 */
static void refresh_btn_event_cb(lv_event_t *e) {
    if (!g_wifi_ui.currently_connected) return;
    
    printf("[WiFi] Refreshing network list...\n");
    
    if (g_wifi_ui.list) lv_obj_clean(g_wifi_ui.list);
    refresh_add_list_internal();

    /* 同步当前连接状态显示 */
    char ssid[128] = {0};
    bool linked = wifi_query_status(ssid, sizeof(ssid));
    if (g_wifi_ui.current_ssid_label) {
        if (linked && ssid[0] != '\0') {
            lv_label_set_text(g_wifi_ui.current_ssid_label, ssid);
        } else {
            lv_label_set_text(g_wifi_ui.current_ssid_label, "未连接");
        }
    }
}

/*============================================================================
 * 公共函数实现
 *============================================================================*/

/**
 * @brief 初始化WiFi连接页面
 * @param parent 父对象
 */
void WIFIConnect_init(lv_obj_t *parent) {
    printf("[WiFi] Initializing WiFi page...\n");
    
    /* 如果已经初始化过，先销毁旧的资源 */
    if (g_wifi_ui.is_initialized) {
        printf("[WiFi] Already initialized, cleaning up first...\n");
        WIFIConnect_deinit();
    }

    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* 创建主卡片容器 */
    g_wifi_ui.card = lv_obj_create(parent);
    lv_obj_set_size(g_wifi_ui.card, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(g_wifi_ui.card, PADDING_XL, 0);
    lv_obj_set_style_bg_color(g_wifi_ui.card, TH_BG_MAIN, 0);
    lv_obj_set_style_bg_opa(g_wifi_ui.card, LV_OPA_COVER, 0);
    lv_obj_clear_flag(g_wifi_ui.card, LV_OBJ_FLAG_SCROLLABLE);

    /* 标题 */
    lv_obj_t *connect_label = lv_label_create(g_wifi_ui.card);
    lv_obj_set_style_text_font(connect_label, &v853Font_WIFI_42, 0);
    lv_label_set_text(connect_label, get_string_for_language(ota_language_setting, "wifi_title"));
    lv_obj_set_style_text_color(connect_label, TH_TEXT_PRIMARY, 0);
    lv_obj_align(connect_label, LV_ALIGN_TOP_LEFT, PADDING_LG, 18);

    /* 当前已连接 WiFi 名称（位于标题与开关之间） */
    g_wifi_ui.current_ssid_label = lv_label_create(g_wifi_ui.card);
    lv_obj_set_style_text_font(g_wifi_ui.current_ssid_label, &v853Font_WIFI_18, 0);
    lv_obj_set_style_text_color(g_wifi_ui.current_ssid_label, TH_TEXT_SECONDARY, 0);
    lv_obj_align(g_wifi_ui.current_ssid_label, LV_ALIGN_TOP_LEFT, PADDING_LG, 70);

    /* WiFi开关按钮 */
    g_wifi_ui.connect_btn = lv_btn_create(g_wifi_ui.card);
    lv_obj_set_size(g_wifi_ui.connect_btn, 110, 56);
    lv_obj_align(g_wifi_ui.connect_btn, LV_ALIGN_TOP_RIGHT, -140, 12);
    lv_obj_set_style_bg_opa(g_wifi_ui.connect_btn, LV_OPA_TRANSP, 0);
    
    g_wifi_ui.connect_btn_yes = lv_img_create(g_wifi_ui.connect_btn);
    lv_img_set_src(g_wifi_ui.connect_btn_yes, &wifi_sw_on);
    lv_obj_center(g_wifi_ui.connect_btn_yes);
    
    g_wifi_ui.connect_btn_no = lv_img_create(g_wifi_ui.connect_btn);
    lv_img_set_src(g_wifi_ui.connect_btn_no, &wifi_sw_off);
    lv_obj_center(g_wifi_ui.connect_btn_no);

    /* 根据持久化的“开关”状态和当前实际连接状态初始化 UI */
    char ssid[128] = {0};
    bool linked = wifi_query_status(ssid, sizeof(ssid));
    if (linked) {
        g_wifi_ui.currently_connected = true;
    }

    if (g_wifi_ui.currently_connected) {
        lv_obj_clear_flag(g_wifi_ui.connect_btn_yes, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_wifi_ui.connect_btn_no, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(g_wifi_ui.connect_btn_no, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_wifi_ui.connect_btn_yes, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_wifi_ui.current_ssid_label) {
        if (linked && ssid[0] != '\0') {
            lv_label_set_text(g_wifi_ui.current_ssid_label, ssid);
        } else {
            lv_label_set_text(g_wifi_ui.current_ssid_label, "未连接");
        }
    }
    lv_obj_add_event_cb(g_wifi_ui.connect_btn, wifi_connect_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(g_wifi_ui.connect_btn, TH_BTN_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(g_wifi_ui.connect_btn, LV_OPA_40, LV_STATE_PRESSED);

    /* 刷新按钮 */
    g_wifi_ui.refresh_btn = lv_btn_create(g_wifi_ui.card);
    lv_obj_set_size(g_wifi_ui.refresh_btn, 56, 56);
    lv_obj_align(g_wifi_ui.refresh_btn, LV_ALIGN_TOP_RIGHT, -40, 12);
    lv_obj_set_style_radius(g_wifi_ui.refresh_btn, 28, 0);
    lv_obj_add_event_cb(g_wifi_ui.refresh_btn, refresh_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    g_wifi_ui.refresh_img = lv_label_create(g_wifi_ui.refresh_btn);
    lv_label_set_text(g_wifi_ui.refresh_img, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(g_wifi_ui.refresh_img, TH_TEXT_SECONDARY, 0);
    lv_obj_center(g_wifi_ui.refresh_img);
    lv_obj_set_style_bg_color(g_wifi_ui.refresh_btn, TH_BTN_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(g_wifi_ui.refresh_btn, LV_OPA_40, LV_STATE_PRESSED);

    /* WiFi列表容器 */
    g_wifi_ui.list_cont = lv_obj_create(g_wifi_ui.card);
    lv_obj_set_size(g_wifi_ui.list_cont, WIFI_LIST_WIDTH, WIFI_LIST_HEIGHT);
    lv_obj_set_pos(g_wifi_ui.list_cont, 30, 88);
    lv_obj_set_style_radius(g_wifi_ui.list_cont, RADIUS_LG, 0);
    lv_obj_set_style_bg_color(g_wifi_ui.list_cont, TH_BG_LIST, 0);
    lv_obj_set_style_bg_opa(g_wifi_ui.list_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(g_wifi_ui.list_cont, TH_BORDER, 0);
    lv_obj_set_style_border_width(g_wifi_ui.list_cont, BORDER_WIDTH_THIN, 0);
    lv_obj_set_style_shadow_width(g_wifi_ui.list_cont, SHADOW_WIDTH_SM, 0);
    lv_obj_set_style_shadow_color(g_wifi_ui.list_cont, TH_BORDER_LIGHT, 0);
    lv_obj_clear_flag(g_wifi_ui.list_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* WiFi列表 */
    g_wifi_ui.list = lv_list_create(g_wifi_ui.list_cont);
    lv_obj_set_pos(g_wifi_ui.list, 0, 0);
    lv_obj_set_size(g_wifi_ui.list, WIFI_LIST_WIDTH, WIFI_LIST_HEIGHT);
    lv_obj_set_style_bg_color(g_wifi_ui.list, TH_BG_LIST, 0);
    lv_obj_set_style_bg_opa(g_wifi_ui.list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_wifi_ui.list, 0, 0);
    lv_obj_set_style_radius(g_wifi_ui.list, RADIUS_SM, 0);
    lv_obj_set_style_pad_all(g_wifi_ui.list, RADIUS_SM, 0);
    lv_obj_set_style_pad_row(g_wifi_ui.list, RADIUS_SM, 0);
    lv_obj_set_style_text_color(g_wifi_ui.list, TH_TEXT_PRIMARY, 0);
    lv_obj_set_flex_flow(g_wifi_ui.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_wifi_ui.list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(g_wifi_ui.list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_size(g_wifi_ui.list, 8, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(g_wifi_ui.list, TH_BORDER, LV_PART_SCROLLBAR);

    /* 初始隐藏列表 */
    if (g_wifi_ui.currently_connected) {
        lv_obj_clear_flag(g_wifi_ui.list_cont, LV_OBJ_FLAG_HIDDEN);
        if (g_wifi_ui.list) {
            lv_obj_clean(g_wifi_ui.list);
        }
        refresh_add_list_internal();
    } else {
        lv_obj_add_flag(g_wifi_ui.list_cont, LV_OBJ_FLAG_HIDDEN);
    }
    
    g_wifi_ui.is_initialized = true;
    printf("[WiFi] WiFi page initialized\n");
}

/**
 * @brief 刷新WiFi网络列表
 */
void WIFIConnect_refresh_list(void) {
    if (g_wifi_ui.list) lv_obj_clean(g_wifi_ui.list);
    
    int cnt = 0;
    char **nets = scan_networks(&cnt);
    
    if (nets && cnt > 0) {
        for (int i = 0; i < cnt; i++) {
            if (nets[i] && strcmp(nets[i], "[Hidden Network]") == 0) { 
                free(nets[i]); 
                continue; 
            }
            
            lv_obj_t *list_btn = lv_list_add_btn(g_wifi_ui.list, NULL, nets[i]);
            if (list_btn) {
                lv_obj_set_style_bg_color(list_btn, TH_BG_LIST, 0);
                lv_obj_set_style_bg_opa(list_btn, LV_OPA_100, 0);
                lv_obj_set_style_text_color(list_btn, TH_TEXT_PRIMARY, 0);
                lv_obj_set_style_text_font(list_btn, &lv_font_montserrat_30, 0);
                lv_obj_set_style_radius(list_btn, RADIUS_SM, 0);
                lv_obj_set_style_border_width(list_btn, 1, 0);
                lv_obj_set_style_border_color(list_btn, TH_DIVIDER, 0);
                lv_obj_set_style_pad_all(list_btn, PADDING_SM, 0);
                lv_obj_set_style_bg_color(list_btn, TH_BTN_PRESSED, LV_STATE_PRESSED);
                lv_obj_set_style_bg_opa(list_btn, LV_OPA_40, LV_STATE_PRESSED);
                lv_obj_add_event_cb(list_btn, list_event_cb, LV_EVENT_CLICKED, NULL);
            }
            free(nets[i]);
        }
        free(nets);
    }
}

/**
 * @brief 销毁WiFi连接页面
 */
void WIFIConnect_deinit(void) {
    printf("[WiFi] Deinitializing WiFi page...\n");
    
    if (!g_wifi_ui.is_initialized) {
        printf("[WiFi] Not initialized, skip deinit\n");
        return;
    }
    
    /* 清理键盘 */
    if (g_wifi_ui.kb) {
        lv_obj_del(g_wifi_ui.kb);
        g_wifi_ui.kb = NULL;
    }
    
    /* 清理对话框 */
    if (g_wifi_ui.mask_layer_dialog) {
        lv_obj_del(g_wifi_ui.mask_layer_dialog);
        g_wifi_ui.mask_layer_dialog = NULL;
    }
    
    /* 清理列表 */
    if (g_wifi_ui.list) { 
        lv_obj_clean(g_wifi_ui.list); 
        lv_obj_del(g_wifi_ui.list); 
        g_wifi_ui.list = NULL; 
    }
    
    if (g_wifi_ui.list_cont) { 
        lv_obj_del(g_wifi_ui.list_cont); 
        g_wifi_ui.list_cont = NULL; 
    }
    
    if (g_wifi_ui.refresh_btn) { 
        lv_obj_del(g_wifi_ui.refresh_btn); 
        g_wifi_ui.refresh_btn = NULL; 
    }
    
    if (g_wifi_ui.connect_btn) { 
        lv_obj_del(g_wifi_ui.connect_btn); 
        g_wifi_ui.connect_btn = NULL; 
    }
    
    if (g_wifi_ui.card) {
        lv_obj_del(g_wifi_ui.card);
        g_wifi_ui.card = NULL;
    }
    
    /* 重置其他指针（保持 currently_connected 以便下次进入时恢复“开关”状态） */
    g_wifi_ui.connect_btn_yes = NULL;
    g_wifi_ui.connect_btn_no = NULL;
    g_wifi_ui.refresh_img = NULL;
    g_wifi_ui.dialog_bg = NULL;
    g_wifi_ui.ssid_label = NULL;
    g_wifi_ui.password_ta = NULL;
    g_wifi_ui.current_ssid_label = NULL;
    g_wifi_ui.is_initialized = false;
    
    printf("[WiFi] WiFi page deinitialized\n");
}

/**
 * @brief 刷新WiFi页面主题
 */
void WIFIConnect_refresh_theme(void) {
    if (!g_wifi_ui.is_initialized) return;

    printf("[WiFi] Refreshing theme...\n");

    /* 刷新主卡片 */
    if (g_wifi_ui.card) {
        lv_obj_set_style_bg_color(g_wifi_ui.card, TH_BG_MAIN, 0);
    }

    /* 刷新列表容器 */
    if (g_wifi_ui.list_cont) {
        lv_obj_set_style_bg_color(g_wifi_ui.list_cont, TH_BG_LIST, 0);
        lv_obj_set_style_border_color(g_wifi_ui.list_cont, TH_BORDER, 0);
    }
    
    /* 刷新列表 */
    if (g_wifi_ui.list) {
        lv_obj_set_style_bg_color(g_wifi_ui.list, TH_BG_LIST, 0);
        lv_obj_set_style_text_color(g_wifi_ui.list, TH_TEXT_PRIMARY, 0);
    }
    
    printf("[WiFi] Theme refresh completed\n");
}

/*============================================================================
 * 主界面 WiFi 图标：缓存状态与后台轮询（不阻塞 UI 线程）
 *============================================================================*/

#define WIFI_POLL_INTERVAL_SEC  3

static void *wifi_poll_thread_fn(void *arg) {
    (void)arg;
    while (s_wifi_poll_running) {
        s_wifi_connected_cached = wifi_query_status(NULL, 0);
        for (int i = 0; i < WIFI_POLL_INTERVAL_SEC && s_wifi_poll_running; i++) {
            sleep(1);
        }
    }
    return NULL;
}

bool WIFIConnect_is_connected_cached(void) {
    return s_wifi_connected_cached;
}

void WIFIConnect_start_status_poll(void) {
    if (s_wifi_poll_running) {
        return;
    }
    s_wifi_poll_running = true;
    if (pthread_create(&s_wifi_poll_tid, NULL, wifi_poll_thread_fn, NULL) != 0) {
        s_wifi_poll_running = false;
        printf("[WiFi] Failed to start status poll thread\n");
    }
}
