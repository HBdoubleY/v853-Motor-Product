#ifndef TIRE_MANAGER_H
#define TIRE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化胎压模块：加载 /opt/private 配对信息到内存缓存。
 */
void tire_manager_init(void);

/**
 * 串口回调线程调用：将 BLE 行解析为胎压数据，并更新胎压缓存。
 * 只需要在 bt_serial.c 的 LP 分支里调用即可。
 */
void tire_on_ble_line(const char *line);

/**
 * 查询“前轮/后轮”的胎压与温度：
 * - 未配对：返回 false
 * - 已配对但 30s 内未收到最新广播：返回 false
 * - 返回 true：输出单位分别为 kPa 与 ℃
 */
bool tire_front_get_kpa_temp(int *pressure_kpa_out, int *temp_c_out);
bool tire_rear_get_kpa_temp(int *pressure_kpa_out, int *temp_c_out);

bool tire_front_is_paired(void);
bool tire_rear_is_paired(void);

/**
 * 获取后轮当前配对的后3字节（6位hex字符，如 "00EEBC"）。
 * @return true: 已配对且 out_suffix6 有效
 */
bool tire_rear_get_suffix6(char out_suffix6[7]);

/**
 * 获取前轮当前配对的后3字节（6位hex字符，如 "00EEBC"）。
 * @return true: 已配对且 out_suffix6 有效
 */
bool tire_front_get_suffix6(char out_suffix6[7]);

/**
 * 配对：suffix6_hex 需要是 6 位十六进制（例如 "00EEBC"），大小写不敏感。
 */
void tire_pair_set_front_suffix(const char *suffix6_hex);
void tire_pair_set_rear_suffix(const char *suffix6_hex);

/**
 * 清空前后轮配对信息（同时清空缓存，使 UI 回到 "--" 状态）。
 */
void tire_pair_clear_all(void);

#ifdef __cplusplus
}
#endif

#endif /* TIRE_MANAGER_H */

