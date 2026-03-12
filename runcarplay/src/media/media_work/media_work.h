/**
 * @file media_work.h
 * @brief 媒体库工作线程 - 扫描、停预览、存储查询、格式化等耗时任务
 *
 * 任务在工作线程执行，结果通过队列供主线程在 lv_timer 中轮询，主线程仅做 UI 更新。
 * 媒体库路径与常量统一定义于此，供本模块与 UI 层共用。
 */

#ifndef MEDIA_WORK_H
#define MEDIA_WORK_H

#include "media_work_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 媒体库路径与常量（原 media_paths.h 内容） ========== */
#define MEDIA_TF_MOUNT_PATH           "/mnt/extsd"
#define MEDIA_VIDEO_DIR_FRONT         "/mnt/extsd/recorder/frontCamera"
#define MEDIA_VIDEO_DIR_REAR          "/mnt/extsd/recorder/rearCamera"
#define MEDIA_VIDEO_DIR_URGENT_FRONT  "/mnt/extsd/recorderUrgent/frontCamera"
#define MEDIA_VIDEO_DIR_URGENT_REAR   "/mnt/extsd/recorderUrgent/rearCamera"
#define MEDIA_PHOTO_DIR_FRONT         "/mnt/extsd/DVRpic/frontPic"
#define MEDIA_PHOTO_DIR_REAR          "/mnt/extsd/DVRpic/rearPic"
#define MEDIA_MAX_FILES               100
#define MEDIA_MAX_PATH_LEN            256
#define MEDIA_TF_DEVICE                "/dev/mmcblk0"

/** 停预览任务完成回调：在工作线程中执行，仅做 stopPreview 等阻塞调用，勿调 LVGL */
typedef void (*media_work_stop_preview_fn)(void *userdata);

/** 结果入队后通知回调（在工作线程中调用，仅做轻量级通知如投递到主线程队列，勿做重逻辑） */
typedef void (*media_work_result_notify_fn)(void *userdata);

/**
 * @brief 初始化媒体工作线程与队列
 * @return 0 成功，-1 失败
 */
int media_work_init(void);

/**
 * @brief 反初始化，停止工作线程
 */
void media_work_deinit(void);

/**
 * @brief 设置“停预览”任务执行时调用的回调（由应用层实现，内部调用 stopPreview 等）
 */
void media_work_set_stop_preview_callback(media_work_stop_preview_fn fn, void *userdata);

/**
 * @brief 提交停预览任务（异步）
 */
void media_work_submit_stop_preview(void);

/**
 * @brief 主线程轮询：是否有停预览任务在本轮完成（兼容旧接口）
 * @return 1 有任务在本轮完成，0 无
 */
int media_work_poll_done(void);

/**
 * @brief 提交存储查询任务（工作线程调用 TFCard 获取 used/total，结果入队）
 */
void media_work_submit_storage_query(void);

/**
 * @brief 提交 TF 卡格式化任务（工作线程调用 TFCard 格式化，结果入队）
 */
void media_work_submit_format_sd(void);

/**
 * @brief 提交视频列表扫描任务
 * @param tab_index MEDIA_VIDEO_TAB_FRONT / MEDIA_VIDEO_TAB_REAR / MEDIA_VIDEO_TAB_URGENT
 */
void media_work_submit_scan_video(int tab_index);

/**
 * @brief 提交照片列表扫描任务
 * @param tab_index MEDIA_PHOTO_TAB_FRONT / MEDIA_PHOTO_TAB_REAR
 */
void media_work_submit_scan_photo(int tab_index);

/**
 * @brief 设置“有结果入队”时的通知回调（可选）。工作线程在 result_push 后调用，用于主线程用 lv_async_call 等触发一次 drain，减少轮询占用。
 */
void media_work_set_result_notify_callback(media_work_result_notify_fn fn, void *userdata);

/**
 * @brief 主线程轮询结果队列（在 lv_timer 或 lv_async 回调中调用），取出一条结果
 * @param out 输出一条结果，由调用方根据 type 分发并可选调用 media_work_free_result
 * @return 1 取到一条结果，0 队列空
 */
int media_work_poll_results(media_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_WORK_H */
