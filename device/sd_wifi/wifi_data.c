#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

__attribute__((aligned)) static uint8_t wifi_buffer_packet[1792]; // ����֡���ͻ�����
static WiFi_PacketStatus wifi_packet_status; // ����֡����ͨ����ʹ�����

/* AP���ݰ�����ת�� (AP·�ɹ���) */
// ע��: lwip�Դ���IP_FORWARD��LWIP_IPV6_FORWARD������������ת��
// ����-1��ʾת��ʧ��, ����0��ʾδת��, ����������ʾ��ת��
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

/* ��ȡ��Ҫ���յ����ݴ�С */
// ��port=0ʱ, ��ȡ�����յ������Ӧ֡���¼�֡�Ĵ�С
// ��port=1~15ʱ, ��ȡ�����յ�����֡�Ĵ�С, portΪ����ͨ����
uint16_t WiFi_GetDataLength(port_t port)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0L + 2 * port) | (WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0U + 2 * port) << 8);
}

/* ��ȡ����֡���ͻ����� */
// sizeΪ����������֡�Ĵ�С (WiFi_DataTx.payload����)
// size=0��ʾ��Сδȷ��, ����黺������С
WiFi_DataTx *WiFi_GetPacketBuffer(uint16_t size)
{
  WiFi_DataTx *data = (WiFi_DataTx *)wifi_buffer_packet;
  
  if (wifi_packet_status.busy & WIFI_PACKETSTATUS_LOCKED)
    return NULL;
  else if ((sizeof(WiFi_DataTx) - sizeof(data->payload)) + size > sizeof(wifi_buffer_packet))
    return NULL; // ����size������Ŀ����Ϊ�˶Ի�������С���г�������, ��ֹ������ת�Ļ���������¼��ķ���
  
  wifi_packet_status.busy |= WIFI_PACKETSTATUS_LOCKED; // ����������
  return data;
}

/* ��ȡ����ͨ����״̬ */
uint16_t WiFi_GetReadPortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPU) << 8);
}

/* ��ȡ����ͨ����״̬ */
uint16_t WiFi_GetWritePortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPU) << 8);
}

/* �����յ�������֡ */
void WiFi_PacketProcess(WiFi_DataRx *packet, port_t port)
{
  bss_t bss = WIFI_BSS(packet->bss_type, packet->bss_num);
  
  if (packet->rx_packet_type == 2)
  {
    // �յ���̫������֡ (Ethernet version 2 frame)
    WiFi_PacketHandler(packet, port);
  }
  else
  {
    // �յ��������͵�����֡
    printf("[Recv] type=%d, bss=0x%02x\n", packet->rx_packet_type, bss);
    WiFi_LowLevel_Dump(packet, packet->header.length); // ��ʾ����
  }
}

/* �ͷ�����֡���ͻ����� */
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

/* ��������֡ */
// ����ֵ��ʾ�����������õ�ͨ����
// ������Ϻ��뼰ʱ�ͷŷ��ͻ�����
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
  
  // �ȴ�����ͨ������
  port = next_port;
  while ((WiFi_GetWritePortStatus() & _BV(port)) == 0);
  if (wifi_packet_status.busy & _BV(port))
  {
    wifi_packet_status.busy &= ~_BV(port);
#if WIFI_DISPLAY_RESPTIME
    printf("-- Packet port %d released at %dms\n", port, WiFi_LowLevel_GetTicks() - wifi_packet_status.start_time[port - 1]);
#endif
  }
  
  // ��������֡
  i = 0;
  addr = WiFi_GetPortAddress(port);
  do
  {
    ret = WiFi_LowLevel_WriteData(1, addr, packet, packet->header.length, 0, 0);
    if (ret == -2)
      return -1; // ��������
    i++;
  } while (ret == -1 && i < WIFI_LOWLEVEL_MAXRETRY);
  
  // ������һ��ͨ����
  // ����ͨ���ű�������ʹ��, ���ܿ��ʹ��, ���򽫻ᵼ������֡����ʧ��
  // ����, ʹ����ͨ��4��, �´α���ʹ��ͨ��5, ����ʹ��ͨ��6
  if (next_port == WIFI_DATAPORTS_TX_NUM)
    next_port = 1;
  else
    next_port++;
  
  // ��������Ҫ�ȴ�Download Readyλ��1, Ҳ���ǵȴ�����ͨ���ͷ�
  wifi_packet_status.busy |= _BV(port);
  wifi_packet_status.start_time[port - 1] = WiFi_LowLevel_GetTicks();
  
  if (ret == 0)
    return port;
  else
    return -1;
}

/* ��������֡ͷ���ֶ� */
void WiFi_SetPacketHeader(WiFi_DataTx *packet, bss_t bss, uint16_t size)
{
  // �йط������ݰ���ϸ��, ��ο�Firmware Specification PDF��Chapter 3: Data Path
  packet->header.length = sizeof(WiFi_DataTx) - sizeof(packet->payload) + size;
  packet->header.type = WIFI_SDIOFRAME_DATA;
  
  packet->bss_type = WIFI_BSSTYPE(bss);
  packet->bss_num = WIFI_BSSNUM(bss);
  packet->tx_packet_length = size;
  packet->tx_packet_offset = sizeof(WiFi_DataTx) - sizeof(packet->payload) - sizeof(packet->header); // ������SDIOFrameHeader
  packet->tx_packet_type = 0; // normal 802.3
  packet->tx_control = 0;
  packet->priority = 0;
  packet->flags = 0;
  packet->pkt_delay_2ms = 0;
  packet->reserved1 = 0;
  packet->tx_token_id = 0;
  packet->reserved2 = 0;
}

/* ��������ͨ����ʹ��״̬ */
void WiFi_UpdatePacketStatus(void)
{
  port_t port;
  uint16_t bitmap;
  
  bitmap = WiFi_GetWritePortStatus();
  for (port = 1; port <= WIFI_DATAPORTS_TX_NUM; port++) // ��88W8801��, ����ͨ���ͷ�ʱ��������ж�, ���Բ���Ҫ��ͨ��0
  {
    if ((wifi_packet_status.busy & _BV(port)) && (bitmap & _BV(port)))
    {
      // ͨ������Ϊ����, ������ͨ�����ͷ�
      wifi_packet_status.busy &= ~_BV(port);
#if WIFI_DISPLAY_RESPTIME
      printf("Packet port %d released at %dms\n", port, WiFi_LowLevel_GetTicks() - wifi_packet_status.start_time[port - 1]);
#endif
    }
  }
}
