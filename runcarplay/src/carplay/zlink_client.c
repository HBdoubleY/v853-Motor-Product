/**
 * Zlink client: init, register callbacks, feed H264 to carplay_display.
 * Run zlink_client_run() in a dedicated thread.
 */
#ifdef ENABLE_CARPLAY

#include "zlink_client.h"
#include "carplay_display.h"
#include "libzlink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define LINK_TYPE_CARPLAY       2
#define LINK_TYPE_ANDROIDAUTO   3

/* Minimal view of g_sys_Data (defined in lvgl) - first member is linktype */
extern struct { int linktype; } g_sys_Data;

static LIBZLINK_HANDLE g_handle;

#define PREBUF_PACKET_CAP  24
#define PREBUF_PACKET_MAX  (256 * 1024)

typedef struct {
	char *data;
	int len;
} prebuf_packet_t;

static struct {
	prebuf_packet_t slot[PREBUF_PACKET_CAP];
	int count;
	int active;
	int pending_home_link_type;
	pthread_mutex_t mutex;
} g_video_state = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
};

static int packet_has_sps(const char *data, int len)
{
	const unsigned char *d = (const unsigned char *)data;
	for (int i = 0; i + 4 < len; i++) {
		if (d[i] == 0 && d[i + 1] == 0 &&
		    (d[i + 2] == 1 || (d[i + 2] == 0 && d[i + 3] == 1))) {
			int off = (d[i + 2] == 1) ? i + 3 : i + 4;
			if (off < len && (d[off] & 0x1F) == 7)
				return 1;
		}
	}
	return 0;
}

static void prebuf_clear_locked(void)
{
	for (int i = 0; i < g_video_state.count; i++) {
		free(g_video_state.slot[i].data);
		g_video_state.slot[i].data = NULL;
		g_video_state.slot[i].len = 0;
	}
	g_video_state.count = 0;
}

static void prebuf_clear(void)
{
	pthread_mutex_lock(&g_video_state.mutex);
	prebuf_clear_locked();
	pthread_mutex_unlock(&g_video_state.mutex);
}

static void prebuf_push_locked(const char *data, int len)
{
	if (len <= 0 || len > PREBUF_PACKET_MAX)
		return;

	if (packet_has_sps(data, len))
		prebuf_clear_locked();

	if (g_video_state.count >= PREBUF_PACKET_CAP) {
		free(g_video_state.slot[0].data);
		memmove(&g_video_state.slot[0], &g_video_state.slot[1],
		        sizeof(g_video_state.slot[0]) * (PREBUF_PACKET_CAP - 1));
		g_video_state.count = PREBUF_PACKET_CAP - 1;
	}

	char *copy = (char *)malloc((size_t)len);
	if (!copy)
		return;
	memcpy(copy, data, (size_t)len);
	g_video_state.slot[g_video_state.count].data = copy;
	g_video_state.slot[g_video_state.count].len = len;
	g_video_state.count++;
}

static int link_type_from_phone_type(enum PHONE_TYPE phone_type)
{
	switch (phone_type) {
	case CARPLAY_WIRED:
	case CARPLAY_WIRELESS:
		return LINK_TYPE_CARPLAY;
	case AA_WIRED:
	case AA_WIRELESS:
		return LINK_TYPE_ANDROIDAUTO;
	default:
		return 0;
	}
}

static void session_init(void)
{
	struct SESSION_DATA *session_data = (struct SESSION_DATA *)malloc(sizeof(struct SESSION_DATA));
	if (!session_data)
		return;
	memset(session_data, 0, sizeof(struct SESSION_DATA));
	session_data->width = 1440;		// （要和后面解码显示的 session 宽高一致）。
	session_data->height = 720;
	session_data->width_margin = 0;
	session_data->height_margin = 0;
	session_data->fps = 30;
	session_data->density = 120;
	session_data->is_right_hand = 0;
	session_data->is_night_mode = 0;
	session_data->apple_wired = APPLE_WIRED_LINK_NONE;
	session_data->apple_wireless = CARPLAY_WIRELESS_MODE;
	session_data->android_wired = ANDROID_WIRED_LINK_NONE;
	session_data->android_wireless = AA_WIRELESS_MODE;
	session_data->cp_icon_path = "/opt/work/app/carplay/icon/icon_104_104.png";
	session_data->is_use_phone_audio = 0;
	session_data->platform_id = "zlink";
	session_data->vendor_name = "zlink-test";
	session_data->is_force_usb_host = 0;
	session_data->mfi_bus_num = -1;
	session_data->otg_bus_num = -1;

	libzlink_init_session_2(session_data);
	free(session_data);
}

static int video_data_cb(char *data, int len, struct VIDEO_SCREEN_INFO *info, void *user_data)
{
	(void)info;
	(void)user_data;
	if (data && len > 0) {
		pthread_mutex_lock(&g_video_state.mutex);
		prebuf_push_locked(data, len);
		int active = g_video_state.active;
		pthread_mutex_unlock(&g_video_state.mutex);
		if (active)
			carplay_display_feed_h264(data, len);
	}
	return 0;
}

static int session_state_cb(enum LIBZLINK_SESSION_STATE session_state, enum PHONE_TYPE phone_type, void *user_data)
{
	(void)user_data;
	if (session_state == SESSION_WAIT_INIT) {
		session_init();
	} else if (session_state == SESSION_STARTED) {
		int link_type = link_type_from_phone_type(phone_type);
		int active;
		if (link_type)
			g_sys_Data.linktype = link_type;
		pthread_mutex_lock(&g_video_state.mutex);
		active = g_video_state.active;
		pthread_mutex_unlock(&g_video_state.mutex);
		if (active)
			libzlink_video_focus(0);
		else
			libzlink_video_focus(1);
	}
	return 0;
}

static int request_wifi_info_cb(void *user_data)
{
	(void)user_data;
	/* Use default AP info; app can override via config. */
	/* 与开发板 hostapd.conf/udhcpd.conf 一致：AP 为 192.168.1.2，channel 36 */
	libzlink_wifi_info2("carplay_037b", "88888888", "192.168.1.2", 36, "wlan0");
	return 0;
}

static int main_audio_start_cb(enum ZLINK_MEDIA_TYPE media_type, void *user_data) { (void)media_type; (void)user_data; return 0; }
static int main_audio_data_cb(char *data, int len, enum ZLINK_MEDIA_TYPE media_type, int sample, int channels, int bits, void *user_data) { (void)data; (void)len; (void)media_type; (void)sample; (void)channels; (void)bits; (void)user_data; return 0; }
static int main_audio_stop_cb(enum ZLINK_MEDIA_TYPE media_type, void *user_data) { (void)media_type; (void)user_data; return 0; }
// static int video_focus_cb(int is_hu_focus_on, void *user_data) { (void)is_hu_focus_on; (void)user_data; return 0; }
static int audio_focus_cb(int is_hu_focus_on, void *user_data) { (void)is_hu_focus_on; (void)user_data; return 0; }
static int request_p2p_cb(void *user_data) { (void)user_data; return 0; }
static int mic_start_cb(enum ZLINK_MEDIA_TYPE media_type, int sample, int channels, int bits, void *user_data) { (void)media_type; (void)sample; (void)channels; (void)bits; (void)user_data; return 0; }
static int mic_stop_cb(enum ZLINK_MEDIA_TYPE media_type, void *user_data) { (void)media_type; (void)user_data; return 0; }

static int video_focus_cb(int is_hu_focus_on, void *user_data)
{
	(void)user_data;

	if (is_hu_focus_on) {
		printf("video_focus_request: request back to HU HMI\n");
		pthread_mutex_lock(&g_video_state.mutex);
		g_video_state.active = 0;
		g_video_state.pending_home_link_type = g_sys_Data.linktype;
		pthread_mutex_unlock(&g_video_state.mutex);
		carplay_display_destroy();
	} else {
		printf("video_focus_request: request back to phone HMI\n");
	}

	return 0;
}

static void register_callbacks(void)
{
	libzlink_session_state_cb_init(session_state_cb);
	libzlink_video_data_cb_init(video_data_cb);
	libzlink_main_audio_start_cb_init(main_audio_start_cb);
	libzlink_main_data_cb_init(main_audio_data_cb);
	libzlink_main_audio_stop_cb_init(main_audio_stop_cb);
	libzlink_video_focus_request_cb_init(video_focus_cb);
	libzlink_audio_focus_request_cb_init(audio_focus_cb);
	libzlink_request_p2p_start_cb_init(request_p2p_cb);
	libzlink_request_wifi_info_cb_init(request_wifi_info_cb);
	libzlink_mic_start_cb_init(mic_start_cb);
	libzlink_mic_stop_cb_init(mic_stop_cb);
}

void zlink_client_run(void)
{
	register_callbacks();
	g_handle = libzlink_init(NULL);
	if (!g_handle) {
		return;
	}
	while (1) {
		if (libzlink_check_ready(g_handle) == 0)
			break;
		sleep(1);
	}
	libzlink_request_state();
	while (1)
		sleep(60);
}

void zlink_client_set_video_active(int active)
{
	pthread_mutex_lock(&g_video_state.mutex);
	if (active) {
		g_video_state.active = 1;
		for (int i = 0; i < g_video_state.count; i++)
			carplay_display_feed_h264(g_video_state.slot[i].data, g_video_state.slot[i].len);
	} else {
		g_video_state.active = 0;
		prebuf_clear_locked();
	}
	pthread_mutex_unlock(&g_video_state.mutex);
}

int zlink_client_request_video_focus(int is_hu_focus_on)
{
	return libzlink_video_focus(is_hu_focus_on ? 1 : 0);
}

int zlink_client_take_pending_home_request(void)
{
	int link_type;

	pthread_mutex_lock(&g_video_state.mutex);
	link_type = g_video_state.pending_home_link_type;
	g_video_state.pending_home_link_type = 0;
	pthread_mutex_unlock(&g_video_state.mutex);

	return link_type;
}

void zlink_client_reset_video_prebuffer(void)
{
	prebuf_clear();
}

#endif /* ENABLE_CARPLAY */
