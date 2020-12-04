#ifndef __WIFI_STA_H
#define __WIFI_STA_H

// Capability information
#define WIFI_CAPABILITY_ESS _BV(0)
#define WIFI_CAPABILITY_IBSS _BV(1)
#define WIFI_CAPABILITY_CF_POLLABLE _BV(2)
#define WIFI_CAPABILITY_CF_POLL_REQUEST _BV(3)
#define WIFI_CAPABILITY_PRIVACY _BV(4)
#define WIFI_CAPABILITY_SHORT_PREAMBLE _BV(5)
#define WIFI_CAPABILITY_PBCC _BV(6)
#define WIFI_CAPABILITY_CHANNEL_AGILITY _BV(7)
#define WIFI_CAPABILITY_SPECTRUM_MGMT _BV(8)
#define WIFI_CAPABILITY_SHORT_SLOT _BV(10)
#define WIFI_CAPABILITY_DSSS_OFDM _BV(13)

#define WIFI_KEYINFO_DEFAULTKEY _BV(3)
#define WIFI_KEYINFO_KEYENABLED _BV(2)
#define WIFI_KEYINFO_UNICASTKEY _BV(1)
#define WIFI_KEYINFO_MULTICASTKEY _BV(0)

#define WIFI_MACCTRL_RX _BV(0)
#define WIFI_MACCTRL_TX _BV(1) // 此位必须要置1才能发送数据！！！
#define WIFI_MACCTRL_LOOPBACK _BV(2)
#define WIFI_MACCTRL_WEP _BV(3)
#define WIFI_MACCTRL_ETHERNET2 _BV(4)
#define WIFI_MACCTRL_PROMISCUOUS _BV(7)
#define WIFI_MACCTRL_ALLMULTICAST _BV(8)
#define WIFI_MACCTRL_ENFORCEPROTECTION _BV(10) // strict protection
#define WIFI_MACCTRL_ADHOCGPROTECTIONMODE _BV(13) // 802.11g protection mode

#define WIFI_MCASTFILTER_COUNT 32
#define WIFI_MAX_SSIDLEN 32
#define WIFI_MAX_WEPKEYLEN 26
#define WIFI_MAX_WPAKEYLEN 64

#define BSS_STA WIFI_BSS(WIFI_BSSTYPE_STA, 0)
#define BSS_UAP WIFI_BSS(WIFI_BSSTYPE_UAP, 0)

// Authentication Type to be used to authenticate with AP
typedef enum
{
  WIFI_AUTH_MODE_OPEN = 0x00,
  WIFI_AUTH_MODE_SHARED = 0x01,
  WIFI_AUTH_MODE_NETWORK_EAP = 0x80
} WiFi_AuthenticationType;

// BSS type
typedef enum
{
  WIFI_BSS_INFRASTRUCTURE = 0x01,
  WIFI_BSS_INDEPENDENT = 0x02,
  WIFI_BSS_ANY = 0x03,
  
  WIFI_BSSTYPE_STA = 0,
  WIFI_BSSTYPE_UAP = 1
} WiFi_BSSType;

// 断开热点连接的原因
// https://docu.units.it/dokuwiki/_export/xhtml/tabelle:wifi_deauth_reason
// https://mentor.ieee.org/802.11/dcn/14/11-14-1358-04-000m-lb202-mah-assigned-comments.docx
typedef enum
{
  UNSPECIFIED_REASON = 1, // Unspecified reason
  INVALID_AUTHENTICATION = 2, // Previous authentication no longer valid
  LEAVING_NETWORK_DEAUTH = 3, // Deauthenticated because sending STA is leaving (or has left) IBSS or ESS
  REASON_INACTIVITY = 4, // Disassociated due to inactivity
  NO_MORE_STAS = 5, // Disassociated because AP is unable to handle all currently associated STAs
  INVALID_CLASS2_FRAME = 6, // Class 2 frame received from nonauthenticated STA
  INVALID_CLASS3_FRAME = 7, // Class 3 frame received from nonassociated STA
  LEAVING_NETWORK_DISASSOC = 8, // Disassociated because sending STA is leaving (or has left) BSS
  NOT_AUTHENTICATED = 9, // STA requesting (re)association is not authenticated with responding STA
  UNACCEPTABLE_POWER_CAPABILITY = 10, // Disassociated because the information in the Power Capability element is -unacceptable
  UNACCEPTABLE_SUPPORTED_CHANNELS = 11, // Disassociated because the information in the Supported Channels element is -unacceptable
  BSS_TRANSITION_DISASSOC = 12, // Disassociated due to BSS (#1202)transition (Ed)management
  REASON_INVALID_ELEMENT = 13, // Invalid element, i.e., an element defined in this standard for which the content does not meet the specifications in Clause 8 (Frame formats)
  MIC_FAILURE = 14, // Message integrity code (MIC) failure
  _4WAY_HANDSHAKE_TIMEOUT = 15, // 4-Way Handshake timeout
  GK_HANDSHAKE_TIMEOUT = 16, // Group Key Handshake timeout
  HANDSHAKE_ELEMENT_MISMATCH = 17, // (#1509)Element in 4-Way Handshake different from (Re)Association Request/Probe Response/Beacon frame
  REASON_INVALID_GROUP_CIPHER = 18, // Invalid group cipher
  REASON_INVALID_PAIRWISE_CIPHER = 19, // Invalid pairwise cipher
  REASON_INVALID_AKMP = 20, // Invalid AKMP
  UNSUPPORTED_RSNE_VERSION = 21, // Unsupported RSNE version
  INVALID_RSNE_CAPABILITIES = 22, // Invalid RSNE capabilities
  _802_1_X_AUTH_FAILED = 23, // IEEE Std(#130) 802.1X authentication failed
  REASON_CIPHER_OUT_OF_POLICY = 24, // Cipher suite rejected because of the security policy
  TDLS_PEER_UNREACHABLE = 25, // TDLS direct-link teardown due to TDLS peer STA unreachable via the TDLS direct link
  TDLS_UNSPECIFIED_REASON = 26, // TDLS direct-link teardown for unspecified reason
  SSP_REQUESTED_DISASSOC = 27, // Disassociated because session terminated by SSP request
  NO_SSP_ROAMING_AGREEMENT = 28, // Disassociated because of lack of SSP roaming agreement
  BAD_CIPHER_OR_AKM = 29, // Requested service rejected because of SSP cipher suite or AKM requirement
  NOT_AUTHORIZED_THIS_LOCATION = 30, // Requested service not authorized in this location
  SERVICE_CHANGE_PRECLUDES_TS = 31, // TS deleted because QoS AP lacks sufficient bandwidth for this QoS STA due to a change in BSS service characteristics or operational mode (e.g., an HT BSS change from 40 MHz channel to 20 MHz channel)
  UNSPECIFIED_QOS_REASON = 32, // Disassociated for unspecified, QoS-related reason
  NOT_ENOUGH_BANDWIDTH = 33, // Disassociated because QoS AP lacks sufficient bandwidth for this QoS STA
  MISSING_ACKS = 34, // Disassociated because excessive number of frames need to be acknowledged, but are not acknowledged due to AP transmissions and/or poor channel conditions
  EXCEEDED_TXOP = 35, // Disassociated because STA is transmitting outside the limits of its TXOPs
  STA_LEAVING = 36, // Requesting STA(#1258) is leaving the BSS (or resetting)
  END_TS_BA_DLS = 37, // Requesting STA is no longer using the stream or session(#1259)
  UNKNOWN_TS_BA = 38, // Requesting STA received frames using a mechanism for which a setup has not been completed(#1260)
  REASON_TIMEOUT = 39, // Requested from peer STA due to timeout
  PEERKEY_MISMATCH = 45, // Peer STA does not support the requested cipher suite
  PEER_INITIATED = 46, // In a DLS Teardown frame: The teardown was initiated by the DLS peer
                       // In a Disassociation frame: Disassociated because authorized access limit reached
  AP_INITIATED = 47, // In a DLS Teardown frame: The teardown was initiated by the AP
                     // In a Disassociation frame: Disassociated due to external service requirements
  REASON_INVALID_FT_ACTION_FRAME_COUNT = 48, // Invalid FT Action frame count
  REASON_INVALID_PMKID = 49, // Invalid pairwise master key identifier (PMKID)
  REASON_INVALID_MDE = 50, // Invalid MDE
  REASON_INVALID_FTE = 51 // Invalid FTE
} WiFi_DeauthReason;

// WiFi密钥类型
typedef enum
{
  WIFI_KEYTYPE_WEP = 0,
  WIFI_KEYTYPE_TKIP = 1,
  WIFI_KEYTYPE_AES = 2
} WiFi_KeyType;

typedef enum
{
  WIFI_MCASTFILTERSTATE_SYNCHRONIZED, // 和固件已保持同步
  WIFI_MCASTFILTERSTATE_MODIFIED, // 已修改, 需要和固件同步
  WIFI_MCASTFILTERSTATE_SUBMITTED // 已将改动的内容提交到固件, 等待回应
} WiFi_MulticastFilterState;

// 无线认证类型
typedef enum
{
  WIFI_SECURITYTYPE_NONE = 0,
  WIFI_SECURITYTYPE_WEP = 1,
  WIFI_SECURITYTYPE_WPA = 2,
  WIFI_SECURITYTYPE_WPA2 = 3
} WiFi_SecurityType;

typedef enum
{
  WIFI_STASTATE_IDLE,
  WIFI_STASTATE_CONNECTING,
  WIFI_STASTATE_CONNECTED
} WiFi_STAState;

// WEP密钥长度
typedef enum
{
  WIFI_WEPKEYTYPE_40BIT = 1,
  WIFI_WEPKEYTYPE_104BIT = 2
} WiFi_WEPKeyType;

/* TLV (Tag Length Value) of IEEE IE Type Format */
typedef __packed struct
{
  IEEEHeader header;
  uint8_t channel;
} IEEETypes_DsParamSet_t;

typedef __packed struct
{
  IEEEHeader header;
  uint16_t atim_window;
} IEEETypes_IbssParamSet_t;

/* TLV (Tag Length Value) of MrvllEType Format */
typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t auth_type;
} MrvlIETypes_AuthType_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t mac_address[1][WIFI_MACADDR_LEN];
} MrvlIETypes_BSSIDList_t;

typedef __packed struct
{
  MrvlIEHeader header;
  __packed struct
  {
    uint8_t band_config_type;
    uint8_t chan_number;
  } entries[1];
} MrvlIETypes_ChanBandList_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t count;
  uint8_t period;
  uint16_t max_duration;
  uint16_t duration_remaining;
} MrvlIETypes_CfParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  __packed struct
  {
    uint8_t band_config_type;
    uint8_t chan_number;
    uint8_t scan_type;
    uint16_t min_scan_time;
    uint16_t max_scan_time;
  } channels[1];
} MrvlIETypes_ChanListParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t key_type_id;
  uint16_t key_info;
  uint16_t key_len;
  uint8_t key[32];
} MrvlIETypes_KeyParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t mac_address[WIFI_MACADDR_LEN];
  uint8_t key_idx;
  uint8_t key_type;
  uint16_t key_info;
} MrvlIETypes_KeyParamSet_v2_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t passphrase[64];
} MrvlIETypes_Passphrase_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t channel;
} MrvlIETypes_PhyParamDSSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t rates[14];
} MrvlIETypes_RatesParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t rsn[64];
} MrvlIETypes_RsnParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t ssid[WIFI_MAX_SSIDLEN];
} MrvlIETypes_SSIDParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t mac_address[WIFI_MACADDR_LEN];
} MrvlIETypes_StaMacAddr_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint64_t tsf_table[1];
} MrvlIETypes_TsfTimestamp_t;

// 整个结构体的最大大小为256字节
typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t vendor[64]; // 通常情况下64字节已足够
} MrvlIETypes_VendorParamSet_t;

typedef MrvlIETypes_Passphrase_t MrvlIETypes_WPA_Passphrase_t;

/* 命令使用的数据结构 */
typedef struct
{
  uint8_t index; // 0~3
  char keys[4][WIFI_MAX_WEPKEYLEN + 1];
} WiFi_WEPKey;

typedef __packed struct
{
  uint16_t ie_length; // Total information element length (不含sizeof(ie_length))
  uint8_t bssid[WIFI_MACADDR_LEN]; // BSSID
  uint8_t rssi; // RSSI value as received from peer
  
  // Probe Response/Beacon Payload
  uint64_t pkt_time_stamp; // Timestamp
  uint16_t bcn_interval; // Beacon interval
  uint16_t cap_info; // Capabilities information
  IEEEType ie_parameters; // 存放的是一些IEEE类型的数据
} WiFi_BssDescSet;

typedef struct
{
  bss_t bss;
  WiFi_AuthenticationType auth_type;
  WiFi_SecurityType sec_type;
  char ssid[WIFI_MAX_SSIDLEN + 1];
  union
  {
    WiFi_WEPKey wep;
    char wpa[65];
  } password;
} WiFi_Connection;

typedef __packed struct
{
  uint8_t wep_key_index;
  uint8_t is_default_tx_key;
  uint8_t wep_key[13];
} WiFi_KeyParam_WEP_v1;

typedef __packed struct
{
  uint16_t key_len;
  uint8_t wep_key[13];
} WiFi_KeyParam_WEP_v2;

typedef struct
{
  uint8_t list[WIFI_MCASTFILTER_COUNT][WIFI_MACADDR_LEN];
  uint8_t num;
  WiFi_MulticastFilterState state;
} WiFi_MulticastFilter;

// WiFi热点信息
typedef struct
{
  MrvlIETypes_SSIDParamSet_t ssid;
  uint8_t mac_addr[WIFI_MACADDR_LEN];
  uint16_t cap_info;
  uint16_t bcn_period;
  uint8_t channel;
  MrvlIETypes_RatesParamSet_t rates;
  MrvlIETypes_RsnParamSet_t rsn;
  MrvlIETypes_VendorParamSet_t wpa;
  MrvlIETypes_VendorParamSet_t wwm;
  MrvlIETypes_VendorParamSet_t wps;
} WiFi_SSIDInfo;

typedef struct
{
  bss_t bss;
  WiFi_STAState state;
  WiFi_SSIDInfo info;
} WiFi_STAInfo;

typedef __packed struct
{
  uint32_t pkt_init_cnt;
  uint32_t pkt_success_cnt;
  uint32_t tx_attempts;
  uint32_t retry_failure;
  uint32_t expiry_failure;
} WiFi_TxPktStatEntry;

typedef __packed struct
{
  uint8_t oui[3];
  uint8_t oui_type;
  uint16_t version;
  uint8_t multicast_oui[4];
  uint16_t unicast_num;
  uint8_t unicast_oui[1][4]; // 这里假定unicast_num=1
  uint16_t auth_num; // 只有当unicast_num=1时, 该成员才会在这个位置上
  uint8_t auth_oui[1][4];
} WiFi_Vendor;

/* 参数 */
typedef struct
{
  WiFi_ArgBase base;
  WiFi_Connection conn;
  int8_t retry;
} WiFi_Arg_Associate;

typedef struct
{
  WiFi_ArgBase base;
  uint8_t *addr;
} WiFi_Arg_MACAddr;

typedef struct
{
  WiFi_ArgBase base;
  WiFi_SSIDInfo *info;
} WiFi_Arg_ScanSSID;

typedef struct
{
  WiFi_ArgBase base;
  int8_t del_index;
  int8_t deleted;
} WiFi_Arg_SetWEP;

/* 命令和命令回应 */
typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t peer_sta_addr[WIFI_MACADDR_LEN];
  uint16_t reason_code; // Reason code defined in IEEE 802.11 specification section 7.3.1.7 to indicate de-authentication reason
} WiFi_Cmd_Deauthenticate;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  MrvlIETypes_KeyParamSet_t keys;
} WiFi_Cmd_KeyMaterial;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint8_t mac_addr[WIFI_MACADDR_LEN];
} WiFi_Cmd_MACAddr;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint16_t reserved;
} WiFi_Cmd_MACCtrl;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint16_t num_of_adrs;
  uint8_t mac_list[WIFI_MCASTFILTER_COUNT][WIFI_MACADDR_LEN];
} WiFi_Cmd_MulticastAdr;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint16_t tx_key_index; // Key set being used for transmit (0~3)
  uint8_t wep_types[4]; // use 40 or 104 bits
  uint8_t keys[4][16];
} WiFi_Cmd_SetWEP;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint16_t cache_result;
} WiFi_Cmd_SupplicantPMK;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t peer_sta_addr[WIFI_MACADDR_LEN]; // Peer MAC address
  uint16_t cap_info; // Capability information
  uint16_t listen_interval; // Listen interval
  uint16_t bcn_period; // Beacon period
  uint8_t dtim_period; // DTIM period
} WiFi_CmdRequest_Associate;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t bss_type;
  uint8_t bss_id[WIFI_MACADDR_LEN];
} WiFi_CmdRequest_Scan;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t capability;
  uint16_t status_code;
  uint16_t association_id;
  IEEEType ie_buffer;
} WiFi_CmdResponse_Associate;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t buf_size;
  uint8_t num_of_set;
} WiFi_CmdResponse_Scan;

void WiFi_Associate(const WiFi_Connection *conn, int8_t max_retry, WiFi_Callback callback, void *arg);
int WiFi_CheckWEPKey(uint8_t *dest, const char *src);
void WiFi_ConnectionLost(void);
void WiFi_Deauthenticate(bss_t bss, uint16_t reason, WiFi_Callback callback, void *arg);
int WiFi_GetBSSID(bss_t bss, uint8_t mac_addr[WIFI_MACADDR_LEN]);
WiFi_SecurityType WiFi_GetSecurityType(const WiFi_SSIDInfo *info);
WiFi_STAState WiFi_GetSTAState(void);
int WiFi_JoinMulticastGroup(uint8_t addr[WIFI_MACADDR_LEN]);
int WiFi_LeaveMulticastGroup(uint8_t addr[WIFI_MACADDR_LEN]);
void WiFi_MACAddr(bss_t bss, uint8_t addr[WIFI_MACADDR_LEN], WiFi_CommandAction action, WiFi_Callback callback, void *arg);
void WiFi_MACControl(bss_t bss, uint16_t action, WiFi_Callback callback, void *arg);
void WiFi_PrintRates(uint16_t rates);
void WiFi_PrintTxPacketStatus(bss_t bss);
void WiFi_Scan(WiFi_Callback callback, void *arg);
void WiFi_ScanSSID(bss_t bss, const char *ssid, WiFi_SSIDInfo *info, WiFi_Callback callback, void *arg);
void WiFi_SetWEP(bss_t bss, const WiFi_WEPKey *key, WiFi_Callback callback, void *arg);
void WiFi_SetWPA(bss_t bss, const char *ssid, const uint8_t bssid[WIFI_MACADDR_LEN], const char *password, WiFi_Callback callback, void *arg);
void WiFi_ShowKeyMaterials(bss_t bss);
void WiFi_SynchronizeMulticastFilter(bss_t bss);

#endif
