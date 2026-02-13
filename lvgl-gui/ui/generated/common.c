#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "sys/statfs.h"
#include <sys/ioctl.h>
#include "sys/mman.h"

#ifndef AWALIGN
#define AWALIGN(x, a)              ((a) * (((x) + (a) - 1) / (a)))
#endif

#define GPADC2_PATH "/sys/class/gpadc/data"
int MPI_init(void){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);

    MPP_SYS_CONF sysconf;
    memset(&sysconf, 0 , sizeof(MPP_SYS_CONF));
    sysconf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&sysconf);
    int ret = AW_MPI_SYS_Init();
    if(ret < 0){
        printf("sys MPI Init failed!");
        return -1;
    }
    g2d_MemOpen();//ion_mem开启
    return 0;
}

int G2dConvert_scale(G2D_CTX *p_g2d_ctx, int g2dfd)
{
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);

    int ret = 0;
    g2d_blt_h blit;
    //config blit
    memset(&blit, 0, sizeof(g2d_blt_h));

    blit.flag_h = G2D_BLT_NONE_H;       // angle rotation used
    blit.src_image_h.format = p_g2d_ctx->src_frm_info.format;
    blit.src_image_h.laddr[0] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[0];
    blit.src_image_h.laddr[1] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[1];
    blit.src_image_h.laddr[2] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[2];
    //blit.src_image_h.haddr[] = 
    blit.src_image_h.width = p_g2d_ctx->src_frm_info.frm_width;
    blit.src_image_h.height = p_g2d_ctx->src_frm_info.frm_height;
    blit.src_image_h.align[0] = 0;
    blit.src_image_h.align[1] = 0;
    blit.src_image_h.align[2] = 0;
    blit.src_image_h.clip_rect.x = p_g2d_ctx->mSrcRect.RectX;
    blit.src_image_h.clip_rect.y = p_g2d_ctx->mSrcRect.RectY;
    blit.src_image_h.clip_rect.w = p_g2d_ctx->mSrcRect.RectW;
    blit.src_image_h.clip_rect.h = p_g2d_ctx->mSrcRect.RectH;
    blit.src_image_h.gamut = G2D_BT709;
    blit.src_image_h.bpremul = 0;
    //blit.src_image_h.alpha = 0xff;
    blit.src_image_h.mode = G2D_PIXEL_ALPHA;   //G2D_PIXEL_ALPHA, G2D_GLOBAL_ALPHA
    blit.src_image_h.fd = -1;
    blit.src_image_h.use_phy_addr = 1;

    //blit.dst_image_h.bbuff = 1;
    //blit.dst_image_h.color = 0xff;
    blit.dst_image_h.format = p_g2d_ctx->dst_frm_info.format;
    blit.dst_image_h.laddr[0] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[0];
    blit.dst_image_h.laddr[1] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[1];
    blit.dst_image_h.laddr[2] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[2];
    //blit.dst_image_h.haddr[] = 
    blit.dst_image_h.width = p_g2d_ctx->dst_frm_info.frm_width;
    blit.dst_image_h.height = p_g2d_ctx->dst_frm_info.frm_height;
    blit.dst_image_h.align[0] = 0;
    blit.dst_image_h.align[1] = 0;
    blit.dst_image_h.align[2] = 0;
    blit.dst_image_h.clip_rect.x = p_g2d_ctx->mDstRect.RectX;
    blit.dst_image_h.clip_rect.y = p_g2d_ctx->mDstRect.RectY;
    blit.dst_image_h.clip_rect.w = p_g2d_ctx->mDstRect.RectW;
    blit.dst_image_h.clip_rect.h = p_g2d_ctx->mDstRect.RectH;
    blit.dst_image_h.gamut = G2D_BT709;
    blit.dst_image_h.bpremul = 0;
    //blit.dst_image_h.alpha = 0xff;
    blit.dst_image_h.mode = G2D_PIXEL_ALPHA;   //G2D_PIXEL_ALPHA, G2D_GLOBAL_ALPHA
    blit.dst_image_h.fd = -1;
    blit.dst_image_h.use_phy_addr = 1;

    ret = ioctl(g2dfd, G2D_CMD_BITBLT_H, (unsigned long)&blit);
    if(ret < 0)
    {
        printf("fatal error! bit-block(image) transfer failed[%d]", ret);
        system("cd /sys/class/sunxi_dump;echo 0x14A8000,0x14A8100 > dump;cat dump");
    }
    return ret;
}

int G2dConvert_rotate(G2D_CTX *p_g2d_ctx, int g2dfd, g2d_blt_flags_h ops)
{
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);

    int ret = 0;
    g2d_blt_h blit;
    memset(&blit, 0, sizeof(g2d_blt_h));

    blit.flag_h = ops;
    blit.src_image_h.format = p_g2d_ctx->src_frm_info.format;
    blit.src_image_h.laddr[0] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[0];
    blit.src_image_h.laddr[1] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[1];
    blit.src_image_h.laddr[2] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[2];
    //blit.src_image_h.haddr[] = 
    blit.src_image_h.width = p_g2d_ctx->src_frm_info.frm_width;
    blit.src_image_h.height = p_g2d_ctx->src_frm_info.frm_height;
    blit.src_image_h.align[0] = 0;
    blit.src_image_h.align[1] = 0;
    blit.src_image_h.align[2] = 0;
    blit.src_image_h.clip_rect.x = p_g2d_ctx->mSrcRect.RectX;
    blit.src_image_h.clip_rect.y = p_g2d_ctx->mSrcRect.RectY;
    blit.src_image_h.clip_rect.w = p_g2d_ctx->mSrcRect.RectW;
    blit.src_image_h.clip_rect.h = p_g2d_ctx->mSrcRect.RectH;
    blit.src_image_h.gamut = G2D_BT709;
    blit.src_image_h.bpremul = 0;
    blit.src_image_h.mode = G2D_PIXEL_ALPHA;   //G2D_PIXEL_ALPHA, G2D_GLOBAL_ALPHA
    blit.src_image_h.fd = -1;
    blit.src_image_h.use_phy_addr = 1;

    blit.dst_image_h.format = p_g2d_ctx->dst_frm_info.format;
    blit.dst_image_h.laddr[0] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[0];
    blit.dst_image_h.laddr[1] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[1];
    blit.dst_image_h.laddr[2] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[2];
    //blit.dst_image_h.haddr[] = 
    blit.dst_image_h.width = p_g2d_ctx->dst_frm_info.frm_width;
    blit.dst_image_h.height = p_g2d_ctx->dst_frm_info.frm_height;
    blit.dst_image_h.align[0] = 0;
    blit.dst_image_h.align[1] = 0;
    blit.dst_image_h.align[2] = 0;
    blit.dst_image_h.clip_rect.x = p_g2d_ctx->mDstRect.RectX;
    blit.dst_image_h.clip_rect.y = p_g2d_ctx->mDstRect.RectY;
    blit.dst_image_h.clip_rect.w = p_g2d_ctx->mDstRect.RectW;
    blit.dst_image_h.clip_rect.h = p_g2d_ctx->mDstRect.RectH;
    blit.dst_image_h.gamut = G2D_BT709;
    blit.dst_image_h.bpremul = 0;
    //blit.dst_image_h.alpha = 0xff;
    blit.dst_image_h.mode = G2D_PIXEL_ALPHA;   //G2D_PIXEL_ALPHA, G2D_GLOBAL_ALPHA
    blit.dst_image_h.fd = -1;
    blit.dst_image_h.use_phy_addr = 1;

    ret = ioctl(g2dfd, G2D_CMD_BITBLT_H, (unsigned long)&blit);
    if(ret < 0)
    {
        printf("fatal error! bit-block(image) transfer failed[%d]", ret);
        system("cd /sys/class/sunxi_dump;echo 0x14A8000,0x14A8100 > dump;cat dump");
    }

    return ret;
} 

int G2dConvert_formatconversion(G2D_CTX *p_g2d_ctx, int g2dfd)
{
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);

    int ret = 0;
    g2d_blt_h blit;

    memset(&blit, 0, sizeof(g2d_blt_h));

    blit.flag_h = G2D_BLT_NONE_H;       // angle rotation used
    blit.src_image_h.format = p_g2d_ctx->src_frm_info.format;
    blit.src_image_h.laddr[0] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[0];
    blit.src_image_h.laddr[1] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[1];
    blit.src_image_h.laddr[2] = (unsigned int)p_g2d_ctx->src_frm_info.p_phy_addr[2];
    //blit.src_image_h.haddr[] =
    blit.src_image_h.width = p_g2d_ctx->src_frm_info.frm_width;
    blit.src_image_h.height = p_g2d_ctx->src_frm_info.frm_height;
    blit.src_image_h.align[0] = 0;
    blit.src_image_h.align[1] = 0;
    blit.src_image_h.align[2] = 0;
    blit.src_image_h.clip_rect.x = p_g2d_ctx->mSrcRect.RectX;
    blit.src_image_h.clip_rect.y = p_g2d_ctx->mSrcRect.RectY;
    blit.src_image_h.clip_rect.w = p_g2d_ctx->mSrcRect.RectW;
    blit.src_image_h.clip_rect.h = p_g2d_ctx->mSrcRect.RectH;
    blit.src_image_h.gamut = G2D_BT709;
    blit.src_image_h.bpremul = 0;
    //blit.src_image_h.alpha = 0xff;
    blit.src_image_h.mode = G2D_PIXEL_ALPHA;   //G2D_PIXEL_ALPHA, G2D_GLOBAL_ALPHA
    blit.src_image_h.fd = -1;
    blit.src_image_h.use_phy_addr = 1;

    blit.dst_image_h.format = p_g2d_ctx->dst_frm_info.format;
    blit.dst_image_h.laddr[0] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[0];
    blit.dst_image_h.laddr[1] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[1];
    blit.dst_image_h.laddr[2] = (unsigned int)p_g2d_ctx->dst_frm_info.p_phy_addr[2];
    //blit.dst_image_h.haddr[] =
    blit.dst_image_h.width = p_g2d_ctx->dst_frm_info.frm_width;
    blit.dst_image_h.height = p_g2d_ctx->dst_frm_info.frm_height;
    blit.dst_image_h.align[0] = 0;
    blit.dst_image_h.align[1] = 0;
    blit.dst_image_h.align[2] = 0;
    blit.dst_image_h.clip_rect.x = p_g2d_ctx->mDstRect.RectX;
    blit.dst_image_h.clip_rect.y = p_g2d_ctx->mDstRect.RectY;
    blit.dst_image_h.clip_rect.w = p_g2d_ctx->mDstRect.RectW;
    blit.dst_image_h.clip_rect.h = p_g2d_ctx->mDstRect.RectH;
    blit.dst_image_h.gamut = G2D_BT709;
    blit.dst_image_h.bpremul = 0;
    //blit.dst_image_h.alpha = 0xff;
    blit.dst_image_h.mode = G2D_PIXEL_ALPHA;   //G2D_PIXEL_ALPHA, G2D_GLOBAL_ALPHA
    blit.dst_image_h.fd = -1;
    blit.dst_image_h.use_phy_addr = 1;

    ret = ioctl(g2dfd, G2D_CMD_BITBLT_H, (unsigned long)&blit);
    if(ret < 0)
    {
        printf("fatal error! bit-block(image) transfer failed[%d]", ret);
        system("cd /sys/class/sunxi_dump;echo 0x14A8000,0x14A8100 > dump;cat dump");
    }

    return ret;
}


float PressureUnitConversion(float pressureData){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);

    float result = 0;
    if(!g_sys_Data.pressureUnit){//bar
        result = pressureData/14.5;       
    }else{
        result = pressureData*14.5;
    }
    return result;
}

float TempUnitConversion(int tempData){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);

    float result = 0;
    if(!g_sys_Data.tempUnit){//°C
        result = (tempData * 9/5) + 32;
    }else{
        result = (tempData - 32) * 5/9;
    }
    return result;
}

void SetBackLight(int brightness){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    char cmd[128];
    int value = 255 - (brightness * 255 + 50) / 100;
    memset(cmd,0,sizeof(cmd));
    snprintf(cmd, sizeof(cmd)-1, "cd /sys/kernel/debug/dispdbg; echo lcd0 > name; echo setbl > command; echo %d > param; echo 1 > start\n", value);
    system(cmd);
    printf(cmd);
}

int GpadcVoltageValue(void){
    printf("%s:%d\n",__func__,__LINE__);
    system("echo gpadc2,1 > /sys/class/gpadc/status");
    system("echo 2 > /sys/class/gpadc/data");
    system("sync");
    int gpadc_fd = open(GPADC2_PATH, O_RDONLY | O_NONBLOCK);
    if(gpadc_fd <= 0){
        printf("open dev err:%s!!!\n",GPADC2_PATH);
        return;
    }
    char data[24]= {};
    int vol = 0;

        lseek(gpadc_fd, 0, SEEK_SET);
        int ret = read(gpadc_fd,&data,sizeof(data));
        if(ret >= 0){
            data[ret] = '\0';
            vol = atoi(data);
            printf("adc data,vol:%dmv!!!\n",vol);
    }

    close(gpadc_fd);
    return vol;
    }



#define KB 1024.0       // 2^10
#define MB 1048576.0    // 2^20 
#define GB 1073741824.0 // 2^30 
void CheckDiskInfo(){
    printf("%s:%d\n",__func__,__LINE__);
        struct statfs diskInfo;

        statfs("/mnt/extsd", &diskInfo);	
        uint64_t blocksize = diskInfo.f_bsize;                   
        uint64_t totalsize = blocksize * diskInfo.f_blocks;      
        printf("total blocks = %lld\n",diskInfo.f_blocks);
        printf("Total_size = %lld B = %f KB = %f MB = %f GB\n",
                totalsize,
                totalsize / KB,
                totalsize / MB,
                totalsize / GB);

        uint64_t freeDisk = diskInfo.f_bfree * blocksize;       
        uint64_t availableDisk = diskInfo.f_bavail * blocksize;
        printf("Disk_free = %f MB = %f GB\n"
                        "Disk_available = %f MB = %f GB\n",
                freeDisk / MB,
                freeDisk / GB,
                availableDisk / MB,
                availableDisk / GB);
}

int checkTFCardMountProc(void){
//    printf("%s:%d\n",__func__,__LINE__);
    int mounted = 0;
    
    if(access("/dev/mmcblk0", F_OK) == 0){
        mounted = 1;
    }

    return mounted;    
}

int day_of_week_kim_larson(int year, int month, int day) {
    // 注意：1月和2月要当作上一年的13月和14月
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    
    // 基姆拉尔森公式
    int weekday = (day + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400) % 7;
    
    // 返回0-6，0表示星期一，1表示星期二，...，6表示星期日
    return weekday;
}

void RestorefactorySettings(void){
    if (access("/opt/work/storage_data.txt", F_OK) == 0) {
        system("rm /opt/work/storage_data.txt"); 
    }
    system("sync");
    system("reboot");
}
void systemMeminfo(MemoryInfo* mem_info){
    FILE* fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        printf("无法打开/proc/meminfo\n");
        return -1;
    }
    char line[256];
    memset(mem_info, 0, sizeof(MemoryInfo));
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %lu kB", &mem_info->total) == 1) {
            continue;
        } else if (sscanf(line, "MemFree: %lu kB", &mem_info->free) == 1) {
            continue;
        } else if (sscanf(line, "MemAvailable: %lu kB", &mem_info->available) == 1) {
            continue;
        } else if (sscanf(line, "Buffers: %lu kB", &mem_info->buffers) == 1) {
            continue;
        } else if (sscanf(line, "Cached: %lu kB", &mem_info->cached) == 1) {
            continue;
        }
    }
    fclose(fp);
    return 0;  
}
int save_screenshot_to_file(const char *filename, void *data, int width, int height, int bpp) {
    if (bpp != 32 && bpp != 24 && bpp != 16) {
        printf("don't support bit depth: %d\n", bpp);
        return -1;
    }
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        printf("open %s fail!\n", filename);
        return -1;
    }
    int row_size = width * (bpp / 8);
    int padding = (4 - (row_size % 4)) % 4;
    int image_size = (row_size + padding) * height;
    unsigned char file_header[14] = {
        'B', 'M',                     // 文件类型
        0, 0, 0, 0,                   // 文件大小
        0, 0, 0, 0,                   // 保留
        54, 0, 0, 0                   // 像素数据偏移
    };
    unsigned char info_header[40] = {
        40, 0, 0, 0,                  // 信息头大小
        0, 0, 0, 0,                   // 宽度
        0, 0, 0, 0,                   // 高度
        1, 0,                         // 颜色平面数
        32, 0,                        // 位深度
        0, 0, 0, 0,                   // 压缩方式
        0, 0, 0, 0,                   // 图像大小
        0, 0, 0, 0,                   // 水平分辨率
        0, 0, 0, 0,                   // 垂直分辨率
        0, 0, 0, 0,                   // 颜色数
        0, 0, 0, 0                    // 重要颜色数
    };
    int file_size = 54 + image_size;
    file_header[2] = (unsigned char)(file_size);
    file_header[3] = (unsigned char)(file_size >> 8);
    file_header[4] = (unsigned char)(file_size >> 16);
    file_header[5] = (unsigned char)(file_size >> 24);
    info_header[4] = (unsigned char)(width);
    info_header[5] = (unsigned char)(width >> 8);
    info_header[6] = (unsigned char)(width >> 16);
    info_header[7] = (unsigned char)(width >> 24);
    info_header[8] = (unsigned char)(height);
    info_header[9] = (unsigned char)(height >> 8);
    info_header[10] = (unsigned char)(height >> 16);
    info_header[11] = (unsigned char)(height >> 24);
    info_header[14] = (unsigned char)(bpp);
    info_header[15] = (unsigned char)(bpp >> 8);
    fwrite(file_header, 1, 14, fp);
    fwrite(info_header, 1, 40, fp);
    unsigned char *pixels = (unsigned char *)data;
    for (int y = height - 1; y >= 0; y--) {
        unsigned char *row = pixels + y * width * 4;
        fwrite(row, 1, width * 4, fp);
        for (int p = 0; p < padding; p++) {
            fputc(0, fp);
        }
    }
    fclose(fp);
    printf("screenshot save: %s\n", filename);
    return 0;
}
int screenShotFun(char *output_filename){
    int dispfd = -1;
    int fbfd = -1;
    void *fb_address = NULL;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    long int screen_size = 0; 
    dispfd = open(DISP_DEVICE, O_RDWR);
    if (dispfd < 0) {
        printf("open display dev fail!\n");
        return -1;
    }
    fbfd = open(DISP_FB, O_RDWR);
    if (fbfd < 0) {
        printf("open display fb fail!\n");
        close(dispfd);
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        printf("Error reading fixed screen info\n");
        close(fbfd);
        return NULL;
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        printf("Error reading variable screen info\n");
        close(fbfd);
        return NULL;
    }
    screen_size = vinfo.yres_virtual * finfo.line_length;
    fb_address = (char *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fb_address == (char *)-1) {
        printf("Error mapping framebuffer device to memory\n");
        close(fb_address);
        return NULL;
    }
    unsigned long arg[3];
    disp_capture_info info;
    arg[0] = 0;
    int screen_width = ioctl(dispfd, DISP_GET_SCN_WIDTH, (void*)arg);
    int screen_height = ioctl(dispfd, DISP_GET_SCN_HEIGHT, (void*)arg);
    info.window.x = 0;
    info.window.y = 0;
    info.window.width = screen_width;
    info.window.height = screen_height;
    info.out_frame.format = DISP_FORMAT_ARGB_8888;
    info.out_frame.size[0].width = screen_width;
    info.out_frame.size[0].height = screen_height;
    info.out_frame.crop.x = 0;
    info.out_frame.crop.y = 0;
    info.out_frame.crop.width = screen_width;
    info.out_frame.crop.height = screen_height;
    info.out_frame.addr[0] = fb_address; //buffer address
    arg[0] = 0;//显示通道0
    arg[1] = (unsigned long)&info;
    if(ioctl(dispfd, DISP_CAPTURE_COMMIT, (void*)arg) != 0){
        printf("start disp capture fail1\n");
        return -1;
    }
    if(ioctl(dispfd, DISP_CAPTURE_START, (void*)arg) != 0){
        printf("disp capture fail1\n");
        return -1;
    }
    if(ioctl(dispfd, DISP_CAPTURE_STOP, (void*)arg) != 0){
        printf("stop disp capture fail1\n");
        return -1;
    }
    if (output_filename) {
        save_screenshot_to_file(output_filename, fb_address, 
                                screen_width, screen_height, 32);
    }  
    if (fb_address) {
        munmap(fb_address, finfo.smem_len);
    }
    if (fbfd >= 0) {
        close(fbfd);
    }
    if (dispfd >= 0) {
        close(dispfd);
    } 
    return 0;
}