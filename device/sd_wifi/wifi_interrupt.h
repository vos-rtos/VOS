#ifndef __WIFI_INTERRUPT_H
#define __WIFI_INTERRUPT_H
#define __packed __attribute__((packed))
// WiFi事件列表
typedef enum
{
  EVENT_BEACON_LOSS = 0x0003, // Beacon loss/link loss
  EVENT_DEAUTHENTICATE = 0x0008, // Generated when the firmware receives a 802.11 deauthenticate management frame from the AP
  EVENT_DISASSOCIATE = 0x0009, // Generated when the firmware receives a 802.11 disassociate management frame from the AP
  EVENT_WMM_STATUS_CHANGE = 0x0017, // Generated when the firmware monitors a change in the WMM state.
                                    // This is a result of an AP change in the WMM Parameter IE or a change in the
                                    // operational state of one of the AC queues
  EVENT_IBSS_COALESCED = 0x001e, // Generated when the IBSS Coalescing process is finished and the BSSID has changed
  EVENT_ADHOC_STATION_CONNECT = 0x0020, // Generated when a station is ad-hoc connected
  EVENT_ADHOC_STATION_DISCONNECT = 0x0021, // Generated when a station is ad-hoc disconnected

  EVENT_PORT_RELEASE = 0x002b, // Triggered when controlled port are released
  EVENT_MICRO_AP_EV_ID_STA_DEAUTH = 0x002c, // Generated when a client station leaves the BSS
  EVENT_FORWARD_MGMT_FRAME = 0x002d, // Generated when a management frame is forwarded
  EVENT_MICRO_AP_EV_ID_STA_ASSOC = 0x002d, // Generated when a client station joins in the BSS
  EVENT_MICRO_AP_EV_ID_BSS_START = 0x002e, // Generated when BSS is started
  EVENT_DOT11N_ADDBA = 0x0033, // Generated when ADDBA request action frame is received
  EVENT_DOT11N_DELBA = 0x0034, // Generated when DELBA request action frame is received
  EVENT_HEART_BEAT_EVENT = 0x0035,
  EVENT_BA_STREAM_TIMEOUT = 0x0037, // Block Ack inactivity timeout
  EVENT_AMSDU_AGGR_CTRL = 0x0042, // Reports aggregation size available for host
  EVENT_MICRO_AP_EV_ID_BSS_IDLE = 0x0043, // Generated when BSS is idle
  EVENT_MICRO_AP_EV_ID_BSS_ACTIVE = 0x0044, // Generated when BSS is active
  EVENT_EVT_WEP_ICV_ERR = 0x0046, // Triggered by WEP decryption error
  EVENT_HS_ACT_REQ = 0x0047, // For enhanced power management
  EVENT_DOT11N_BWCHANGE_EVENT = 0x0048, // Reports bandwidth change
  EVENT_MICRO_AP_EV_ID_MIC_COUNTERMEASURES = 0x004c, // Generated when firmware starts or stops MIC countermeasure
  EVENT_MEF_HOST_WAKEUP = 0x004f, // Triggered to wakeup host when MEF condition matched
  EVENT_CHANNEL_SWITCH_ANNOUNCEMENT = 0x0050, // Reports channel switch
  EVENT_MICRO_AP_EV_ID_RSN_CONNECT = 0x0051, // Generated when STA successfully completed 4-way handshake
  EVENT_RADAR_DETECTED = 0x0053, // Generated if master mode radar detection mode is enabled in firmware
                                 // and if firmware detects radar on the current operating channel or firmware
                                 // receives a measurement report/autonomous measurement report which indicate
                                 // that radar is detected on the current operating channel
  EVENT_CHANNEL_REPORT_RDY = 0x0054, // Generated when channel report measurement is completed
  EVENT_TX_DATA_PAUSE = 0x0055, // Sent when a uAP or GO client in PS exceeds the allowed PS buffering, 
                                // or when the client has buffering available
  EVENT_SCAN_REPORT_BY_EVENT = 0x0058, // Used to scan results for CMD_802_11_SCAN_EXT
  EVENT_EV_RXBA_SYNC = 0x0059, // Triggered by firmware to synchronize Rx BA window bitmap
  EVENT_REMAIN_ON_CHANNEL_EXPIRED = 0x005c // Informs host about CMD_802_11_REMAIN_ON_CHANNEL command expired
} WiFi_EventList;

// WiFi模块事件帧
typedef struct
{
  WiFi_SDIOFrameHeader header;
  uint16_t event_id; // Enumerated identifier for the event
  uint8_t bss_num;
  uint8_t bss_type;
} __packed WiFi_EventHeader;

// 大部分事件都是这个格式
typedef struct
{
  WiFi_EventHeader header;
  uint16_t reason_code; // IEEE Reason Code as described in the 802.11 standard 
  uint8_t mac_address[WIFI_MACADDR_LEN]; // Peer STA Address
}__packed WiFi_ExtendedEventHeader;

void WiFi_CheckTimeout(void);
const WiFi_DataRx *WiFi_GetReceivedPacket(void);
int WiFi_Input(void);
int WiFi_IsConnectionEstablishedEvent(uint16_t event_id);
int WiFi_IsConnectionLostEvent(uint16_t event_id);
int WiFi_IsExtendedEventFormat(uint16_t event_id);
int WiFi_Wait(uint8_t status, uint32_t timeout);

/* 外部自定义回调函数 */
void WiFi_EventHandler(bss_t bss, const WiFi_EventHeader *event);
void WiFi_PacketHandler(const WiFi_DataRx *data, port_t port);

#endif
