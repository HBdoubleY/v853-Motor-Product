#ifndef BT_SERIAL_H
#define BT_SERIAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 用户回调函数类型，每当收到一行完整数据（以\r\n结尾）时调用。
 * @param line 收到的数据行（已去除末尾的\r\n，以'\0'结尾）
 * @param user_arg 用户注册时传入的任意指针
 */
typedef void (*bt_data_callback)(const char *line, void *user_arg);

/**
 * 初始化串口并启动后台读取线程。
 * @param dev       串口设备路径，例如 "/dev/ttyS0"
 * @param baudrate  波特率，如 9600, 115200, 460800 等
 * @param cb        数据回调函数，不能为NULL
 * @param user_arg  用户自定义参数，传递给回调
 * @return          成功返回0，失败返回-1
 */
int bt_serial_init(const char *dev, int baudrate, bt_data_callback cb, void *user_arg);

/**
 * 发送AT指令（自动附加\r\n）。
 * @param cmd 指令字符串，如 "AT#CA"
 * @return    成功返回发送字节数，失败返回-1
 */
int bt_serial_send(const char *cmd);

/**
 * 发送原始数据（不附加任何东西）。
 * @param data 数据缓冲区
 * @param len  数据长度
 * @return     成功返回发送字节数，失败返回-1
 */
int bt_serial_send_raw(const char *data, int len);

/**
 * 停止后台线程并关闭串口。
 */
void bt_serial_cleanup(void);

// 蓝牙回调函数
void on_bt_data(const char *line, void *arg);

#ifdef __cplusplus
}
#endif

#endif // BT_SERIAL_H