#ifndef __WIFI_UAP_H
#define __WIFI_UAP_H

#define __packed __attribute__((packed))

#define WIFI_ENCRYPTIONPROTOCOL_NORSN _BV(0)
#define WIFI_ENCRYPTIONPROTOCOL_WEPSTATIC _BV(1)
#define WIFI_ENCRYPTIONPROTOCOL_WEPDYNAMIC _BV(2)
#define WIFI_ENCRYPTIONPROTOCOL_WPA _BV(3)
#define WIFI_ENCRYPTIONPROTOCOL_WPANONE _BV(4)
#define WIFI_ENCRYPTIONPROTOCOL_WPA2 _BV(5)
#define WIFI_ENCRYPTIONPROTOCOL_CCKM _BV(6)
#define WIFI_ENCRYPTIONPROTOCOL_WAPI _BV(7)

#define WIFI_KEY_MGMT_EAP _BV(0)
#define WIFI_KEY_MGMT_PSK _BV(1)
#define WIFI_KEY_MGMT_NONE _BV(2)

#define WIFI_WPA_CIPHER_WEP40 _BV(0)
#define WIFI_WPA_CIPHER_WEP104 _BV(1)
#define WIFI_WPA_CIPHER_TKIP _BV(2)
#define WIFI_WPA_CIPHER_CCMP _BV(3)

#define WIFI_MICROAP_BROADCASTSSID _BV(0) // 是否允许热点被搜索到
#define WIFI_MICROAP_USEDEFAULTSSID _BV(1) // 忽略结构体中的ssid成员, 使用默认的热点名称

typedef struct
{
  MrvlIEHeader header;
  uint16_t key_mgmt; // Key management bitmap
  uint16_t operation;
}__packed MrvlIETypes_AKMP_t;

typedef struct
{
  MrvlIEHeader header;
  uint8_t bcast_ssid_ctl;
}__packed MrvlIETypes_ApBCast_SSID_Ctrl_t;

typedef struct
{
  MrvlIEHeader header;
  uint16_t protocol;
}__packed MrvlIETypes_EncryptionProtocol_t;

typedef struct
{
  MrvlIEHeader header;
  uint8_t groupwise_cipher;
  uint8_t reserved;
}__packed MrvlIETypes_GTK_Cipher_t;

typedef struct
{
  MrvlIEHeader header;
  uint8_t pkt_fwd_ctl;
}__packed MrvlIETypes_PKT_Fwd_Ctrl_t;

typedef struct
{
  MrvlIEHeader header;
  uint16_t protocol;
  uint8_t pairwise_cipher;
  uint8_t reserved;
}__packed MrvlIETypes_PTK_Cipher_t;

typedef struct
{
  MrvlIEHeader header;
  uint8_t mac_address[WIFI_MACADDR_LEN];
  uint8_t power_mgt_status;
  int8_t rssi;
}__packed MrvlIETypes_STA_Info_t;

typedef struct
{
  MrvlIEHeader header;
  uint8_t peer_mac_addr[WIFI_MACADDR_LEN];
  uint8_t tx_pause_state;
  uint8_t total_queued;
}__packed MrvlIETypes_Tx_Data_Pause_t;

typedef struct
{
  MrvlIEHeader header;
  uint8_t key_index;
  uint8_t is_default;
  uint8_t key[WIFI_MAX_WEPKEYLEN / 2];
}__packed MrvlIETypes_WEP_Key_t;

typedef WiFi_Connection WiFi_AccessPoint;

typedef struct
{
  WiFi_CommandHeader header;
  uint8_t sta_mac_address[WIFI_MACADDR_LEN];
  uint16_t reason;
}__packed WiFi_APCmd_STADeauth;

typedef struct
{
  WiFi_CommandHeader header;
  uint16_t action;
}__packed WiFi_APCmd_SysConfigure;

typedef struct
{
  WiFi_CommandHeader header;
  uint16_t sta_count;
  MrvlIETypes_STA_Info_t sta_list[1];
}__packed WiFi_APCmdResponse_STAList;

typedef struct
{
  WiFi_CommandHeader header;
  char sys_info[64];
}__packed WiFi_APCmdResponse_SysInfo;

void WiFi_DeauthenticateClient(bss_t bss, uint8_t mac_addr[WIFI_MACADDR_LEN], uint16_t reason, WiFi_Callback callback, void *arg);
void WiFi_ShowAPClientList(bss_t bss, WiFi_Callback callback, void *arg);
void WiFi_ShowAPSysInfo(bss_t bss, WiFi_Callback callback, void *arg);
void WiFi_StartMicroAP(const WiFi_AccessPoint *ap, uint32_t flags, WiFi_Callback callback, void *arg);
void WiFi_StopMicroAP(bss_t bss, WiFi_Callback callback, void *arg);

#endif
