/**
 * Zlink client - run in a thread, feed H264 to carplay_display.
 * When ENABLE_CARPLAY is off, this header and implementation are not used.
 */
#ifndef ZLINK_CLIENT_H
#define ZLINK_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#ifdef ENABLE_CARPLAY

/** Run zlink client loop (blocking). Call from a dedicated thread. Returns when init fails. */
void zlink_client_run(void);

/* Enable/disable forwarding buffered H264 packets to carplay_display. */
void zlink_client_set_video_active(int active);

/* Drop cached H264 prebuffer so next enter uses a fresh stream. */
void zlink_client_reset_video_prebuffer(void);

/* Request zlink video focus: 0 = phone projection, 1 = HU UI. */
int zlink_client_request_video_focus(int is_hu_focus_on);

/* Fetch and clear a pending "return to home" UI request. Returns link type or 0. */
int zlink_client_take_pending_home_request(void);

void carplay_is_running2(void);

#endif /* ENABLE_CARPLAY */

#ifdef __cplusplus
}
#endif

#endif /* ZLINK_CLIENT_H */
