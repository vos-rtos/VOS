#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wifi.h"

static void WiFi_EventProcess(WiFi_EventHeader *event);

__attribute__((aligned)) static uint8_t wifi_buffer_rx[2048]; // 帧接收缓冲区
#if !WIFI_LOWLEVEL_NOINTPIN
static uint32_t wifi_input_time; // 最后一次调用SDIO中断处理函数的时间
#endif

/* 命令发送超时处理 */
void WiFi_CheckTimeout(void)
{
#if !WIFI_LOWLEVEL_NOINTPIN
  int stat, real_stat;
  uint32_t diff;
#endif
  
  // 定时检查INTSTATUS寄存器, 避免因为没有检测到SDIOIT中断而导致程序卡死
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
  
  WiFi_CheckCommandTimeout(); // 命令超时检测
  WiFi_SynchronizeMulticastFilter(BSS_STA); // 多播过滤器同步
}

/* 处理WiFi事件 */
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
      // 收不到信号 (例如和手机热点建立连接后, 把手机拿走), WiFi模块不会自动重连
      printf("Beacon Loss/Link Loss\n");
      break;
    case EVENT_DEAUTHENTICATE:
      // 认证已解除 (例如手机关闭了热点, 或者连接路由器后因认证失败而自动断开连接)
      printf("Deauthenticated!\n");
      break;
    case EVENT_DISASSOCIATE:
      // 解除了关联
      printf("Disassociated!\n");
      break;
    case EVENT_WMM_STATUS_CHANGE:
      printf("WMM status change event occurred!\n");
      break;
    case EVENT_IBSS_COALESCED:
      printf("IBSS coalescing process is finished and BSSID has changed!\n");
      break;
    case EVENT_EVT_WEP_ICV_ERR:
      // WEP解密出错
      printf("WEP decryption error!\n");
      break;
    case EVENT_PORT_RELEASE:
      // 成功关联热点并完成WPA/WPA2认证
      // WEP模式和无密码模式无需认证, 只要关联上就会产生此事件
      printf("Authenticated!\n");
      break;
    case EVENT_MICRO_AP_EV_ID_STA_DEAUTH:
      // 客户端断开了热点
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
    case EVENT_MICRO_AP_EV_ID_STA_ASSOC: // EVENT_FORWARD_MGMT_FRAME和这个是同一个ID号, 用bss来区分
      if (bss == BSS_STA)
      {
        // EVENT_FORWARD_MGMT_FRAME事件
      }
      else if (bss == BSS_UAP)
      {
        // EVENT_MICRO_AP_EV_ID_STA_ASSOC事件
        // 客户端连接了热点
        printf("Client connected to AP!\n");
      }
      break;
    case EVENT_MICRO_AP_EV_ID_BSS_START:
      // 热点创建成功
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
      // 所有客户端都断开了连接, 只剩下了AP本身
      printf("AP is idle!\n");
      break;
    case EVENT_MICRO_AP_EV_ID_BSS_ACTIVE:
      // 有一个或一个以上客户端连接了AP热点
      printf("AP is active!\n");
      break;
    case EVENT_MICRO_AP_EV_ID_RSN_CONNECT:
      // 客户端认证成功
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

/* 获取收到的数据帧 */
const WiFi_DataRx *WiFi_GetReceivedPacket(void)
{
  WiFi_DataRx *data = (WiFi_DataRx *)wifi_buffer_rx;
  
  if (data->header.type == WIFI_SDIOFRAME_DATA)
    return data;
  else
    return NULL;
}

/* 处理WiFi模块中断的函数 */
// 请勿直接在中断服务函数(ISR)中调用此函数
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
  flags = WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS); // 获取需要处理的中断标志位
  if (flags == 0)
    return 0;
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~flags); // 必须先清除这些标志位, 然后再进行处理, 这样可以避免清除掉处理过程中新来的中断
  
  // 如果触发了Download Ready中断
  if (flags & WIFI_INTSTATUS_DNLD)
    WiFi_UpdatePacketStatus();
  
  // 如果触发了Upload Ready中断
  if (flags & WIFI_INTSTATUS_UPLD)
  {
    // 如果当前数据通道或命令通道可用, 就接收数据
    // 静态变量next_port保证了数据通道能够按顺序使用
    while ((bitmap = WiFi_GetReadPortStatus()) & (_BV(next_port) | 1))
    {
      if (bitmap & 1)
        port = 0; // 先读命令通道
      else
        port = next_port;
      addr = WiFi_GetPortAddress(port);
      
      i = 0;
      do
      {
        len = WiFi_GetDataLength(port);
        ret = WiFi_LowLevel_ReadData(1, addr, wifi_buffer_rx, len, sizeof(wifi_buffer_rx), WIFI_RWDATA_ALLOWMULTIBYTE);
        if (ret == 0 && len < rx_header->length)
          ret = -1; // 数据没有读完整, 视为出错
        else if (ret == -2)
        {
          // 丢弃数据, 防止卡死
          ret = WiFi_LowLevel_ReadData(1, addr, NULL, len, 0, WIFI_RWDATA_ALLOWMULTIBYTE);
          if (ret == 0)
            ret = -2; // 丢弃成功
          // 丢弃失败, 仍要重试
        }
        
        i++;
      } while (ret == -1 && i < WIFI_LOWLEVEL_MAXRETRY);
      
      // 当前数据通道的数据读取成功后, 下次就接收下一个通道
      if (port == next_port)
      {
        if (next_port == WIFI_DATAPORTS_RX_NUM)
          next_port = 1;
        else
          next_port++;
      }
      
      if (ret != 0)
        continue; // 丢弃当前通道中的数据
      
      // 将缓冲区未用的部分清零
      if (len != sizeof(wifi_buffer_rx))
        memset(wifi_buffer_rx + len, 0, sizeof(wifi_buffer_rx) - len);
      
      switch (rx_header->type)
      {
        case WIFI_SDIOFRAME_DATA:
          // 收到数据帧
          WiFi_PacketProcess((WiFi_DataRx *)wifi_buffer_rx, port);
          break;
        case WIFI_SDIOFRAME_COMMAND:
          // 收到命令回应帧
          WiFi_CommandResponseProcess((WiFi_CommandHeader *)wifi_buffer_rx);
          break;
        case WIFI_SDIOFRAME_EVENT:
          // 收到事件帧
          WiFi_EventProcess((WiFi_EventHeader *)wifi_buffer_rx);
          break;
      }
    }
  }
  return 1;
}

/* 是否为WiFi连接成功事件 */
int WiFi_IsConnectionEstablishedEvent(uint16_t event_id)
{
  return event_id == EVENT_PORT_RELEASE || event_id == EVENT_MICRO_AP_EV_ID_BSS_ACTIVE;
}

/* 是否为WiFi掉线事件 */
int WiFi_IsConnectionLostEvent(uint16_t event_id)
{
  return event_id == EVENT_BEACON_LOSS || event_id == EVENT_DEAUTHENTICATE || event_id == EVENT_DISASSOCIATE || event_id == EVENT_MICRO_AP_EV_ID_BSS_IDLE;
}

/* 事件是否为扩展格式 */
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

/* 在规定的超时时间内, 等待指定的卡状态位置位, 并清除相应的中断标志位 */
// 本函数能够等待单个标志位置位, 而不需要事先清除SDIOIT, 也不用管其他标志位的状态
int WiFi_Wait(uint8_t status, uint32_t timeout)
{
  uint32_t diff, start;
  
  start = WiFi_LowLevel_GetTicks();
  while ((WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS) & status) != status)
  {
    diff = WiFi_LowLevel_GetTicks() - start;
    if (timeout != 0 && diff > timeout)
    {
      // 若超时时间已到
      printf("WiFi_Wait(0x%02x): timeout!\n", status);
      return -1;
    }
  }
  
  // 清除对应的中断标志位
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~status); // 不需要清除的位必须为1
  // 不能将SDIOIT位清除掉! 否则有可能导致该位永远不再置位
  return 0;
}
