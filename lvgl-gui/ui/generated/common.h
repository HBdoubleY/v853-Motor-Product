#ifndef COMMON_H
#define COMMON_H
// #include "bluetoothadapter.h"

#if defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif
#include "lvgl_system.h"
#include "math.h"
#include "lv_drivers/display/g2d_driver.h"
#include "lv_drivers/display/sample_g2d_mem.h"
#include "hwdisplay.h"
#define DISP_DEVICE "/dev/disp"
#define DISP_FB "/dev/fb0"
#define DISP_CAPTUER_PATH "/mnt/SDCARD/screenShot"
typedef struct {
    unsigned long total;      // 总内存，单位kB
    unsigned long free;       // 空闲内存，单位kB
    unsigned long available;  // 可用内存，单位kB
    unsigned long buffers;    // 缓冲内存，单位kB
    unsigned long cached;     // 缓存内存，单位kB
} MemoryInfo;

//帧信息
typedef struct
{
    g2d_fmt_enh format;
    unsigned int frm_width;
    unsigned int frm_height;
    void *p_vir_addr[3];
    void *p_phy_addr[3];
}FRM_INFO;

typedef struct MPP_SYS_CONF
{
    unsigned int nAlignWidth;
    char mkfcTmpDir[256];
} MPP_SYS_CONF;

typedef struct{
    int RectX;
    int RectY;
    int RectW;
    int RectH;
}G2D_Rect;

typedef struct
{ 
    FRM_INFO src_frm_info;
    FRM_INFO dst_frm_info;
    G2D_Rect mSrcRect;
    G2D_Rect mDstRect;
    FILE *fd_in;
    FILE *fd_out; 
}G2D_CTX;

int MPI_init(void);
int G2dConvert_scale(G2D_CTX *p_g2d_ctx, int g2dfd);
int G2dConvert_rotate(G2D_CTX *p_g2d_ctx, int g2dfd, g2d_blt_flags_h ops);
int G2dConvert_formatconversion(G2D_CTX *p_g2d_ctx, int g2dfd);

//压力单位转换 
float PressureUnitConversion(float pressureData);
//温度单位转换
float TempUnitConversion(int tempData);

void SetBackLight(int brightness);
int Gpadc2Service(void);
int checkTFCardMountProc(void);
int day_of_week_kim_larson(int year, int month, int day);
#if defined(__cplusplus)||defined(c_plusplus)
}
#endif
#endif
