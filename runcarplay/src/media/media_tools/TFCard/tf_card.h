/**
 * @file tf_card.h
 * @brief TF 卡模块 - TF 检测、格式化、存储容量查询
 *
 * 供 UI 或 media_work 调用；耗时操作建议在工作线程中执行，结果通过队列回主线程。
 */

#ifndef TF_CARD_H
#define TF_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

/** 默认 TF 卡挂载路径 */
#define TF_CARD_MOUNT_PATH     "/mnt/extsd"
/** 默认块设备 */
#define TF_CARD_DEVICE         "/dev/mmcblk0"

/**
 * @brief 检测 TF 卡是否已挂载（设备是否存在）
 * @return 1 已挂载/设备存在，0 未挂载
 */
int tf_card_is_mounted(void);

/**
 * @brief 获取 TF 卡存储容量（单位：字节）
 * @param used_bytes  输出已使用字节数，可为 NULL
 * @param total_bytes 输出总字节数，可为 NULL
 * @return 0 成功，-1 失败（如未挂载或 statvfs 失败）
 */
int tf_card_get_storage_bytes(unsigned long long *used_bytes, unsigned long long *total_bytes);

/**
 * @brief 获取 TF 卡存储容量（单位：MB，兼容旧逻辑）
 * @param used_mb  输出已使用 MB，可为 NULL
 * @param total_mb 输出总 MB，可为 NULL
 * @return 0 成功，-1 失败
 */
int tf_card_get_storage_mb(long long *used_mb, long long *total_mb);

/**
 * @brief 格式化 TF 卡（在工作线程中调用）
 * @param device 块设备路径，如 "/dev/mmcblk0"
 * @param volume_label 卷标，如 "SD_CARD"
 * @param quick 1 快速格式化，0 完整格式化
 * @return 0 成功，-1 失败
 */
int tf_card_format(const char *device, const char *volume_label, int quick);

#ifdef __cplusplus
}
#endif

#endif /* TF_CARD_H */
