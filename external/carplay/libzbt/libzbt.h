#ifndef ZBT_H
#define ZBT_H

typedef void* ZBT_HANDLE;

#define ENABLE_TYPE_NONE			0b0000	// 0x00
#define ENABLE_TYPE_CARPLAY			0b0001	//0x01
#define ENABLE_TYPE_AA				0b0010	//0x02
#define ENABLE_TYPE_ANDROID_CARLIFE		0b0100	//0x04
#define ENABLE_TYPE_ANDROID_HICAR		0b1000	//0x08
#define ENABLE_TYPE_ANDROID_CARLINK		0b10000	//0x10

#define ENABLE_TYPE_CP_AA			(ENABLE_TYPE_CARPLAY | ENABLE_TYPE_AA)	//0x03
#define ENABLE_TYPE_CP_ANDROIDCARLIFE		(ENABLE_TYPE_CARPLAY | ENABLE_TYPE_ANDROID_CARLIFE)	//0x05
#define ENABLE_TYPE_CP_HICAR		(ENABLE_TYPE_CARPLAY | ENABLE_TYPE_ANDROID_HICAR)	//0x09

#define ENABLE_TYPE_RECONN  		0x0020	//32
#define ENABLE_TYPE_CLEAN_LIST  	0x0040 // 64
#define ENABLE_TYPE_DEINIT_BT  		0x0060 // 96

//recv rfcomm data from zlink
typedef void ( *rfcomm_data_recv_f ) (unsigned char *data, int len, void *user_data);

//zlink request a2dp enable or disable,   is_a2dp_enable = ( 1 :enable,  0:disable)
typedef void ( *request_a2dp_enable_f ) (int is_a2dp_enable, void *user_data);

//zlink request bluetooth enable/disable, enable_type(ENABLE_TYPE_CP_AA / ENABLE_TYPE_CP_ANDROIDCARLIFE/ENABLE_TYPE_CARPLAY/.....)
typedef void ( *request_bt_enable_f ) (int is_bt_enable, int enable_type, void *user_data);

//zlink report phone link state , link_type(ENABLE_TYPE_CARPLAY / ENABLE_TYPE_AA/ENABLE_TYPE_ANDROID_HICAR/.....)
typedef void ( *phone_link_state_f ) (int is_linked, int link_type, void *user_data);

//zlink request link phone bt by phone_bt_mac
typedef void ( *request_link_phone_f ) (int is_request_link, char *phone_bt_mac, void *user_data);

//zlink request hicar ble start
typedef void ( *request_hicar_ble_start_f ) (char* serviceUuid, char* readUuid, char* writeUuid, int mininterval_ms, int maxinterval_ms, int timeout_s, 
												char *advdata, int advLen);
//zlink request hicar ble stop
typedef void ( *request_hicar_ble_stop_f ) ();
//zlink request hicar service register,  "session_uuid" is in param, "out_rfcomm_port" and "out_local_bt_mac" are out param
typedef void ( *hicar_service_register_f ) (char *session_uuid, 
													int *out_rfcomm_port, char *out_local_bt_mac);
typedef void ( *hicar_service_handle_f ) (char *session_uuid, int is_connect);

//recv hicar rfcomm data from zlink
typedef void ( *hicar_service_data_recv_f ) (char *session_uuid, char *data, int len);

//recv rfcomm data from zj
typedef void ( *zj_rfcomm_data_recv_f ) (unsigned char *data, int len, void *user_data);

enum ZBT_PHONE_TYPE
{
    ZBT_CARPLAY = 0,
    ZBT_ANDROID_AUTO = 1,
    ZBT_ANDROID_CARLIFE = 2,
    ZBT_ANDROID_HICAR = 3,
    ZBT_ANDROID_CARLINK = 4,
};

ZBT_HANDLE libzbt_init(char *vendor_id, void *user_data);
int libzbt_local_bt_info(ZBT_HANDLE handle, char *local_bt_name, char *local_bt_mac);
void libzbt_deinit(ZBT_HANDLE handle);

//main function: 
//local_mac_address: AABBCCDDEE . HU BT MAC
//local_bt_name: HU BT name
int libzbt_phonelink_state2(ZBT_HANDLE handle, enum ZBT_PHONE_TYPE phone_type, int is_connect, 
    															int is_pair, char *local_mac_address, char *local_bt_name, char *phone_bt_mac_addr, char *phone_bt_name);
int libzbt_phonelink_state(ZBT_HANDLE handle, enum ZBT_PHONE_TYPE phone_type, int is_connect, int is_pair, char *local_mac_address);

int libzbt_phone_hfp_link_state(ZBT_HANDLE handle, enum ZBT_PHONE_TYPE phone_type, int is_hfp_connect, 
    															int is_pair, char *local_mac_address);

//send rfcomm data to zlink, data from phone
int libzbt_rfcomm_data_send(ZBT_HANDLE handle, unsigned char *data, int data_len);

//send bt pair key and state  pair_key:Bluetooth pairing PIN code  is_already_pair:Send 0 when initiating pairing, and 1 when pairing is successful
int libzbt_bt_pair_info_send(ZBT_HANDLE handle, unsigned char *pair_key, int is_already_pair);


/***************************CALLBACK*****************************/

//call back, zlink request BT a2dp enable or disable
void libzbt_request_a2dp_enable_CB_init(ZBT_HANDLE handle, request_a2dp_enable_f fun);

//call back, zlink request BT enable or disable
void libzbt_request_bt_enable_CB_init(ZBT_HANDLE handle, request_bt_enable_f fun);

//call back, zlink report phone linked or disconnect
void libzbt_phone_link_state_CB_init(ZBT_HANDLE handle, phone_link_state_f fun);

//call back, recv rfcomm data from zlink, need send to phone
void libzbt_rfcomm_data_recv_CB_init(ZBT_HANDLE handle, rfcomm_data_recv_f fun);


void libzbt_zj_rfcomm_data_recv_CB_init(ZBT_HANDLE handle, zj_rfcomm_data_recv_f fun);

//call back, zlink request to link phone bt
void libzbt_request_link_phone_CB_init(ZBT_HANDLE handle, request_link_phone_f fun);

//only for hicar
//hicar ble init param
void libzbt_hicar_ble_start_CB_init(ZBT_HANDLE handle, request_hicar_ble_start_f fun);
void libzbt_hicar_ble_stop_CB_init(ZBT_HANDLE handle, request_hicar_ble_stop_f fun);
//hicar request service(UUID) register
void libzbt_hicar_service_register_CB_init(ZBT_HANDLE handle, hicar_service_register_f fun);
void libzbt_hicar_service_handle_CB_init(ZBT_HANDLE handle, hicar_service_handle_f fun);

//call back, recv hicar rfcomm data from zlink, need send to phone
void libzbt_hicar_service_data_recv_CB_init(ZBT_HANDLE handle, hicar_service_data_recv_f fun);

//send hicar rfcomm session state to zlink, when phone connected or disconnected
int libzbt_hicar_rfcomm_state_send(ZBT_HANDLE handle, char *session_uuid, int is_session_linked, char *phone_bt_mac_addr);
//send hicar rfcomm data to zlink, data from phone
int libzbt_hicar_rfcomm_data_send(ZBT_HANDLE handle, char *session_uuid, char *data, int len);


int libzbt_zj_rfcomm_data_send(ZBT_HANDLE handle, char *session_uuid, char *data, int len);

//send phone time date  data to zlink, data from phone
int libzbt_phone_time_data_send(ZBT_HANDLE handle, char *data, int data_len);

//only for carlink
//BLE广播data
typedef struct {
	char *uuid;			//data UUID
	char data[32];			//数据buffer
	int 	data_len;			//数据真实长度
} zj_StBleData;

//BLE广播信息
typedef struct {
	char *advertising_service_uuid;
	zj_StBleData advertising_data;
	zj_StBleData scan_reponse_data;
	char *gatt_service_uuid;
	char *gatt_write_uuid;
	char *gatt_notify_uuid;
} zj_StBleAdvertisingInfo;

typedef void (*StartAdvertise_f)(zj_StBleAdvertisingInfo *advertising_info);
typedef void (*StopAdvertise_f)();
typedef void (*BleGattSend_f)(char *data, int len);

int libzbt_carlink_StartAdvertise_CB_init(ZBT_HANDLE handle, StartAdvertise_f fun);
int libzbt_carlink_StopAdvertise_CB_init(ZBT_HANDLE handle, StopAdvertise_f fun);
//call back, recv HU carlink data from zlink, need send to phone
int libzbt_carlink_BleGattSend_CB_init(ZBT_HANDLE handle, BleGattSend_f fun);

//send carlink data to zlink, data from phone
int libzbt_carlink_BleGattReceived(ZBT_HANDLE handle, char *data, int len);
#endif

