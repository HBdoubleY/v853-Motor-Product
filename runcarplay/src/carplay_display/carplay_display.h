/**
 * CarPlay H.264 decode and VO display module - public API
 * Assumes zlink already connected; receives H264 from video_data callback,
 * decodes via Allwinner VDEC and displays on VO with configurable rect.
 */
#ifndef CARPLAY_DISPLAY_H
#define CARPLAY_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_CARPLAY

/* ========== Display region (same as VO stDispRect) ========== */
/* disp_x, disp_y: top-left of carplay window on screen (pixels) */
/* disp_width, disp_height: size of carplay window (0 = use default/session size) */

/**
 * Create and start the carplay display module.
 * session_width/height must match libzlink_init_session_2() (e.g. 1280x720).
 * Returns 0 on success, -1 on error.
 */
int carplay_display_create(int disp_x, int disp_y, int disp_width, int disp_height,
                          int session_width, int session_height);

/**
 * Feed H264 data from libzlink video_data callback. Copy data and return quickly.
 * Returns 0 on success, -1 if queue full (packet dropped).
 */
int carplay_display_feed_h264(const char *data, int len);

/**
 * Update display rectangle at runtime (e.g. for window resize).
 */
void carplay_display_set_rect(int x, int y, int width, int height);

/**
 * Destroy module: stop decode thread, release VDEC/VO, free queue.
 */
void carplay_display_destroy(void);

/* ========== Split screen (CarPlay left, DVR right) ========== */
/** 1 = enable split (CarPlay left, vipp0/vipp8 right); 0 = full screen CarPlay */
extern int carplay_split_screen_enable;
/** Right edge X of CarPlay region (pixels); right region = [carplay_split_line_x, screen_width) */
extern int carplay_split_line_x;

/* ========== Reserved touch coordinates (assign from your "get screen click" API) ========== */
extern int carplay_touch_screen_x;   /* screen X (pixels) */
extern int carplay_touch_screen_y;   /* screen Y (pixels) */
extern int carplay_touch_screen_down; /* 1 = press, 0 = release */

/**
 * Read reserved vars, apply offset/scale, send to phone via libzlink_touch_event.
 * Call after assigning carplay_touch_screen_*.
 */
void carplay_touch_send(void);

/**
 * Pass screen coordinates directly; module does offset/scale and sends to phone.
 * Use when you already have (x, y, is_touch_down) in hand.
 */
void carplay_touch_send_xy(int screen_x, int screen_y, int is_touch_down);

#endif /* ENABLE_CARPLAY */

#ifdef __cplusplus
}
#endif

#endif /* CARPLAY_DISPLAY_H */
