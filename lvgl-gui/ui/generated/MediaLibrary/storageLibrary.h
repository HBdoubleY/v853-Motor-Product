/**
 * @file storageLibrary.h
 * @brief 存储空间模块 - 管理SD卡存储空间显示和格式化
 * 
 * 提供SD卡容量显示、格式化等功能
 */

#pragma once

#include "../lvgl-gui/lvgl/lvgl.h"
#include "media_library_setting.h"
#include "media_theme_manager.h"
#include "media_ui_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*====================================================================================
 * FAT32常量定义
 *====================================================================================*/

#define BYTES_PER_SECTOR     512
#define SECTORS_PER_CLUSTER  8
#define RESERVED_SECTORS     32
#define FAT_COUNT            2
#define ROOT_DIR_CLUSTER     2
#define FAT_ENTRY_SIZE       4
#define FAT_END_OF_CHAIN     0x0FFFFFF8
#define FAT_BAD_CLUSTER      0x0FFFFFF7
#define FAT_FREE_CLUSTER     0x00000000
#define BOOT_SIGNATURE_55AA  0xAA55
#define TARGET_PROCESS       "SDUpgrade"
#define SD_CARD_DEVICE       "/dev/mmcblk0"

/*====================================================================================
 * 字体资源声明
 *====================================================================================*/

LV_FONT_DECLARE(v853Font_hyby_30)
LV_FONT_DECLARE(v853Font_hyby_40)

/*====================================================================================
 * 类型定义
 *====================================================================================*/

typedef struct {
    lv_obj_t *pb;               // 图库和视频库界面
    lv_obj_t *container;        // 主界面容器
    lv_obj_t *back_btn;
    lv_obj_t *back_icon;
    lv_obj_t *title_label;
    lv_obj_t *progress_bar;
    lv_obj_t *percent_label;
    lv_obj_t *details_label;
    lv_obj_t *format_btn;
    lv_obj_t *format_btn_icon;
} storageLibrary_t;

/**
 * @brief 格式化对话框结构体
 */
typedef struct {
    lv_obj_t *mask_layer;           /* 遮罩层 */
    lv_obj_t *dialog_bg;            /* 对话框背景 */
    lv_obj_t *msg_label;            /* 消息标签 */
    lv_obj_t *yes_btn;              /* 确认按钮 */
    lv_obj_t *no_btn;               /* 取消按钮 */
    lv_obj_t *result_dialog;        /* 结果对话框 */
    lv_timer_t *close_timer;        /* 自动关闭定时器 */
} storageLibrary_format_window_t;

/**
 * @brief 存储库主结构体
 */
typedef struct {
    storageLibrary_t sl;                    /* 主界面UI组件 */
    storageLibrary_format_window_t sf;      /* 格式化对话框UI组件 */
    bool is_initialized;                    /* 初始化标志 */
} storageLibrary_ctx_t;

/*====================================================================================
 * 全局变量声明
 *====================================================================================*/

/* 存储库全局实例（在storageLibrary.c中定义） */
extern storageLibrary_ctx_t g_storage;

/*====================================================================================
 * FAT32结构定义
 *====================================================================================*/

#pragma pack(push, 1)
typedef struct {
    uint8_t  jump[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    
    // FAT32 扩展部分
    uint32_t sectors_per_fat_32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
    uint8_t  boot_code[420];
    uint16_t boot_signature_55aa;
} Fat32BootSector;

typedef struct {
    uint32_t signature1;
    uint8_t  reserved1[480];
    uint32_t signature2;
    uint32_t free_clusters;
    uint32_t next_free_cluster;
    uint8_t  reserved2[12];
    uint32_t signature3;
} FSInfoSector;
#pragma pack(pop)

// 入口函数
void create_storage_select_ui(lv_obj_t *parent);
// 回收资源
void cleanup_storage_library(void);

void reflush_storage_display(void);

#ifdef __cplusplus
}
#endif