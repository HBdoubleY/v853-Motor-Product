---
name: 插入TF卡崩溃修复方案
overview: 插入 TF 卡后程序崩溃的根因是：在 LVGL 主线程里同步执行整套录像启动逻辑，与 CarPlay 使用的 VDEC/G2D/VO 等资源产生竞争，且主线程长时间阻塞导致 z-link 断开与资源释放并发，最终触发 Segmentation fault。修复以“CarPlay/AA 连接时禁止自动开始录像”为主方案，并可选地将录像启动改为异步或增加 display 侧防护。
todos: []
isProject: false
---

# 插入 TF 卡导致程序崩溃 - 分析与修复方案

## 一、崩溃时间线（来自日志）


| 时间           | 事件                                                                     |
| ------------ | ---------------------------------------------------------------------- |
| 05:49:55     | CarPlay 已连接，AirPlay session started，HID 启动                             |
| 05:49:58.294 | **"start to recording!!!!!"** — 检测到 TF 卡后自动开始录像                        |
| 05:50:09.171 | z-link video/AA/audio/control 各 loop 报 **recv fail、client disconnect** |
| 紧接着          | **Segmentation fault**，进程退出                                            |
| 之后           | "Mount state changed: 0 -> 1"（挂载状态输出在崩溃后）                              |


结论：**不是“先插入 TF 再挂载导致崩溃”，而是“插入 TF 后触发了自动开始录像，录像启动与 CarPlay 运行并发，约 10 秒后发生段错误”。**

---

## 二、根因分析

### 2.1 “start to recording” 的调用链

- **来源**：[lvgl-gui/lvgl_main.c](lvgl-gui/lvgl_main.c) 第 106 行。
- **触发路径**：
  - 定时器 **bt_status_check_timer**（**500ms 周期**）在 [lvgl_main.c](lvgl-gui/lvgl_main.c) 第 46–183 行执行。
  - 当 `g_sys_Data.TFmounted` 从 0 变为 1 时置 `recorderFlag = true`（第 63 行）。
  - 下一轮或同轮若满足：`g_sys_Data.TFmounted && (frontCamera \|\| rearCamera) && recorderFlag && powerOnRecorder && recorderMode == RECORDER_NONE`，则在 **同一 LVGL 主线程** 里执行第 94–131 行：
    - 设置 `recorderMode = RECORDER_NORMAL`
    - 调用 `CreateMsgQueueThread`、`createAIChn`、`createAencChn`、`createVencChn`、`createMuxChn`、`prepare`、`startVideoRecording`（vipp0 + vipp8）
    - 以及 `dashSpeedMark`、`SoundRecording`、创建 `DVRstaTimer` 等。

即：**插入 TF 卡后，在 LVGL 主线程里同步、长时间执行整套录像启动逻辑。**

### 2.2 与 CarPlay 的关系（资源与线程）

- **CarPlay 显示模块** [runcarplay/src/carplay_display/carplay_display.c](runcarplay/src/carplay_display/carplay_display.c) 使用：
  - **VDEC**（通道 0）解码 H.264
  - **G2D**（`g_g2dfd`）旋转
  - **VO** 显示
  - 运行在 **decode_thread** 与 **display_thread**，与主线程并发。
- **录像模块**（mpp_camera、record 等）使用：
  - **VI**（vipp0/vipp8，在 lvgl_main 初始化时已 createViChn）
  - **VENC**、**G2D**（[mpp_camera.c](runcarplay/src/app/video/mpp_camera.c) 中 `G2D_Convert_rotate` 使用同一 `g_g2dfd`）、VO/显示相关资源。
- **共享点**：
  - 同一 **G2D 设备** `g_g2dfd`（CarPlay display 与录像同时调用 `ioctl(g_g2dfd, G2D_CMD_BITBLT_H, ...)`）。
  - 同一进程内的 **VO/MPP/cedarc** 等可能共享底层资源或全局状态。
- **主线程阻塞**：
  - 录像启动在 **主线程** 执行数秒，期间 `lv_task_handler()` 与 `lvgl_handle_zlink_ui_requests()` 无法及时执行。
  - z-link 的 **video_main_loop**、**control_main_loop** 等运行在独立线程，可能因超时或异常进入 **client disconnect**，触发 session 清理或 display 销毁。
  - 此时 CarPlay 的 **decode/display 线程** 可能仍在访问 VDEC/G2D/VO，与“录像初始化”或“z-link 断开后的资源释放”产生 **竞态**，导致非法内存访问 → **Segmentation fault**。

因此：**崩溃与 CarPlay 有关，但不是 CarPlay 协议逻辑错误，而是“录像在 TF 插入后自动启动”与“CarPlay 正在使用 VDEC/G2D/VO”之间的资源竞争与主线程阻塞导致的并发问题。**

---

## 三、修复方案（按优先级）

### 方案 A：CarPlay/AA 连接时禁止自动开始录像（推荐，你方可独立完成）

- **思路**：当 z-link session 已建立（CarPlay 或 Android Auto 已连接）时，**不自动开始录像**，仅保留“拔卡自动停录像”等逻辑；用户可在 DVR 界面手动点击开始。
- **实现位置**：[lvgl-gui/lvgl_main.c](lvgl-gui/lvgl_main.c) 的 `bt_status_check_timer` 中，在“TF 已挂载且要自动开始录像”的分支（约第 92–132 行）。
- **修改要点**：
  - 在 `#ifdef ENABLE_CARPLAY` 下增加条件：若 `zlink_client_is_session_started()` 为 true，则**本次不自动启动录像**（例如：保持 `recorderFlag = true` 或置为 false 并跳过整段启动代码，避免在 session 存在时执行 CreateMsgQueueThread/startVideoRecording 等）。
  - 确保仅影响“自动开始录像”逻辑，不影响“拔卡停录像”和手动在 DVR 界面点击开始/停止。
- **优点**：不改录像模块、不引入新线程，改动小、风险低，能避免“插 TF + CarPlay 连接”场景下的资源竞争与崩溃。

### 方案 B：录像启动改为异步（需录像/UI 模块配合）

- **思路**：不在 LVGL timer 回调里同步执行 `CreateMsgQueueThread`、`createVencChn`、`startVideoRecording` 等，改为向**独立工作线程**投递“延迟启动录像”任务，由该线程在合适的时机执行，避免长时间阻塞主线程。
- **实现要点**：
  - 在 [lvgl_main.c](lvgl-gui/lvgl_main.c) 中，检测到需自动开始录像时，仅设置状态或投递请求（如 `recorderMode = RECORDER_NORMAL`、更新 UI 状态），实际启动逻辑放到工作线程中执行。
  - 需与录像模块负责人确认：启动接口是否可被从非主线程调用、是否有线程安全要求。
- **优点**：主线程不再长时间阻塞，可降低与 z-link 断开/清理的竞态概率；仍建议与方案 A 结合，在 CarPlay/AA 连接时禁止自动开始录像，避免资源冲突。

### 方案 C：CarPlay display 侧防护（治标）

- **思路**：在 [carplay_display.c](runcarplay/src/carplay_display/carplay_display.c) 的 decode_thread/display_thread 中，对 VDEC、G2D、VO 的调用增加**有效性检查**（如 `g_g2dfd > 0`、通道/句柄有效、运行状态标志等），在资源被抢占或已释放时快速退出或重试，避免非法访问导致 segfault。
- **注意**：可减轻崩溃或避免无意义调用，但无法消除“录像与 CarPlay 同时争用 G2D/VO”的根因，建议作为方案 A（或 A+B）的补充。

---

## 四、推荐实施顺序

1. **先做方案 A**：在 [lvgl_main.c](lvgl-gui/lvgl_main.c) 的 `bt_status_check_timer` 里，当 `zlink_client_is_session_started()` 为真时跳过“自动开始录像”分支；验证插入 TF 卡且 CarPlay 已连接时不再崩溃。
2. **可选方案 B**：若产品要求“CarPlay 连接时也允许自动录像”，再与录像模块一起把自动启动改为异步，并保留“CarPlay/AA 连接时不自动录像”的策略或加互斥保护。
3. **可选方案 C**：在 carplay_display 的 decode/display 线程中增加空指针与状态检查，作为防御性保护。

---

## 五、关键代码位置汇总


| 文件                                                                                                   | 作用                                                                   |
| ---------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------- |
| [lvgl-gui/lvgl_main.c](lvgl-gui/lvgl_main.c) 第 46–183 行                                              | `bt_status_check_timer`：TF 挂载检测、`recorderFlag`、**自动开始录像**（106–131 行） |
| [lvgl-gui/lvgl_main.c](lvgl-gui/lvgl_main.c) 第 420 行                                                 | 创建 500ms 的 `bt_status_check_timer`                                   |
| [runcarplay/src/carplay/zlink_client.c](runcarplay/src/carplay/zlink_client.c)                       | `zlink_client_is_session_started()` 实现                               |
| [runcarplay/src/carplay_display/carplay_display.c](runcarplay/src/carplay_display/carplay_display.c) | VDEC/G2D/VO 使用；可加方案 C 的防护                                            |
| [runcarplay/src/app/video/mpp_camera.c](runcarplay/src/app/video/mpp_camera.c)                       | 录像 G2D 使用（与 CarPlay 共用 `g_g2dfd`）                                    |


---

## 六、小结

- **崩溃原因**：插入 TF 卡后，在 LVGL 主线程中同步执行整套录像启动，与 CarPlay 的 VDEC/G2D/VO 使用产生资源竞争，且主线程长时间阻塞导致 z-link 断开与资源释放并发，引发 **Segmentation fault**。
- **与 CarPlay 的关系**：有直接关系（共享 G2D/MPP 资源、主线程阻塞影响 z-link），但根因是“录像在 TF 插入时的自动启动方式与主线程/资源使用”设计问题。
- **修复建议**：优先在“CarPlay/AA 已连接时禁止自动开始录像”（方案 A）；如需进一步稳健，再考虑录像异步启动（方案 B）和 display 侧防护（方案 C）。

