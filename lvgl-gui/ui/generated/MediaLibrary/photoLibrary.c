/**
 * @file photoLibrary.c
 * @brief 照片库模块实现
 * 
 * 实现照片文件列表显示、图片查看、缩放、旋转、删除等功能
 */

#include "photoLibrary.h"

/*====================================================================================
 * 全局变量定义
 *====================================================================================*/

/* 照片库全局实例 */
photoLibrary_t pl = {0};

/*====================================================================================
 * 私有函数声明
 *====================================================================================*/

/* UI创建 */
static void create_ui_components(lv_obj_t *parent);
static void pl_init(void);

/* 事件处理 */
static void back_btn_event_handler(lv_event_t *e);
static void refresh_btn_event_handler(lv_event_t *e);
static void prev_btn_event_handler(lv_event_t *e);
static void next_btn_event_handler(lv_event_t *e);
static void on_image_touch(lv_event_t *e);
static void small_btn_event_handler(lv_event_t *e);
static void big_btn_event_handler(lv_event_t *e);
static void rota_btn_event_handler(lv_event_t *e);
static void reset_btn_event_handler(lv_event_t *e);
static void list_btn_event_handler(lv_event_t *e);
static void del_btn_event_handler(lv_event_t *e);

/* 内部工具函数 */
static void show_photo_list(void);
static void scan_photo_directory(void);
static void scan_photo_directory_recursive(const char *dir_path);
static void free_photo_paths(void);
static void ensure_capacity(void);
static int compare_strings(const void *a, const void *b);
static int is_jpg_file(const char *filename);
static void add_photo_path(const char *full_path);
static void show_photo_viewer(int index);
static void load_and_display_image(const char *path);
static void update_zoom_label(void);

/* canvas相关 */
static int canvas_display_image(lv_obj_t *canvas, const char *img_path);
static int canvas_zoom(lv_obj_t *canvas, float scale);
static int canvas_move(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y);
static bool rotate_canvas_90_clockwise_argb8888(lv_obj_t *canvas);

/*====================================================================================
 * UI创建函数
 *====================================================================================*/

static void create_ui_components(lv_obj_t *parent) {
    // 创建主容器（全屏）
    pl.pm.cont = lv_obj_create(parent);
    lv_obj_remove_style_all(pl.pm.cont);
    // lv_obj_set_size(pl.pm.cont, lv_pct(100), lv_pct(100) - 5);
    lv_obj_set_size(pl.pm.cont, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(pl.pm.cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(pl.pm.cont, 0, 0);
    lv_obj_set_style_bg_color(pl.pm.cont, TM_BG_PRIMARY, 0);

    // 创建顶部栏
    pl.pm.top_bar = lv_obj_create(pl.pm.cont);
    lv_obj_set_pos(pl.pm.top_bar, 0, 0);
    lv_obj_set_size(pl.pm.top_bar, lv_pct(100), 80);
    lv_obj_set_style_bg_color(pl.pm.top_bar, TM_BG_SECONDARY, 0);
    // lv_obj_set_style_border_width(pl.pm.top_bar, 0, 0);
    lv_obj_set_style_pad_all(pl.pm.top_bar, 5, 0);
    lv_obj_set_flex_flow(pl.pm.top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pl.pm.top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(pl.pm.top_bar, 0, 0);

    // // 添加左侧占位控件
    // pl.pm.left_placeholder = lv_obj_create(pl.pm.top_bar);
    // lv_obj_set_size(pl.pm.left_placeholder, 40, 40);
    // lv_obj_set_style_bg_opa(pl.pm.left_placeholder, LV_OPA_0, 0);
    // lv_obj_set_style_border_width(pl.pm.left_placeholder, 0, 0);

    // 创建标题
    pl.pm.title = lv_label_create(pl.pm.top_bar);
    lv_obj_set_style_text_font(pl.pm.title, &v853Font_hyby_40, 0);
    lv_label_set_text(pl.pm.title, get_string_for_language(languageSetting, "Photo_Library"));
    lv_obj_set_style_text_color(pl.pm.title, TM_TEXT_PRIMARY, 0);

    pl.pm.scan_bar = lv_obj_create(pl.pm.cont);
    lv_obj_remove_style_all(pl.pm.scan_bar);
    lv_obj_set_pos(pl.pm.scan_bar, 0, 80);
    lv_obj_set_size(pl.pm.scan_bar, 1340, 60);
    // lv_obj_set_style_bg_color(pl.pm.scan_bar, lv_color_hex(0x000ff), 0);
    lv_obj_set_style_bg_opa(pl.pm.scan_bar, LV_OPA_0, 0);
    lv_obj_set_style_border_width(pl.pm.scan_bar, 0, 0);
    lv_obj_set_style_pad_all(pl.pm.scan_bar, 0, 0);
    lv_obj_set_flex_flow(pl.pm.scan_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pl.pm.scan_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(pl.pm.scan_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 创建信息标签
    pl.pm.info_label = lv_label_create(pl.pm.scan_bar);
    lv_obj_set_height(pl.pm.info_label, 60);
    lv_obj_set_style_pad_all(pl.pm.info_label, 15, 0);
    lv_obj_set_style_text_font(pl.pm.info_label, &v853Font_hyby_30, 0);
    lv_label_set_text(pl.pm.info_label, get_string_for_language(languageSetting, "Photo_scanning"));
    lv_obj_set_style_text_color(pl.pm.info_label, TM_TEXT_SECONDARY, 0);
    // lv_obj_set_flex_grow(pl.pm.info_label, 1);
    // lv_obj_align(pl.pm.info_label, LV_ALIGN_TOP_LEFT, 20, 0);

    // 【关键】创建中间占位符，将按钮推到右边
    lv_obj_t *spacer = lv_obj_create(pl.pm.scan_bar);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_0, 0);
    lv_obj_set_flex_grow(spacer, 1);  // 占据所有剩余空间

    // 1. 创建样式
    // lv_style_t style_btn;
    lv_style_init(&pl.pm.style_btn);
    lv_style_set_pad_all(&pl.pm.style_btn, 0);
    lv_style_set_pad_right(&pl.pm.style_btn, 40);
    lv_style_set_bg_opa(&pl.pm.style_btn, LV_OPA_0);
    // // 3. 设置按下状态 - 直接放大
    // lv_style_set_transform_zoom(&style_btn, 280, LV_STATE_PRESSED);  // 瞬间变为110%

    // 创建刷新按钮
    pl.pm.refresh_btn = lv_btn_create(pl.pm.scan_bar);
    lv_obj_set_size(pl.pm.refresh_btn, 100, 60);
    // lv_obj_set_style_pad_all(pl.pm.refresh_btn, 0, 0);
    // lv_obj_set_style_pad_right(pl.pm.refresh_btn, 40, 0);
    // lv_obj_set_style_bg_opa(pl.pm.refresh_btn, LV_OPA_0, 0);
    // // 4. 应用样式
    lv_obj_add_style(pl.pm.refresh_btn, &pl.pm.style_btn, 0);
    lv_obj_add_event_cb(pl.pm.refresh_btn, refresh_btn_event_handler, LV_EVENT_CLICKED, NULL);
    /* 照片库 - 刷新按钮图标 */
    pl.pm.refresh_icon = lv_label_create(pl.pm.refresh_btn);
    lv_obj_set_style_text_font(pl.pm.refresh_icon, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(pl.pm.refresh_icon, TM_ICON_PRIMARY, 0);  /* 刷新图标颜色 */
    lv_label_set_text(pl.pm.refresh_icon, LV_SYMBOL_REFRESH);
    lv_obj_center(pl.pm.refresh_icon);

    // 创建列表容器
    pl.pm.list_cont = lv_obj_create(pl.pm.cont);
    lv_obj_set_size(pl.pm.list_cont, lv_pct(100), 720 - 80 - 60);
    lv_obj_set_pos(pl.pm.list_cont, 0, 80 + 60);
    lv_obj_set_style_pad_all(pl.pm.list_cont, 0, 0);
    lv_obj_set_style_bg_color(pl.pm.list_cont, TM_BG_PRIMARY, 0);
    lv_obj_clear_flag(pl.pm.list_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(pl.pm.list_cont, 0, 0);
    // lv_obj_add_flag(pl.pm.list_cont, LV_OBJ_FLAG_HIDDEN);

    /* 照片库 - 照片列表 */
    pl.pm.list = lv_list_create(pl.pm.list_cont);
    lv_obj_set_pos(pl.pm.list, 0, 0);
    lv_obj_set_size(pl.pm.list, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(pl.pm.list, TM_BG_PRIMARY, 0);    /* 列表背景色，与容器一致 */
    lv_obj_set_style_bg_opa(pl.pm.list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pl.pm.list, 0, 0);
    lv_obj_set_flex_flow(pl.pm.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pl.pm.list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(pl.pm.list, 15, 0);
    lv_obj_set_style_pad_right(pl.pm.list, 15, 0);
    lv_obj_set_style_radius(pl.pm.list, 0, 0);
    lv_obj_set_style_pad_bottom(pl.pm.list, 20, 0);             /* 列表底部边距 */


    // 创建查看容器（初始隐藏）
    pl.pv.cont = lv_obj_create(pl.pm.cont);
    lv_obj_set_size(pl.pv.cont, lv_pct(100), lv_pct(100));
    lv_obj_add_flag(pl.pv.cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_pad_all(pl.pv.cont, 0, 0);
    lv_obj_set_style_bg_color(pl.pv.cont, TM_BG_PRIMARY, 0);
    lv_obj_clear_flag(pl.pv.cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(pl.pv.cont);
    // lv_obj_move_foreground(pl.pv.cont);

    pl.pv.img_wrapper = lv_obj_create(pl.pv.cont);
    lv_obj_remove_style_all(pl.pv.img_wrapper);
    lv_obj_set_size(pl.pv.img_wrapper, lv_pct(100), lv_pct(100));
    // lv_obj_set_y(pl.pv.img_wrapper, 60);
    // lv_obj_set_height(pl.pv.img_wrapper, lv_pct(100) - 120);              // 减去顶部栏和底部栏高度
    lv_obj_set_style_bg_color(pl.pv.img_wrapper, TM_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(pl.pv.img_wrapper, 0, 0);
    lv_obj_set_style_pad_all(pl.pv.img_wrapper, 0, 0);

    // lv_obj_clear_flag(pl.pv.img_wrapper, LV_OBJ_FLAG_CLICKABLE);
    // lv_obj_clear_flag(pl.pv.img_wrapper, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(pl.pv.img_wrapper, LV_OBJ_FLAG_SCROLLABLE);
    // lv_obj_clear_flag(pl.pv.img_wrapper, LV_OBJ_FLAG_SCROLL_CHAIN);
    // lv_obj_clear_flag(pl.pv.img_wrapper, LV_OBJ_FLAG_SCROLL_ELASTIC);
    // lv_obj_clear_flag(pl.pv.img_wrapper, LV_OBJ_FLAG_SCROLL_MOMENTUM);

    // lv_obj_set_scrollbar_mode(pl.pv.img_wrapper, LV_SCROLLBAR_MODE_OFF);
    // lv_obj_set_scroll_dir(pl.pv.img_wrapper, LV_DIR_NONE);
    lv_obj_set_style_clip_corner(pl.pv.img_wrapper, false, 0);
    lv_obj_set_style_radius(pl.pv.img_wrapper, 0, 0);

    // 创建图片对象
    pl.pv.img = lv_canvas_create(pl.pv.img_wrapper);
    lv_obj_add_flag(pl.pv.img, LV_OBJ_FLAG_CLICKABLE);
    // lv_obj_add_flag(pl.pv.img, LV_OBJ_FLAG_GESTURE_BUBBLE);
    // 添加触摸事件
    lv_obj_add_event_cb(pl.pv.img, on_image_touch, LV_EVENT_ALL, NULL);

    // 创建查看模式顶部栏
    pl.pv.top_bar = lv_obj_create(pl.pv.cont);
    lv_obj_remove_style_all(pl.pv.top_bar);
    lv_obj_set_size(pl.pv.top_bar, lv_pct(100), 50);
    lv_obj_align(pl.pv.top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(pl.pv.top_bar, TM_BG_SECONDARY, 0);
    lv_obj_set_style_bg_opa(pl.pv.top_bar, LV_OPA_50, 0);
    lv_obj_set_style_border_width(pl.pv.top_bar, 0, 0);
    lv_obj_set_style_pad_all(pl.pv.top_bar, 0, 0);
    lv_obj_set_style_radius(pl.pv.top_bar, 0, 0);
    lv_obj_set_flex_flow(pl.pv.top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pl.pv.top_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // lv_obj_set_flex_align(pl.pv.top_bar, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(pl.pv.top_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 创建返回按钮
    pl.pv.back_btn = lv_btn_create(pl.pv.top_bar);
    lv_obj_set_size(pl.pv.back_btn, 70, 50);
    lv_obj_set_style_bg_opa(pl.pv.back_btn, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.back_btn, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_bg_color(pl.pv.back_btn, TM_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(pl.pv.back_btn, LV_OPA_30, LV_STATE_PRESSED);  // 50%透明度
    // lv_obj_set_flex_grow(pl.pv.back_btn, 1);
    // 设置返回按钮为左对齐
    lv_obj_set_style_base_dir(pl.pv.back_btn, LV_BASE_DIR_LTR, 0);
    lv_obj_set_style_flex_main_place(pl.pv.back_btn, LV_FLEX_ALIGN_START, 0);
    lv_obj_add_event_cb(pl.pv.back_btn, back_btn_event_handler, LV_EVENT_CLICKED, NULL);
    /* 照片查看器 - 返回按钮图标 */
    pl.pv.back_icon = lv_img_create(pl.pv.back_btn);
    lv_img_set_src(pl.pv.back_icon, &Photo_viewer_back);
    lv_obj_set_style_img_recolor(pl.pv.back_icon, TM_ICON_PRIMARY, 0);    /* 图标颜色 */
    lv_obj_set_style_img_recolor_opa(pl.pv.back_icon, LV_OPA_COVER, 0);
    lv_obj_set_size(pl.pv.back_icon, 70, 50);
    lv_obj_center(pl.pv.back_icon);

    // 创建文件名标签
    pl.pv.file_label = lv_label_create(pl.pv.top_bar);
    lv_obj_set_style_text_font(pl.pv.file_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(pl.pv.file_label, TM_TEXT_PRIMARY, 0);
    lv_obj_set_flex_grow(pl.pv.file_label, 1);
    // 设置标签居中
    lv_obj_set_style_text_align(pl.pv.file_label, LV_TEXT_ALIGN_CENTER, 0);
    // 设置标签自身居中
    lv_obj_set_style_flex_main_place(pl.pv.file_label, LV_FLEX_ALIGN_CENTER, 0);

    // 创建上一张/下一张按钮容器
    pl.pv.nav_btn_cont = lv_obj_create(pl.pv.top_bar);
    lv_obj_set_size(pl.pv.nav_btn_cont, 120, 50);
    // lv_obj_set_style_bg_color(pl.pv.nav_btn_cont, lv_color_hex(0x1A1A1A), 0);
    // lv_obj_set_style_border_width(pl.pv.nav_btn_cont, 0, 0);
    lv_obj_set_style_bg_opa(pl.pv.nav_btn_cont, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.nav_btn_cont, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_pad_all(pl.pv.nav_btn_cont, 0, 0);
    // lv_obj_set_flex_grow(pl.pv.nav_btn_cont, 1);
    // 设置导航容器为右对齐
    lv_obj_set_style_flex_main_place(pl.pv.nav_btn_cont, LV_FLEX_ALIGN_END, 0);
    // lv_obj_set_flex_flow(pl.pv.nav_btn_cont, LV_FLEX_FLOW_ROW);
    // lv_obj_set_flex_align(pl.pv.nav_btn_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(pl.pv.nav_btn_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 创建上一张按钮
    pl.pv.prev_btn = lv_btn_create(pl.pv.nav_btn_cont);
    lv_obj_set_pos(pl.pv.prev_btn, 0, 0);
    lv_obj_set_size(pl.pv.prev_btn, 40, 50);
    lv_obj_set_style_bg_opa(pl.pv.prev_btn, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.prev_btn, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_bg_color(pl.pv.prev_btn, TM_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(pl.pv.prev_btn, LV_OPA_30, LV_STATE_PRESSED);  // 50%透明度
    // lv_obj_set_flex_grow(pl.pv.prev_btn, 1);
    lv_obj_add_event_cb(pl.pv.prev_btn, prev_btn_event_handler, LV_EVENT_CLICKED, NULL);
    /* 照片查看器 - 上一张按钮图标 */
    pl.pv.prev_icon = lv_img_create(pl.pv.prev_btn);
    lv_img_set_src(pl.pv.prev_icon, &Photo_viewer_prev);
    lv_obj_set_style_img_recolor(pl.pv.prev_icon, TM_ICON_PRIMARY, 0);    /* 图标颜色 */
    lv_obj_set_style_img_recolor_opa(pl.pv.prev_icon, LV_OPA_COVER, 0);
    lv_obj_set_size(pl.pv.prev_icon, 40, 50);
    lv_obj_center(pl.pv.prev_icon);

    // 创建下一张按钮
    pl.pv.next_btn = lv_btn_create(pl.pv.nav_btn_cont);
    lv_obj_set_pos(pl.pv.next_btn, 60, 0);
    lv_obj_set_size(pl.pv.next_btn, 40, 50);
    lv_obj_set_style_bg_opa(pl.pv.next_btn, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.next_btn, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_bg_color(pl.pv.next_btn, TM_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(pl.pv.next_btn, LV_OPA_30, LV_STATE_PRESSED);  // 50%透明度
    // lv_obj_set_flex_grow(pl.pv.next_btn, 1);
    lv_obj_add_event_cb(pl.pv.next_btn, next_btn_event_handler, LV_EVENT_CLICKED, NULL);
    /* 照片查看器 - 下一张按钮图标 */
    pl.pv.next_icon = lv_img_create(pl.pv.next_btn);
    lv_img_set_src(pl.pv.next_icon, &Photo_viewer_next);
    lv_obj_set_style_img_recolor(pl.pv.next_icon, TM_ICON_PRIMARY, 0);    /* 图标颜色 */
    lv_obj_set_style_img_recolor_opa(pl.pv.next_icon, LV_OPA_COVER, 0);
    lv_obj_set_size(pl.pv.next_icon, 40, 50);
    lv_obj_center(pl.pv.next_icon);

    // 创建底部控制栏
    pl.pv.bottom_bar = lv_obj_create(pl.pv.cont);
    lv_obj_remove_style_all(pl.pv.bottom_bar);
    lv_obj_set_size(pl.pv.bottom_bar, lv_pct(100), 50);
    lv_obj_align(pl.pv.bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(pl.pv.bottom_bar, TM_BG_SECONDARY, 0);
    lv_obj_set_style_bg_opa(pl.pv.bottom_bar, LV_OPA_50, 0);
    lv_obj_set_style_border_width(pl.pv.bottom_bar, 0, 0);
    lv_obj_set_style_pad_all(pl.pv.bottom_bar, 5, 0);
    lv_obj_set_style_radius(pl.pv.img_wrapper, 0, 0);
    lv_obj_set_flex_flow(pl.pv.bottom_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pl.pv.bottom_bar, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(pl.pv.bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    pl.pv.small_btn = lv_btn_create(pl.pv.bottom_bar);
    lv_obj_set_size(pl.pv.small_btn, 60, 50);
    lv_obj_set_style_bg_opa(pl.pv.small_btn, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.small_btn, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_bg_color(pl.pv.small_btn, TM_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(pl.pv.small_btn, LV_OPA_30, LV_STATE_PRESSED);  // 50%透明度
    lv_obj_set_flex_grow(pl.pv.small_btn, 1);
    lv_obj_add_event_cb(pl.pv.small_btn, small_btn_event_handler, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_flag(pl.pv.small_btn, LV_OBJ_FLAG_CHECKABLE);
    /* 照片查看器 - 缩小按钮图标 */
    pl.pv.small_btn_icon = lv_img_create(pl.pv.small_btn);
    lv_img_set_src(pl.pv.small_btn_icon, &Photo_viewer_big);
    lv_obj_set_style_img_recolor(pl.pv.small_btn_icon, TM_ICON_PRIMARY, 0);
    lv_obj_set_style_img_recolor_opa(pl.pv.small_btn_icon, LV_OPA_COVER, 0);
    lv_obj_set_size(pl.pv.small_btn_icon, 60, 50);
    lv_obj_center(pl.pv.small_btn_icon);

    pl.pv.big_btn = lv_btn_create(pl.pv.bottom_bar);
    lv_obj_set_size(pl.pv.big_btn, 60, 50);
    lv_obj_set_style_bg_opa(pl.pv.big_btn, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.big_btn, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_bg_color(pl.pv.big_btn, TM_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(pl.pv.big_btn, LV_OPA_30, LV_STATE_PRESSED);  // 50%透明度
    lv_obj_set_flex_grow(pl.pv.big_btn, 1);
    lv_obj_add_event_cb(pl.pv.big_btn, big_btn_event_handler, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_flag(pl.pv.big_btn, LV_OBJ_FLAG_CHECKABLE);
    /* 照片查看器 - 放大按钮图标 */
    pl.pv.big_btn_icon = lv_img_create(pl.pv.big_btn);
    lv_img_set_src(pl.pv.big_btn_icon, &Photo_viewer_small);
    lv_obj_set_style_img_recolor(pl.pv.big_btn_icon, TM_ICON_PRIMARY, 0);
    lv_obj_set_style_img_recolor_opa(pl.pv.big_btn_icon, LV_OPA_COVER, 0);
    lv_obj_set_size(pl.pv.big_btn_icon, 60, 50);
    lv_obj_center(pl.pv.big_btn_icon);

    pl.pv.rota_btn = lv_btn_create(pl.pv.bottom_bar);
    lv_obj_set_size(pl.pv.rota_btn, 70, 50);
    lv_obj_set_style_bg_opa(pl.pv.rota_btn, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.rota_btn, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_bg_color(pl.pv.rota_btn, TM_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(pl.pv.rota_btn, LV_OPA_30, LV_STATE_PRESSED);  // 50%透明度
    lv_obj_set_flex_grow(pl.pv.rota_btn, 1);
    lv_obj_add_event_cb(pl.pv.rota_btn, rota_btn_event_handler, LV_EVENT_CLICKED, NULL);
    // lv_obj_add_flag(pl.pv.rota_btn, LV_OBJ_FLAG_CHECKABLE);
    /* 照片查看器 - 旋转按钮图标 */
    pl.pv.rota_btn_icon = lv_img_create(pl.pv.rota_btn);
    lv_img_set_src(pl.pv.rota_btn_icon, &Photo_viewer_rota);
    lv_obj_set_style_img_recolor(pl.pv.rota_btn_icon, TM_ICON_PRIMARY, 0);
    lv_obj_set_style_img_recolor_opa(pl.pv.rota_btn_icon, LV_OPA_COVER, 0);
    lv_obj_set_size(pl.pv.rota_btn_icon, 60, 50);
    lv_obj_center(pl.pv.rota_btn_icon);

    // 创建重置按钮
    pl.pv.reset_btn = lv_btn_create(pl.pv.bottom_bar);
    lv_obj_set_size(pl.pv.reset_btn, 70, 50);
    lv_obj_set_style_bg_opa(pl.pv.reset_btn, LV_OPA_TRANSP, 0);        // 设置背景透明
    lv_obj_set_style_border_opa(pl.pv.reset_btn, LV_OPA_TRANSP, 0);    // 设置边缘透明
    lv_obj_set_style_bg_color(pl.pv.reset_btn, TM_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(pl.pv.reset_btn, LV_OPA_30, LV_STATE_PRESSED);  // 50%透明度
    lv_obj_set_flex_grow(pl.pv.reset_btn, 1);
    lv_obj_add_event_cb(pl.pv.reset_btn, reset_btn_event_handler, LV_EVENT_CLICKED, NULL);
    /* 照片查看器 - 重置按钮图标 */
    pl.pv.reset_btn_icon = lv_img_create(pl.pv.reset_btn);
    lv_img_set_src(pl.pv.reset_btn_icon, &Photo_viewer_reset);
    lv_obj_set_style_img_recolor(pl.pv.reset_btn_icon, TM_ICON_PRIMARY, 0);
    lv_obj_set_style_img_recolor_opa(pl.pv.reset_btn_icon, LV_OPA_COVER, 0);
    lv_obj_set_size(pl.pv.reset_btn_icon, 70, 50);
    lv_obj_center(pl.pv.reset_btn_icon);

    // 创建缩放标签
    pl.pv.zoom_label = lv_label_create(pl.pv.bottom_bar);
    lv_obj_set_size(pl.pv.zoom_label, 80, 50);
    lv_obj_set_style_text_font(pl.pv.zoom_label, &v853Font_hyby_30, 0);
    lv_label_set_text(pl.pv.zoom_label, "zoom: 100%");
    lv_obj_set_style_text_color(pl.pv.zoom_label, TM_TEXT_SECONDARY, 0);
    lv_obj_set_style_pad_top(pl.pv.zoom_label, 10, 0);
    lv_obj_set_flex_grow(pl.pv.zoom_label, 1);

    printf("\n\n\n\nlanguageSetting = %d\n\n\n\n\n", languageSetting);
}

/* 双线性插值函数 */
static void bilinear_interpolation(const uint8_t* src, uint32_t src_w, uint32_t src_h,
                                 uint8_t* dst, uint32_t dst_w, uint32_t dst_h) {
    float x_ratio = (float)(src_w - 1) / dst_w;
    float y_ratio = (float)(src_h - 1) / dst_h;
    
    for (uint32_t y = 0; y < dst_h; y++) {
        for (uint32_t x = 0; x < dst_w; x++) {
            float x_src = x * x_ratio;
            float y_src = y * y_ratio;
            
            uint32_t x_int = (uint32_t)x_src;
            uint32_t y_int = (uint32_t)y_src;
            float x_frac = x_src - x_int;
            float y_frac = y_src - y_int;
            
            uint32_t x1 = x_int;
            uint32_t y1 = y_int;
            uint32_t x2 = (x_int + 1 < src_w) ? x_int + 1 : x_int;
            uint32_t y2 = (y_int + 1 < src_h) ? y_int + 1 : y_int;
            
            /* 计算四个相邻像素的索引 */
            uint32_t idx_tl = (y1 * src_w + x1) * 4;  /* Top-Left */
            uint32_t idx_tr = (y1 * src_w + x2) * 4;  /* Top-Right */
            uint32_t idx_bl = (y2 * src_w + x1) * 4;  /* Bottom-Left */
            uint32_t idx_br = (y2 * src_w + x2) * 4;  /* Bottom-Right */
            
            /* 对每个颜色通道进行双线性插值 */
            for (int c = 0; c < 4; c++) {
                float top = src[idx_tl + c] * (1 - x_frac) + src[idx_tr + c] * x_frac;
                float bottom = src[idx_bl + c] * (1 - x_frac) + src[idx_br + c] * x_frac;
                float value = top * (1 - y_frac) + bottom * y_frac;
                
                /* 计算目标像素索引 */
                uint32_t dst_idx = (y * dst_w + x) * 4 + c;
                dst[dst_idx] = (uint8_t)(value > 255 ? 255 : (value < 0 ? 0 : value));
            }
        }
    }
}

static int canvas_move(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y) {
    if (!canvas) {
        return -1;
    }
    
    /* 直接设置Canvas的位置 */
    lv_obj_set_pos(canvas, x, y);
    
    return 0;
}

static int canvas_display_image(lv_obj_t *canvas, const char *img_path) {
    if (!canvas || !img_path) {
        printf("canvas_display_image: invalid parameters\n");
        return -1;
    }

    /* 清理上一张图片的缓存 */
    if (pl.old_img_path) {
        printf("[PhotoLibrary] Releasing %s\n", pl.old_img_path);
        lv_img_cache_invalidate_src(pl.old_img_path);
        pl.old_img_path = NULL;
    }

    /* 步骤2: 获取当前canvas的缓冲区（如果有的话） */
    lv_img_dsc_t *current_img = lv_canvas_get_img(canvas);
    void *old_buf = NULL;
    if (current_img && current_img->data) {
        old_buf = (void *)current_img->data;  // 保存旧缓冲区的指针
        if (old_buf) {
            printf("old_img free %zu\n", sizeof(&old_buf));
            lv_mem_free(old_buf);
        }
    }

    /* 步骤1: 获取图片信息 */
    lv_img_header_t header;
    lv_res_t res = lv_img_decoder_get_info(img_path, &header);
    if (res != LV_RES_OK) {
        printf("canvas_display_image: failed to get image info\n");
        return -1;
    }
    /* 步骤2: 设置canvas缓冲区为图片原始大小，强制使用ARGB8888格式 */
    /* 使用LV_IMG_CF_TRUE_COLOR_ALPHA格式（ARGB8888） */
    lv_img_cf_t canvas_cf = LV_IMG_CF_TRUE_COLOR_ALPHA;

    /* 计算缓冲区大小：宽 × 高 × 4字节（ARGB8888） */
    uint32_t buf_size = header.w * header.h * 4;

    /* 分配canvas缓冲区 */
    void *orig_buf = lv_mem_alloc(buf_size);
    if (!orig_buf) {
        printf("canvas_display_image: failed to allocate buffer\n");
        return -1;
    }

    /* 设置canvas为原始图片大小，使用ARGB8888格式 */
    lv_canvas_set_buffer(canvas, orig_buf, header.w, header.h, canvas_cf);
    lv_obj_set_size(canvas, header.w, header.h);

    // if (old_buf) {
    //     lv_mem_free(old_buf);
    // }

    /* 清除canvas背景为黑色 */
    lv_canvas_fill_bg(canvas, lv_color_make(0xff, 0, 0), LV_OPA_COVER);

    lv_draw_img_dsc_t draw_dsc;
    lv_draw_img_dsc_init(&draw_dsc);
    draw_dsc.opa = LV_OPA_COVER;
    lv_canvas_draw_img(canvas, 0, 0, img_path, &draw_dsc);

    /* 步骤4: 获取canvas层的图片数据 */
    lv_img_dsc_t *canvas_img = lv_canvas_get_img(canvas);
    if (!canvas_img || !canvas_img->data) {
        printf("canvas_display_image: failed to get canvas image data\n");
        lv_mem_free(orig_buf);
        return -1;
    }

    printf("Canvas image: w=%d, h=%d, cf=%d(%d)\n", 
                canvas_img->header.w, canvas_img->header.h, 
                canvas_img->header.cf, canvas_img->header.cf);

    /* 步骤5: 根据lv_canvas_get_img获取的数据，通过双线性插值法生成960*540大小的canvas并显示 */
    const uint32_t target_w = 960;
    const uint32_t target_h = 540;

    /* 分配目标缓冲区（ARGB8888格式） */
    uint32_t target_buf_size = target_w * target_h * 4;
    void *target_buf = lv_mem_alloc(target_buf_size);
    if (!target_buf) {
        printf("canvas_display_image: failed to allocate target buffer\n");
        lv_mem_free(orig_buf);
        return -1;
    }

    /* 执行双线性插值缩放 */
    /* 注意：这里假设canvas_img->data是ARGB8888格式 */
    bilinear_interpolation((const uint8_t *)canvas_img->data, 
                                    canvas_img->header.w, canvas_img->header.h,
                                    (uint8_t *)target_buf, target_w, target_h);

    // /* 重新设置canvas为960x540大小，使用ARGB8888格式 */
    lv_canvas_set_buffer(canvas, target_buf, target_w, target_h, LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_obj_set_size(canvas, target_w, target_h);
    if (orig_buf) {
        printf("orig_buf free %zu\n", sizeof(&orig_buf));
        lv_mem_free(orig_buf);
    }

    printf("canvas_display_image: success - scaled from %dx%d to %dx%d\n", 
                header.w, header.h, target_w, target_h);

    pl.old_img_path = img_path;

    return 0;
}

static int canvas_zoom(lv_obj_t *canvas, float scale) {
    if (!canvas || scale <= 0) {
        return -1;
    }

    /* 获取当前Canvas图像数据 */
    void *old_buf = NULL;
    lv_img_dsc_t *canvas_dsc = lv_canvas_get_img(canvas);
    if (!canvas_dsc || !canvas_dsc->data) {
        return -1;
    } else {
        old_buf = (void *)canvas_dsc->data;  // 保存旧缓冲区的指针
    }

    uint32_t src_w = canvas_dsc->header.w;
    uint32_t src_h = canvas_dsc->header.h;

    /* 计算新尺寸 */
    uint32_t dst_w = (uint32_t)(src_w * scale);
    uint32_t dst_h = (uint32_t)(src_h * scale);

    if (dst_w == 0 || dst_h == 0) {
        return -1;
    }

    /* 分配新缓冲区 */
    uint8_t *dst_buf = (uint8_t *)lv_mem_alloc(dst_w * dst_h * 4);
    if (!dst_buf) {
        return -1;
    }

    /* 执行双线性插值缩放 */
    bilinear_interpolation((const uint8_t*)canvas_dsc->data, src_w, src_h,
                          dst_buf, dst_w, dst_h);

    /* 重新设置Canvas缓冲区 */
    lv_canvas_set_buffer(canvas, dst_buf, dst_w, dst_h, LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_obj_set_size(canvas, dst_w, dst_h);
    printf("w = %d, h = %d\n", dst_w, dst_h);

    if (old_buf) {
        lv_mem_free(old_buf);
    }
    return 0;
}

static bool rotate_canvas_90_clockwise_argb8888(lv_obj_t *canvas) {
    if (canvas == NULL) {
        return false;
    }
    
    // 获取canvas的图片描述符
    void *old_buf = NULL;
    lv_img_dsc_t *current_img = lv_canvas_get_img(canvas);
    if (current_img == NULL || current_img->data == NULL) {
        return false;
    } else {
        old_buf = (void *)current_img->data;  // 保存旧缓冲区的指针
    }
    
    // 检查是否为ARGB8888格式
    if (current_img->header.cf != LV_IMG_CF_TRUE_COLOR_ALPHA) {
        LV_LOG_WARN("Canvas format is not ARGB8888, found: %d", current_img->header.cf);
        return false;
    }
    
    // 获取原始图片的宽高
    uint16_t original_width = current_img->header.w;
    uint16_t original_height = current_img->header.h;
    
    // ARGB8888每个像素4字节
    const uint8_t PIXEL_SIZE = 4;
    
    // 创建旋转后的缓冲区（宽高互换）
    uint32_t buffer_size = original_height * original_width * PIXEL_SIZE;
    uint8_t *rotated_data = (uint8_t *)lv_mem_alloc(buffer_size);
    if (rotated_data == NULL) {
        LV_LOG_ERROR("Failed to allocate memory for rotated image");
        return false;
    }
    
    // 获取原始数据指针
    const uint8_t *src_data = current_img->data;
    
    // 执行像素级的90度顺时针旋转（优化版，避免重复计算）
    for (uint16_t y = 0; y < original_height; y++) {
        // 预计算源数据行起始位置
        const uint8_t *src_row = src_data + (y * original_width * PIXEL_SIZE);
        
        for (uint16_t x = 0; x < original_width; x++) {
            // 源像素位置
            const uint8_t *src_pixel = src_row + (x * PIXEL_SIZE);
            
            // 旋转后的目标位置：顺时针90度公式: (x, y) -> (y, width-1-x)
            uint16_t dst_x = y;
            uint16_t dst_y = original_width - 1 - x;
            
            // 目标像素位置
            uint8_t *dst_pixel = rotated_data + 
                                 (dst_y * original_height + dst_x) * PIXEL_SIZE;
            
            // 复制ARGB四个通道
            dst_pixel[0] = src_pixel[0]; // A
            dst_pixel[1] = src_pixel[1]; // R
            dst_pixel[2] = src_pixel[2]; // G
            dst_pixel[3] = src_pixel[3]; // B
        }
    }
    
    // 将旋转后的数据设置回canvas
    lv_canvas_set_buffer(canvas, rotated_data, original_height, original_width, 
                         LV_IMG_CF_TRUE_COLOR_ALPHA);
    
    // 可选：更新canvas显示尺寸
    lv_obj_set_size(canvas, original_height, original_width);
    
    if (old_buf) {
        lv_mem_free(old_buf);
    }

    // // 强制重绘
    // lv_obj_invalidate(canvas);
    
    return true;
}

static void free_photo_paths(void) {
    if (pl.pm.photo_paths) {
        for (int i = 0; i < pl.pm.photo_count; i++) {
            if (pl.pm.photo_paths[i]) {
                free(pl.pm.photo_paths[i]);
            }
        }
        free(pl.pm.photo_paths);
        pl.pm.photo_paths = NULL;
    }
    pl.pm.photo_count = 0;
    pl.pm.photo_capacity = 0;
}

static void ensure_capacity(void) {
    if (pl.pm.photo_count >= pl.pm.photo_capacity) {
        int new_capacity = pl.pm.photo_capacity == 0 ? 50 : pl.pm.photo_capacity * 2;
        char **new_paths = realloc(pl.pm.photo_paths, new_capacity * sizeof(char*));
        if (!new_paths) {
            printf("Failed to realloc photo_paths\n");
            return;
        }
        pl.pm.photo_paths = new_paths;
        pl.pm.photo_capacity = new_capacity;
    }
}

static void add_photo_path(const char *full_path) {
    ensure_capacity();
    pl.pm.photo_paths[pl.pm.photo_count] = strdup(full_path);
    if (pl.pm.photo_paths[pl.pm.photo_count]) {
        pl.pm.photo_count++;
    }
}

static int is_jpg_file(const char *filename) {
    if (!filename) return 0;

    size_t len = strlen(filename);
    if (len < 4) return 0;

    const char *ext = filename + len - 4;
    if (strcasecmp(ext, ".jpg") == 0) return 1;

    // 也支持.jpeg扩展名
    if (len >= 5) {
        ext = filename + len - 5;
        if (strcasecmp(ext, ".jpeg") == 0) return 1;
    }

    return 0;
}

static void scan_photo_directory_recursive(const char *dir_path) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[512];
    
    // 打开目录
    dir = opendir(dir_path);
    if (!dir) {
        printf("Cannot open directory: %s\n", dir_path);
        return;
    }

    // 遍历目录
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 "." 和 ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 构建完整路径
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // 获取文件状态
        if (stat(full_path, &file_stat) != 0) {
            continue;
        }

        // 如果是目录，递归扫描
        if (S_ISDIR(file_stat.st_mode)) {
            scan_photo_directory_recursive(full_path);
        }
        // 如果是普通文件，检查是否为jpg文件
        else if (S_ISREG(file_stat.st_mode) && is_jpg_file(entry->d_name)) {
            add_photo_path(full_path);
        }
    }

    closedir(dir);
}

static int compare_strings(const void *a, const void *b) {
    const char *str_a = *(const char**)a;
    const char *str_b = *(const char**)b;
    return strcmp(str_a, str_b);
}

static void scan_photo_directory(void) {
    // 释放之前的内存
    free_photo_paths();

    printf("Scanning directory: %s\n", pl.pm.photo_dir);

    // 递归扫描目录
    scan_photo_directory_recursive(pl.pm.photo_dir);

    // 按字母顺序排序
    if (pl.pm.photo_count > 1) {
        qsort(pl.pm.photo_paths, pl.pm.photo_count, sizeof(char*), compare_strings);
    }

    printf("Found %d photos\n", pl.pm.photo_count);
}

static void close_format_dialog() {
    if (pl.pm.mask_layer_del_dialog) {
        lv_obj_del(pl.pm.mask_layer_del_dialog);
        pl.pm.mask_layer_del_dialog = NULL;
    }
}

static void ask_dialog_yes_event_cb(lv_event_t *e) {
    int index = (int)(intptr_t)lv_event_get_user_data(e);

    if (index < 0 || index >= pl.pm.photo_count) {
        return;
    }

    const char *full_path = pl.pm.photo_paths[index];
    if (full_path) {
        // 删除文件
        if (remove(full_path) == 0) {
            printf("Deleted file: %s\n", full_path);
        } else {
            printf("Failed to delete file: %s\n", full_path);
            return;
        }
    }

    // 重新扫描目录并刷新列表
    show_photo_list();

    close_format_dialog();
}

static void ask_dialog_no_event_cb(lv_event_t *e) {
    (void)e; // 未使用参数
    close_format_dialog();
}

static void create_ask_dialog(lv_obj_t *parent, int index) {
    /* 照片库 - 删除确认对话框背景 */
    lv_obj_t *dialog_bg = lv_obj_create(parent);
    lv_obj_set_size(dialog_bg, LV_PCT(40), LV_PCT(40));
    lv_obj_set_pos(dialog_bg, 382, 216);
    lv_obj_set_style_bg_color(dialog_bg, TM_BG_SECONDARY, 0);             /* 对话框背景 */
    lv_obj_set_style_radius(dialog_bg, 10, 0);
    lv_obj_set_style_pad_all(dialog_bg, 20, 0);
    lv_obj_set_style_border_width(dialog_bg, 2, 0);
    lv_obj_set_style_border_color(dialog_bg, TM_BORDER_SECONDARY, 0);     /* 对话框边框 */
    lv_obj_clear_flag(dialog_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* 照片库 - 删除确认消息 */
    lv_obj_t *msg_label = lv_label_create(dialog_bg);
    lv_label_set_text(msg_label, get_string_for_language(languageSetting, "Delete_photo_confirm"));
    lv_obj_set_style_text_font(msg_label, &v853Font_hyby_30, 0);
    lv_obj_set_width(msg_label, LV_PCT(100));
    lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(msg_label, TM_TEXT_PRIMARY, 0);           /* 消息文字颜色 */
    lv_obj_align(msg_label, LV_ALIGN_TOP_MID, 0, LV_PCT(20));

    /* 照片库 - 确认按钮 */
    lv_obj_t *yes_btn = lv_btn_create(dialog_bg);
    lv_obj_set_size(yes_btn, 100, 40);
    lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(yes_btn, TM_SUCCESS, 0);                    /* 确认按钮背景(绿色) */
    lv_obj_set_style_radius(yes_btn, 8, 0);
    lv_obj_add_event_cb(yes_btn, ask_dialog_yes_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)index);
    lv_obj_t *yes_label = lv_label_create(yes_btn);
    lv_label_set_text(yes_label, get_string_for_language(languageSetting, "Yes"));
    lv_obj_set_style_text_font(yes_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(yes_label, lv_color_hex(0xFFFFFF), 0);    /* 按钮文字始终白色 */
    lv_obj_center(yes_label);
    
    /* 照片库 - 取消按钮 */
    lv_obj_t *no_btn = lv_btn_create(dialog_bg);
    lv_obj_set_size(no_btn, 100, 40);
    lv_obj_align(no_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(no_btn, TM_ERROR, 0);                       /* 取消按钮背景(红色) */
    lv_obj_set_style_radius(no_btn, 8, 0);
    lv_obj_add_event_cb(no_btn, ask_dialog_no_event_cb, LV_EVENT_CLICKED, NULL);    
    lv_obj_t *no_label = lv_label_create(no_btn);
    lv_label_set_text(no_label, get_string_for_language(languageSetting, "No"));
    lv_obj_set_style_text_font(no_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(no_label, lv_color_hex(0xFFFFFF), 0);     /* 按钮文字始终白色 */
    lv_obj_center(no_label);
}

static void show_format_sd_dialog(lv_obj_t *parent, int index) {
    // 如果已经存在对话框，先关闭
    close_format_dialog();

    /* 照片库 - 删除对话框遮罩层 */
    pl.pm.mask_layer_del_dialog = lv_obj_create(parent);
    lv_obj_set_size(pl.pm.mask_layer_del_dialog, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(pl.pm.mask_layer_del_dialog, TM_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(pl.pm.mask_layer_del_dialog, LV_OPA_50, 0);   /* 半透明遮罩 */
    lv_obj_set_style_radius(pl.pm.mask_layer_del_dialog, 0, 0);
    lv_obj_set_style_border_width(pl.pm.mask_layer_del_dialog, 0, 0);
    lv_obj_clear_flag(pl.pm.mask_layer_del_dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(pl.pm.mask_layer_del_dialog);

    // 创建询问对话框
    create_ask_dialog(pl.pm.mask_layer_del_dialog, index);
}

static void show_photo_list(void) {
    // 隐藏查看容器，显示列表容器
    if (pl.pv.cont && pl.pm.list_cont) {
        lv_obj_add_flag(pl.pv.cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(pl.pm.list_cont, LV_OBJ_FLAG_HIDDEN);
    }

    // 清空当前列表内容
    if (pl.pm.list) {
        lv_obj_clean(pl.pm.list);
    }

    // 扫描目录,将list元素赋值
    scan_photo_directory();

    if (!pl.pm.list) return;

    if (pl.pm.photo_count == 0) {
        /* 照片库 - 空列表状态 */
        lv_obj_clear_flag(pl.pm.list, LV_OBJ_FLAG_SCROLLABLE);

        // lv_obj_set_style_text_font(pl.pm.info_label, &v853Font_hyby_30, 0);
        // lv_label_set_text(pl.pm.info_label, get_string_for_language(languageSetting, "Photo_scanning"));

        /* 空列表 - 主提示文字 */
        pl.pm.empty_label1 = lv_label_create(pl.pm.list);
        lv_obj_set_style_text_font(pl.pm.empty_label1, &v853Font_hyby_30, 0);
        lv_obj_set_style_text_color(pl.pm.empty_label1, TM_TEXT_PRIMARY, 0);   /* 主提示文字颜色 */
        lv_label_set_text(pl.pm.empty_label1, get_string_for_language(languageSetting, "No_photo_found"));
        lv_obj_center(pl.pm.empty_label1);

        char buff[256];
        memset(buff, 0, sizeof(buff));
        sprintf(buff, "%s %s\n", get_string_for_language(languageSetting, "check_contents"), pl.pm.photo_dir);
        pl.pm.empty_label2 = lv_label_create(pl.pm.list);
        lv_obj_set_style_text_font(pl.pm.empty_label2, &v853Font_hyby_30, 0);
        lv_label_set_text(pl.pm.empty_label2, buff);
        lv_obj_set_style_text_color(pl.pm.empty_label2, TM_TEXT_SECONDARY, 0); /* 提示文字 */
        lv_obj_align(pl.pm.empty_label2, LV_ALIGN_BOTTOM_MID, 0, -30);

        pl.pm.empty_label3 = lv_label_create(pl.pm.list);
        lv_obj_set_style_text_font(pl.pm.empty_label3, &v853Font_hyby_30, 0);
        lv_label_set_text(pl.pm.empty_label3, get_string_for_language(languageSetting, "Supported_formats"));
        lv_obj_set_style_text_color(pl.pm.empty_label3, TM_TEXT_DISABLED, 0);   /* 较淡的提示文字 */
        lv_obj_align(pl.pm.empty_label3, LV_ALIGN_BOTTOM_MID, 0, 0);
    } else {
        lv_obj_add_flag(pl.pm.list, LV_OBJ_FLAG_SCROLLABLE);

        // 创建列表项（替换原有 lv_list_add_btn(...) 调用）
        for (int i = 0; i < pl.pm.photo_count; i++) {
            const char *full_path = pl.pm.photo_paths[i];
            const char *relative_path = full_path + strlen(pl.pm.photo_dir);
            if (*relative_path == '/') relative_path++;

            // 先创建无图标的按钮（不传 symbol）
            lv_obj_t *list_btn = lv_btn_create(pl.pm.list);
            lv_obj_remove_style_all(list_btn); // 移除默认样式
            lv_obj_set_width(list_btn, lv_pct(100));
            lv_obj_set_height(list_btn, 80);
            lv_obj_add_flag(list_btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
            /* 照片列表项 - 按钮样式 */
            lv_obj_set_style_bg_color(list_btn, TM_BG_TERTIARY, 0);           /* 列表项背景 */
            lv_obj_set_style_bg_opa(list_btn, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(list_btn, TM_BG_HOVER, LV_STATE_PRESSED); /* 按下状态背景 */
            lv_obj_set_style_bg_opa(list_btn, LV_OPA_COVER, LV_STATE_PRESSED);
            lv_obj_set_style_border_color(list_btn, TM_BORDER_SECONDARY, 0);  /* 分割线 */
            lv_obj_set_style_border_width(list_btn, 1, 0);
            lv_obj_set_style_border_opa(list_btn, LV_OPA_50, 0);
            lv_obj_set_style_border_side(list_btn, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_TOP, 0);
            lv_obj_set_style_radius(list_btn, 0, 0);
            lv_obj_set_style_pad_all(list_btn, 0, 0);

            // 创建真正的图片控件作为图标（第0个子对象）
            lv_obj_t *img_icon = lv_obj_create(list_btn);
            lv_obj_set_size(img_icon, 80, 80);   // 根据需要设置图标尺寸（高低相同以便垂直居中）
            lv_obj_align(img_icon, LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_set_style_pad_all(img_icon, 0, 0);
            lv_obj_set_style_bg_opa(img_icon, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(img_icon, 0, 0);
            lv_obj_t *icon = lv_img_create(img_icon);
            // 如果你的LVGL文件系统需要前缀，比如 "S:", 加上前缀：
            // char img_src[512];
            // snprintf(img_src, sizeof(img_src), "S:%s", full_path);
            /* 照片列表项 - 图标 */
            lv_obj_set_style_text_font(icon, &v853Font_hyby_30, 0);
            lv_obj_set_style_text_color(icon, TM_ICON_PRIMARY, 0);           /* 图标颜色 */
            lv_img_set_src(icon, LV_SYMBOL_IMAGE);
            lv_obj_center(icon);

            /* 照片列表项 - 文件名标签 */
            lv_obj_t *label = lv_label_create(list_btn);
            lv_obj_set_style_text_font(label, &v853Font_hyby_30, 0);
            lv_obj_set_style_text_color(label, TM_TEXT_PRIMARY, 0);          /* 文件名颜色 */
            lv_label_set_text_fmt(label, "%s", relative_path);
            lv_obj_align(label, LV_ALIGN_LEFT_MID, 80, 0);

            // 存储索引并添加事件
            lv_obj_set_user_data(list_btn, (void*)(intptr_t)i);
            lv_obj_add_event_cb(list_btn, list_btn_event_handler, LV_EVENT_CLICKED, NULL);

            // 删除按钮（保持你现有实现）
            lv_obj_t *del_btn = lv_btn_create(list_btn);
            lv_obj_set_size(del_btn, 50, 50);
            lv_obj_set_style_bg_opa(del_btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(del_btn, 0, 0);
            lv_obj_align(del_btn, LV_ALIGN_RIGHT_MID, -30, 0);
            /* 照片列表项 - 删除按钮图标（与视频库保持一致使用TM_ICON_SECONDARY） */
            lv_obj_t *del_icon = lv_img_create(del_btn);
            lv_obj_set_style_img_recolor(del_icon, TM_ICON_SECONDARY, 0);    /* 删除图标颜色 */
            lv_obj_set_style_img_recolor_opa(del_icon, LV_OPA_COVER, 0);
            lv_obj_set_style_img_recolor(del_icon, TM_ICON_DISABLED, LV_STATE_PRESSED); /* 按下状态 */
            lv_obj_set_style_img_recolor_opa(del_icon, LV_OPA_COVER, LV_STATE_PRESSED);
            lv_img_set_src(del_icon, &delete);
            lv_obj_center(del_icon);
            lv_obj_add_event_cb(del_btn, del_btn_event_handler, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        }
    }

    // 更新信息标签
    if (pl.pm.info_label) {
        char buff[256];
        memset(buff, 0, sizeof(buff));
        sprintf(buff, "%s %d %s", get_string_for_language(languageSetting, "Found"), \
                                pl.pm.photo_count, \
                                get_string_for_language(languageSetting, "photos"));
        // lv_label_set_text_fmt(pl.pm.info_label, "Found %d photos", pl.pm.photo_count);
        lv_obj_set_style_text_font(pl.pm.info_label, &v853Font_hyby_30, 0);
        lv_label_set_text(pl.pm.info_label, buff);
    }
}

static void load_and_display_image(const char *path) {
    if (!path || !pl.pv.img) return;

    char full_path[512];
    memset(full_path, 0, sizeof(full_path));
    snprintf(full_path, sizeof(full_path), "S:%s", path);
    printf("Loading image: %s\n", full_path);

    // 从路径中提取相对路径
    const char *relative_path = path + strlen(pl.pm.photo_dir);
    if (*relative_path == '/') {
        relative_path++;  // 跳过开头的'/'
    }

    canvas_display_image(pl.pv.img, full_path);
    pl.pv.has_photo_cache = 1;

    // 更新文件名标签
    if (pl.pv.file_label) {
        lv_label_set_text_fmt(pl.pv.file_label, "%s", relative_path);
    }
}

static void update_zoom_label(void) {
    if (pl.pv.zoom_label) {
        int percent = 100;
        switch (pl.pv.current_zoom_value) {
        case PERCENT50:
            percent = 50;
            break;
        case PERCENT75:
            percent = 75;
            break;
        case PERCENT100:
            percent = 100;
            break;
        case PERCENT150:
            percent = 150;
            break;
        case PERCENT200:
            percent = 200;
            break;
        default:
            break;
        }
        lv_label_set_text_fmt(pl.pv.zoom_label, "zoom: %d%%", percent);
    }
}

static void show_photo_viewer(int index) {
    if (index < 0 || index >= pl.pm.photo_count) {
        return;
    }
    if (!pl.pv.img) return;

    pl.pm.current_index = index;

    // 隐藏列表容器，显示查看容器
    if (pl.pm.list_cont && pl.pv.cont) {
        lv_obj_add_flag(pl.pm.list_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(pl.pv.cont, LV_OBJ_FLAG_HIDDEN);
    }

    // 初始化拖拽的偏移起始位置
    pl.pv.offset_x = IMGORIGINX;
    pl.pv.offset_y = IMGORIGINY;
    // 重新加载图片
    load_and_display_image(pl.pm.photo_paths[pl.pm.current_index]);
    // 新图片移动到初始位置
    canvas_move(pl.pv.img, IMGORIGINX, IMGORIGINY);
    // 更新底部栏的标签
    pl.pv.current_zoom_value = PERCENT100;  // 重置为100%    
    update_zoom_label();
}

static void list_btn_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(btn);
    show_photo_viewer(index);
}

static void del_btn_event_handler(lv_event_t *e) {
    lv_event_stop_bubbling(e); // 阻止事件冒泡到父对象
    lv_obj_t *btn = lv_event_get_target(e);
    int index = (int)(intptr_t)lv_event_get_user_data(e);

    
    if (index < 0 || index >= pl.pm.photo_count) {
        return;
    }

    show_format_sd_dialog(lv_scr_act(), index);
}

static void back_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    // 隐藏查看容器，显示列表容器
    if (pl.pv.cont && pl.pm.list_cont) {
        lv_obj_add_flag(pl.pv.cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(pl.pm.list_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

static void refresh_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    // if (pl.is_initialized) {
    show_photo_list();
    // }
}

static void prev_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    // if (pl.is_initialized && pl.pm.current_index > 0) {
    if (pl.pm.current_index > 0) {
        show_photo_viewer(pl.pm.current_index - 1);
    }
}

static void next_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    // if (pl.is_initialized && pl.pm.current_index >= 0 && 
    if (pl.pm.current_index >= 0 && pl.pm.current_index < pl.pm.photo_count - 1) {
        show_photo_viewer(pl.pm.current_index + 1);
    }
}

static void on_image_touch(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_PRESSED:                              // 记录按下的位置和按下的时间辍
        {
#if GESTURECHANGEENBALE
            lv_indev_t *indev = lv_indev_get_act();
            lv_indev_data_t data;
            _lv_indev_read(indev, &data);

            pl.pv.gesture_start_x = data.point.x;
            pl.pv.gesture_start_y = data.point.y;
            printf("gesture_start_x = %d, gesture_start_y = %d\n", pl.pv.gesture_start_x, pl.pv.gesture_start_y);
            pl.pv.gesture_start_time = lv_tick_get();
            pl.pv.is_gesture_active = 1;
#endif
        }
        break;
    case LV_EVENT_PRESSING:                             // 移动图片
        {
            lv_indev_t *indev = lv_indev_get_act();
            lv_indev_data_t data;
            _lv_indev_read(indev, &data);

            /* 拖动图片逻辑 */
            if (!pl.pv.is_dragging) {
                /* 开始拖动 */
                pl.pv.is_dragging = 1;
                pl.last_x = data.point.x;
                pl.last_y = data.point.y;
            } else {
                /* 计算相对移动 */
                int dx = data.point.x - pl.last_x;
                int dy = data.point.y - pl.last_y;

                /* 更新图片位置（竖屏适配） */
                pl.pv.offset_x = pl.pv.offset_x - dy;
                pl.pv.offset_y = pl.pv.offset_y + dx;

                printf("[PhotoLibrary] Dragging: dx=%d dy=%d pos=(%d,%d)\n", 
                    dx, dy, pl.pv.offset_x, pl.pv.offset_y);

                /* 应用变换 */
                if (pl.pv.img) {
                    canvas_move(pl.pv.img, pl.pv.offset_x, pl.pv.offset_y);
                }

                /* 更新最后位置 */
                pl.last_x = data.point.x;
                pl.last_y = data.point.y;
            }
        }
        break;    
    case LV_EVENT_RELEASED:                             // 记录释放的位置和释放的时间辍
        {
            pl.pv.is_dragging = 0;
#if GESTURECHANGEENBALE
            // 检测快速滑动（手势切换图片）
            if (pl.pv.is_gesture_active) {
                lv_indev_t *indev = lv_indev_get_act();
                lv_indev_data_t data;
                _lv_indev_read(indev, &data);

                int dx = data.point.x - pl.pv.gesture_start_x;
                int dy = data.point.y - pl.pv.gesture_start_y;
                uint32_t time_diff = lv_tick_elaps(pl.pv.gesture_start_time);

                printf("elaps time = %d\n", time_diff);

                // 如果是水平快速滑动且时间很短，切换图片
                if (time_diff < 180 && abs(dx) > 150 && abs(dx) > abs(dy) * 2) {
                    if (dx > 0) {
                        // 右滑，上一张
                        prev_btn_event_handler(e);
                    } else {
                        // 左滑，下一张
                        next_btn_event_handler(e);
                    }
                }

                pl.pv.is_gesture_active = 0;
            }
#endif
        }
        break;
    }
}

static void small_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    --pl.pv.current_zoom_value;
    if (pl.pv.current_zoom_value >= 5 || pl.pv.current_zoom_value < 1) {
        printf("Don't zoom this value\n");
        ++pl.pv.current_zoom_value;
        return;
    }

    // 设置缩放
    switch (pl.pv.current_zoom_value)
    {
    case 4:
        canvas_zoom(pl.pv.img, (float)(1.5 / 2.0));
        break;
    case 3:
        canvas_zoom(pl.pv.img, (float)(1.0 / 1.5));
        break;
    case 2:
        canvas_zoom(pl.pv.img, (float)(0.75 / 1.0));
        break;
    case 1:
        canvas_zoom(pl.pv.img, (float)(0.5 / 0.75));
        break;
    default:
        break;
    }
    update_zoom_label();
}

static void big_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    // set_image_zoom_level(1);  // 100%
    ++pl.pv.current_zoom_value;
    if (pl.pv.current_zoom_value > 5 || pl.pv.current_zoom_value <= 1) {
        printf("Don't zoom this value\n");
        --pl.pv.current_zoom_value;
        return;
    }

    // 设置缩放
    switch (pl.pv.current_zoom_value)
    {
    case 5:
        canvas_zoom(pl.pv.img, (float)(2.0 / 1.5));
        break;
    case 4:
        canvas_zoom(pl.pv.img, (float)(1.5 / 1.0));
        break;
    case 3:
        canvas_zoom(pl.pv.img, (float)(1.0 / 0.75));
        break;
    case 2:
        canvas_zoom(pl.pv.img, (float)(0.75 / 0.5));
        break;
    default:
        break;
    }
    // apply_image_transform();
    // lv_obj_invalidate(pl.pv.img);
    // if (pl.pv.img->parent) {
    //     lv_obj_invalidate(pl.pv.img->parent);  // 也使父容器失效
    // }
    update_zoom_label();
}

static void rota_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    rotate_canvas_90_clockwise_argb8888(pl.pv.img);
}

static void reset_btn_event_handler(lv_event_t *e) {
    LV_UNUSED(e);
    // 初始化拖拽的偏移起始位置
    pl.pv.offset_x = IMGORIGINX;
    pl.pv.offset_y = IMGORIGINY;
    // 重新加载图片
    load_and_display_image(pl.pm.photo_paths[pl.pm.current_index]);
    // 新图片移动到初始位置
    canvas_move(pl.pv.img, IMGORIGINX, IMGORIGINY);
    // 更新底部栏的标签
    pl.pv.current_zoom_value = PERCENT100;  // 重置为100%    
    update_zoom_label();
}

/**
 * @brief 初始化照片库数据结构
 */
static void pl_init(void) {
    printf("[PhotoLibrary] Initializing data structure...\n");
    
    /* 重置主界面数据 */
    pl.pm.photo_paths = NULL;
    pl.pm.photo_count = 0;
    pl.pm.photo_capacity = 0;
    pl.pm.current_index = -1;
    strncpy(pl.pm.photo_dir, DEFAULT_PHOTO_DIR, sizeof(pl.pm.photo_dir) - 1);
    pl.pm.photo_dir[sizeof(pl.pm.photo_dir) - 1] = '\0';
    pl.pm.cont = NULL;
    pl.pm.top_bar = NULL;
    pl.pm.scan_bar = NULL;
    pl.pm.left_placeholder = NULL;
    pl.pm.title = NULL;
    pl.pm.refresh_btn = NULL;
    pl.pm.refresh_icon = NULL;
    pl.pm.info_label = NULL;
    pl.pm.list_cont = NULL;
    pl.pm.list = NULL;
    pl.pm.empty_label1 = NULL;
    pl.pm.empty_label2 = NULL;
    pl.pm.empty_label3 = NULL;
    pl.pm.mask_layer_del_dialog = NULL;

    /* 重置图片查看器数据 */
    pl.pv.has_photo_cache = 0;
    pl.pv.current_zoom_value = PERCENT100;
    pl.pv.offset_x = 0;
    pl.pv.offset_y = 0;
    pl.pv.drag_start_x = 0;
    pl.pv.drag_start_y = 0;
    pl.pv.img_start_x = 0;
    pl.pv.img_start_y = 0;
    pl.pv.is_dragging = 0;

    pl.pv.gesture_start_x = 0;
    pl.pv.gesture_start_y = 0;
    pl.pv.is_gesture_active = 0;
    pl.pv.gesture_start_time = 0;
    pl.pv.cont = NULL;
    pl.pv.top_bar = NULL;
    pl.pv.back_btn = NULL;
    pl.pv.back_icon = NULL;       
    pl.pv.file_label = NULL;
    pl.pv.nav_btn_cont = NULL;
    pl.pv.prev_btn = NULL;
    pl.pv.prev_icon = NULL;
    pl.pv.next_btn = NULL;
    pl.pv.next_icon = NULL;
    pl.pv.img_wrapper = NULL;

    pl.pv.img = NULL;
    pl.pv.current_canvas_buf = NULL;
    pl.pv.current_buf_size = 0;
    pl.pv.cached_img_path = NULL;

    pl.pv.bottom_bar = NULL;
    pl.pv.small_btn = NULL;
    pl.pv.small_btn_icon = NULL;
    pl.pv.big_btn = NULL;
    pl.pv.big_btn_icon = NULL;
    pl.pv.rota_btn = NULL;
    pl.pv.rota_btn_icon = NULL;
    pl.pv.reset_btn = NULL;
    pl.pv.reset_btn_icon = NULL;
    pl.pv.zoom_label = NULL;
    
    /* 重置拖动状态变量 */
    pl.last_x = 0;
    pl.last_y = 0;
    pl.old_img_path = NULL;
    
    /* 重置初始化标志 */
    pl.is_initialized = false;
    
    printf("[PhotoLibrary] Data structure initialized\n");
}

/*====================================================================================
 * 公共API实现
 *====================================================================================*/

/**
 * @brief 创建照片选择界面
 */
void create_photo_select_ui(lv_obj_t *parent) {
    if (!parent) return;

    /* 初始化数据结构 */
    pl_init();

    /* 创建UI组件 */
    create_ui_components(parent);

    /* 显示照片列表 */
    show_photo_list();

    /* 设置初始化标志 */
    pl.is_initialized = true;
    printf("[PhotoLibrary] UI created successfully\n");
}

/**
 * @brief 清理照片库资源
 */
void cleanup_photo_library(void) {
    printf("[PhotoLibrary] Starting cleanup...\n");

    /* 1. 清理图片缓存 */
    if (pl.old_img_path) {
        printf("[PhotoLibrary] Invalidating old image cache: %s\n", pl.old_img_path);
        lv_img_cache_invalidate_src(pl.old_img_path);
        pl.old_img_path = NULL;
    }
    
    // 2. 如果当前有加载的图片，也清理其缓存
    if (pl.pv.has_photo_cache && pl.pm.photo_paths && 
        pl.pm.current_index >= 0 && pl.pm.current_index < pl.pm.photo_count) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "S:%s", 
                pl.pm.photo_paths[pl.pm.current_index]);
        printf("Invalidating current image cache: %s\n", full_path);
        lv_img_cache_invalidate_src(full_path);
    }

    // 3. 安全释放canvas缓冲区（如果存在）
    if (pl.pv.img) {
        lv_img_dsc_t *canvas_dsc = lv_canvas_get_img(pl.pv.img);
        if (canvas_dsc && canvas_dsc->data) {
            void *buf_to_free = (void *)canvas_dsc->data;
            if (buf_to_free) {
                printf("buf_to_free %zu\n", sizeof(&buf_to_free));
                lv_mem_free(buf_to_free);
            }
            
            // 设置一个最小缓冲区，防止LVGL访问已释放内存
            static uint8_t dummy_buf[4] = {0};
            lv_canvas_set_buffer(pl.pv.img, dummy_buf, 1, 1, LV_IMG_CF_TRUE_COLOR_ALPHA);
            // lv_canvas_set_buffer(pl.pv.img, NULL, 0, 0, LV_IMG_CF_TRUE_COLOR_ALPHA);


            printf("Freed canvas buffer\n");
        }
        
        // 移除所有事件回调
        lv_obj_remove_event_cb(pl.pv.img, on_image_touch);
    }

    // 4. 移除其他按钮的事件回调（防止在删除过程中触发）
    if (pl.pm.refresh_btn) {
        lv_obj_remove_event_cb(pl.pm.refresh_btn, refresh_btn_event_handler);
    }
    if (pl.pv.back_btn) {
        lv_obj_remove_event_cb(pl.pv.back_btn, back_btn_event_handler);
    }
    if (pl.pv.prev_btn) {
        lv_obj_remove_event_cb(pl.pv.prev_btn, prev_btn_event_handler);
    }
    if (pl.pv.next_btn) {
        lv_obj_remove_event_cb(pl.pv.next_btn, next_btn_event_handler);
    }
    if (pl.pv.small_btn) {
        lv_obj_remove_event_cb(pl.pv.small_btn, small_btn_event_handler);
    }
    if (pl.pv.big_btn) {
        lv_obj_remove_event_cb(pl.pv.big_btn, big_btn_event_handler);
    }
    if (pl.pv.rota_btn) {
        lv_obj_remove_event_cb(pl.pv.rota_btn, rota_btn_event_handler);
    }
    if (pl.pv.reset_btn) {
        lv_obj_remove_event_cb(pl.pv.reset_btn, reset_btn_event_handler);
    }

    // 5. 释放照片路径内存
    free_photo_paths();

    // 6. 删除UI容器及其所有子对象
    if (pl.pm.cont) {
        lv_obj_del(pl.pm.cont);
        printf("Deleted UI container\n");
    }

    // 7. 重新初始化所有结构体和静态变量
    pl_init();

    printf("Photo library cleanup completed.\n");
}

void reflush_photo_library(void) {
    show_photo_list();
}