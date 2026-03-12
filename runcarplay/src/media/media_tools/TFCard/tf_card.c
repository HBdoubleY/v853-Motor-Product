/**
 * @file tf_card.c
 * @brief TF 卡模块实现 - TF 检测、存储容量查询、格式化（桩，实机由 storageLibrary 迁入）
 */

#include "tf_card.h"
#include <sys/statvfs.h>
#include <unistd.h>

int tf_card_is_mounted(void)
{
    if (access(TF_CARD_DEVICE, F_OK) == 0)
        return 1;
    return 0;
}

int tf_card_get_storage_bytes(unsigned long long *used_bytes, unsigned long long *total_bytes)
{
    struct statvfs fs_info;

    /* 拔卡后设备节点可能消失，先检查避免 statvfs 在部分平台上阻塞；设备不存在则直接返回失败 */
    if (access(TF_CARD_DEVICE, F_OK) != 0)
        return -1;
    if (statvfs(TF_CARD_MOUNT_PATH "/", &fs_info) == -1)
        return -1;

    unsigned long long total = (unsigned long long)fs_info.f_blocks * fs_info.f_frsize;
    unsigned long long free = (unsigned long long)fs_info.f_bfree * fs_info.f_frsize;
    unsigned long long used = total > free ? total - free : 0;

    if (used_bytes)
        *used_bytes = used;
    if (total_bytes)
        *total_bytes = total;
    return 0;
}

int tf_card_get_storage_mb(long long *used_mb, long long *total_mb)
{
    unsigned long long used_bytes, total_bytes;

    if (tf_card_get_storage_bytes(&used_bytes, &total_bytes) != 0)
        return -1;

    if (used_mb)
        *used_mb = (long long)(used_bytes / (1024 * 1024));
    if (total_mb)
        *total_mb = (long long)(total_bytes / (1024 * 1024));
    return 0;
}

/* tf_card_format() 实现在 tf_card_format.c 中（FAT32 格式化） */
