/**
 * @file storageLibrary.c
 * @brief 存储空间模块实现
 * 
 * 实现SD卡容量显示、格式化等功能
 */

#include "storageLibrary.h"
#include <stdbool.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>

/*====================================================================================
 * 全局变量定义
 *====================================================================================*/

/* 存储库全局实例 */
storageLibrary_ctx_t g_storage = {0};

/* 快捷引用宏，简化代码 */
#define sl  (g_storage.sl)
#define sf  (g_storage.sf)

/*====================================================================================
 * 私有函数声明
 *====================================================================================*/

static void create_ui_components(lv_obj_t *parent);
static void sl_init(void);
// 回调接口
static void back_btn_event_handler(lv_event_t * e);
static void format_btn_event_handler(lv_event_t * e);
static void ask_dialog_no_event_cb(lv_event_t *e);
static void ask_dialog_yes_event_cb(lv_event_t *e);
static void close_timer_callback(lv_timer_t *timer);
// 内部函数声明
static void update_storage_display(void);
static int64_t get_tf_card_used_capacity_mb(const char *path);
static int64_t get_tf_card_total_capacity_mb(const char *path);
static void show_format_sd_dialog(lv_obj_t *parent);
static void close_format_dialog(void);
static void create_ask_dialog(lv_obj_t *parent);
static void create_result_dialog(lv_obj_t *parent, const char *text, lv_color_t color);
// 格式化函数
static bool format_sd_card(const char* device_path, const char* volume_label);
static bool format_sd_card_ex(const char* device_path, const char* volume_label, bool quick_format);
// 辅助函数
static int calculate_fat_size(uint32_t total_sectors);
static uint32_t calculate_total_clusters(uint32_t total_sectors, uint32_t fat_sectors);
static uint64_t get_device_size(const char* device);
static void print_boot_sector_info(const Fat32BootSector* boot);
// 进程控制函数
static bool pause_target_process(const char* process_name);
static bool resume_target_process(const char* process_name);
static void send_process_result(bool success, const char* process_name);
// 信号处理
static void setup_signal_handler(void);
static void set_formatting_callback(void (*callback)(void));

// 调试信息标志
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

// TF卡挂载路径
#define TF_CARD_MOUNT_PATH "/mnt/extsd"

// 格式化回调函数指针
static void (*format_callback)(void) = NULL;

// 查找进程PID
static pid_t find_process_by_name(const char* process_name) {
    DIR* dir;
    struct dirent* entry;
    pid_t pid = -1;
    
    dir = opendir("/proc");
    if (!dir) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // 只检查数字目录（进程）
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            char path[256];
            char cmdline[256];
            FILE* fp;
            
            // 读取cmdline文件
            snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
            fp = fopen(path, "r");
            if (fp) {
                if (fgets(cmdline, sizeof(cmdline), fp)) {
                    // 提取程序名（去掉路径）
                    char* base_name = strrchr(cmdline, '/');
                    if (base_name) {
                        base_name++;
                    } else {
                        base_name = cmdline;
                    }
                    
                    // 检查是否匹配目标进程名
                    if (strstr(base_name, process_name) != NULL) {
                        pid = atoi(entry->d_name);
                        fclose(fp);
                        break;
                    }
                }
                fclose(fp);
            }
        }
    }
    
    closedir(dir);
    return pid;
}

// 暂停目标进程
static bool pause_target_process(const char* process_name) {
    printf("Searching for the process: %s\n", process_name);
    
    pid_t pid = find_process_by_name(process_name);
    if (pid == -1) {
        printf("Process not found: %s\n", process_name);
        return false;
    }
    
    printf("Find the process PID: %d\n", pid);
    
    if (kill(pid, SIGSTOP) == 0) {
        printf("The SIGSTOP signal has been sent to the process %d\n", pid);
        return true;
    } else {
        perror("Failed to send SIGSTOP");
        return false;
    }
}

// 恢复目标进程
static bool resume_target_process(const char* process_name) {
    pid_t pid = find_process_by_name(process_name);
    if (pid == -1) {
        printf("Failed to find the process during the restoration process: %s\n", process_name);
        return false;
    }
    
    if (kill(pid, SIGCONT) == 0) {
        printf("The SIGCONT signal has been sent to the process %d\n", pid);
        return true;
    } else {
        perror("发送SIGCONT失败");
        return false;
    }
}

// 发送格式化结果给进程
static void send_process_result(bool success, const char* process_name) {
    pid_t pid = find_process_by_name(process_name);
    if (pid == -1) {
        return;
    }
    
    // 发送自定义信号表示结果
    int signal = success ? SIGUSR1 : SIGUSR2;
    kill(pid, signal);
    printf("The result signal has been sent %s to %d\n", 
           success ? "SIGUSR1(successful)" : "SIGUSR2(failed)", pid);
}

// SIGUSR1信号处理函数
static void sigusr1_handler(int sig) {
    printf("Received the SIGUSR1 signal, initiating the formatting process\n");
    
    if (format_callback != NULL) {
        format_callback();
    }
}

// 设置信号处理器
static void setup_signal_handler(void) {
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags = SA_RESTART;
    
    // 注册SIGUSR1处理
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(1);
    }
    
    // // 忽略其他信号
    // signal(SIGINT, SIG_IGN);
    // signal(SIGTERM, SIG_IGN);
    
    printf("The signal processor has been set up and is now waiting for the SIGUSR1 signal...\n");
}

// 设置格式化回调函数
static void set_formatting_callback(void (*callback)(void)) {
    format_callback = callback;
}

// 获取设备大小
static uint64_t get_device_size(const char* device) {
    if (!device) {
        fprintf(stderr, "Error: Device path is NULL\n");
        return 0;
    }
    
    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device %s: %s\n", device, strerror(errno));
        return 0;
    }
    
    uint64_t size = 0;
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && S_ISBLK(st.st_mode)) {
            uint32_t sectors = 0;
            if (ioctl(fd, BLKGETSIZE, &sectors) == 0) {
                size = (uint64_t)sectors * 512;
            } else {
                size = st.st_size;
            }
        } else {
            fprintf(stderr, "Failed to get device size using ioctl and stat\n");
            close(fd);
            return 0;
        }
    }
    
    close(fd);
    return size;
}

// 计算FAT表大小（扇区）- 迭代计算
static int calculate_fat_size(uint32_t total_sectors) {
    uint32_t fat_sectors = 0;
    uint32_t prev_fat_sectors = 0;
    int iteration = 0;
    
    // 迭代计算直到收敛
    do {
        uint32_t data_sectors = total_sectors - RESERVED_SECTORS - (FAT_COUNT * fat_sectors);
        uint32_t total_clusters = data_sectors / SECTORS_PER_CLUSTER;
        
        // 每个FAT项4字节
        uint32_t new_fat_sectors = (total_clusters * FAT_ENTRY_SIZE + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR;
        
        prev_fat_sectors = fat_sectors;
        fat_sectors = new_fat_sectors;
        
        iteration++;
        DEBUG_PRINT("Iteration %d: fat_sectors=%u, data_sectors=%u, total_clusters=%u\n", 
                   iteration, fat_sectors, data_sectors, total_clusters);
        
        // 防止无限循环
        if (iteration > 100) {
            fprintf(stderr, "Error: FAT size calculation failed to converge after %d iterations\n", iteration);
            return fat_sectors;
        }
        
        if (fat_sectors > total_sectors / 2) {
            fprintf(stderr, "Error: FAT size calculation exceeded reasonable limit\n");
            break;
        }
    } while (fat_sectors != prev_fat_sectors);
    
    return fat_sectors;
}

// 计算总簇数
static uint32_t calculate_total_clusters(uint32_t total_sectors, uint32_t fat_sectors) {
    uint32_t data_sectors = total_sectors - RESERVED_SECTORS - (FAT_COUNT * fat_sectors);
    return data_sectors / SECTORS_PER_CLUSTER;
}

// 卸载设备
static bool unmount_device(const char* device_path) {
    if (!device_path) return false;
    
    char cmd[512];
    int ret;
    
    // 尝试卸载设备的所有分区
    snprintf(cmd, sizeof(cmd), "umount %s 2>/dev/null", device_path);
    ret = system(cmd);
    DEBUG_PRINT("Unmount command '%s' returned %d\n", cmd, ret);
    
    // 等待卸载完成
    usleep(500000);
    
    return true;
}

// 验证设备路径
static bool validate_device_path(const char* device_path) {
    if (!device_path || strlen(device_path) == 0) {
        fprintf(stderr, "Error: Device path is NULL or empty\n");
        return false;
    }
    
    // 防止路径遍历攻击
    if (strstr(device_path, "../") || strstr(device_path, "..")) {
        fprintf(stderr, "Error: Invalid device path (path traversal detected)\n");
        return false;
    }
    
    // 检查是否是块设备路径
    if (strncmp(device_path, "/dev/", 5) != 0) {
        fprintf(stderr, "Warning: Device path doesn't start with /dev/\n");
    }
    
    struct stat st;
    
    if (stat(device_path, &st) < 0) {
        fprintf(stderr, "Device %s does not exist: %s\n", device_path, strerror(errno));
        return false;
    }
    
    if (!S_ISBLK(st.st_mode)) {
        fprintf(stderr, "%s is not a block device\n", device_path);
        return false;
    }
    
    return true;
}

// 验证卷标
static bool validate_volume_label(const char* volume_label) {
    if (!volume_label) return true;
    
    size_t len = strlen(volume_label);
    if (len > 11) {
        fprintf(stderr, "Error: Volume label too long (max 11 characters, got %zu)\n", len);
        return false;
    }
    
    // 检查是否包含非法字符
    for (size_t i = 0; i < len; i++) {
        unsigned char c = volume_label[i];
        if (!isalnum(c) && c != ' ' && c != '_' && c != '-' && c != '.' && c != '!' &&
            c != '#' && c != '$' && c != '%' && c != '&' && c != '\'' && 
            c != '(' && c != ')' && c != '@' && c != '^' && c != '`' && 
            c != '{' && c != '}' && c != '~') {
            fprintf(stderr, "Error: Volume label contains invalid character '%c' (0x%02x)\n", c, c);
            return false;
        }
    }
    
    return true;
}

// 打印引导扇区信息
static void print_boot_sector_info(const Fat32BootSector* boot) {
    if (!boot) {
        printf("Boot sector is NULL\n");
        return;
    }
    
    printf("\n=== Boot Sector Information ===\n");
    printf("OEM Name: %.8s\n", boot->oem_name);
    printf("Bytes per Sector: %u\n", boot->bytes_per_sector);
    printf("Sectors per Cluster: %u\n", boot->sectors_per_cluster);
    printf("Reserved Sectors: %u\n", boot->reserved_sectors);
    printf("FAT Count: %u\n", boot->fat_count);
    printf("Sectors per FAT: %u\n", boot->sectors_per_fat_32);
    printf("Root Cluster: %u\n", boot->root_cluster);
    printf("Volume Label: %.11s\n", boot->volume_label);
    printf("File System Type: %.8s\n", boot->fs_type);
    printf("Total Sectors: %u\n", boot->total_sectors_32);
    printf("Boot Signature: 0x%04X\n", boot->boot_signature_55aa);
    printf("===============================\n\n");
}

// 写入引导扇区
static bool write_boot_sector(int fd, uint32_t total_sectors, const char* volume_label) {
    Fat32BootSector boot = {0};
    
    // 跳转指令
    boot.jump[0] = 0xEB;
    boot.jump[1] = 0x58;
    boot.jump[2] = 0x90;
    
    // OEM名称
    memcpy(boot.oem_name, "MSWIN4.1", 8);
    
    // BPB参数
    boot.bytes_per_sector = BYTES_PER_SECTOR;
    boot.sectors_per_cluster = SECTORS_PER_CLUSTER;
    boot.reserved_sectors = RESERVED_SECTORS;
    boot.fat_count = FAT_COUNT;
    boot.root_entries = 0;
    boot.total_sectors_16 = 0;
    boot.media_type = 0xF8;
    boot.sectors_per_fat_16 = 0;
    boot.sectors_per_track = 63;
    boot.head_count = 255;
    boot.hidden_sectors = 0;
    
    // 总扇区数
    boot.total_sectors_32 = total_sectors;
    
    // 计算FAT大小
    uint32_t fat_sectors = calculate_fat_size(total_sectors);
    
    // FAT32 特定参数
    boot.sectors_per_fat_32 = fat_sectors;
    boot.flags = 0;
    boot.version = 0;
    boot.root_cluster = ROOT_DIR_CLUSTER;
    boot.fsinfo_sector = 1;
    boot.backup_boot_sector = 6;
    boot.drive_number = 0x80;
    boot.boot_signature = 0x29;
    
    // 生成卷ID
    srand((unsigned int)time(NULL));
    boot.volume_id = (uint32_t)rand() << 16 | (uint32_t)rand();
    
    // 卷标
    if (volume_label && strlen(volume_label) > 0) {
        strncpy(boot.volume_label, volume_label, 11);
        for (int i = strlen(volume_label); i < 11; i++) {
            boot.volume_label[i] = ' ';
        }
    } else {
        strcpy(boot.volume_label, "NO NAME    ");
    }
    
    // 文件系统类型
    strcpy(boot.fs_type, "FAT32   ");
    
    // 引导签名（正确）
    boot.boot_signature_55aa = BOOT_SIGNATURE_55AA;
    
    // 写入引导扇区
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Failed to seek to boot sector");
        return false;
    }
    
    ssize_t written = write(fd, &boot, sizeof(boot));
    if (written != sizeof(boot)) {
        fprintf(stderr, "Failed to write boot sector: wrote %zd of %zu bytes: %s\n", 
                written, sizeof(boot), strerror(errno));
        return false;
    }
    
    printf("Boot sector written: %zd bytes\n", written);
    return true;
}

// 写入备份引导扇区
static bool write_backup_boot_sector(int fd) {
    Fat32BootSector boot;
    
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Failed to seek to read boot sector");
        return false;
    }
    
    ssize_t read_bytes = read(fd, &boot, sizeof(boot));
    if (read_bytes != sizeof(boot)) {
        fprintf(stderr, "Failed to read boot sector for backup: %s\n", strerror(errno));
        return false;
    }
    
    if (lseek(fd, 6 * BYTES_PER_SECTOR, SEEK_SET) < 0) {
        perror("Failed to seek to backup boot sector");
        return false;
    }
    
    ssize_t written = write(fd, &boot, sizeof(boot));
    if (written != sizeof(boot)) {
        fprintf(stderr, "Failed to write backup boot sector: %s\n", strerror(errno));
        return false;
    }
    
    DEBUG_PRINT("Backup boot sector written at sector 6\n");
    return true;
}

// 写入FSInfo扇区
static bool write_fsinfo_sector(int fd, uint32_t total_clusters) {
    FSInfoSector fsinfo = {0};
    
    fsinfo.signature1 = 0x41615252;  // "RRaA"
    fsinfo.signature2 = 0x61417272;  // "rrAa"
    fsinfo.free_clusters = total_clusters - 1;
    fsinfo.next_free_cluster = ROOT_DIR_CLUSTER + 1;
    fsinfo.signature3 = 0xAA550000;  // 修正：正确的32位签名
    
    if (lseek(fd, BYTES_PER_SECTOR, SEEK_SET) < 0) {
        perror("Failed to seek to FSInfo sector");
        return false;
    }
    
    ssize_t written = write(fd, &fsinfo, sizeof(fsinfo));
    if (written != sizeof(fsinfo)) {
        fprintf(stderr, "Failed to write FSInfo sector: %s\n", strerror(errno));
        return false;
    }
    
    DEBUG_PRINT("FSInfo sector written: free_clusters=%u, next_free=%u\n", 
               fsinfo.free_clusters, fsinfo.next_free_cluster);
    
    return true;
}

// 初始化FAT表
static bool initialize_fat_table(int fd, uint32_t fat_sectors, uint32_t total_clusters) {
    size_t fat_size = fat_sectors * BYTES_PER_SECTOR;
    uint8_t* fat_buffer = (uint8_t*)calloc(1, fat_size);
    if (!fat_buffer) {
        fprintf(stderr, "Failed to allocate FAT buffer (%zu bytes)\n", fat_size);
        return false;
    }
    
    uint32_t* fat = (uint32_t*)fat_buffer;
    uint32_t fat_entries = fat_size / FAT_ENTRY_SIZE;
    
    // 设置FAT表前三个条目
    fat[0] = 0xFFFFFFF8;
    fat[1] = 0xFFFFFFFF;
    fat[2] = FAT_END_OF_CHAIN;
    
    // 标记剩余的簇为空闲
    uint32_t limit = (total_clusters < fat_entries - 1) ? total_clusters : fat_entries - 1;
    for (uint32_t i = 3; i < limit; i++) {
        fat[i] = FAT_FREE_CLUSTER;
    }
    
    DEBUG_PRINT("FAT initialized: %u entries, %u clusters\n", fat_entries, total_clusters);
    
    // 写入第一个FAT表
    off_t fat1_offset = RESERVED_SECTORS * BYTES_PER_SECTOR;
    if (lseek(fd, fat1_offset, SEEK_SET) < 0) {
        perror("Failed to seek to FAT1");
        free(fat_buffer);
        return false;
    }
    
    ssize_t written1 = write(fd, fat_buffer, fat_size);
    if (written1 != (ssize_t)fat_size) {
        fprintf(stderr, "Failed to write FAT1: %s\n", strerror(errno));
        free(fat_buffer);
        return false;
    }
    
    // 写入第二个FAT表
    off_t fat2_offset = fat1_offset + fat_size;
    if (lseek(fd, fat2_offset, SEEK_SET) < 0) {
        perror("Failed to seek to FAT2");
        free(fat_buffer);
        return false;
    }
    
    ssize_t written2 = write(fd, fat_buffer, fat_size);
    if (written2 != (ssize_t)fat_size) {
        fprintf(stderr, "Failed to write FAT2: %s\n", strerror(errno));
        free(fat_buffer);
        return false;
    }
    
    free(fat_buffer);
    return true;
}

// 初始化根目录
static bool initialize_root_directory(int fd, uint32_t fat_sectors) {
    uint8_t zero_sector[BYTES_PER_SECTOR] = {0};
    
    off_t root_dir_offset = (RESERVED_SECTORS + (FAT_COUNT * fat_sectors)) * BYTES_PER_SECTOR;
    
    if (lseek(fd, root_dir_offset, SEEK_SET) < 0) {
        perror("Failed to seek to root directory");
        return false;
    }
    
    DEBUG_PRINT("Initializing root directory at offset %lld\n", (long long)root_dir_offset);
    
    for (int i = 0; i < SECTORS_PER_CLUSTER; i++) {
        ssize_t written = write(fd, zero_sector, sizeof(zero_sector));
        if (written != sizeof(zero_sector)) {
            fprintf(stderr, "Failed to write root directory sector %d: %s\n", i, strerror(errno));
            return false;
        }
    }
    
    return true;
}

// 快速格式化
static bool quick_format(int fd, uint32_t total_sectors, uint32_t fat_sectors, 
                         uint32_t total_clusters, const char* volume_label) {
    printf("Performing quick format...\n");
    
    if (!write_boot_sector(fd, total_sectors, volume_label)) return false;
    if (!write_backup_boot_sector(fd)) return false;
    if (!write_fsinfo_sector(fd, total_clusters)) return false;
    if (!initialize_fat_table(fd, fat_sectors, total_clusters)) return false;
    if (!initialize_root_directory(fd, fat_sectors)) return false;
    
    return true;
}

// 完整格式化
static bool full_format(int fd, uint32_t total_sectors, uint32_t fat_sectors,
                        uint32_t total_clusters, const char* volume_label) {
    printf("Performing full format...\n");
    
    const size_t buffer_size = 64 * 1024;
    uint8_t* zero_buffer = (uint8_t*)calloc(1, buffer_size);
    if (!zero_buffer) {
        perror("Failed to allocate zero buffer");
        return false;
    }
    
    uint64_t bytes_written = 0;
    uint64_t total_bytes = total_sectors * BYTES_PER_SECTOR;
    
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Failed to seek to start of device");
        free(zero_buffer);
        return false;
    }
    
    printf("Clearing device...\n");
    while (bytes_written < total_bytes) {
        size_t to_write = (total_bytes - bytes_written) > buffer_size ? 
                          buffer_size : (size_t)(total_bytes - bytes_written);
        
        ssize_t written = write(fd, zero_buffer, to_write);
        if (written != (ssize_t)to_write) {
            fprintf(stderr, "Failed to clear device: %s\n", strerror(errno));
            free(zero_buffer);
            return false;
        }
        
        bytes_written += written;
        
        if (bytes_written % (10 * 1024 * 1024) == 0 || bytes_written == total_bytes) {
            int percent = (int)((bytes_written * 100) / total_bytes);
            printf("Progress: %llu/%llu MB (%d%%)\r", 
                   bytes_written / (1024 * 1024), 
                   total_bytes / (1024 * 1024), 
                   percent);
            fflush(stdout);
        }
    }
    
    free(zero_buffer);
    printf("\nDevice cleared.\n");
    
    // 重新写入元数据
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Failed to seek after clearing");
        return false;
    }
    
    bool success = quick_format(fd, total_sectors, fat_sectors, total_clusters, volume_label);
    
    return success;
}

// 扩展格式化函数
static bool format_sd_card_ex(const char* device_path, const char* volume_label, bool quick_format_mode) {
    printf("\n========== FAT32 Formatter ==========\n");
    printf("Device: %s\n", device_path ? device_path : "(null)");
    printf("Volume Label: %s\n", volume_label ? volume_label : "(default)");
    printf("Mode: %s\n", quick_format_mode ? "Quick Format" : "Full Format");
    printf("=====================================\n\n");

    // 验证设备路径
    if (!validate_device_path(device_path)) {
        return false;
    }
    
    // 验证卷标
    if (!validate_volume_label(volume_label)) {
        return false;
    }

    // 卸载设备
    printf("Step 1/7: Unmounting device...\n");
    // if (!unmount_device(device_path)) {
    //     fprintf(stderr, "Warning: Failed to unmount device\n");
    // }
    if (system("umount /mnt/extsd") == 0) {
        printf("Device unmounted from /mnt/extsd\n");
    }

    // 打开设备
    printf("Step 2/7: Opening device...\n");
    int fd = open(device_path, O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device %s: %s\n", device_path, strerror(errno));
        return false;
    }
    
    // 获取设备大小
    printf("Step 3/7: Getting device size...\n");
    uint64_t device_size = get_device_size(device_path);
    if (device_size == 0) {
        fprintf(stderr, "Failed to get device size\n");
        close(fd);
        return false;
    }
    
    // 计算参数
    printf("Step 4/7: Calculating parameters...\n");
    uint32_t total_sectors = (uint32_t)(device_size / BYTES_PER_SECTOR);
    
    if (total_sectors < 65536) {
        fprintf(stderr, "Error: Device is too small for FAT32 (minimum 32MB)\n");
        close(fd);
        return false;
    }
    
    uint32_t fat_sectors = calculate_fat_size(total_sectors);
    uint32_t total_clusters = calculate_total_clusters(total_sectors, fat_sectors);
    
    if (fat_sectors == 0) {
        fprintf(stderr, "Error: Failed to calculate FAT size\n");
        close(fd);
        return false;
    }
    
    if (total_clusters < 65525) {
        fprintf(stderr, "Error: Not enough clusters for FAT32 (minimum 65525, got %u)\n", total_clusters);
        close(fd);
        return false;
    }
    
    // 显示信息
    printf("\n--- Device Information ---\n");
    printf("Total Size: %.2f GB\n", (double)device_size / (1024 * 1024 * 1024));
    printf("Total Sectors: %u\n", total_sectors);
    printf("Total Clusters: %u\n", total_clusters);
    printf("FAT Size: %u sectors\n", fat_sectors);
    printf("Cluster Size: %u bytes\n", SECTORS_PER_CLUSTER * BYTES_PER_SECTOR);
    printf("--------------------------\n\n");
    
    // // 确认格式化
    // printf("WARNING: This will erase ALL data on %s\n", device_path);
    // printf("Type 'YES' to continue: ");
    
    // char response[16];
    // if (fgets(response, sizeof(response), stdin) == NULL) {
    //     printf("Cancelled by user\n");
    //     close(fd);
    //     return false;
    // }
    
    // // 移除换行符
    // response[strcspn(response, "\n")] = 0;
    
    // if (strcmp(response, "YES") != 0) {
    //     printf("Cancelled\n");
    //     close(fd);
    //     return false;
    // }
    
    // 执行格式化
    printf("Step 5/7: Formatting...\n");
    bool success;

    if (quick_format_mode) {
        success = quick_format(fd, total_sectors, fat_sectors, total_clusters, volume_label);
    } else {
        success = full_format(fd, total_sectors, fat_sectors, total_clusters, volume_label);
    }

    if (!success) {
        fprintf(stderr, "Formatting failed!\n");
        close(fd);
        return false;
    }

    // 同步到磁盘
    printf("Step 6/7: Syncing to disk...\n");
    if (fsync(fd) < 0) {
        fprintf(stderr, "Warning: Failed to sync to disk: %s\n", strerror(errno));
    }

    // 验证格式化
    printf("Step 7/7: Verifying format...\n");
    Fat32BootSector boot;
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Failed to seek for verification");
        close(fd);
        return false;
    }

    if (read(fd, &boot, sizeof(boot)) != sizeof(boot)) {
        perror("Failed to read boot sector for verification");
        close(fd);
        return false;
    }

    if (boot.boot_signature_55aa != BOOT_SIGNATURE_55AA) {
        fprintf(stderr, "Error: Boot signature verification failed\n");
        close(fd);
        return false;
    }

    close(fd);

    // mount
    system("mount /dev/mmcblk0 /mnt/extsd");

    printf("\n========== Formatting Complete ==========\n");
    printf("Device successfully formatted as FAT32\n");
    printf("Volume Label: %.11s\n", boot.volume_label);
    printf("=========================================\n");

    return true;
}

// 主格式化函数
static bool format_sd_card(const char* device_path, const char* volume_label) {
    return format_sd_card_ex(device_path, volume_label, true);
}

static void show_format_sd_dialog(lv_obj_t *parent) {
    // 如果已经存在对话框，先关闭
    close_format_dialog();
    
    // 创建遮罩层
    sf.mask_layer = lv_obj_create(parent);
    lv_obj_set_size(sf.mask_layer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(sf.mask_layer, TM_BG_PRIMARY, 0);
    lv_obj_set_style_bg_opa(sf.mask_layer, LV_OPA_50, 0);
    lv_obj_set_style_radius(sf.mask_layer, 0, 0);
    lv_obj_set_style_border_width(sf.mask_layer, 0, 0);
    lv_obj_clear_flag(sf.mask_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(sf.mask_layer);
    
    // 创建询问对话框
    create_ask_dialog(sf.mask_layer);
}

static void close_format_dialog(void) {
    // 关闭定时器
    if (sf.close_timer) {
        lv_timer_del(sf.close_timer);
        sf.close_timer = NULL;
    }
    // 销毁结果对话框
    if (sf.result_dialog) {
        lv_obj_del(sf.result_dialog);
        sf.result_dialog = NULL;
    }
    // 销毁询问对话框和遮罩层
    if (sf.mask_layer) {
        lv_obj_del(sf.mask_layer);
        sf.mask_layer = NULL;
        sf.dialog_bg = NULL;
        sf.msg_label = NULL;
        sf.yes_btn = NULL;
        sf.no_btn = NULL;
    }
}

static void create_ask_dialog(lv_obj_t *parent) {
    /* 存储空间 - 格式化确认对话框背景 */
    sf.dialog_bg = lv_obj_create(parent);
    lv_obj_set_size(sf.dialog_bg, LV_PCT(40), LV_PCT(40));
    lv_obj_set_pos(sf.dialog_bg, 382, 216);
    lv_obj_set_style_bg_color(sf.dialog_bg, TM_BG_SECONDARY, 0);
    lv_obj_set_style_radius(sf.dialog_bg, 10, 0);
    lv_obj_set_style_pad_all(sf.dialog_bg, 20, 0);
    lv_obj_set_style_border_width(sf.dialog_bg, 2, 0);
    lv_obj_set_style_border_color(sf.dialog_bg, TM_BORDER_SECONDARY, 0);
    lv_obj_clear_flag(sf.dialog_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* 存储空间 - 格式化确认消息 */
    sf.msg_label = lv_label_create(sf.dialog_bg);
    lv_label_set_text(sf.msg_label, get_string_for_language(languageSetting, "Format_confirm"));
    lv_obj_set_style_text_font(sf.msg_label, &v853Font_hyby_30, 0);
    lv_obj_set_width(sf.msg_label, LV_PCT(100));
    lv_obj_set_style_text_align(sf.msg_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(sf.msg_label, TM_TEXT_PRIMARY, 0);
    lv_obj_align(sf.msg_label, LV_ALIGN_TOP_MID, 0, LV_PCT(20));
    
    /* 存储空间 - 确认按钮 */
    sf.yes_btn = lv_btn_create(sf.dialog_bg);
    lv_obj_set_size(sf.yes_btn, 100, 40);
    lv_obj_align(sf.yes_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(sf.yes_btn, TM_SUCCESS, 0);
    lv_obj_set_style_radius(sf.yes_btn, 8, 0);
    lv_obj_add_event_cb(sf.yes_btn, ask_dialog_yes_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *yes_label = lv_label_create(sf.yes_btn);
    lv_label_set_text(yes_label, get_string_for_language(languageSetting, "Yes"));
    lv_obj_set_style_text_font(yes_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(yes_label, lv_color_hex(0xFFFFFF), 0);  /* 按钮文字始终白色 */
    lv_obj_center(yes_label);
    
    /* 存储空间 - 取消按钮 */
    sf.no_btn = lv_btn_create(sf.dialog_bg);
    lv_obj_set_size(sf.no_btn, 100, 40);
    lv_obj_align(sf.no_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(sf.no_btn, TM_ERROR, 0);
    lv_obj_set_style_radius(sf.no_btn, 8, 0);
    lv_obj_add_event_cb(sf.no_btn, ask_dialog_no_event_cb, LV_EVENT_CLICKED, NULL);    
    lv_obj_t *no_label = lv_label_create(sf.no_btn);
    lv_label_set_text(no_label, get_string_for_language(languageSetting, "No"));
    lv_obj_set_style_text_font(no_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(no_label, lv_color_hex(0xFFFFFF), 0);   /* 按钮文字始终白色 */
    lv_obj_center(no_label);
}

static void create_result_dialog(lv_obj_t *parent, const char *text, lv_color_t color) {
    sf.result_dialog = lv_obj_create(parent);
    lv_obj_set_size(sf.result_dialog, LV_PCT(40), LV_PCT(40));
    lv_obj_set_pos(sf.result_dialog, 382, 216);
    // lv_obj_center(sf.result_dialog);
    lv_obj_set_style_bg_color(sf.result_dialog, TM_BG_SECONDARY, 0);
    lv_obj_set_style_radius(sf.result_dialog, 10, 0);
    lv_obj_set_style_pad_all(sf.result_dialog, 20, 0);
    lv_obj_set_style_border_width(sf.result_dialog, 2, 0);
    lv_obj_set_style_border_color(sf.result_dialog, color, 0);
    lv_obj_clear_flag(sf.result_dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *result_label = lv_label_create(sf.result_dialog);
    lv_label_set_text(result_label, text);
    lv_obj_set_style_text_color(result_label, color, 0);
    lv_obj_set_style_text_font(result_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_align(result_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(result_label, 210);
    lv_obj_center(result_label);
}

static void close_timer_callback(lv_timer_t *timer) {
    (void)timer; // 未使用参数
    close_format_dialog();
    // 更新页面显示
    update_storage_display();
}

static void ask_dialog_yes_event_cb(lv_event_t *e) {
    (void)e; // 未使用参数

#if 0
    // 模拟调用格式化接口（这里替换为你的实际接口）
    static int counter = 0;
    counter++;
    bool format_success = counter % 2; // 这是你的实际格式化函数
#else
    printf("Call the SD card formatting interface...\n");

    bool process_paused = false;
	bool format_success = false;
	
	printf("\n=== Start the SD card formatting process ===\n");
	
	// // 1. 暂停目标进程
	// printf("Step 1: Suspend the target process...\n");
	// if (pause_target_process(TARGET_PROCESS)) {
	// 	process_paused = true;
	// 	printf("The target process has been paused\n");
	// } else {
	// 	printf("Warning: Unable to pause the target process. Proceed with formatting...\n");
	// }
    // if (system("umount /mnt/extsd") == 0) {     //  卸载SD卡
    //     printf("SD card unmounted successfully.\n");
    // } else {
    //     printf("Warning: Failed to unmount SD card. It may not be mounted.\n");
    // }

	// 2. 格式化SD卡
	printf("\nStep 2: Format the SD card...\n");
	format_success = format_sd_card(SD_CARD_DEVICE, "SD_CARD");
	
	// if (format_success) {
	// 	printf("SD card formatting completed successfully!\n");
    //     if (system("mount /dev/mmcblk0 /mnt/extsd") == 0) {     //  重新挂载SD卡
    //         printf("SD card mounted successfully.\n");
    //     } else {
    //         printf("Warning: Failed to mount SD card after formatting.\n");
    //     }
	// } else {
	// 	printf("SD card formatting failed!\n");
	// }
	
	// // 3. 恢复目标进程（如果之前暂停了）
	// printf("\nStep 3: Restore the state of the target process...\n");
	// if (process_paused) {
	// 	if (resume_target_process(TARGET_PROCESS)) {
	// 		printf("The target process has been restored.\n");
	// 	} else {
	// 		printf("Warning: The target process cannot be restored.\n");
	// 	}
	// }
#endif

    // 销毁询问对话框
    if (sf.dialog_bg) {
        lv_obj_del(sf.dialog_bg);
        sf.dialog_bg = NULL;
        sf.msg_label = NULL;
        sf.yes_btn = NULL;
        sf.no_btn = NULL;
    }
    
    // 根据格式化结果显示不同的消息
    if (format_success) {
        // // 调用成功回调
        // if (format_success_callback) {
        //     format_success_callback();
        // }
        
        /* 显示成功对话框 */
        create_result_dialog(sf.mask_layer, get_string_for_language(languageSetting, "Format_success"), TM_SUCCESS);
    } else {
        /* 显示失败对话框 */
        create_result_dialog(sf.mask_layer, get_string_for_language(languageSetting, "Format_failed"), TM_ERROR);
    }

    // 创建1秒后关闭的定时器
    sf.close_timer = lv_timer_create(close_timer_callback, 1000, NULL);
}

static void ask_dialog_no_event_cb(lv_event_t *e) {
    (void)e; // 未使用参数
    close_format_dialog();
}

/**
 * @brief 检查TF卡是否已挂载
 * 
 * 通过读取/proc/mounts检查指定路径是否已挂载
 * 
 * @param mount_point 挂载点路径，如 "/mnt/extsd"
 * @return true TF卡已挂载
 * @return false TF卡未挂载
 */
static bool is_tf_card_mounted(const char *mount_point) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        printf("[Storage] Failed to open /proc/mounts: %s\n", strerror(errno));
        return false;
    }
    
    char line[512];
    bool mounted = false;
    
    while (fgets(line, sizeof(line), fp)) {
        /* /proc/mounts 格式: device mount_point fs_type options ... */
        /* 检查该行是否包含挂载点 */
        if (strstr(line, mount_point) != NULL) {
            /* 进一步验证是否真正是挂载点（而不是设备名的一部分） */
            char device[128], mnt[128];
            if (sscanf(line, "%127s %127s", device, mnt) == 2) {
                if (strcmp(mnt, mount_point) == 0) {
                    mounted = true;
                    printf("[Storage] TF card mounted: %s -> %s\n", device, mnt);
                    break;
                }
            }
        }
    }
    
    fclose(fp);
    
    if (!mounted) {
        printf("[Storage] TF card not mounted at %s\n", mount_point);
    }
    
    return mounted;
}

static int64_t get_tf_card_total_capacity_mb(const char *path)
{
    /* 先检查TF卡是否挂载 */
    if (!is_tf_card_mounted(TF_CARD_MOUNT_PATH)) {
        printf("[Storage] TF card not mounted, cannot get total capacity\n");
        return -1;
    }
    
    struct statvfs fs_info;
    
    if (statvfs(path, &fs_info) == -1) {
        printf("Failed to get filesystem info: %s\n", strerror(errno));
        return -1;
    }
    
    // 总容量 = 块数 * 块大小
    uint64_t total_bytes = (uint64_t)fs_info.f_blocks * fs_info.f_frsize;
    int64_t total_mb = (int64_t)(total_bytes / (1024 * 1024));
    
    return total_mb;
}

static int64_t get_tf_card_used_capacity_mb(const char *path)
{
    /* 先检查TF卡是否挂载 */
    if (!is_tf_card_mounted(TF_CARD_MOUNT_PATH)) {
        printf("[Storage] TF card not mounted, cannot get used capacity\n");
        return -1;
    }
    
    struct statvfs fs_info;
    
    if (statvfs(path, &fs_info) == -1) {
        printf("Failed to get filesystem info: %s\n", strerror(errno));
        return -1;
    }
    
    // 总容量
    uint64_t total_bytes = (uint64_t)fs_info.f_blocks * fs_info.f_frsize;
    
    // 可用容量（给普通用户的）
    uint64_t avail_bytes = (uint64_t)fs_info.f_bavail * fs_info.f_frsize;
    
    // 已使用容量
    uint64_t used_bytes = total_bytes - avail_bytes;
    int64_t used_mb = (int64_t)(used_bytes / (1024 * 1024));
    
    return used_mb;
}

static void update_storage_display(void) {
    int64_t total_mem = get_tf_card_total_capacity_mb(TF_CARD_MOUNT_PATH "/");
    int64_t used_mem = get_tf_card_used_capacity_mb(TF_CARD_MOUNT_PATH "/");
    
    /* 检查TF卡是否可用 */
    if (total_mem <= 0 || used_mem < 0) {
        printf("[Storage] TF card unavailable, showing error state\n");
        
        /* TF卡未挂载或读取失败，显示错误状态 */
        lv_bar_set_value(sl.progress_bar, 0, LV_ANIM_OFF);
        lv_label_set_text(sl.percent_label, "--");
        lv_label_set_text_fmt(sl.details_label, "%s\n%s",
                            get_string_for_language(languageSetting, "SDCardNotFound"),
                            get_string_for_language(languageSetting, "SDCardInsertTip"));
        return;
    }
    
    int percentage = (int)((used_mem * 100) / total_mem);

    // 更新进度条
    lv_bar_set_value(sl.progress_bar, percentage, LV_ANIM_ON);

    // 更新百分比标签
    lv_label_set_text_fmt(sl.percent_label, "%d%%", percentage);

    // 更新详细使用情况标签
    lv_label_set_text_fmt(sl.details_label, 
                         "%s: %lld MB / %lld MB\n"
                         "%s: %lld MB",
                         get_string_for_language(languageSetting, "SDCardUsed"), (long long)used_mem, (long long)total_mem,
                         get_string_for_language(languageSetting, "SDCardFree"), (long long)(total_mem - used_mem));
}

static void format_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if (LV_EVENT_CLICKED == code) {
        show_format_sd_dialog(lv_scr_act());
    }
}

static void back_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if (LV_EVENT_CLICKED == code) {
        lv_obj_del(sl.container);
        // lv_obj_clear_flag(sl.pb, LV_OBJ_FLAG_HIDDEN);
        lv_sdcard_widgets();
    }
}

static void create_ui_components(lv_obj_t *parent) {
    /*==================== 存储空间界面 - 主容器 ====================*/
    sl.container = lv_obj_create(parent);
    lv_obj_set_size(sl.container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(sl.container, TM_BG_PRIMARY, 0);            /* 页面背景 */
    lv_obj_set_style_bg_opa(sl.container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_opa(sl.container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(sl.container, 0, 0);
    lv_obj_center(sl.container);
    lv_obj_clear_flag(sl.container, LV_OBJ_FLAG_SCROLLABLE);

    /*==================== 存储空间界面 - 标题 ====================*/
    sl.title_label = lv_label_create(sl.container);
    lv_obj_set_style_text_color(sl.title_label, TM_TEXT_PRIMARY, 0);      /* 标题文字颜色 */
    lv_obj_set_style_text_font(sl.title_label, &v853Font_hyby_40, 0);
    lv_label_set_text(sl.title_label, get_string_for_language(languageSetting, "SDCardTitle"));
    lv_obj_align(sl.title_label, LV_ALIGN_TOP_MID, 0, 25);

    /*==================== 存储空间界面 - 进度条 ====================*/
    sl.progress_bar = lv_bar_create(sl.container);
    lv_obj_set_size(sl.progress_bar, 1000, 30);
    lv_obj_align(sl.progress_bar, LV_ALIGN_TOP_MID, 0, 275);
    lv_obj_set_style_bg_color(sl.progress_bar, TM_PROGRESS_BG, LV_PART_MAIN);  /* 进度条背景 */
    lv_obj_set_style_bg_opa(sl.progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(sl.progress_bar, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl.progress_bar, TM_SUCCESS, LV_PART_INDICATOR); /* 进度条前景(绿色) */
    lv_obj_set_style_bg_opa(sl.progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(sl.progress_bar, 10, LV_PART_INDICATOR);
    lv_bar_set_range(sl.progress_bar, 0, 100);

    /*==================== 存储空间界面 - SD卡图标 ====================*/
    lv_obj_t *label_left = lv_label_create(sl.container);
    lv_obj_set_style_text_font(label_left, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(label_left, TM_ICON_PRIMARY, 0);          /* SD卡图标颜色 */
    lv_obj_set_style_bg_opa(label_left, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_label_set_text(label_left, LV_SYMBOL_SD_CARD);
    lv_obj_align_to(label_left, sl.progress_bar, LV_ALIGN_OUT_TOP_LEFT, 10, -15);

    /*==================== 存储空间界面 - 百分比标签 ====================*/
    sl.percent_label = lv_label_create(sl.container);
    lv_obj_set_style_text_font(sl.percent_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(sl.percent_label, TM_ACCENT, 0);          /* 百分比文字(强调色) */
    lv_obj_align_to(sl.percent_label, sl.progress_bar, LV_ALIGN_OUT_TOP_RIGHT, 0, -15);

    /*==================== 存储空间界面 - 使用情况详情 ====================*/
    sl.details_label = lv_label_create(sl.container);
    lv_obj_set_style_text_font(sl.details_label, &v853Font_hyby_30, 0);
    lv_obj_set_style_text_color(sl.details_label, TM_TEXT_SECONDARY, 0);  /* 详情文字颜色 */
    lv_obj_set_style_text_align(sl.details_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_line_space(sl.details_label, 8, 0);
    lv_obj_align_to(sl.details_label, sl.progress_bar, LV_ALIGN_OUT_RIGHT_MID, -350, 100);

    /*==================== 存储空间界面 - 格式化按钮 ====================*/
    sl.format_btn = lv_btn_create(sl.container);
    lv_obj_set_size(sl.format_btn, 400, 50);
    lv_obj_set_style_bg_color(sl.format_btn, TM_BG_TERTIARY, 0);          /* 按钮背景 */
    lv_obj_set_style_bg_opa(sl.format_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sl.format_btn, TM_BORDER_PRIMARY, 0);   /* 按钮边框 */
    lv_obj_set_style_border_width(sl.format_btn, 1, 0);
    lv_obj_set_style_border_opa(sl.format_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(sl.format_btn, TM_ACCENT, LV_STATE_PRESSED); /* 按下状态 */
    lv_obj_set_style_bg_opa(sl.format_btn, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_radius(sl.format_btn, 8, 0);
    lv_obj_align(sl.format_btn, LV_ALIGN_TOP_MID, 0, 550);
    lv_obj_add_event_cb(sl.format_btn, format_btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    /*==================== 存储空间界面 - 格式化按钮文字 ====================*/
    sl.format_btn_icon = lv_label_create(sl.format_btn);
    lv_obj_set_style_text_font(sl.format_btn_icon, &v853Font_hyby_30, 0);
    lv_label_set_text(sl.format_btn_icon, get_string_for_language(languageSetting, "SDCardFormat"));
    lv_obj_set_style_text_color(sl.format_btn_icon, TM_ACCENT, 0);        /* 按钮文字(强调色) */
    lv_obj_center(sl.format_btn_icon);

    /* 初始更新显示 */
    update_storage_display();
}
#if 1
static void sl_init() {
    sl.pb = NULL;
    sl.container = NULL;
    sl.back_btn = NULL;
    sl.back_icon = NULL;
    sl.title_label = NULL;
    sl.progress_bar = NULL;
    sl.percent_label = NULL;
    sl.details_label = NULL;
    sl.format_btn = NULL;
    sl.format_btn_icon = NULL;
    sf.mask_layer = NULL;
    sf.dialog_bg = NULL;
    sf.msg_label = NULL;
    sf.yes_btn = NULL;
    sf.no_btn = NULL;
    sf.result_dialog = NULL;
    sf.close_timer = NULL;

    format_callback = NULL;
}
#else

#endif
void create_storage_select_ui(lv_obj_t *parent) {
    if (!parent) return;

    sl.pb = parent;

    // 初始化数据（不创建UI）
    sl_init();

    // 创建UI组件
    create_ui_components(parent);
}

// 完整的 cleanup_storage_library 函数
void cleanup_storage_library(void) {
    printf("Cleaning up storage library resources\n");
    
    // 1. 先关闭格式化对话框和相关资源
    // 关闭定时器
    if (sf.close_timer) {
        lv_timer_del(sf.close_timer);
        sf.close_timer = NULL;
    }

    // 销毁结果对话框
    if (sf.result_dialog) {
        lv_obj_del(sf.result_dialog);
        sf.result_dialog = NULL;
    }
    
    // 销毁询问对话框和遮罩层
    if (sf.mask_layer) {
        lv_obj_del(sf.mask_layer);
        sf.mask_layer = NULL;
        sf.dialog_bg = NULL;    // 这些是子对象，会被自动删除，但指针需要置NULL
        sf.msg_label = NULL;
        sf.yes_btn = NULL;
        sf.no_btn = NULL;
    }
    
    // 2. 销毁主界面组件
    // 注意：销毁容器会自动销毁其所有子对象
    if (sl.container) {
        lv_obj_del(sl.container);
        sl.container = NULL;
        
        // 子对象会被自动删除，只需将指针置NULL
        sl.back_btn = NULL;
        sl.back_icon = NULL;
        sl.title_label = NULL;
        sl.progress_bar = NULL;
        sl.percent_label = NULL;
        sl.details_label = NULL;
        sl.format_btn = NULL;
        sl.format_btn_icon = NULL;
    }
    
    // 3. 最后重置父对象指针
    sl.pb = NULL;

    format_callback = NULL;
    
    printf("Storage library cleanup completed\n");
}

void reflush_storage_display(void) {
    update_storage_display();
}