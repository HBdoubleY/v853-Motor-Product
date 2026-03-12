/**
 * @file photo_view.h
 * @brief 图库图片查看逻辑 - 放大、缩小、旋转等（与 LVGL canvas 解耦）
 *
 * UI 层负责调用接口并更新 canvas/控件；本模块提供比例、角度、纯缓冲算法（双线性缩放、90° 旋转）。
 */

#ifndef PHOTO_VIEW_H
#define PHOTO_VIEW_H

#ifdef __cplusplus
extern "C" {
#endif

/** 缩放档位 */
typedef enum {
    PHOTO_ZOOM_50  = 50,
    PHOTO_ZOOM_75  = 75,
    PHOTO_ZOOM_100 = 100,
    PHOTO_ZOOM_150 = 150,
    PHOTO_ZOOM_200 = 200
} photo_view_zoom_t;

/** 旋转角度（度） */
typedef enum {
    PHOTO_ROTATE_0   = 0,
    PHOTO_ROTATE_90  = 90,
    PHOTO_ROTATE_180 = 180,
    PHOTO_ROTATE_270 = 270
} photo_view_rotate_t;

/**
 * @brief 下一档放大
 */
photo_view_zoom_t photo_view_zoom_in(photo_view_zoom_t current);

/**
 * @brief 下一档缩小
 */
photo_view_zoom_t photo_view_zoom_out(photo_view_zoom_t current);

/**
 * @brief 顺时针旋转 90 度（档位）
 */
photo_view_rotate_t photo_view_rotate_90_cw(photo_view_rotate_t current);

/**
 * @brief ARGB8888 双线性插值缩放（纯缓冲，无 LVGL）
 * @param src 源缓冲 (ARGB8888)
 * @param src_w, src_h 源宽高
 * @param dst 目标缓冲（由调用方分配，至少 dst_w*dst_h*4 字节）
 * @param dst_w, dst_h 目标宽高
 */
void photo_view_bilinear_scale_argb8888(const unsigned char *src, unsigned int src_w, unsigned int src_h,
                                       unsigned char *dst, unsigned int dst_w, unsigned int dst_h);

/**
 * @brief ARGB8888 顺时针旋转 90 度（纯缓冲）
 * @param src 源缓冲
 * @param src_w, src_h 源宽高（旋转后变为 src_h x src_w）
 * @param dst 目标缓冲（由调用方分配，至少 src_h*src_w*4 字节）
 */
void photo_view_rotate_90_cw_argb8888(const unsigned char *src, unsigned int src_w, unsigned int src_h,
                                      unsigned char *dst);

#ifdef __cplusplus
}
#endif

#endif /* PHOTO_VIEW_H */
