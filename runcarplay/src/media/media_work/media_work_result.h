/**
 * @file media_work_result.h
 * @brief 媒体库工作线程 - 结果类型与结构定义
 *
 * 主线程在 lv_timer 中 poll 到结果后按类型分发；含动态内存的结果由主线程或 media_work_free_result 释放。
 */

#ifndef MEDIA_WORK_RESULT_H
#define MEDIA_WORK_RESULT_H

#ifdef __cplusplus
extern "C" {
#endif

/** 视频 tab 类型（与 videoLibrary 一致） */
#define MEDIA_VIDEO_TAB_FRONT   0
#define MEDIA_VIDEO_TAB_REAR    1
#define MEDIA_VIDEO_TAB_URGENT  2
#define MEDIA_VIDEO_TAB_COUNT   3

/** 照片 tab 类型 */
#define MEDIA_PHOTO_TAB_FRONT   0
#define MEDIA_PHOTO_TAB_REAR    1
#define MEDIA_PHOTO_TAB_COUNT   2

/** 结果类型枚举 */
typedef enum {
    MEDIA_RESULT_NONE = 0,
    MEDIA_RESULT_STOP_PREVIEW,
    MEDIA_RESULT_STORAGE_QUERY,
    MEDIA_RESULT_FORMAT_SD,
    MEDIA_RESULT_SCAN_VIDEO,
    MEDIA_RESULT_SCAN_PHOTO
} media_result_type_t;

/** 存储查询结果：主线程只读后丢弃，无需 free */
typedef struct {
    long long used_mb;   /* -1 表示未挂载/失败 */
    long long total_mb;
} media_result_storage_t;

/** 格式化结果：主线程只读后丢弃 */
typedef struct {
    int success;        /* 1 成功，0 失败 */
} media_result_format_t;

/** 视频扫描结果：主线程接管 paths 后需调用 media_work_free_result(MEDIA_RESULT_SCAN_VIDEO, &r) 或自行 free 每条 path 再 free paths */
typedef struct {
    int tab_index;      /* MEDIA_VIDEO_TAB_* */
    char **paths;       /* 路径数组，主线程负责释放 */
    int count;
} media_result_scan_video_t;

/** 照片扫描结果：主线程接管 paths 后需调用 media_work_free_result(MEDIA_RESULT_SCAN_PHOTO, &r) 或自行 free */
typedef struct {
    int tab_index;      /* MEDIA_PHOTO_TAB_* */
    char **paths;       /* 路径数组，主线程负责释放 */
    int count;
} media_result_scan_photo_t;

/** 单条结果（联合体） */
typedef struct {
    media_result_type_t type;
    union {
        media_result_storage_t storage;
        media_result_format_t format;
        media_result_scan_video_t scan_video;
        media_result_scan_photo_t scan_photo;
    } u;
} media_result_t;

/**
 * @brief 释放结果中的动态内存（仅对 SCAN_VIDEO/SCAN_PHOTO 有效）
 * @param r 指向 poll 得到的 media_result_t，释放后不再使用
 */
void media_work_free_result(media_result_t *r);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_WORK_RESULT_H */
