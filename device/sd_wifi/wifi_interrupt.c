#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wifi.h"

static void WiFi_EventProcess(WiFi_EventHeader *event);

__attribute__((aligned)) static uint8_t wifi_buffer_rx[2048]; // ֡���ջ�����
#if !WIFI_LOWLEVEL_NOINTPIN
static uint32_t wifi_input_time; // ���һ�ε���SDIO�жϴ�������ʱ��
#endif

/* ����ͳ�ʱ���� */
void WiFi_CheckTimeout(void)
{
#if !WIFI_LOWLEVEL_NOINTPIN
  int stat, real_stat;
  uint32_t diff;
#endif
  
  // ��ʱ���INTSTATUS�Ĵ���, ������Ϊû�м�⵽SDIOIT�ж϶����³�����
#if WIFI_LOWLEVEL_NOINTPIN
  WiFi_Input();
#else
  diff = WiFi_LowLevel_GetTicks() - wifi_input_time;
  if (diff > 3000)
  {
    stat = WiFi_LowLevel_GetITStatus(0);
    real_stat = WiFi_Input();
    if (!stat && real_stat)
      printf("%s: missed an interrupt!\n", __FUNCTION__);
  }
#endif
  
  WiFi_CheckCommandTimeout(); // ���ʱ���
  WiFi_SynchronizeMulticastFilter(BSS_STA); // �ಥ������ͬ��
}

/* ����WiFi�¼� */
static void WiFi_EventProcess(WiFi_EventHeader *event)
{
  bss_t bss = WIFI_BSS(event->bss_type, event->bss_num);
  MrvlIEType *tlv;
  MrvlIETypes_PKT_Fwd_Ctrl_t *pktfwd;
  MrvlIETypes_Tx_Data_Pause_t *txpause;
  WiFi_ExtendedEventHeader *ext = NULL;
  
  printf("[Event] code=0x%04x, bss=0x%02x, size=%d", event->event_id, bss, event->header.length);
  if (WiFi_IsExtendedEventFormat(event->event_id))
  {
    ext = (WiFi_ExtendedEventHeader *)event;
    if (event->header.length >= sizeof(WiFi_ExtendedEventHeader) - WIFI_MACADDR_LEN)
      printf(", reason=%u", ext->reason_code);
    if (event->header.length >= sizeof(WiFi_ExtendedEventHeader))
    {
      printf(", mac=%02X:%02X:%02X:%02X:%02X:%02X", ext->mac_address[0], ext->mac_address[1], ext->mac_address[2], 
        ext->mac_address[3], ext->mac_address[4], ext->mac_address[5]);
    }
  }
  printf("\n");
  
  if (WiFi_IsConnectionLostEvent(event->event_id))
  {
    if (bss == BSS_STA)
      WiFi_ConnectionLost();
  }
  
  switch (event->event_id)
  {
    case EVENT_BEACON_LOSS:
      // �ղ����ź� (������ֻ��ȵ㽨�����Ӻ�, ���ֻ�����), WiFiģ�鲻���Զ�����
      printf("Beacon Loss/Link Loss\n");
      break;
    case EVENT_DEAUTHENTICATE:
      // ��֤�ѽ�� (�����ֻ��ر����ȵ�, ��������·����������֤ʧ�ܶ��Զ��Ͽ�����)
      printf("Deauthenticated!\n");
      break;
    case EVENT_DISASSOCIATE:
      // ����˹���
      printf("Disassociated!\n");
      break;
    case EVENT_WMM_STATUS_CHANGE:
      printf("WMM status change event occurred!\n");
      break;
    case EVENT_IBSS_COALESCED:
      printf("IBSS coalescing process is finished and BSSID has changed!\n");
      break;
    case EVENT_EVT_WEP_ICV_ERR:
      // WEP���ܳ���
      printf("WEP decryption error!\n");
      break;
    case EVENT_PORT_RELEASE:
      // �ɹ������ȵ㲢���WPA/WPA2��֤
      // WEPģʽ��������ģʽ������֤, ֻҪ�����Ͼͻ�������¼�
      printf("Authenticated!\n");
      break;
    case EVENT_MICRO_AP_EV_ID_STA_DEAUTH:
      // �ͻ��˶Ͽ����ȵ�
      printf("Client disconnected from AP!\n");
      if (ext->reason_code <= 9)
      {
        printf("Reason for leaving: ");
        switch (ext->reason_code)
        {
          case 0:
            printf("unspecified");
            break;
          case 1:
            printf("client station leaving network");
            break;
          case 2:
            printf("client station aged out");
            break;
          case 3:
            printf("client station de-authenticated by user's request");
            break;
          case 4:
            printf("client station authentication failed");
            break;
          case 5:
            printf("client station association failed");
            break;
          case 6:
            printf("client station MAC address is blocked by filter table");
            break;
          case 7:
            printf("reached maximum station count limit that uAP supports");
            break;
          case 8:
            printf("4-way handshake timeout");
            break;
          case 9:
            printf("group key handshake timeout ");
            break;
        }
        printf("\n");
      }
      break;
    case EVENT_MICRO_AP_EV_ID_STA_ASSOC: // EVENT_FORWARD_MGMT_FRAME�������ͬһ��ID��, ��bss������
      if (bss == BSS_STA)
      {
        // EVENT_FORWARD_MGMT_FRAME�¼�
      }
      else if (bss == BSS_UAP)
      {
        // EVENT_MICRO_AP_EV_ID_STA_ASSOC�¼�
        // �ͻ����������ȵ�
        printf("Client connected to AP!\n");
      }
      break;
    case EVENT_MICRO_AP_EV_ID_BSS_START:
      // �ȵ㴴���ɹ�
      printf("AP is started!\n");
      tlv = (MrvlIEType *)(ext + 1);
      while ((uint8_t *)tlv - (uint8_t *)event != event->header.length)
      {
        if (tlv->header.type == WIFI_MRVLIETYPES_PKTFWDCTRL)
        {
          pktfwd = (MrvlIETypes_PKT_Fwd_Ctrl_t *)tlv;
          if (pktfwd->pkt_fwd_ctl & 1)
            printf("Packet forwarding mode: by firmware\n");
          else
            printf("Packet forwarding mode: by host\n");
        }
        tlv = (MrvlIEType *)TLV_NEXT(tlv);
      }
      break;
    case EVENT_DOT11N_ADDBA:
      printf("AP ADDBA request is received!\n");
      break;
    case EVENT_DOT11N_DELBA:
      printf("AP DELBA request is received!\n");
      break;
    case EVENT_MICRO_AP_EV_ID_BSS_IDLE:
      // ���пͻ��˶��Ͽ�������, ֻʣ����AP����
      printf("AP is idle!\n");
      break;
    case EVENT_MICRO_AP_EV_ID_BSS_ACTIVE:
      // ��һ����һ�����Ͽͻ���������AP�ȵ�
      printf("AP is active!\n");
      break;
    case EVENT_MICRO_AP_EV_ID_RSN_CONNECT:
      // �ͻ�����֤�ɹ�
      printf("AP client is authenticated!\n");
      break;
    case EVENT_TX_DATA_PAUSE:
      printf("AP client Tx pause state has changed!");
      txpause = (MrvlIETypes_Tx_Data_Pause_t *)(event + 1);
      if (txpause->header.type == WIFI_MRVLIETYPES_TXDATAPAUSE)
      {
        printf("\nclient=%02X:%02X:%02X:%02X:%02X:%02X", txpause->peer_mac_addr[0], txpause->peer_mac_addr[1], 
          txpause->peer_mac_addr[2], txpause->peer_mac_addr[3], txpause->peer_mac_addr[4], txpause->peer_mac_addr[5]);
        printf(", paused=%u, queued_num=%u", txpause->tx_pause_state, txpause->total_queued);
      }
      printf("\n");
      break;
    default:
      WiFi_LowLevel_Dump(event, event->header.length);
  }
  
  WiFi_EventHandler(bss, event);
}

/* ��ȡ�յ�������֡ */
const WiFi_DataRx *WiFi_GetReceivedPacket(void)
{
  WiFi_DataRx *data = (WiFi_DataRx *)wifi_buffer_rx;
  
  if (data->header.type == WIFI_SDIOFRAME_DATA)
    return data;
  else
    return NULL;
}

/* ����WiFiģ���жϵĺ��� */
// ����ֱ�����жϷ�����(ISR)�е��ô˺���
int WiFi_Input(void)
{
  static port_t next_port = 1;
  int i, ret;
  port_t port;
  uint8_t flags;
  uint16_t bitmap, len;
  uint32_t addr;
  WiFi_SDIOFrameHeader *rx_header = (WiFi_SDIOFrameHeader *)wifi_buffer_rx;
  
#if !WIFI_LOWLEVEL_NOINTPIN
  wifi_input_time = WiFi_LowLevel_GetTicks();
#endif
  flags = WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS); // ��ȡ��Ҫ������жϱ�־λ
  if (flags == 0)
    return 0;
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~flags); // �����������Щ��־λ, Ȼ���ٽ��д���, �������Ա������������������������ж�
  
  // ���������Download Ready�ж�
  if (flags & WIFI_INTSTATUS_DNLD)
    WiFi_UpdatePacketStatus();
  
  // ���������Upload Ready�ж�
  if (flags & WIFI_INTSTATUS_UPLD)
  {
    // �����ǰ����ͨ��������ͨ������, �ͽ�������
    // ��̬����next_port��֤������ͨ���ܹ���˳��ʹ��
    while ((bitmap = WiFi_GetReadPortStatus()) & (_BV(next_port) | 1))
    {
      if (bitmap & 1)
        port = 0; // �ȶ�����ͨ��
      else
        port = next_port;
      addr = WiFi_GetPortAddress(port);
      
      i = 0;
      do
      {
        len = WiFi_GetDataLength(port);
        ret = WiFi_LowLevel_ReadData(1, addr, wifi_buffer_rx, len, sizeof(wifi_buffer_rx), WIFI_RWDATA_ALLOWMULTIBYTE);
        if (ret == 0 && len < rx_header->length)
          ret = -1; // ����û�ж�����, ��Ϊ����
        else if (ret == -2)
        {
          // ��������, ��ֹ����
          ret = WiFi_LowLevel_ReadData(1, addr, NULL, len, 0, WIFI_RWDATA_ALLOWMULTIBYTE);
          if (ret == 0)
            ret = -2; // �����ɹ�
          // ����ʧ��, ��Ҫ����
        }
        
        i++;
      } while (ret == -1 && i < WIFI_LOWLEVEL_MAXRETRY);
      
      // ��ǰ����ͨ�������ݶ�ȡ�ɹ���, �´ξͽ�����һ��ͨ��
      if (port == next_port)
      {
        if (next_port == WIFI_DATAPORTS_RX_NUM)
          next_port = 1;
        else
          next_port++;
      }
      
      if (ret != 0)
        continue; // ������ǰͨ���е�����
      
      // ��������δ�õĲ�������
      if (len != sizeof(wifi_buffer_rx))
        memset(wifi_buffer_rx + len, 0, sizeof(wifi_buffer_rx) - len);
      
      switch (rx_header->type)
      {
        case WIFI_SDIOFRAME_DATA:
          // �յ�����֡
          WiFi_PacketProcess((WiFi_DataRx *)wifi_buffer_rx, port);
          break;
        case WIFI_SDIOFRAME_COMMAND:
          // �յ������Ӧ֡
          WiFi_CommandResponseProcess((WiFi_CommandHeader *)wifi_buffer_rx);
          break;
        case WIFI_SDIOFRAME_EVENT:
          // �յ��¼�֡
          WiFi_EventProcess((WiFi_EventHeader *)wifi_buffer_rx);
          break;
      }
    }
  }
  return 1;
}

/* �Ƿ�ΪWiFi���ӳɹ��¼� */
int WiFi_IsConnectionEstablishedEvent(uint16_t event_id)
{
  return event_id == EVENT_PORT_RELEASE || event_id == EVENT_MICRO_AP_EV_ID_BSS_ACTIVE;
}

/* �Ƿ�ΪWiFi�����¼� */
int WiFi_IsConnectionLostEvent(uint16_t event_id)
{
  return event_id == EVENT_BEACON_LOSS || event_id == EVENT_DEAUTHENTICATE || event_id == EVENT_DISASSOCIATE || event_id == EVENT_MICRO_AP_EV_ID_BSS_IDLE;
}

/* �¼��Ƿ�Ϊ��չ��ʽ */
int WiFi_IsExtendedEventFormat(uint16_t event_id)
{
  switch (event_id)
  {
    case EVENT_BEACON_LOSS:
    case EVENT_DEAUTHENTICATE:
    case EVENT_DISASSOCIATE:
    case EVENT_IBSS_COALESCED:
    case EVENT_ADHOC_STATION_CONNECT:
    case EVENT_ADHOC_STATION_DISCONNECT:
    case EVENT_MICRO_AP_EV_ID_STA_DEAUTH:
    case EVENT_MICRO_AP_EV_ID_STA_ASSOC:
    case EVENT_MICRO_AP_EV_ID_BSS_START:
    case EVENT_EVT_WEP_ICV_ERR:
    case EVENT_MICRO_AP_EV_ID_RSN_CONNECT:
      return 1;
    default:
      return 0;
  }
}

/* �ڹ涨�ĳ�ʱʱ����, �ȴ�ָ���Ŀ�״̬λ��λ, �������Ӧ���жϱ�־λ */
// �������ܹ��ȴ�������־λ��λ, ������Ҫ�������SDIOIT, Ҳ���ù�������־λ��״̬
int WiFi_Wait(uint8_t status, uint32_t timeout)
{
  uint32_t diff, start;
  
  start = WiFi_LowLevel_GetTicks();
  while ((WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS) & status) != status)
  {
    diff = WiFi_LowLevel_GetTicks() - start;
    if (timeout != 0 && diff > timeout)
    {
      // ����ʱʱ���ѵ�
      printf("WiFi_Wait(0x%02x): timeout!\n", status);
      return -1;
    }
  }
  
  // �����Ӧ���жϱ�־λ
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~status); // ����Ҫ�����λ����Ϊ1
  // ���ܽ�SDIOITλ�����! �����п��ܵ��¸�λ��Զ������λ
  return 0;
}
