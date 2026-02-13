#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>   
#include <string.h>


#include "vencoder.h"
#include "vdecoder.h"
#include "memoryAdapter.h"


#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include "lvgl_main.h"


//#define	V851_VENC_EN    1
#define	USE_ALLWIN_Decoder_EN	    1




#ifdef __cplusplus
extern "C" {
#endif

#include "queue.h"
extern Queue *gUiQueue;

extern char g_str_name[32];
extern char g_str_pwd[32];

#ifdef __cplusplus
}
#endif




extern int g_bt_connected;
extern char phone_name[32];






static int gwidth = 1440;
static int gheight = 720;



#if USE_ALLWIN_Decoder_EN
VideoDecoder * pVideoDec = NULL;
VConfig         mVideoDecConf;
VideoStreamInfo mVideoDecStreamInfo;
#endif



#if V851_VENC_EN
static unsigned char* gCodecSpecificData = NULL;
static int gCodecSpecificDataLen = 0;
#endif

int init_video(unsigned char *h264spsbuf, int spsppsLen){
	
#if USE_ALLWIN_Decoder_EN

	if (h264spsbuf == NULL || spsppsLen == 0 )
		return -1;
	
	int nRet;

	printf("initVdecoder 0\n");
	pVideoDec = CreateVideoDecoder(); //创建解码器实例
	if (pVideoDec == NULL)
	{
		printf(" create video decoder failed");
		return -1;
	}

	printf("initVdecoder 1\n");

	memset(&mVideoDecStreamInfo, 0, sizeof(VideoStreamInfo));

        mVideoDecStreamInfo.eCodecFormat = VIDEO_CODEC_FORMAT_H264;  //pVideoStreamInfo
        mVideoDecStreamInfo.nFrameRate = 30;
        mVideoDecStreamInfo.nWidth = gwidth;
        mVideoDecStreamInfo.nHeight = gheight;
        mVideoDecStreamInfo.bIsFramePackage = 1;

	//填充sps pps
	mVideoDecStreamInfo.pCodecSpecificData = (char *)malloc(spsppsLen  );
	if (mVideoDecStreamInfo.pCodecSpecificData == NULL)
	{
		DestroyVideoDecoder(pVideoDec);
		printf("malloc for codec specific data failed");
		return -1;
	}
	memcpy(mVideoDecStreamInfo.pCodecSpecificData, h264spsbuf, spsppsLen );
	mVideoDecStreamInfo.nCodecSpecificDataLen = spsppsLen ;
	
	printf("initVdecoder 2\n");
	//设置spspps end
 	memset(&mVideoDecConf, 0, sizeof(VConfig));
              
        mVideoDecConf.bNoBFrames = 1;       
        mVideoDecConf.nFrameBufferNum = 1;
      
	mVideoDecConf.bGpuBufValid = 0;
	mVideoDecConf.nAlignStride = 16;
	mVideoDecConf.nDecodeSmoothFrameBufferNum = 3;
	mVideoDecConf.eOutputPixelFormat = PIXEL_FORMAT_NV21;
	mVideoDecConf.memops = MemAdapterGetOpsS();
	if (mVideoDecConf.memops == NULL)
	{
		DestroyVideoDecoder(pVideoDec);
		free(mVideoDecStreamInfo.pCodecSpecificData);
		mVideoDecStreamInfo.pCodecSpecificData = NULL;
		printf("MemAdapterGetOpsS failed\n");
		return -1;
	}
	printf("initVdecoder 3\n");
	CdcMemOpen(mVideoDecConf.memops);
	printf("initVdecoder 4\n");
	nRet = InitializeVideoDecoder(pVideoDec, &mVideoDecStreamInfo, &mVideoDecConf);
	if (nRet < 0)
	{
		DestroyVideoDecoder(pVideoDec);
		free(mVideoDecStreamInfo.pCodecSpecificData);
		mVideoDecStreamInfo.pCodecSpecificData = NULL;
		
		CdcMemClose(mVideoDecConf.memops);
		printf(" init video decoder failed");
		return -1;
	}
	printf("initVdecoder 5\n");

#endif

#if V851_VENC_EN
	if (h264spsbuf == NULL || spsppsLen == 0 ){
		printf("init_video null\n");
		return -1;
	}

	//填充sps pps
	gCodecSpecificData = (unsigned char *)malloc(spsppsLen);
	if (gCodecSpecificData == NULL)
	{
		printf("malloc for codec specific data failed\n");
		return -1;
	}
	memcpy(gCodecSpecificData, h264spsbuf, spsppsLen);
	gCodecSpecificDataLen = spsppsLen;

	//设置spspps end
	printf("initVdecoder 2\n");
	vdec_start(gwidth, gheight, 2048*1024, gwidth*gheight*3/2);
	printf("initVdecoder 3\n");
	vdec_SetSpsPpsHead(gCodecSpecificData, gCodecSpecificDataLen);
	printf("initVdecoder 4\n");
	vdec_send_data(gCodecSpecificData, gCodecSpecificDataLen, 1);
	printf("initVdecoder 5\n");
#endif	
	return 1;
}





/**
   * @Video frame recv callback
   * @param datas [in] h264 video frame datas
   * @param len[in] frame data len
   * @param idrFrame[in] is idr frame(has sps nal and pps nal)
   */
  //static int cur_time;
  //static int last_time;
 // static int fps_tol;
void sink_video_play(int type, void * datas, int len, int idrFrame)
{
    printf("#####%s %d type=%d  idrFrame=%d  len=%d\n", __FUNCTION__, __LINE__, type, idrFrame, len);


	if (idrFrame){
		init_video(datas, len);
	}
	else{

#ifdef USE_ALLWIN_Decoder_EN	
	
	int nRequireSize = len;
	char *pBuf = NULL;
	char *pRingBuf = NULL;
	int nBufSize = 0;
	int nRingBufSize = 0;
	int nStreamBufIndex = 0;
	int nRet;
#if 0
	VencInputBuffer  inputBuffer;
    VencOutputBuffer outputBuffer;
#endif
	do
	{
		//获取一个空闲的码流buffer
		nRet = RequestVideoStreamBuffer(pVideoDec, nRequireSize, &pBuf, &nBufSize,
						&pRingBuf, &nRingBufSize, nStreamBufIndex);
		// printf("reqeust stream buffer, ret = %d, nRequireSize = %d", nRet, nRequireSize);
		if (nRet < 0 || (nBufSize + nRingBufSize) < nRequireSize)
		{
			printf(" reqeust video stream buffer failed: nRet[%d], reqeustSize[%d], realSize[%d]",
			       nRet, nRequireSize, nBufSize + nRingBufSize);
		}
	} while (nRet < 0 || (nBufSize + nRingBufSize) < nRequireSize);

	//把h264码流coppy到空闲buffer
	memcpy(pBuf, datas, nBufSize);
	if (pRingBuf && nRingBufSize > 0)
		memcpy(pRingBuf, datas+ nBufSize, nRingBufSize);

	pBuf[0]=0x00;pBuf[1]=0x00;pBuf[2]=0x00;pBuf[3]=0x01;

#if WR_FILE_TEST
	printf("nRequireSize=%d nBufSize=%d nRingBufSize=%d\n",nRequireSize, nBufSize, nRingBufSize);
	if (g_h264_fp){
		fwrite(datas, 1, len, g_h264_fp);
	}
#endif

	VideoStreamDataInfo dataInfo;
	memset(&dataInfo, 0, sizeof(VideoStreamDataInfo));
	dataInfo.nPts = 0;
	dataInfo.pData = pBuf;
	dataInfo.nLength = nBufSize + nRingBufSize;
	dataInfo.nStreamIndex = 0;
	dataInfo.bIsFirstPart = 1;
	dataInfo.bIsLastPart = 1;
	dataInfo.bValid = 1;

	nRet = SubmitVideoStreamData(pVideoDec, &dataInfo, nStreamBufIndex);
	int bVideoStreamEosFlag = 0;
	VideoPicture *pVideoPic = NULL;
	//解码h264码流
	nRet = DecodeVideoStream(pVideoDec, bVideoStreamEosFlag, 0, 0, 0);
	// 4, request picture an return
	pVideoPic = RequestPicture(pVideoDec, nStreamBufIndex);
	//printf("pVideoPic = %p\n",pVideoPic);
	if (pVideoPic)
	{
		int nSaveLen = pVideoPic->nWidth * pVideoPic->nHeight * 3 / 2;
		CdcMemFlushCache(mVideoDecConf.memops, pVideoPic->pData0, nSaveLen);
		
		// printf("ePixelFormat= %d,line=%d right=%d,top=%d\n", pVideoPic->ePixelFormat, pVideoPic->nLineStride, pVideoPic->nRightOffset, pVideoPic->nTopOffset);
		// printf("nWidth= %d,nHeight=%d pVideoPic->pData0=%d\n", pVideoPic->nWidth, pVideoPic->nHeight, pVideoPic->pData0);
		// printf("ePixelFormat = %d\n",pVideoPic->ePixelFormat);

		// /*		
		int nSize=(pVideoPic->nWidth * pVideoPic->nHeight*3)/2;   //yuv
		QueueData* qd=(QueueData*)malloc(sizeof(QueueData)+nSize);
		if(qd){
			qd->type=0;//视频数据
			qd->size=nSize;
			qd->width = pVideoPic->nWidth;
			qd->height = pVideoPic->nHeight;
			memcpy(qd->data,pVideoPic->pData0,nSize);
			queue_push(gUiQueue,qd);
			// fps_tol++;
			// cur_time = the_tick_to_extern();
			// printf("------------push qd cur time:%d-----------\n",cur_time);
			// if(cur_time - last_time > 1000){
			// 	printf("#######-carplay--current time:%d----last time:%d--FPS: %d/s--------\n",cur_time,last_time,fps_tol);
			// 	last_time = cur_time;
			// 	fps_tol=0;
			// }
		}
		// MapVideoBufferToFamerbuffer(pVideoPic->pData0,pVideoPic->nWidth,pVideoPic->nHeight);
		ReturnPicture(pVideoDec, pVideoPic);
		pVideoPic = NULL;
	}


#endif



#if V851_VENC_EN

		int nSize = gwidth*gheight * 3 / 2;		
		unsigned char* qd=(unsigned char*)malloc(nSize);
		if(qd){
			vdec_send_data((unsigned char *)datas, len, 0);			
			if (nSize == vdec_get_Yuvdata((unsigned char *)qd)){ 
	//			qd->data[0]=0x00;qd->data[1]=0x00;qd->data[2]=0x00;qd->data[3]=0x01;
				
				printf("### ScreenStream data  len=%d\n", nSize);	
			}else{
				free(qd);
				printf("### ScreenStream data nSize=%d != qd->size=%d\n", nSize, qd->size);
			}
		}else{
			printf("### ScreenStream data malloc error. len=%d\n", nSize);
		}

	
#endif
	}
}

/**
   * @Video stop callback
   */
void sink_video_stop(int type)
{
    printf("#####%s %d\n", __FUNCTION__, __LINE__);
	
#if USE_ALLWIN_Decoder_EN
	
	DestroyVideoDecoder(pVideoDec);
	CdcMemClose(mVideoDecConf.memops);
	
	if (mVideoDecStreamInfo.pCodecSpecificData){
		free(mVideoDecStreamInfo.pCodecSpecificData);
		mVideoDecStreamInfo.pCodecSpecificData = NULL;
	}
#endif

#if V851_VENC_EN
	vdec_stop();

	if (gCodecSpecificData){
		free(gCodecSpecificData);
		gCodecSpecificData = NULL;
	}
#endif
}



char* bufbuf(char* pbSrc, int wSrcLen, char* pbSub, int wSubLen)
{
    char* pbSrcEnd = pbSrc + wSrcLen;	
    while (pbSrc + wSubLen <= pbSrcEnd)
    {
        if (memcmp(pbSrc, pbSub, wSubLen) == 0)
        {
            return pbSrc;
        }		
        pbSrc++;
    }
	return NULL;
}

int dd_read_sn(char *str_sn){	
	int fd = open("/var/info.txt", O_RDWR, S_IREAD | S_IWRITE);
	if (fd < 0) {
		system("cat /sys/class/sunxi_info/sys_info > /var/info.txt");
		fd = open("/var/info.txt", O_RDWR, S_IREAD | S_IWRITE);
		if (fd < 0) {
			return -1;
		}
	}
	char buf[1024];
	int num = read(fd, buf, sizeof(buf));
	close(fd);

	printf("dd_read_sn len=%d\r\n", num);
	
	char *p = bufbuf(buf, num, "sunxi_serial      : ", 20);
	if(p != NULL){
		p += (20);
		
		int Tmpfd = open("/tmp/uuid", O_CREAT|O_TRUNC|O_RDWR|O_NOFOLLOW, 0666);
	    if (Tmpfd < 0) {
			printf("open failed %s : %s", "uuid", strerror(errno));			
	    }
	    if (write(Tmpfd, p, 32) < 0) {
			printf("write failed %s: %s", "uuid", strerror(errno));		
	    }
	    close(Tmpfd);

		char *pnext = p + 4;
		pnext[0] = 0;
		memcpy(str_sn, p, 4);
		str_sn[4] = 0;
		printf("dd_read_sn sn=%s\r\n", str_sn);
		return 4;
	}
	return -1;
}

void wifi_init(char *strName){
	char hostap_conf[64] = "/var/hostapd.conf";

	int ret;
	do{
		ret = system("ifconfig wlan0 192.168.1.100 netmask 255.255.255.0 up");	
		usleep(10*1000);
	}while(ret != 0);
/*
	system("ip -6 route del fe80::/64 dev wlan0");
	system("echo 1 > /proc/sys/net/ipv6/conf/wlan0/disable_ipv6");
	system("echo 1 > /proc/sys/net/ipv6/conf/wlan0/disable_ipv6");
	usleep(10*1000);

	int ip1 = 192, ip2 = 168, ip3 = 0, ip4 = 1;
	char cmd[128];
	snprintf(cmd, sizeof(cmd), "ip rule add to %d.%d.%d.0/24 table wlan0 pref 100", ip1, ip2, ip3);	
	system(cmd);
	printf("%s\n", cmd);
	system("ip rule add to 192.168.1.0/24 table wlan0 pref 100");
	
	system("ip rule flush");
	system("ip rule add from 192.168.1.0/24 table 1 pref 100");
	system("ip rule add default via 192.168.1.100 dev wlan0 pref 101");
	system("ip route add table 1 default via 192.168.1.100 dev wlan0");

	system("ip route add default dev wlan0");
	system("route add -net 192.168.1.0 netmask 255.255.255.0 gw 192.168.1.1 dev wlan0");
	system("route");
*/	 	
	//printf("wifi ok\n");
	//usleep(10*1000*1000);

	int wifi_chl = 149;//36;
	char confbuf[640];

	sprintf(confbuf, "interface=wlan0\nctrl_interface=/var/run/hostapd\ndriver=nl80211\nssid=%s\nchannel=%d\nhw_mode=a\nieee80211n=1\nieee80211ac=1\nmacaddr_acl=0\nwmm_enabled=1\nauth_algs=1\nht_capab=[HT40+]\nvht_oper_chwidth=0\nignore_broadcast_ssid=0\ninterworking=1\naccess_network_type=3\ninternet=0\nasra=0\nesr=0\nuesa=0\nvenue_group=10\nvenue_type=1\nvendor_elements=DD0800A0400000020023\nwpa=2\nwpa_passphrase=%s\nwpa_key_mgmt=WPA-PSK\nwpa_pairwise=CCMP\nrsn_pairwise=CCMP\ndtim_period=1\n",   strName, wifi_chl, g_str_pwd);


    int fd = open(hostap_conf, O_CREAT|O_TRUNC|O_RDWR|O_NOFOLLOW, 0666);
    if (fd < 0) {
        printf("open failed %s : %s", hostap_conf, strerror(errno));
		return ;
    }
    if (write(fd, confbuf, strlen(confbuf)) < 0) {
        printf("write failed %s: %s", hostap_conf, strerror(errno));
		close(fd);
		return ;
    }
    close(fd);
	system("hostapd -B /var/hostapd.conf &");
	
	system("mdnsd");
	system("udhcpd");
	//system("dnsmasq");
	
	printf("wifi ap ok\n");

	//system("busybox udhcpd");
	//system("/opt/work/app/carplay/lib/shttpd -root /opt/work/app/carplay/lib/html -ports 80 &");	

}


char g_keySN[32] = {0};



void init_wifi_name(void){
	
#if 1
	
	if (g_keySN[0] == 0){		
		int length = dd_read_sn(g_keySN);
		if(length == 4){
			strcat(g_str_name, g_keySN);
			printf("str_name=%s\r\n", g_str_name);
		}else{
			strcat(g_str_name,"8866");
		}
	}else{
		strcat(g_str_name, g_keySN);
	}
	
#endif
	
	wifi_init(g_str_name);
	
	return NULL;
}


