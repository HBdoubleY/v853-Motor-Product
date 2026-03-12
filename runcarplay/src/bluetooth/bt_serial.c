#include "bt_serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <errno.h>

/* 内部状态 */
static int g_fd = -1;                     // 串口文件描述符
static pthread_t g_thread;                 // 读取线程ID
static volatile int g_running = 0;          // 运行标志
static bt_data_callback g_callback = NULL;  // 用户回调
static void *g_user_arg = NULL;             // 用户参数
static pthread_mutex_t g_write_mutex = PTHREAD_MUTEX_INITIALIZER; // 写锁

/* 波特率转换表 */
typedef struct {
    int speed;
    speed_t code;
} baudrate_map_t;

static const baudrate_map_t baudrate_table[] = {
    {9600,   B9600},
    {19200,  B19200},
    {38400,  B38400},
    {57600,  B57600},
    {115200, B115200},
    {230400, B230400},
    {460800, B460800},
    {500000, B500000},
    {576000, B576000},
    {921600, B921600},
    {1000000,B1000000},
    {0,      0} // 结束标记
};

/* 根据整数波特率获取termios宏 */
static speed_t int_to_baudrate(int baudrate) {
    for (const baudrate_map_t *p = baudrate_table; p->speed != 0; p++) {
        if (p->speed == baudrate)
            return p->code;
    }
    return B0; // 无效
}

/* 配置串口 */
static int configure_serial(int fd, int baudrate) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }

    speed_t speed = int_to_baudrate(baudrate);
    if (speed == B0) {
        fprintf(stderr, "Unsupported baudrate: %d\n", baudrate);
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    // 8N1, 无流控, 原始模式
    tty.c_cflag &= ~PARENB;         // 无校验
    tty.c_cflag &= ~CSTOPB;         // 1位停止位
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;              // 8位数据
    tty.c_cflag &= ~CRTSCTS;         // 禁用硬件流控
    tty.c_cflag |= CREAD | CLOCAL;   // 启用接收，忽略调制解调器控制

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 原始输入
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // 禁用软件流控
    tty.c_iflag &= ~(INLCR | ICRNL | IGNCR);        // 不转换换行符
    tty.c_oflag &= ~OPOST;                           // 原始输出

    // 设置读取超时：VTIME 为 0 表示无超时，VMIN 为 1 表示至少读 1 个字符才返回
    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN] = 1;

    // 清空缓冲区
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

/* 读取线程函数 */
static void *reader_thread(void *arg) {
    char buf[256];
    char line[512];       // 行缓冲区
    int pos = 0;

    while (g_running) {
        ssize_t n = read(g_fd, buf, sizeof(buf));
        if (n <= 0) {
            if (n < 0 && errno != EINTR) {
                perror("read");
            }
            continue;
        }

        for (ssize_t i = 0; i < n; i++) {
            char ch = buf[i];
            if (ch == '\n') {
                // 行结束，忽略空行
                if (pos > 0) {
                    line[pos] = '\0';
                    // 去除末尾可能存在的 '\r'
                    if (pos > 0 && line[pos-1] == '\r')
                        line[pos-1] = '\0';
                    if (g_callback) {
                        g_callback(line, g_user_arg);
                    }
                }
                pos = 0;
            } else {
                if (pos < (int)sizeof(line) - 1) {
                    line[pos++] = ch;
                } else {
                    // 行过长，丢弃当前行并重置
                    pos = 0;
                }
            }
        }
    }
    return NULL;
}

/* 初始化 */
int bt_serial_init(const char *dev, int baudrate, bt_data_callback cb, void *user_arg) {
    if (!dev || !cb) return -1;

    g_fd = open(dev, O_RDWR | O_NOCTTY);
    if (g_fd < 0) {
        perror("open");
        return -1;
    }

    if (configure_serial(g_fd, baudrate) < 0) {
        close(g_fd);
        g_fd = -1;
        return -1;
    }

    g_callback = cb;
    g_user_arg = user_arg;
    g_running = 1;

    if (pthread_create(&g_thread, NULL, reader_thread, NULL) != 0) {
        perror("pthread_create");
        close(g_fd);
        g_fd = -1;
        g_running = 0;
        return -1;
    }

    return 0;
}

/* 发送AT指令（自动加\r\n） */
int bt_serial_send(const char *cmd) {
    if (g_fd < 0 || !cmd) return -1;

    // 构造完整指令
    char buf[512];
    int len = snprintf(buf, sizeof(buf), "%s\r\n", cmd);
    if (len >= (int)sizeof(buf)) {
        fprintf(stderr, "Command too long\n");
        return -1;
    }

    pthread_mutex_lock(&g_write_mutex);
    int ret = write(g_fd, buf, len);
    pthread_mutex_unlock(&g_write_mutex);
    return ret;
}

/* 发送原始数据 */
int bt_serial_send_raw(const char *data, int len) {
    if (g_fd < 0 || !data || len <= 0) return -1;
    pthread_mutex_lock(&g_write_mutex);
    int ret = write(g_fd, data, len);
    pthread_mutex_unlock(&g_write_mutex);
    return ret;
}

/* 清理 */
void bt_serial_cleanup(void) {
    if (g_running) {
        g_running = 0;
        // 取消线程（read是取消点）
        pthread_cancel(g_thread);
        pthread_join(g_thread, NULL);
    }
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
    }
    g_callback = NULL;
    g_user_arg = NULL;
}

// 自定义回调处理收到的数据行
static int hfp_connected = 0;
static char connected_addr[13] = {0};

void on_bt_data(const char *line, void *arg) {
    if (strncmp(line, "IB", 2) == 0) {
        // HFP连接成功，line+2为12位蓝牙地址
        printf("HFP connected = 1\n\n\n\n\n\n\n");
        strncpy(connected_addr, line+2, 12);
        connected_addr[12] = '\0';
        hfp_connected = 1;
        
        printf("HFP connected to %s\n", connected_addr);
    }
    else if (strncmp(line, "IA", 2) == 0) {
        // HFP断开
        hfp_connected = 0;
        connected_addr[0] = '\0';
        printf("HFP disconnected\n");
    }
    else if (strncmp(line, "JH", 2) == 0 || strncmp(line, "AD", 2) == 0) {
        // 当前连接设备地址（部分版本用JH，部分用AD）
        strncpy(connected_addr, line+2, 12);
        connected_addr[12] = '\0';
        printf("Current connected device: %s\n", connected_addr);
    }
    // 其他协议解析...
}