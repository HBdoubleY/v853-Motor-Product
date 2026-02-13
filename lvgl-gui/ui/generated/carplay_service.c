#include <stdio.h>
#include "carplay_service.h"
#include "queue.h"
#include <stdlib.h>
#include <linux/input.h>
#include <errno.h>   // 用于 errno
#include <sys/mman.h>
#include <linux/fb.h>

#include "gui_guider.h"


#ifndef AWALIGN
#define AWALIGN(x, a)              ((a) * (((x) + (a) - 1) / (a)))
#endif

#define ANDROIAUTO_SRC_WIDTH (1920)
#define ANDROIAUTO_SRC_HEIGHT (1088)

#define CARPLAY_SRC_WIDTH (1440)
#define CARPLAY_SRC_HEIGHT (736)

#define CARPLAY_DST_WIDTH (1440)
#define CARPLAY_DST_HEIGHT (720)

#define CARPLAY_SPLIT_DST_WIDTH (1000)
#define CARPLAY_SPLIT_DST_HEIGHT (720)

extern int g_g2dfd;
extern Queue* gUiQueue;
static pthread_t carplay_threadID;
static int carplay_thread_exitflag = 0;
g2d_blt_h *blit = NULL;
g2d_blt_h *blit1 = NULL;
G2D_CTX *p_g2d_ctx_Rotate = NULL;
G2D_CTX *p_g2d_ctx_scale= NULL;
G2D_CTX *p_g2d_ctx_convFormat = NULL;
libhwclayerpara_t *picture = NULL;
static int hlay = -1; //
struct src_info *srcinfo = NULL;

static int prepareRotateFrmBuff(G2D_CTX *p_g2d_ctx){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    if(g_sys_Data.linktype ==  LINK_TYPE_ANDROIDAUTO){
        p_g2d_ctx->src_frm_info.frm_width = ANDROIAUTO_SRC_WIDTH;
        p_g2d_ctx->src_frm_info.frm_height = ANDROIAUTO_SRC_HEIGHT;
    }else if(g_sys_Data.linktype ==  LINK_TYPE_CARPLAY){
        p_g2d_ctx->src_frm_info.frm_width = CARPLAY_SRC_WIDTH;
        p_g2d_ctx->src_frm_info.frm_height = CARPLAY_SRC_HEIGHT;
    }
    p_g2d_ctx->dst_frm_info.frm_width = CARPLAY_DST_HEIGHT;
    p_g2d_ctx->dst_frm_info.frm_height = CARPLAY_DST_WIDTH;   

    p_g2d_ctx->mSrcRect.RectX = 0;
    p_g2d_ctx->mSrcRect.RectY = 0;
    p_g2d_ctx->mSrcRect.RectW = CARPLAY_DST_WIDTH;
    p_g2d_ctx->mSrcRect.RectH = CARPLAY_DST_HEIGHT;

    p_g2d_ctx->mDstRect.RectX = 0;
    p_g2d_ctx->mDstRect.RectY = 0;
    p_g2d_ctx->mDstRect.RectW = p_g2d_ctx->dst_frm_info.frm_width;
    p_g2d_ctx->mDstRect.RectH = p_g2d_ctx->dst_frm_info.frm_height;

    p_g2d_ctx->src_frm_info.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
    p_g2d_ctx->dst_frm_info.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;

    int awalign_size = AWALIGN(p_g2d_ctx->src_frm_info.frm_width, 16)*AWALIGN(p_g2d_ctx->src_frm_info.frm_height, 16);
    p_g2d_ctx->src_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size);
    if(NULL == p_g2d_ctx->src_frm_info.p_vir_addr[0])
    {
        printf("malloc_src_frm_y_mem_failed");
        return -1;
    }
    p_g2d_ctx->src_frm_info.p_vir_addr[1] = (void *)g2d_allocMem(awalign_size/2);
    if(NULL == p_g2d_ctx->src_frm_info.p_vir_addr[1])
    {
        g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
        printf("malloc_src_frm_c_mem_failed");    
        return -1;
    }
    p_g2d_ctx->src_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->src_frm_info.p_vir_addr[0]); 
    p_g2d_ctx->src_frm_info.p_phy_addr[1] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->src_frm_info.p_vir_addr[1]);

    p_g2d_ctx->dst_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size);
    if(NULL == p_g2d_ctx->dst_frm_info.p_vir_addr[0])
    {
        if(p_g2d_ctx->src_frm_info.p_vir_addr[0] != NULL)
        {
            g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
        }
        if(p_g2d_ctx->src_frm_info.p_vir_addr[1] != NULL)
        {
            g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[1]);
        }
        printf("malloc_dst_frm_y_mem_failed");
        return -1;
    }
    p_g2d_ctx->dst_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->dst_frm_info.p_vir_addr[0]);
    p_g2d_ctx->dst_frm_info.p_vir_addr[1] = (void *)g2d_allocMem(awalign_size /2);
    if(NULL == p_g2d_ctx->dst_frm_info.p_vir_addr[1])
    {
        if(p_g2d_ctx->src_frm_info.p_vir_addr[0] != NULL)
        {
            g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
        }
        if(p_g2d_ctx->src_frm_info.p_vir_addr[1] != NULL)
        {
            g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[1]);
        }
        if(p_g2d_ctx->dst_frm_info.p_vir_addr[0] != NULL)
        {
            g2d_freeMem(p_g2d_ctx->dst_frm_info.p_vir_addr[0]);
        }
        printf("malloc_dst_frm_y_mem_failed");
        return -1;
    }
    p_g2d_ctx->dst_frm_info.p_phy_addr[1] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->dst_frm_info.p_vir_addr[1]); 
}

static int prepareConvFormatFrmBuff(G2D_CTX *p_g2d_ctx){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);

    p_g2d_ctx->src_frm_info.frm_width = CARPLAY_DST_HEIGHT;
    p_g2d_ctx->src_frm_info.frm_height = CARPLAY_DST_WIDTH;


    p_g2d_ctx->dst_frm_info.frm_width = CARPLAY_DST_HEIGHT;
    p_g2d_ctx->dst_frm_info.frm_height = CARPLAY_DST_WIDTH; 

    p_g2d_ctx->mSrcRect.RectX = 0;
    p_g2d_ctx->mSrcRect.RectY = 0;
    p_g2d_ctx->mSrcRect.RectW = CARPLAY_DST_HEIGHT;
    p_g2d_ctx->mSrcRect.RectH = CARPLAY_DST_WIDTH;

    p_g2d_ctx->mDstRect.RectX = 0;
    p_g2d_ctx->mDstRect.RectY = 0;
    p_g2d_ctx->mDstRect.RectW = p_g2d_ctx->dst_frm_info.frm_width;
    p_g2d_ctx->mDstRect.RectH = p_g2d_ctx->dst_frm_info.frm_height;

    p_g2d_ctx->src_frm_info.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
    p_g2d_ctx->dst_frm_info.format = G2D_FORMAT_ABGR8888;

    int awalign_size = AWALIGN(p_g2d_ctx->src_frm_info.frm_width, 16)*AWALIGN(p_g2d_ctx->src_frm_info.frm_height, 16);
    p_g2d_ctx->src_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size);
    if(NULL == p_g2d_ctx->src_frm_info.p_vir_addr[0])
    {
        printf("malloc_src_frm_y_mem_failed");
        return -1;
    }
    p_g2d_ctx->src_frm_info.p_vir_addr[1] = (void *)g2d_allocMem(awalign_size/2);
    if(NULL == p_g2d_ctx->src_frm_info.p_vir_addr[1])
    {
        g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
        printf("malloc_src_frm_c_mem_failed");    
        return -1;
    }
    p_g2d_ctx->src_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->src_frm_info.p_vir_addr[0]); 
    p_g2d_ctx->src_frm_info.p_phy_addr[1] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->src_frm_info.p_vir_addr[1]);

    p_g2d_ctx->dst_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size*4);
    if(NULL == p_g2d_ctx->dst_frm_info.p_vir_addr[0])
    {
        if(p_g2d_ctx->src_frm_info.p_vir_addr[0] != NULL)
        {
            g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
        }
        if(p_g2d_ctx->src_frm_info.p_vir_addr[1] != NULL)
        {
            g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[1]);
        }
        printf("malloc_dst_frm_y_mem_failed");
        return -1;
    }
    p_g2d_ctx->dst_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->dst_frm_info.p_vir_addr[0]);           
}

static int prepareScaleFrmBuff(G2D_CTX *p_g2d_ctx, g2d_fmt_enh format){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    p_g2d_ctx->src_frm_info.frm_width = CARPLAY_DST_HEIGHT;
    p_g2d_ctx->src_frm_info.frm_height = CARPLAY_DST_WIDTH;

    switch (g_sys_Data.splitScreenDisp)
    {
    case DISPLAY_NORMAL:
        p_g2d_ctx->dst_frm_info.frm_width = CARPLAY_DST_HEIGHT;
        p_g2d_ctx->dst_frm_info.frm_height = CARPLAY_DST_WIDTH;
    case DISPLAY_FRONT:
    case DISPLAY_FRONT_REAR:
    case DISPLAY_REAR: 
        p_g2d_ctx->dst_frm_info.frm_width = CARPLAY_SPLIT_DST_HEIGHT;
        p_g2d_ctx->dst_frm_info.frm_height = CARPLAY_SPLIT_DST_WIDTH;
    break;
    default:
    break;
    }


    p_g2d_ctx->mSrcRect.RectX = 0;
    p_g2d_ctx->mSrcRect.RectY = 0;
    p_g2d_ctx->mSrcRect.RectW = p_g2d_ctx->src_frm_info.frm_width;
    p_g2d_ctx->mSrcRect.RectH = p_g2d_ctx->src_frm_info.frm_height;

    p_g2d_ctx->mDstRect.RectX = 0;
    p_g2d_ctx->mDstRect.RectY = 0;
    p_g2d_ctx->mDstRect.RectW = p_g2d_ctx->dst_frm_info.frm_width;
    p_g2d_ctx->mDstRect.RectH = p_g2d_ctx->dst_frm_info.frm_height;

    p_g2d_ctx->src_frm_info.format = format;
    p_g2d_ctx->dst_frm_info.format = format;   

    int awalign_size = AWALIGN(p_g2d_ctx->src_frm_info.frm_width, 16)*AWALIGN(p_g2d_ctx->src_frm_info.frm_height, 16);

    switch (format)
    {
    case G2D_FORMAT_YUV420UVC_V1U1V0U0:
        /* code */
        p_g2d_ctx->src_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size);
        if(NULL == p_g2d_ctx->src_frm_info.p_vir_addr[0])
        {
            printf("malloc_src_frm_y_mem_failed");
            return -1;
        }
        p_g2d_ctx->src_frm_info.p_vir_addr[1] = (void *)g2d_allocMem(awalign_size/2);
        if(NULL == p_g2d_ctx->src_frm_info.p_vir_addr[1])
        {
            g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
            printf("malloc_src_frm_c_mem_failed");    
            return -1;
        }
        p_g2d_ctx->src_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->src_frm_info.p_vir_addr[0]); 
        p_g2d_ctx->src_frm_info.p_phy_addr[1] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->src_frm_info.p_vir_addr[1]);        
        break;
    case G2D_FORMAT_ABGR8888:
    /* code */
        p_g2d_ctx->src_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size*4);
        if(NULL == p_g2d_ctx->src_frm_info.p_vir_addr[0])
        {
            printf("malloc_src_frm_y_mem_failed");
            return -1;
        }
        p_g2d_ctx->src_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->src_frm_info.p_vir_addr[0]); 
    break;
    default:
        break;
    }

    switch (format)
    {
    case G2D_FORMAT_YUV420UVC_V1U1V0U0:
        /* code */
        p_g2d_ctx->dst_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size);
        if(NULL == p_g2d_ctx->dst_frm_info.p_vir_addr[0])
        {
            if(p_g2d_ctx->src_frm_info.p_vir_addr[0] != NULL)
            {
                g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
            }
            if(p_g2d_ctx->src_frm_info.p_vir_addr[1] != NULL)
            {
                g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[1]);
            }
            printf("malloc_dst_frm_y_mem_failed");
            return -1;
        }
        p_g2d_ctx->dst_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->dst_frm_info.p_vir_addr[0]);
        p_g2d_ctx->dst_frm_info.p_vir_addr[1] = (void *)g2d_allocMem(awalign_size /2);
        if(NULL == p_g2d_ctx->dst_frm_info.p_vir_addr[1])
        {
            if(p_g2d_ctx->src_frm_info.p_vir_addr[0] != NULL)
            {
                g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
            }
            if(p_g2d_ctx->src_frm_info.p_vir_addr[1] != NULL)
            {
                g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[1]);
            }
            if(p_g2d_ctx->dst_frm_info.p_vir_addr[0] != NULL)
            {
                g2d_freeMem(p_g2d_ctx->dst_frm_info.p_vir_addr[0]);
            }
            printf("malloc_dst_frm_y_mem_failed");
            return -1;
        }
        p_g2d_ctx->dst_frm_info.p_phy_addr[1] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->dst_frm_info.p_vir_addr[1]); 
        break;
    case G2D_FORMAT_ABGR8888:
    /* code */
        p_g2d_ctx->dst_frm_info.p_vir_addr[0] = (void *)g2d_allocMem(awalign_size*4);
        if(NULL == p_g2d_ctx->dst_frm_info.p_vir_addr[0])
        {
            if(p_g2d_ctx->src_frm_info.p_vir_addr[0] != NULL)
            {
                g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
            }
            printf("malloc_dst_frm_y_mem_failed");
            return -1;
        }
        p_g2d_ctx->dst_frm_info.p_phy_addr[0] = (void *)g2d_getPhyAddrByVirAddr(p_g2d_ctx->dst_frm_info.p_vir_addr[0]);  
    break;
    default:
        break;
    }    
}

static void convertRotate_by_g2d(char* yuv_in, G2D_CTX *p_g2d_ctx){
    int in_len = p_g2d_ctx->src_frm_info.frm_width * p_g2d_ctx->src_frm_info.frm_height;
    memcpy(p_g2d_ctx->src_frm_info.p_vir_addr[0],yuv_in,in_len);
    memcpy(p_g2d_ctx->src_frm_info.p_vir_addr[1],yuv_in+in_len,in_len/2);
    g2d_flushCache((void *)p_g2d_ctx->src_frm_info.p_vir_addr[0], in_len);
    g2d_flushCache((void *)p_g2d_ctx->src_frm_info.p_vir_addr[1], in_len/2);      
    G2dConvert_rotate(p_g2d_ctx, g_g2dfd, G2D_ROT_270);
    int out_len = p_g2d_ctx->dst_frm_info.frm_width * p_g2d_ctx->dst_frm_info.frm_height;
    g2d_flushCache((void *)p_g2d_ctx->dst_frm_info.p_vir_addr[0], out_len);
    g2d_flushCache((void *)p_g2d_ctx->dst_frm_info.p_vir_addr[1], out_len/2);
}
static int num = 0;
static void NV21ToArgb_by_g2d(G2D_CTX *p_g2d_ctx, FRM_INFO *addr){
    int in_len = p_g2d_ctx->src_frm_info.frm_width * p_g2d_ctx->src_frm_info.frm_height;
    memcpy(p_g2d_ctx->src_frm_info.p_vir_addr[0],addr->p_vir_addr[0],in_len);
    memcpy(p_g2d_ctx->src_frm_info.p_vir_addr[1],addr->p_vir_addr[1],in_len/2);
    g2d_flushCache((void *)p_g2d_ctx->src_frm_info.p_vir_addr[0], in_len);
    g2d_flushCache((void *)p_g2d_ctx->src_frm_info.p_vir_addr[1], in_len/2);      
    G2dConvert_formatconversion(p_g2d_ctx, g_g2dfd);
    int out_len = p_g2d_ctx->dst_frm_info.frm_width * p_g2d_ctx->dst_frm_info.frm_height;
    g2d_flushCache((void *)p_g2d_ctx->dst_frm_info.p_vir_addr[0], out_len*4);
    if(num == 10){
        FILE *fd = fopen("/mnt/extsd/userrgb.rgb","wb");
        if(fd != NULL){
            fwrite(p_g2d_ctx->dst_frm_info.p_vir_addr[0], out_len*4, 1, fd);
        }
        fclose(fd);
    }
    num++;
}

static void Scale_by_g2d(G2D_CTX *p_g2d_ctx, FRM_INFO *addr, g2d_fmt_enh format){
    int in_len = p_g2d_ctx->src_frm_info.frm_width * p_g2d_ctx->src_frm_info.frm_height;
    switch (format)
    {
    case G2D_FORMAT_YUV420UVC_V1U1V0U0:
        /* code */
        memcpy(p_g2d_ctx->src_frm_info.p_vir_addr[0],addr->p_vir_addr[0],in_len);
        memcpy(p_g2d_ctx->src_frm_info.p_vir_addr[1],addr->p_vir_addr[1],in_len/2);
        g2d_flushCache((void *)p_g2d_ctx->src_frm_info.p_vir_addr[0], in_len);
        g2d_flushCache((void *)p_g2d_ctx->src_frm_info.p_vir_addr[1], in_len/2);
        break;
    case G2D_FORMAT_ABGR8888:
        memcpy(p_g2d_ctx->src_frm_info.p_vir_addr[0],addr->p_vir_addr[0],in_len*4);
        g2d_flushCache((void *)p_g2d_ctx->src_frm_info.p_vir_addr[0], in_len*4);
        break;
    default:
        break;
    }
    G2dConvert_scale(p_g2d_ctx, g_g2dfd);
    int out_len = p_g2d_ctx->dst_frm_info.frm_width * p_g2d_ctx->dst_frm_info.frm_height;
    switch (format)
    {
    case G2D_FORMAT_YUV420UVC_V1U1V0U0:
        /* code */
        g2d_flushCache((void *)p_g2d_ctx->dst_frm_info.p_vir_addr[0], out_len);
        g2d_flushCache((void *)p_g2d_ctx->dst_frm_info.p_vir_addr[1], out_len/2);
        break;
    case G2D_FORMAT_ABGR8888:
        g2d_flushCache((void *)p_g2d_ctx->dst_frm_info.p_vir_addr[0], out_len*4);
        break;
    default:
        break;
    }
}

static void FreeFrmBuff(G2D_CTX *p_g2d_ctx){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    if(p_g2d_ctx->src_frm_info.p_vir_addr[0] != NULL){
        g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[0]);
    }
    if(p_g2d_ctx->src_frm_info.p_vir_addr[1] != NULL){
        g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[1]);
    }
    if(p_g2d_ctx->src_frm_info.p_vir_addr[2] != NULL){
        g2d_freeMem(p_g2d_ctx->src_frm_info.p_vir_addr[2]);
    }
    if(p_g2d_ctx->dst_frm_info.p_vir_addr[0] != NULL){
        g2d_freeMem(p_g2d_ctx->dst_frm_info.p_vir_addr[0]);
    }
    if(p_g2d_ctx->dst_frm_info.p_vir_addr[1] != NULL){
        g2d_freeMem(p_g2d_ctx->dst_frm_info.p_vir_addr[1]);
    }
    if(p_g2d_ctx->dst_frm_info.p_vir_addr[1] != NULL){
        g2d_freeMem(p_g2d_ctx->dst_frm_info.p_vir_addr[1]);
    }
}

void init_g2d_carplayStruct(){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    p_g2d_ctx_Rotate = (G2D_CTX *)malloc(sizeof(G2D_CTX));
    p_g2d_ctx_convFormat = (G2D_CTX *)malloc(sizeof(G2D_CTX));
    picture = (libhwclayerpara_t *)malloc(sizeof(libhwclayerpara_t));
    srcinfo = (struct src_info *)malloc(sizeof(struct src_info));
    prepareRotateFrmBuff(p_g2d_ctx_Rotate);
    prepareConvFormatFrmBuff(p_g2d_ctx_convFormat);
    p_g2d_ctx_scale = (G2D_CTX *)malloc(sizeof(G2D_CTX));
    prepareScaleFrmBuff(p_g2d_ctx_scale, G2D_FORMAT_YUV420UVC_V1U1V0U0);  


    picture->top_y = p_g2d_ctx_scale->dst_frm_info.p_phy_addr[0];
    picture->top_c = p_g2d_ctx_scale->dst_frm_info.p_phy_addr[1];
    srcinfo->w = p_g2d_ctx_scale->dst_frm_info.frm_width;
    srcinfo->h = p_g2d_ctx_scale->dst_frm_info.frm_height;
    srcinfo->crop_x = 0;
    srcinfo->crop_y = 0;
    
    srcinfo->crop_w = p_g2d_ctx_scale->dst_frm_info.frm_width;;
    srcinfo->crop_h = p_g2d_ctx_scale->dst_frm_info.frm_height;

    srcinfo->format = HWC_FORMAT_YUV420VUC;    //HWC_FORMAT_YUV420VUC HWC_FORMAT_RGBA_8888
    srcinfo->color_space = 0x101;    //disp_color_space    0x101 BT_709   0x104 BT_601
}


void destroy_g2d_carplayStruct(){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    FreeFrmBuff(p_g2d_ctx_Rotate);
    FreeFrmBuff(p_g2d_ctx_convFormat);
    FreeFrmBuff(p_g2d_ctx_scale);
    free(picture);
    free(srcinfo);
    free(p_g2d_ctx_Rotate);
    free(p_g2d_ctx_convFormat);
    free(p_g2d_ctx_scale);
}

int setCarPlay2De(){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    int ret = RET_OK;
    VIEW_INFO view;
    switch (g_sys_Data.splitScreenDisp)
    {
    case DISPLAY_NORMAL:
        view.x = 0;
        view.y = 0;
        view.w = CARPLAY_DST_HEIGHT;
        view.h = CARPLAY_DST_WIDTH;
        break;
    case DISPLAY_FRONT:
    case DISPLAY_FRONT_REAR:
    case DISPLAY_REAR:
        view.x = 0;
        view.y = 1440 - CARPLAY_SPLIT_DST_WIDTH;
        view.w = CARPLAY_SPLIT_DST_HEIGHT;
        view.h = CARPLAY_SPLIT_DST_WIDTH;
        break;    
    default:
        break;
    } 
    if(hlay >= 0){
        return -1;
    }
    hlay = HLAY(0, 0);
    ret = hw_display_init();
    if(ret != RET_OK){
        printf("hw_display_init fail!!!\n");
        return ret;
    }
    ret = hwd_layer_request_hlay(hlay);
    if(ret != RET_OK){
        printf("hwd_layer_request_hlay fail!!!\n");
        return ret;
    }
    // hwd_set_rot(HD2CHN(hlay),HD2LYL(hlay),0);
    ret = hwd_layer_set_src(hlay,srcinfo);
    if(ret != RET_OK){
        printf("hwd_layer_set_src fail!!!\n");
        return ret;
    }

    ret = hwd_layer_set_rect(hlay, &view);
    if(ret != RET_OK){
        printf("hwd_layer_set_rect fail!!!\n");
        return ret;
    }
    ret = hwd_layer_switch(hlay,1);
    if(ret != RET_OK){
        printf("hwd_layer_set_rect fail!!!\n");
        return ret;
    }
    // hwd_layer_set_zorder(hlay,ZORDER_MAX);
    ret = hwd_layer_render(hlay,picture);
    if(ret != RET_OK){
        printf("hwd_layer_set_rect fail!!!\n");
        return ret;
    }
    return ret; 
}

void freeCarPlay2De(){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    if(hlay < 0)
        return;
    hwd_layer_switch(hlay,0);
    hwd_layer_release(hlay);
    hlay = -1;
}
 

void* threadFun_lvgl_carplay(void* arg) {
    // 线程执行的代码
	printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    while(carplay_thread_exitflag){
        QueueData *qd = (QueueData *)queue_front(gUiQueue);
        if (qd != NULL){
            int ret = queue_pop(gUiQueue);
            if(ret == 0){
                    convertRotate_by_g2d(qd->data,p_g2d_ctx_Rotate); 
                    // NV21ToArgb_by_g2d(p_g2d_ctx_convFormat, &p_g2d_ctx_Rotate->dst_frm_info); 
                    Scale_by_g2d(p_g2d_ctx_scale, &p_g2d_ctx_Rotate->dst_frm_info,G2D_FORMAT_YUV420UVC_V1U1V0U0);
                free(qd);
                qd = NULL;
            }
        }else{
            usleep(100);
        }  
        int cont = queue_size(gUiQueue);
        if (cont > 16)
        {
	    cont -= 8;
            printf("too slow del %d frames\n", cont);
            while (cont)
            {
                qd = (QueueData *)queue_front(gUiQueue);
                int ret = queue_pop(gUiQueue);
                if(ret == 0){
			free(qd);
		}
                cont--;
            }
        }
    }
    g_sys_Data.linktype = LINK_TYPE_UNKOW;
    printf("###########Thread is exit!!!!!!!\n");
    return NULL;
}

int lvgl_carplay_service(){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    if(carplay_thread_exitflag != 1){
        carplay_thread_exitflag = 1;
        int ret = pthread_create(&carplay_threadID,NULL,threadFun_lvgl_carplay,NULL);
        if(ret != 0){
            printf("#####create lvgl carplay thread fail!!!!!\n");
            return -1;
        }
    }
	return 0;
}

void stop_lvgl_carplay_service(){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    carplay_thread_exitflag = 0;
    pthread_join(carplay_threadID, NULL);
    freeCarPlay2De();
    destroy_g2d_carplayStruct();
}

void autoLinkExitTheFace(int eventtype){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    g_sys_Data.eventtype = eventtype;
    lv_obj_t* current_screen = lv_scr_act();
    if(current_screen == guider_ui.screen_androidAuto || current_screen == guider_ui.screen_carPlay){
        freeCarPlay2De();
        switch(g_sys_Data.splitScreenDisp){
            case DISPLAY_NORMAL:
            break;
            case DISPLAY_FRONT_REAR:
                printf("---------------mode: DISPLAY_FRONT_REAR-------------------------\n");
                stopPreview(&g_sys_Data.vipp0_config); 
                stopPreview(&g_sys_Data.vipp8_config);
                break;                        
            case DISPLAY_FRONT:
                printf("---------------mode: DISPLAY_FRONT-------------------------\n");
                stopPreview(&g_sys_Data.vipp0_config);
                break;
            case DISPLAY_REAR:
                printf("---------------mode: DISPLAY_REAR-------------------------\n");
                stopPreview(&g_sys_Data.vipp8_config);               
            break;
            default:
            break;
        }
    }  
    if(g_sys_Data.linktype == LINK_TYPE_CARPLAY && current_screen == guider_ui.screen_carPlay){
		ui_load_scr_animation(&guider_ui, &guider_ui.screen, guider_ui.screen_del, &guider_ui.screen_carPlay_del, setup_scr_screen, LV_SCR_LOAD_ANIM_NONE, 200, 200, true, true);
    }else if(g_sys_Data.linktype == LINK_TYPE_ANDROIDAUTO || current_screen == guider_ui.screen_androidAuto){
		ui_load_scr_animation(&guider_ui,&guider_ui.screen,guider_ui.screen_del,&guider_ui.screen_androidAuto_del,setup_scr_screen,LV_SCR_LOAD_ANIM_NONE,200,200,true,true);
    }  
}

void CarLinkStart(LinkType type){
    printf("#######--------------%s---%d---------------------\n",__func__,__LINE__);
    lv_obj_t* current_screen = lv_scr_act();
    g_sys_Data.linktype = type;
    init_g2d_carplayStruct();
	if(type == LINK_TYPE_CARPLAY){
        if(current_screen != guider_ui.screen_carPlay)
            return;
        if(!lv_obj_has_flag(guider_ui.screen_carPlay_cont_prompt, LV_OBJ_FLAG_HIDDEN)){
			lv_obj_set_style_bg_img_src(guider_ui.screen_carPlay, NULL, LV_PART_MAIN|LV_STATE_DEFAULT);
			lv_obj_add_flag(guider_ui.screen_carPlay_btn_return, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(guider_ui.screen_carPlay_cont_prompt, LV_OBJ_FLAG_HIDDEN);
        }
	}else if(type == LINK_TYPE_ANDROIDAUTO){
        if(current_screen != guider_ui.screen_androidAuto)
            return;

            
        if(!lv_obj_has_flag(guider_ui.screen_androidAuto_cont_prompt, LV_OBJ_FLAG_HIDDEN)){
            lv_obj_set_style_bg_img_src(guider_ui.screen_androidAuto, NULL, LV_PART_MAIN|LV_STATE_DEFAULT);
 			lv_obj_add_flag(guider_ui.screen_androidAuto_btn_return, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(guider_ui.screen_androidAuto_cont_prompt, LV_OBJ_FLAG_HIDDEN);    
        }       
	}
    if(current_screen == guider_ui.screen_androidAuto || current_screen == guider_ui.screen_carPlay){
        setCarPlay2De();
        switch(g_sys_Data.splitScreenDisp){
            case DISPLAY_NORMAL:
            break;
            case DISPLAY_FRONT_REAR:
                printf("---------------mode: DISPLAY_FRONT_REAR-------------------------\n");
                setDstSizeAndDispPos(&g_sys_Data.vipp0_config,360,440,0,0);
                createVoChn(&g_sys_Data.vipp0_config, 0, 0);
                startPreview(&g_sys_Data.vipp0_config);  
                setDstSizeAndDispPos(&g_sys_Data.vipp8_config,360,440,360,0);
                createVoChn(&g_sys_Data.vipp8_config, 0, 0);
                startPreview(&g_sys_Data.vipp8_config);   
                break;                        
            case DISPLAY_FRONT:
                printf("---------------mode: DISPLAY_FRONT-------------------------\n");
                setDstSizeAndDispPos(&g_sys_Data.vipp0_config,720,440,0,0);
                createVoChn(&g_sys_Data.vipp0_config, 0, 0);
                startPreview(&g_sys_Data.vipp0_config);
                break;
            case DISPLAY_REAR:
                printf("---------------mode: DISPLAY_REAR-------------------------\n");
                setDstSizeAndDispPos(&g_sys_Data.vipp8_config,720,440,0,0);
                createVoChn(&g_sys_Data.vipp8_config, 0, 0);
                startPreview(&g_sys_Data.vipp8_config);                
            break;
            default:
            break;
        }
    }
}
