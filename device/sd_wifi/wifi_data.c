#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

__attribute__((aligned)) static uint8_t wifi_buffer_packet[1792]; // 数据帧发送缓冲区
static WiFi_PacketStatus wifi_packet_status; // 数据帧发送通道的使用情况

/* AP数据包二层转发 (AP路由功能) */
// 注意: lwip自带的IP_FORWARD和LWIP_IPV6_FORWARD功能属于三层转发
// 返回-1表示转发失败, 返回0表示未转发, 返回正数表示已转发
int WiFi_ForwardPacket(const WiFi_DataRx *packet, uint8_t ap_mac[WIFI_MACADDR_LEN])
{
  bss_t bss;
  port_t port;
  WiFi_DataTx *forward;
  
  if (packet->bss_type == WIFI_BSSTYPE_UAP && packet->rx_packet_length >= 14)
  {
    if (memcmp(packet->payload, ap_mac, WIFI_MACADDR_LEN) != 0)
    {
      forward = WiFi_GetPacketBuffer(packet->rx_packet_length);
      if (forward == NULL)
        return -1;
      
      bss = WIFI_BSS(packet->bss_type, packet->bss_num);
      WiFi_SetPacketHeader(forward, bss, packet->rx_packet_length);
      memcpy(forward->payload, packet->payload, packet->rx_packet_length);
      port = WiFi_SendPacket(forward);
      WiFi_ReleasePacketBuffer();
      return port;
    }
  }
  return 0;
}

/* 获取需要接收的数据大小 */
// 当port=0时, 获取待接收的命令回应帧或事件帧的大小
// 当port=1~15时, 获取待接收的数据帧的大小, port为数据通道号
uint16_t WiFi_GetDataLength(port_t port)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0L + 2 * port) | (WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0U + 2 * port) << 8);
}

/* 获取数据帧发送缓冲区 */
// size为待发送数据帧的大小 (WiFi_DataTx.payload部分)
// size=0表示大小未确定, 不检查缓冲区大小
WiFi_DataTx *WiFi_GetPacketBuffer(uint16_t size)
{
  WiFi_DataTx *data = (WiFi_DataTx *)wifi_buffer_packet;
  
  if (wifi_packet_status.busy & WIFI_PACKETSTATUS_LOCKED)
    return NULL;
  else if ((sizeof(WiFi_DataTx) - sizeof(data->payload)) + size > sizeof(wifi_buffer_packet))
    return NULL; // 设置size参数的目的是为了对缓冲区大小进行初步检验, 防止不可逆转的缓冲区溢出事件的发生
  
  wifi_packet_status.busy |= WIFI_PACKETSTATUS_LOCKED; // 锁定缓冲区
  return data;
}

/* 获取接收通道的状态 */
uint16_t WiFi_GetReadPortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPU) << 8);
}

/* 获取发送通道的状态 */
uint16_t WiFi_GetWritePortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPU) << 8);
}

/* 处理收到的数据帧 */
void WiFi_PacketProcess(WiFi_DataRx *packet, port_t port)
{
  bss_t bss = WIFI_BSS(packet->bss_type, packet->bss_num);
  
  if (packet->rx_packet_type == 2)
  {
    // 收到以太网数据帧 (Ethernet version 2 frame)
    WiFi_PacketHandler(packet, port);
  }
  else
  {
    // 收到其他类型的数据帧
    printf("[Recv] type=%d, bss=0x%02x\n", packet->rx_packet_type, bss);
    WiFi_LowLevel_Dump(packet, packet->header.length); // 显示内容
  }
}

/* 释放数据帧发送缓冲区 */
int WiFi_ReleasePacketBuffer(void)
{
  if (wifi_packet_status.busy & WIFI_PACKETSTATUS_LOCKED)
  {
    wifi_packet_status.busy &= ~WIFI_PACKETSTATUS_LOCKED;
    return 0;
  }
  else
    return -1;
}

/* 发送数据帧 */
// 返回值表示发送数据所用的通道号
// 发送完毕后请及时释放发送缓冲区
port_t WiFi_SendPacket(const WiFi_DataTx *packet)
{
  static port_t next_port = 1;
  int i, ret;
  port_t port;
  uint32_t addr;
  
  if (packet == (const WiFi_DataTx *)wifi_buffer_packet && packet->header.length > sizeof(wifi_buffer_packet))
  {
    printf("%s: Packet buffer overflowed! size=%d\n", __FUNCTION__, packet->header.length);
    abort();
  }
  
  // 等待数据通道可用
  port = next_port;
  while ((WiFi_GetWritePortStatus() & _BV(port)) == 0);
  if (wifi_packet_status.busy & _BV(port))
  {
    wifi_packet_status.busy &= ~_BV(port);
#if WIFI_DISPLAY_RESPTIME
    printf("-- Packet port %d released at %dms\n", port, WiFi_LowLevel_GetTicks() - wifi_packet_status.start_time[port - 1]);
#endif
  }
  
  // 发送数据帧
  i = 0;
  addr = WiFi_GetPortAddress(port);
  do
  {
    ret = WiFi_LowLevel_WriteData(1, addr, packet, packet->header.length, 0, 0);
    if (ret == -2)
      return -1; // 不再重试
    i++;
  } while (ret == -1 && i < WIFI_LOWLEVEL_MAXRETRY);
  
  // 计算下一个通道号
  // 数据通道号必须连续使用, 不能跨号使用, 否则将会导致数据帧发送失败
  // 例如, 使用了通道4后, 下次必须使用通道5, 不能使用通道6
  if (next_port == WIFI_DATAPORTS_TX_NUM)
    next_port = 1;
  else
    next_port++;
  
  // 接下来需要等待Download Ready位置1, 也就是等待数据通道释放
  wifi_packet_status.busy |= _BV(port);
  wifi_packet_status.start_time[port - 1] = WiFi_LowLevel_GetTicks();
  
  if (ret == 0)
    return port;
  else
    return -1;
}

/* 设置数据帧头部字段 */
void WiFi_SetPacketHeader(WiFi_DataTx *packet, bss_t bss, uint16_t size)
{
  // 有关发送数据包的细节, 请参考Firmware Specification PDF的Chapter 3: Data Path
  packet->header.length = sizeof(WiFi_DataTx) - sizeof(packet->payload) + size;
  packet->header.type = WIFI_SDIOFRAME_DATA;
  
  packet->bss_type = WIFI_BSSTYPE(bss);
  packet->bss_num = WIFI_BSSNUM(bss);
  packet->tx_packet_length = size;
  packet->tx_packet_offset = sizeof(WiFi_DataTx) - sizeof(packet->payload) - sizeof(packet->header); // 不包括SDIOFrameHeader
  packet->tx_packet_type = 0; // normal 802.3
  packet->tx_control = 0;
  packet->priority = 0;
  packet->flags = 0;
  packet->pkt_delay_2ms = 0;
  packet->reserved1 = 0;
  packet->tx_token_id = 0;
  packet->reserved2 = 0;
}

/* 更新数据通道的使用状态 */
void WiFi_UpdatePacketStatus(void)
{
  port_t port;
  uint16_t bitmap;
  
  bitmap = WiFi_GetWritePortStatus();
  for (port = 1; port <= WIFI_DATAPORTS_TX_NUM; port++) // 在88W8801中, 命令通道释放时不会产生中断, 所以不需要管通道0
  {
    if ((wifi_packet_status.busy & _BV(port)) && (bitmap & _BV(port)))
    {
      // 通道现在为空闲, 表明该通道已释放
      wifi_packet_status.busy &= ~_BV(port);
#if WIFI_DISPLAY_RESPTIME
      printf("Packet port %d released at %dms\n", port, WiFi_LowLevel_GetTicks() - wifi_packet_status.start_time[port - 1]);
#endif
    }
  }
}
