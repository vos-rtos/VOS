#ifndef __WIFI_DATA_H
#define __WIFI_DATA_H

// 数据通道状态
#define WIFI_PACKETSTATUS_LOCKED _BV(0)

// WiFi模块接收的数据帧
typedef struct
{
  WiFi_SDIOFrameHeader header;
  uint8_t bss_type;
  uint8_t bss_num;
  uint16_t rx_packet_length; // Number of bytes in the payload
  uint16_t rx_packet_offset; // Offset from the start of the packet to the beginning of the payload data packet
  uint16_t rx_packet_type;
  uint16_t seq_num; // Sequence number
  uint8_t priority; // Specifies the user priority of received packet
  uint8_t rx_rate; // Rate at which this packet is received
  int8_t snr; // Signal to noise ratio for this packet (dB)
  uint8_t nf; // Noise floor for this packet (dBm). Noise floor is always negative. The absolute value is passed.
  uint8_t ht_info; // HT information
  uint8_t reserved1[3];
  uint8_t flags; // TDLS flags
  uint8_t reserved2[43];
  uint8_t payload[1]; // 数据链路层上的帧
}__packed WiFi_DataRx;

// WiFi模块发送的数据帧
typedef struct
{
  WiFi_SDIOFrameHeader header;
  uint8_t bss_type;
  uint8_t bss_num;
  uint16_t tx_packet_length; // Number of bytes in the payload data frame
  uint16_t tx_packet_offset; // Offset of the beginning of the payload data packet (802.3 or 802.11 frames) from the beginning of the packet (bytes)
  uint16_t tx_packet_type;
  uint32_t tx_control; // See 3.2.1 Per-Packet Settings
  uint8_t priority; // Specifies the user priority of transmit packet
  uint8_t flags;
  uint8_t pkt_delay_2ms; // Amount of time the packet has been queued in the driver layer for WMM implementations
  uint16_t reserved1;
  uint8_t tx_token_id;
  uint16_t reserved2;
  uint8_t payload[1]; // 数据链路层上的帧
}__packed WiFi_DataTx;

// 数据通道状态
typedef struct
{
  uint32_t busy;
  uint32_t start_time[WIFI_DATAPORTS_TX_NUM];
} WiFi_PacketStatus;

int WiFi_ForwardPacket(const WiFi_DataRx *packet, uint8_t ap_mac[WIFI_MACADDR_LEN]);
uint16_t WiFi_GetDataLength(port_t port);
WiFi_DataTx *WiFi_GetPacketBuffer(uint16_t size);
uint16_t WiFi_GetReadPortStatus(void);
uint16_t WiFi_GetWritePortStatus(void);
void WiFi_PacketProcess(WiFi_DataRx *packet, port_t port);
int WiFi_ReleasePacketBuffer(void);
port_t WiFi_SendPacket(const WiFi_DataTx *packet);
void WiFi_SetPacketHeader(WiFi_DataTx *packet, bss_t bss, uint16_t size);
void WiFi_UpdatePacketStatus(void);

#endif
