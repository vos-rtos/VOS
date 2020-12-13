#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

static void WiFi_Associate_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_Deauthenticate_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_MACAddr_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_PrintTxPacketStatus_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_Scan_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_ScanSSID_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_SetWEP_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_ShowKeyMaterials_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_SynchronizeMulticastFilter_Callback(void *arg, void *data, WiFi_Status status);

static const float wifi_ratelist[] = {1.0f, 2.0f, 5.5f, 11.0f, 0.0f, 6.0f, 9.0f, 12.0f, 18.0f, 24.0f, 36.0f, 48.0f, 54.0f};
static WiFi_MulticastFilter wifi_mcast_filter;
static WiFi_STAInfo wifi_sta_info; // ��ǰ���ӵ��ȵ���Ϣ

/* ����һ���ȵ㲢�������� */
// ����WPA�͵��ȵ�ʱ, security��Աֱ�Ӹ�ֵWIFI_SECURITYTYPE_WPA����, ����Ҫ��ȷָ��WPA�汾��
void WiFi_Associate(const WiFi_Connection *conn, int8_t max_retry, WiFi_Callback callback, void *arg)
{
  uint8_t len;
  uint16_t ctrl;
  WiFi_Arg_Associate *p;
  
  if (wifi_sta_info.state != WIFI_STASTATE_IDLE)
  {
    if (wifi_sta_info.state == WIFI_STASTATE_CONNECTED)
      printf("%s: Already connected!\n", __FUNCTION__);
    else
      printf("%s: Association is already in progress!\n", __FUNCTION__);
    goto err;
  }
  
  len = strlen(conn->ssid);
  if (len == 0 || len > WIFI_MAX_SSIDLEN)
  {
    printf("%s: The length of SSID is invalid!\n", __FUNCTION__);
    goto err;
  }
  
  p = (WiFi_Arg_Associate *)WiFi_AllocateArgument(sizeof(WiFi_Arg_Associate), __FUNCTION__, conn->bss, callback, arg);
  if (p == NULL)
    return;
  p->conn = *conn;
  p->retry = max_retry; // ������������ӵĴ���, -1��ʾ���޴���, 0��ʾ������
  
  wifi_sta_info.bss = conn->bss;
  wifi_sta_info.state = WIFI_STASTATE_CONNECTING;
  
  ctrl = WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX;
  if (conn->sec_type == WIFI_SECURITYTYPE_WEP)
    ctrl |= WIFI_MACCTRL_WEP;
  WiFi_MACControl(conn->bss, ctrl, WiFi_Associate_Callback, p);
  return;
  
err:
  if (callback != NULL)
    callback(arg, NULL, WIFI_STATUS_INVALID);
}

static void WiFi_Associate_Callback(void *arg, void *data, WiFi_Status status)
{
  uint16_t cmd_code, cmd_size;
  WiFi_Arg_Associate *p = arg;
  WiFi_CmdRequest_Associate *cmd;
  WiFi_CmdResponse_Associate *resp;
  WiFi_SecurityType real_sec_type;
  MrvlIETypes_PhyParamDSSet_t *ds;
  MrvlIETypes_CfParamSet_t *cf;
  MrvlIETypes_AuthType_t *auth;
  MrvlIETypes_VendorParamSet_t *vendor;
  MrvlIETypes_RsnParamSet_t *rsn;
  
  cmd_code = WiFi_GetCommandCode(data);
  if (cmd_code == CMD_802_11_ASSOCIATE && status == WIFI_STATUS_OK)
  {
    // ��������ִ����ϲ��յ��˻�Ӧ, ������Ҫ����Ƿ�����ɹ�
    resp = (WiFi_CmdResponse_Associate *)data;
#if WIFI_DISPLAY_ASSOCIATE_RESULT
    printf("capability=0x%04x, status_code=0x%04x, aid=0x%04x\n", resp->capability, resp->status_code, resp->association_id);
#endif
    
    real_sec_type = WiFi_GetSecurityType(&wifi_sta_info.info);
    if (resp->association_id == 0xffff)
      status = WIFI_STATUS_FAIL; // ����ʧ�� (�ڻص�������data�м��resp->capability��resp->status_code��ֵ�ɻ����ϸԭ��)
    else if (real_sec_type == WIFI_SECURITYTYPE_WPA || real_sec_type == WIFI_SECURITYTYPE_WPA2)
      status = WIFI_STATUS_INPROGRESS; // �ȴ���֤
  }
  
  if ((cmd_code == CMD_802_11_ASSOCIATE && status != WIFI_STATUS_OK && status != WIFI_STATUS_INPROGRESS)
      || (cmd_code == CMD_802_11_SCAN && status != WIFI_STATUS_OK))
  {
    // ����ʧ��, ����
    if (p->retry != 0)
    {
      if (p->retry != -1)
        p->retry--;
      
      // ����
      if (cmd_code == CMD_802_11_SCAN)
        cmd_code = CMD_MAC_CONTROL;
      else
        cmd_code = CMD_SUPPLICANT_PMK;
      status = WIFI_STATUS_OK;
      printf("WiFi_Associate failed! Retry...\n");
    }
  }
  
  if (cmd_code == CMD_802_11_ASSOCIATE && (status == WIFI_STATUS_OK || status == WIFI_STATUS_INPROGRESS))
  {
    // �����ɹ�
    wifi_sta_info.state = WIFI_STASTATE_CONNECTED;
    WiFi_FreeArgument((void **)&p, data, status);
    return;
  }
  
  if (status != WIFI_STATUS_OK)
  {
    // ����ʧ��
    printf("WiFi_Associate failed!\n");
    wifi_sta_info.state = WIFI_STASTATE_IDLE;
    WiFi_FreeArgument((void **)&p, data, status);
    return;
  }
  
  switch (cmd_code)
  {
    case CMD_MAC_CONTROL:
      WiFi_ScanSSID(p->conn.bss, p->conn.ssid, &wifi_sta_info.info, WiFi_Associate_Callback, p);
      break;
    case CMD_802_11_SCAN:
      if (p->conn.sec_type == WIFI_SECURITYTYPE_WEP)
      {
        WiFi_SetWEP(p->conn.bss, &p->conn.password.wep, WiFi_Associate_Callback, p);
        break;
      }
      else if (p->conn.sec_type == WIFI_SECURITYTYPE_WPA || p->conn.sec_type == WIFI_SECURITYTYPE_WPA2)
      {
        WiFi_SetWPA(p->conn.bss, p->conn.ssid, wifi_sta_info.info.mac_addr, p->conn.password.wpa, WiFi_Associate_Callback, p);
        break;
      }
    case CMD_802_11_KEY_MATERIAL:
    case CMD_SUPPLICANT_PMK:
      cmd = (WiFi_CmdRequest_Associate *)WiFi_GetSubCommandBuffer((void **)&p);
      if (cmd == NULL)
      {
        wifi_sta_info.state = WIFI_STASTATE_IDLE;
        return;
      }
      memcpy(cmd->peer_sta_addr, wifi_sta_info.info.mac_addr, sizeof(wifi_sta_info.info.mac_addr));
      cmd->cap_info = wifi_sta_info.info.cap_info;
      cmd->listen_interval = 10;
      cmd->bcn_period = wifi_sta_info.info.bcn_period;
      cmd->dtim_period = 1;
      memcpy(cmd + 1, &wifi_sta_info.info.ssid, TLV_STRUCTLEN(wifi_sta_info.info.ssid));
      
      ds = (MrvlIETypes_PhyParamDSSet_t *)((uint8_t *)(cmd + 1) + TLV_STRUCTLEN(wifi_sta_info.info.ssid));
      ds->header.type = WIFI_MRVLIETYPES_PHYPARAMDSSET;
      ds->header.length = 1;
      ds->channel = wifi_sta_info.info.channel;
      
      cf = (MrvlIETypes_CfParamSet_t *)(ds + 1);
      memset(cf, 0, sizeof(MrvlIETypes_CfParamSet_t));
      cf->header.type = WIFI_MRVLIETYPES_CFPARAMSET;
      cf->header.length = TLV_PAYLOADLEN(*cf);
      
      memcpy(cf + 1, &wifi_sta_info.info.rates, TLV_STRUCTLEN(wifi_sta_info.info.rates));
      auth = (MrvlIETypes_AuthType_t *)((uint8_t *)(cf + 1) + TLV_STRUCTLEN(wifi_sta_info.info.rates));
      auth->header.type = WIFI_MRVLIETYPES_AUTHTYPE;
      auth->header.length = TLV_PAYLOADLEN(*auth);
      auth->auth_type = p->conn.auth_type;
      
      cmd_size = (uint8_t *)(auth + 1) - (uint8_t *)cmd;
      real_sec_type = WiFi_GetSecurityType(&wifi_sta_info.info);
      if (real_sec_type == WIFI_SECURITYTYPE_WPA)
      {
        // WPA��������������м���Vendor�������ܳɹ�����
        vendor = (MrvlIETypes_VendorParamSet_t *)(auth + 1);
        memcpy(vendor, &wifi_sta_info.info.wpa, TLV_STRUCTLEN(wifi_sta_info.info.wpa));
        cmd_size += TLV_STRUCTLEN(wifi_sta_info.info.wpa);
      }
      else if (real_sec_type == WIFI_SECURITYTYPE_WPA2)
      {
        // WPA2��������������м���RSN�������ܳɹ�����
        rsn = (MrvlIETypes_RsnParamSet_t *)(auth + 1);
        memcpy(rsn, &wifi_sta_info.info.rsn, TLV_STRUCTLEN(wifi_sta_info.info.rsn));
        cmd_size += TLV_STRUCTLEN(wifi_sta_info.info.rsn);
      }
      
      WiFi_SetCommandHeader(&cmd->header, p->base.bss, CMD_802_11_ASSOCIATE, cmd_size, 1);
      WiFi_SendCommand(WiFi_Associate_Callback, arg);
      // ����arg�ڴ�, �ȹ����ɹ������ͷ�
      break;
  }
}

/* ��������WEP��Կ�Ƿ�Ϸ�, ����ʮ�������ַ�����Կת��Ϊ��������Կ */
// dest������\0����, ����ֵΪ��Կ����������
int WiFi_CheckWEPKey(uint8_t *dest, const char *src)
{
  int i, len = 0;
  uint32_t temp;
  
  if (src != NULL)
  {
    len = strlen(src);
    if (len == 5 || len == 13)
      memcpy(dest, src, len); // 5����13��ASCII��Կ�ַ�
    else if (len == 10 || len == 26)
    {
      // 10����26��16������Կ�ַ�
      for (i = 0; i < len; i += 2)
      {
        if (!isxdigit(src[i]) || !isxdigit(src[i + 1]))
          return -1; // ��Կ�к��зǷ��ַ�
        
        sscanf(src + i, "%02x", &temp);
        dest[i / 2] = temp;
      }
      len /= 2;
    }
    else
      len = 0; // ���Ȳ��Ϸ�
  }
  return len;
}

/* ����Ͽ����� */
void WiFi_ConnectionLost(void)
{
  wifi_sta_info.state = WIFI_STASTATE_IDLE;
}

/* ���ȵ�Ͽ����� */
void WiFi_Deauthenticate(bss_t bss, uint16_t reason, WiFi_Callback callback, void *arg)
{
  int ret;
  uint8_t bssid[WIFI_MACADDR_LEN];
  WiFi_ArgBase *p;
  WiFi_Cmd_Deauthenticate *cmd;
  
  ret = WiFi_GetBSSID(bss, bssid);
  if (ret == -1)
  {
    printf("%s: Not connected!\n", __FUNCTION__);
    if (callback != NULL)
      callback(arg, NULL, WIFI_STATUS_INVALID);
    return;
  }
  
  p = WiFi_AllocateArgument(sizeof(WiFi_ArgBase), __FUNCTION__, bss, callback, arg);
  if (p == NULL)
    return;
  
  cmd = (WiFi_Cmd_Deauthenticate *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  
  memcpy(cmd->peer_sta_addr, bssid, WIFI_MACADDR_LEN);
  cmd->reason_code = reason;
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_802_11_DEAUTHENTICATE, sizeof(WiFi_Cmd_Deauthenticate), 1);
  WiFi_SendCommand(WiFi_Deauthenticate_Callback, p);
}

static void WiFi_Deauthenticate_Callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
    wifi_sta_info.state = WIFI_STASTATE_IDLE;
  else
    printf("WiFi_Deauthenticate failed!\n");
  
  WiFi_FreeArgument(&arg, data, status);
}

/* ��ȡ�����ȵ��MAC��ַ */
int WiFi_GetBSSID(bss_t bss, uint8_t mac_addr[WIFI_MACADDR_LEN])
{
  if (wifi_sta_info.state == WIFI_STASTATE_CONNECTED)
  {
    memcpy(mac_addr, wifi_sta_info.info.mac_addr, WIFI_MACADDR_LEN);
    return 0;
  }
  else
  {
    memset(mac_addr, 0, WIFI_MACADDR_LEN);
    return -1;
  }
}

/* ��ȡ�ȵ����֤���� */
WiFi_SecurityType WiFi_GetSecurityType(const WiFi_SSIDInfo *info)
{
  if (info->cap_info & WIFI_CAPABILITY_PRIVACY)
  {
    if (info->rsn.header.type)
      return WIFI_SECURITYTYPE_WPA2;
    else if (info->wpa.header.type)
      return WIFI_SECURITYTYPE_WPA;
    else
      return WIFI_SECURITYTYPE_WEP;
  }
  else
    return WIFI_SECURITYTYPE_NONE;
}

/* ��ȡ����״̬ */
WiFi_STAState WiFi_GetSTAState(void)
{
  return wifi_sta_info.state;
}

/* ����ಥ�� */
int WiFi_JoinMulticastGroup(uint8_t addr[WIFI_MACADDR_LEN])
{
  int i;
  
  for (i = 0; i < wifi_mcast_filter.num; i++)
  {
    if (memcmp(wifi_mcast_filter.list[i], addr, WIFI_MACADDR_LEN) == 0)
      return i;
  }
  
  if (wifi_mcast_filter.num == WIFI_MCASTFILTER_COUNT)
  {
    printf("%s: Multicast filter is full!\n", __FUNCTION__);
    return -1;
  }
  
  memcpy(wifi_mcast_filter.list[wifi_mcast_filter.num], addr, WIFI_MACADDR_LEN);
  wifi_mcast_filter.num++;
  wifi_mcast_filter.state = WIFI_MCASTFILTERSTATE_MODIFIED;
  return i;
}

/* �뿪�ಥ�� */
int WiFi_LeaveMulticastGroup(uint8_t addr[WIFI_MACADDR_LEN])
{
  int i;
  
  for (i = 0; i < wifi_mcast_filter.num; i++)
  {
    if (memcmp(wifi_mcast_filter.list[i], addr, WIFI_MACADDR_LEN) == 0)
      break;
  }
  
  if (i == wifi_mcast_filter.num)
    return -1;
  
  if (i != wifi_mcast_filter.num - 1)
    memmove(wifi_mcast_filter.list[i], wifi_mcast_filter.list[i + 1], (wifi_mcast_filter.num - i - 1) * WIFI_MACADDR_LEN);
  wifi_mcast_filter.num--;
  wifi_mcast_filter.state = WIFI_MCASTFILTERSTATE_MODIFIED;
  return i;
}

/* ��ȡ������MAC��ַ */
void WiFi_MACAddr(bss_t bss, uint8_t addr[WIFI_MACADDR_LEN], WiFi_CommandAction action, WiFi_Callback callback, void *arg)
{
  WiFi_Arg_MACAddr *p;
  WiFi_Cmd_MACAddr *cmd;
  
  cmd = (WiFi_Cmd_MACAddr *)WiFi_GetCommandBuffer(callback, arg);
  if (cmd == NULL)
    return;
  
  cmd->action = action;
  if (action == WIFI_ACT_GET)
  {
    memset(cmd->mac_addr, 0, sizeof(cmd->mac_addr));
    memset(addr, 0, WIFI_MACADDR_LEN);
    
    p = (WiFi_Arg_MACAddr *)WiFi_AllocateArgument(sizeof(WiFi_Arg_MACAddr), __FUNCTION__, bss, callback, arg);
    if (p == NULL)
    {
      WiFi_ReleaseCommandBuffer();
      return;
    }
    p->addr = addr;
    
    callback = WiFi_MACAddr_Callback;
    arg = p;
  }
  else
    memcpy(cmd->mac_addr, addr, sizeof(cmd->mac_addr));
  
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_802_11_MAC_ADDR, sizeof(WiFi_Cmd_MACAddr), 1);
  WiFi_SendCommand(callback, arg);
}

static void WiFi_MACAddr_Callback(void *arg, void *data, WiFi_Status status)
{
  WiFi_Arg_MACAddr *p = arg;
  WiFi_Cmd_MACAddr *resp = (WiFi_Cmd_MACAddr *)data;
  
  if (status == WIFI_STATUS_OK)
    memcpy(p->addr, resp->mac_addr, sizeof(resp->mac_addr));
  else
    printf("WiFi_MACAddr failed!\n");
  
  WiFi_FreeArgument((void **)&p, data, status);
}

/* ����MAC */
void WiFi_MACControl(bss_t bss, uint16_t action, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_MACCtrl *cmd;
  
  cmd = (WiFi_Cmd_MACCtrl *)WiFi_GetCommandBuffer(callback, arg);
  if (cmd == NULL)
    return;
  cmd->action = action;
  cmd->reserved = 0;
  
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_MAC_CONTROL, sizeof(WiFi_Cmd_MACCtrl), 1);
  WiFi_SendCommand(callback, arg);
}

/* ��ʾ�����б� */
void WiFi_PrintRates(uint16_t rates)
{
  // �����б��ڹ̼�API�ֲ��3.1 Receive Packet Descriptorһ�������RxRate�ֶ���������
  int i, space = 0;
  
  for (i = 0; rates != 0; i++)
  {
    if (i == sizeof(wifi_ratelist) / sizeof(wifi_ratelist[0]))
      break;
    if (rates & 1)
    {
      if (space)
        printf(" ");
      else
        space = 1;
      printf("%.1fMbps", wifi_ratelist[i]);
    }
    rates >>= 1;
  }
}

/* ��ʾ����֡�ķ������ */
void WiFi_PrintTxPacketStatus(bss_t bss)
{
  WiFi_CommandHeader *cmd;
  
  cmd = WiFi_GetCommandBuffer(NULL, NULL);
  if (cmd != NULL)
  {
    WiFi_SetCommandHeader(cmd, bss, CMD_TX_PKT_STATS, sizeof(WiFi_CommandHeader), 1);
    WiFi_SendCommand(WiFi_PrintTxPacketStatus_Callback, NULL);
  }
}

static void WiFi_PrintTxPacketStatus_Callback(void *arg, void *data, WiFi_Status status)
{
  int i;
  uint8_t none = 1;
  WiFi_CommandHeader *header = (WiFi_CommandHeader *)data;
  WiFi_TxPktStatEntry empty_entry = {0};
  WiFi_TxPktStatEntry *entries = (WiFi_TxPktStatEntry *)(header + 1);
  
  if (status == WIFI_STATUS_OK)
  {
    printf("[Tx packet status] bss=0x%02x\n", header->bss);
    for (i = 0; i < sizeof(wifi_ratelist) / sizeof(wifi_ratelist[0]); i++)
    {
      if (wifi_ratelist[i] != 0.0f && memcmp(&entries[i], &empty_entry, sizeof(WiFi_TxPktStatEntry)) != 0)
      {
        none = 0;
        printf("%.1fMbps: PktInitCnt=%d, PktSuccessCnt=%d, TxAttempts=%d\n", wifi_ratelist[i], entries[i].pkt_init_cnt, entries[i].pkt_success_cnt, entries[i].tx_attempts);
        printf("  RetryFailure=%d, ExpiryFailure=%d\n", entries[i].retry_failure, entries[i].expiry_failure);
      }
    }
    if (none)
      printf("(None)\n");
  }
  else
    printf("WiFi_PrintTxRates failed!\n");
}

/* ɨ��ȫ���ȵ� (����ʾ) */
void WiFi_Scan(WiFi_Callback callback, void *arg)
{
  int i;
  WiFi_ArgBase *p;
  WiFi_CmdRequest_Scan *cmd; // Ҫ���͵�����
  MrvlIETypes_ChanListParamSet_t *chanlist;

  p = WiFi_AllocateArgument(sizeof(WiFi_ArgBase), __FUNCTION__, 0, callback, arg);
  if (p == NULL)
    return;
  
  cmd = (WiFi_CmdRequest_Scan *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  
  cmd->bss_type = WIFI_BSS_ANY;
  memset(cmd->bss_id, 0, sizeof(cmd->bss_id));
  
  // ͨ���Ļ�������
  chanlist = (MrvlIETypes_ChanListParamSet_t *)(cmd + 1); // �����+1ָ����ǰ��sizeof(ָ������)����ַ��Ԫ, ����ֻǰ��1����ַ��Ԫ
  chanlist->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chanlist->header.length = 4 * sizeof(chanlist->channels);
  for (i = 0; i < 4; i++) // ��ɨ��ǰ4��ͨ�� (i���±�, i+1����ͨ����)
  {
    chanlist->channels[i].band_config_type = 0; // 2.4GHz band, 20MHz channel width
    chanlist->channels[i].chan_number = i + 1; // ͨ����
    chanlist->channels[i].scan_type = 0;
    chanlist->channels[i].min_scan_time = 0;
    chanlist->channels[i].max_scan_time = 100;
  }

  WiFi_SetCommandHeader(&cmd->header, p->bss, CMD_802_11_SCAN, sizeof(WiFi_CmdRequest_Scan) + TLV_STRUCTLEN(*chanlist), 1);
  WiFi_SendCommand(WiFi_Scan_Callback, p);
}

static void WiFi_Scan_Callback(void *arg, void *data, WiFi_Status status)
{
  int i, j, n;
  WiFi_ArgBase *p = arg;
  WiFi_CmdRequest_Scan *cmd;
  MrvlIETypes_ChanListParamSet_t *chanlist;
  
  uint8_t ssid[WIFI_MAX_SSIDLEN + 1], channel;
  uint16_t len, ie_size;
  WiFi_CmdResponse_Scan *resp = (WiFi_CmdResponse_Scan *)data;
  WiFi_BssDescSet *bss_desc_set;
  WiFi_SecurityType security;
  WiFi_Vendor *vendor;
  IEEEType *ie_params;
  
#if WIFI_DISPLAY_SCAN_DETAILS
  IEEEType *rates;
  MrvlIETypes_TsfTimestamp_t *tsf_table;
#if WIFI_DISPLAY_SCAN_DETAILS == 2
  MrvlIETypes_ChanBandList_t *band_list;
#endif
#endif
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_Scan failed!\n");
    WiFi_FreeArgument((void **)&p, data, status);
    return;
  }
  
  // ����ɨ���������4��ͨ��������
  cmd = (WiFi_CmdRequest_Scan *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  
  chanlist = (MrvlIETypes_ChanListParamSet_t *)(cmd + 1);
  j = chanlist->channels[0].chan_number + 4;
  if (j < 14)
  {
    if (j == 13)
      n = 2;
    else
      n = 4;
    
    chanlist->header.length = n * sizeof(chanlist->channels);
    for (i = 0; i < n; i++)
      chanlist->channels[i].chan_number = i + j;
    
    WiFi_SetCommandHeader(&cmd->header, p->bss, CMD_802_11_SCAN, sizeof(WiFi_CmdRequest_Scan) + TLV_STRUCTLEN(*chanlist), 1);
    WiFi_SendCommand(WiFi_Scan_Callback, p);
  }
  else
  {
    n = 0;
    WiFi_ReleaseCommandBuffer();
  }
  
  // ��ʾ����ɨ����, num_of_setΪ�ȵ���
  if (resp->num_of_set > 0)
  {
    bss_desc_set = (WiFi_BssDescSet *)(resp + 1);
    
#if WIFI_DISPLAY_SCAN_DETAILS
    // resp->buf_size����bss_desc_set���ܴ�С
    tsf_table = (MrvlIETypes_TsfTimestamp_t *)((uint8_t *)bss_desc_set + resp->buf_size);
    if ((uint8_t *)tsf_table - (uint8_t *)data == resp->header.header.length)
    {
#if WIFI_DISPLAY_SCAN_DETAILS == 2
      printf("BssDescSet is the end!\n");
#endif
      tsf_table = NULL;
    }
#if WIFI_DISPLAY_SCAN_DETAILS == 2
    else
    {
      band_list = (MrvlIETypes_ChanBandList_t *)TLV_NEXT(tsf_table);
      if ((uint8_t *)band_list - (uint8_t *)data == resp->header.header.length)
      {
        printf("TSF timestamp table is the end!\n");
        band_list = NULL;
      }
      else if (TLV_NEXT(band_list) - (uint8_t *)data == resp->header.header.length)
        printf("Channel/band table is the end!\n");
    }
#endif
#endif
    
    for (i = 0; i < resp->num_of_set; i++)
    {
      security = WIFI_SECURITYTYPE_WEP;
#if WIFI_DISPLAY_SCAN_DETAILS
      rates = NULL;
#endif
      
      ie_params = &bss_desc_set->ie_parameters;
      ie_size = bss_desc_set->ie_length - (sizeof(WiFi_BssDescSet) - sizeof(bss_desc_set->ie_length) - sizeof(bss_desc_set->ie_parameters));
      while (ie_size > 0)
      {
        if (TLV_STRUCTLEN(*ie_params) > ie_size)
        {
          ie_params->header.length = ie_size - sizeof(IEEEHeader);
          printf("Found incomplete IEEE TLV! type=%#x\n", ie_params->header.type);
        }
        
        switch (ie_params->header.type)
        {
          case WIFI_MRVLIETYPES_SSIDPARAMSET:
            // SSID����
            len = ie_params->header.length;
            if (len > WIFI_MAX_SSIDLEN)
              len = WIFI_MAX_SSIDLEN;
            memcpy(ssid, ie_params->data, len);
            ssid[len] = '\0';
            break;
#if WIFI_DISPLAY_SCAN_DETAILS
          case WIFI_MRVLIETYPES_RATESPARAMSET:
            // ����
            rates = ie_params;
            break;
#endif
          case WIFI_MRVLIETYPES_PHYPARAMDSSET:
            // ͨ����
            channel = ie_params->data[0];
            break;
          case WIFI_MRVLIETYPES_RSNPARAMSET:
            security = WIFI_SECURITYTYPE_WPA2;
            break;
          case WIFI_MRVLIETYPES_VENDORPARAMSET:
            if (security != WIFI_SECURITYTYPE_WPA2)
            {
              vendor = (WiFi_Vendor *)ie_params->data;
              if (vendor->oui[0] == 0x00 && vendor->oui[1] == 0x50 && vendor->oui[2] == 0xf2 && vendor->oui_type == 0x01)
                security = WIFI_SECURITYTYPE_WPA;
            }
            break;
        }
        ie_size -= TLV_STRUCTLEN(*ie_params);
        ie_params = (IEEEType *)TLV_NEXT(ie_params);
      }
      if ((bss_desc_set->cap_info & WIFI_CAPABILITY_PRIVACY) == 0)
        security = WIFI_SECURITYTYPE_NONE;
      
      printf("SSID '%s', ", ssid); // �ȵ�����
      printf("MAC %02X:%02X:%02X:%02X:%02X:%02X, ", bss_desc_set->bssid[0], bss_desc_set->bssid[1], bss_desc_set->bssid[2], bss_desc_set->bssid[3], bss_desc_set->bssid[4], bss_desc_set->bssid[5]); // MAC��ַ
      printf("RSSI %d, Channel %d\n", bss_desc_set->rssi, channel); // �ź�ǿ�Ⱥ�ͨ����
#if WIFI_DISPLAY_SCAN_DETAILS
      printf("  Timestamp %lld, Beacon Interval %d", bss_desc_set->pkt_time_stamp, bss_desc_set->bcn_interval);
      if (tsf_table != NULL)
        printf(", TSF timestamp: %lld\n", tsf_table->tsf_table[i]);
      else
        printf("\n");
#endif
      
      printf("  Capability: 0x%04x (Security: ", bss_desc_set->cap_info);
      switch (security)
      {
        case WIFI_SECURITYTYPE_NONE:
          printf("Unsecured");
          break;
        case WIFI_SECURITYTYPE_WEP:
          printf("WEP");
          break;
        case WIFI_SECURITYTYPE_WPA:
          printf("WPA");
          break;
        case WIFI_SECURITYTYPE_WPA2:
          printf("WPA2");
          break;
      }
      printf(", Mode: ");
      if (bss_desc_set->cap_info & WIFI_CAPABILITY_IBSS)
        printf("Ad-Hoc");
      else
        printf("Infrastructure");
      printf(")\n");
      
#if WIFI_DISPLAY_SCAN_DETAILS
      if (rates != NULL)
      {
        // ��������ֵ�ı�ʾ����, �����802.11-2016.pdf��9.4.2.3 Supported Rates and BSS Membership Selectors elementһ�ڵ�����
        printf("  Rates:");
        for (j = 0; j < rates->header.length; j++)
          printf(" %.1fMbps", (rates->data[j] & 0x7f) * 0.5);
        printf("\n");
      }
#endif
      
      // ת����һ���ȵ���Ϣ
      bss_desc_set = (WiFi_BssDescSet *)((uint8_t *)bss_desc_set + sizeof(bss_desc_set->ie_length) + bss_desc_set->ie_length);
    }
  }
  
  // ɨ�����ʱ���ûص�����
  if (n == 0)
    WiFi_FreeArgument((void **)&p, data, status);
}

/* ��ȡָ�����Ƶ��ȵ����ϸ��Ϣ */
void WiFi_ScanSSID(bss_t bss, const char *ssid, WiFi_SSIDInfo *info, WiFi_Callback callback, void *arg)
{
  int i;
  MrvlIETypes_ChanListParamSet_t *chan_list;
  WiFi_Arg_ScanSSID *p;
  WiFi_CmdRequest_Scan *cmd;
  
  p = (WiFi_Arg_ScanSSID *)WiFi_AllocateArgument(sizeof(WiFi_Arg_ScanSSID), __FUNCTION__, bss, callback, arg);
  if (p == NULL)
    return;
  p->info = info;
  memset(info, 0, sizeof(WiFi_SSIDInfo)); // ������info�ṹ������
  
  cmd = (WiFi_CmdRequest_Scan *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  cmd->bss_type = WIFI_BSS_ANY;
  memset(cmd->bss_id, 0, sizeof(cmd->bss_id));
  
  // ��info->ssid��Ա��ֵ
  info->ssid.header.type = WIFI_MRVLIETYPES_SSIDPARAMSET;
  info->ssid.header.length = strlen(ssid);
  memcpy(info->ssid.ssid, ssid, info->ssid.header.length);
  memcpy(cmd + 1, &info->ssid, TLV_STRUCTLEN(info->ssid)); // ��info->ssid���Ƶ������͵�����������
  
  chan_list = (MrvlIETypes_ChanListParamSet_t *)((uint8_t *)(cmd + 1) + TLV_STRUCTLEN(info->ssid));
  chan_list->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chan_list->header.length = 14 * sizeof(chan_list->channels); // һ����ɨ��14��ͨ��
  for (i = 0; i < 14; i++) // i���±�, i+1����ͨ����
  {
    chan_list->channels[i].band_config_type = 0;
    chan_list->channels[i].chan_number = i + 1;
    chan_list->channels[i].scan_type = 0;
    chan_list->channels[i].min_scan_time = 0;
    chan_list->channels[i].max_scan_time = 100;
  }
  
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_802_11_SCAN, ((uint8_t *)chan_list - (uint8_t *)cmd) + TLV_STRUCTLEN(*chan_list), 1);
  WiFi_SendCommand(WiFi_ScanSSID_Callback, p);
}

static void WiFi_ScanSSID_Callback(void *arg, void *data, WiFi_Status status)
{
  uint16_t ie_size;
  WiFi_Arg_ScanSSID *p = arg;
  WiFi_CmdResponse_Scan *resp = (WiFi_CmdResponse_Scan *)data;
  WiFi_BssDescSet *bss_desc_set = (WiFi_BssDescSet *)(resp + 1);
  IEEEType *ie_params;
  WiFi_Vendor *vendor;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_ScanSSID failed!\n");
    goto end;
  }
  else if (resp->num_of_set == 0)
  {
    // δ�ҵ�ָ����AP�ȵ�, ��ʱinfo�ṹ���г���ssid��Ա��, ����ĳ�Ա��Ϊ0
    // resp�е����ݵ���num_of_set��Ա���û����
    status = WIFI_STATUS_NOTFOUND;
    printf("SSID not found!\n");
    goto end;
  }
  
  // bss_desc_set��ɨ�赽�ĵ�һ����Ϣ��Ϊ׼
  memcpy(p->info->mac_addr, bss_desc_set->bssid, sizeof(p->info->mac_addr));
  p->info->cap_info = bss_desc_set->cap_info;
  p->info->bcn_period = bss_desc_set->bcn_interval;
  
  // ��info->xxx.header.type=0, �����û�и������Ϣ (��SSID�ṹ����, ��ΪSSID��type=WIFI_MRVLIETYPES_SSIDPARAMSET=0)
  ie_params = &bss_desc_set->ie_parameters;
  ie_size = bss_desc_set->ie_length - (sizeof(WiFi_BssDescSet) - sizeof(bss_desc_set->ie_length) - sizeof(bss_desc_set->ie_parameters)); // ����IEEE_Type���ݵ��ܴ�С
  while (ie_size > 0)
  {
    if (TLV_STRUCTLEN(*ie_params) > ie_size)
    {
      ie_params->header.length = ie_size - sizeof(IEEEHeader);
      printf("Found incomplete IEEE TLV! type=%#x\n", ie_params->header.type);
    }
    
    switch (ie_params->header.type)
    {
      case WIFI_MRVLIETYPES_RATESPARAMSET:
        // ����
        WiFi_TranslateTLV((MrvlIEType *)&p->info->rates, ie_params, sizeof(p->info->rates.rates));
        break;
      case WIFI_MRVLIETYPES_PHYPARAMDSSET:
        // ͨ����
        p->info->channel = ie_params->data[0];
        break;
      case WIFI_MRVLIETYPES_RSNPARAMSET:
        // ͨ��ֻ��һ��RSN��Ϣ (��WPA2���)
        WiFi_TranslateTLV((MrvlIEType *)&p->info->rsn, ie_params, sizeof(p->info->rsn.rsn));
        break;
      case WIFI_MRVLIETYPES_VENDORPARAMSET:
        // ͨ�����ж���VENDOR��Ϣ (��WPA���)
        vendor = (WiFi_Vendor *)ie_params->data;
        if (vendor->oui[0] == 0x00 && vendor->oui[1] == 0x50 && vendor->oui[2] == 0xf2)
        {
          switch (vendor->oui_type)
          {
            case 0x01:
              // wpa_oui
              WiFi_TranslateTLV((MrvlIEType *)&p->info->wpa, ie_params, sizeof(p->info->wpa.vendor));
              break;
            case 0x02:
              // wmm_oui
              if (ie_params->header.length == 24) // �Ϸ���С
                WiFi_TranslateTLV((MrvlIEType *)&p->info->wwm, ie_params, sizeof(p->info->wwm.vendor));
              break;
            case 0x04:
              // wps_oui
              WiFi_TranslateTLV((MrvlIEType *)&p->info->wps, ie_params, sizeof(p->info->wps.vendor));
              break;
          }
        }
        break;
    }
    
    // ת����һ��TLV
    ie_size -= TLV_STRUCTLEN(*ie_params);
    ie_params = (IEEEType *)TLV_NEXT(ie_params);
  }
  
end:
  WiFi_FreeArgument((void **)&p, data, status);
}

/* ʹ��CMD_802_11_KEY_MATERIAL��������WEP��Կ */
// ��֧�ֵ�����Կ
void WiFi_SetWEP(bss_t bss, const WiFi_WEPKey *key, WiFi_Callback callback, void *arg)
{
  static int last = -1;
  int len;
  WiFi_Arg_SetWEP *p;
  WiFi_Cmd_KeyMaterial *cmd;
  MrvlIETypes_KeyParamSet_v2_t *keyparamset; // ������Կʱ����ʹ��V2�汾��TLV����
  WiFi_KeyParam_WEP_v2 *keyparam;
  
  cmd = (WiFi_Cmd_KeyMaterial *)WiFi_GetCommandBuffer(callback, arg);
  if (cmd == NULL)
    return;
  cmd->action = WIFI_ACT_SET;
  
  keyparamset = (MrvlIETypes_KeyParamSet_v2_t *)&cmd->keys;
  keyparamset->header.type = WIFI_MRVLIETYPES_KEYPARAMSETV2;
  keyparamset->header.length = TLV_PAYLOADLEN(*keyparamset);
  memset(keyparamset->mac_address, 0, sizeof(keyparamset->mac_address)); // ���MAC��ַ��Ϊ0, �����Ҫ�������ȵ�֮��������óɹ�
  keyparamset->key_idx = key->index;
  keyparamset->key_type = WIFI_KEYTYPE_WEP;
  keyparamset->key_info = WIFI_KEYINFO_DEFAULTKEY | WIFI_KEYINFO_KEYENABLED | WIFI_KEYTYPE_WEP;
  
  keyparam = (WiFi_KeyParam_WEP_v2 *)(keyparamset + 1);
  len = WiFi_CheckWEPKey(keyparam->wep_key, key->keys[key->index]);
  if (len == -1 || len == 0)
  {
    if (len == -1)
      printf("%s: The hex key contains invalid characters!\n", __FUNCTION__);
    else
      printf("%s: The length of key is invalid!\n", __FUNCTION__);
    
    WiFi_ReleaseCommandBuffer();
    if (callback != NULL)
      callback(arg, NULL, WIFI_STATUS_INVALID);
    return;
  }
  
  keyparam->key_len = len;
  keyparamset->header.length += sizeof(keyparam->key_len) + len;
  
  // ��ɾ����Կ�����
  if (last != key->index)
  {
    if (last != -1)
    {
      p = (WiFi_Arg_SetWEP *)WiFi_AllocateArgument(sizeof(WiFi_Arg_SetWEP), __FUNCTION__, bss, callback, arg);
      if (p == NULL)
      {
        WiFi_ReleaseCommandBuffer();
        return;
      }
      p->del_index = last;
      p->deleted = 0;
      
      callback = WiFi_SetWEP_Callback;
      arg = p;
    }
    last = key->index;
  }
  
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_802_11_KEY_MATERIAL, (uint8_t *)TLV_NEXT(keyparamset) - (uint8_t *)cmd, 1);
  WiFi_SendCommand(callback, arg);
}

static void WiFi_SetWEP_Callback(void *arg, void *data, WiFi_Status status)
{
  // �Ƴ�����Կ
  WiFi_Arg_SetWEP *p = arg;
  WiFi_Cmd_KeyMaterial *cmd;
  MrvlIETypes_KeyParamSet_v2_t *keyparamset;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_SetWEP failed!\n");
    WiFi_FreeArgument((void **)&p, data, status);
    return;
  }
  
  if (p->deleted)
  {
    printf("Removed old WEP key %d\n", p->del_index);
    WiFi_FreeArgument((void **)&p, data, status);
    return;
  }
  
  cmd = (WiFi_Cmd_KeyMaterial *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  cmd->action = WIFI_ACT_REMOVE;
  
  keyparamset = (MrvlIETypes_KeyParamSet_v2_t *)&cmd->keys;
  keyparamset->header.type  = WIFI_MRVLIETYPES_KEYPARAMSETV2;
  keyparamset->header.length = TLV_PAYLOADLEN(*keyparamset);
  memset(keyparamset->mac_address, 0, sizeof(keyparamset->mac_address));
  keyparamset->key_idx = p->del_index;
  keyparamset->key_type = WIFI_KEYTYPE_WEP;
  keyparamset->key_info = 0;
  
  p->deleted = 1;
  WiFi_SetCommandHeader(&cmd->header, p->base.bss, CMD_802_11_KEY_MATERIAL, TLV_NEXT(keyparamset) - (uint8_t *)cmd, 1);
  WiFi_SendCommand(WiFi_SetWEP_Callback, p);
}

/* ����WPA���� */
// �����е�����TLV����ȱһ����, �����̼������ڹ����ȵ�֮ǰ�Ͱѷ�ʱ��PTK��Կ�����
// ���ֻ��passphrase��TLV, ��PTK��Կ�ļ�������Ƴٵ������ȵ�֮��, �п��ܵ���·��������֤��ʱ
void WiFi_SetWPA(bss_t bss, const char *ssid, const uint8_t bssid[WIFI_MACADDR_LEN], const char *password, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_SupplicantPMK *cmd;
  MrvlIETypes_SSIDParamSet_t *ssid_param;
  MrvlIETypes_BSSIDList_t *bssid_list;
  MrvlIETypes_WPA_Passphrase_t *passphrase;
  
  cmd = (WiFi_Cmd_SupplicantPMK *)WiFi_GetCommandBuffer(callback, arg);
  if (cmd == NULL)
    return;
  
  cmd->action = WIFI_ACT_SET;
  cmd->cache_result = 0; // �����Ӧ��, 0��ʾ�ɹ�, 1��ʾʧ��
  
  ssid_param = (MrvlIETypes_SSIDParamSet_t *)(cmd + 1);
  ssid_param->header.type = WIFI_MRVLIETYPES_SSIDPARAMSET;
  ssid_param->header.length = strlen(ssid);
  if (ssid_param->header.length > sizeof(ssid_param->ssid))
    ssid_param->header.length = sizeof(ssid_param->ssid);
  memcpy(ssid_param->ssid, ssid, ssid_param->header.length);
  
  bssid_list = (MrvlIETypes_BSSIDList_t *)TLV_NEXT(ssid_param);
  bssid_list->header.type = WIFI_MRVLIETYPES_BSSIDLIST;
  bssid_list->header.length = TLV_PAYLOADLEN(*bssid_list);
  memcpy(bssid_list->mac_address[0], bssid, WIFI_MACADDR_LEN);
  
  passphrase = (MrvlIETypes_WPA_Passphrase_t *)TLV_NEXT(bssid_list);
  passphrase->header.type = WIFI_MRVLIETYPES_WPAPASSPHRASE;
  passphrase->header.length = strlen(password);
  strcpy((char *)passphrase->passphrase, password);
  
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_SUPPLICANT_PMK, TLV_NEXT(passphrase) - (uint8_t *)cmd, 1);
  WiFi_SendCommand(callback, arg);
}

/* ��ʾ��Կ */
void WiFi_ShowKeyMaterials(bss_t bss)
{
  WiFi_Cmd_KeyMaterial *cmd;
  
  cmd = (WiFi_Cmd_KeyMaterial *)WiFi_GetCommandBuffer(NULL, NULL);
  if (cmd == NULL)
    return;
  
  cmd->action = WIFI_ACT_GET;
  cmd->keys.header.type = WIFI_MRVLIETYPES_KEYPARAMSET;
  cmd->keys.header.length = 6; // ���TLV����ֻ�ܰ������������ֶ�
  cmd->keys.key_type_id = 0;
  cmd->keys.key_info = WIFI_KEYINFO_UNICASTKEY | WIFI_KEYINFO_MULTICASTKEY; // ͬʱ��ȡPTK��GTK��Կ
  cmd->keys.key_len = 0;
  
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_802_11_KEY_MATERIAL, cmd->keys.key - (uint8_t *)cmd, 1);
  WiFi_SendCommand(WiFi_ShowKeyMaterials_Callback, NULL);
}

static void WiFi_ShowKeyMaterials_Callback(void *arg, void *data, WiFi_Status status)
{
  uint8_t keyparam_len;
  WiFi_Cmd_KeyMaterial *resp = (WiFi_Cmd_KeyMaterial *)data;
  MrvlIETypes_KeyParamSet_t *key = &resp->keys;
  MrvlIETypes_StaMacAddr_t *mac;
  WiFi_KeyParam_WEP_v1 *keyparam;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_ShowKeyMaterials failed!\n");
    return;
  }
  
  // ������л�ȡ������Կ
  // info=0x6Ϊ������Կ(PTK), info=0x5Ϊ�㲥��Կ(GTK)
  while ((uint8_t *)key < (uint8_t *)&resp->header + resp->header.size)
  {
    if (key->key_type_id == WIFI_KEYTYPE_WEP)
      printf("[WEP] ");
    else
    {
      if (key->key_info & WIFI_KEYINFO_UNICASTKEY)
        printf("[PTK] ");
      else if (key->key_info & WIFI_KEYINFO_MULTICASTKEY)
        printf("[GTK] ");
    }
    printf("type=%d, info=0x%x, len=%d", key->key_type_id, key->key_info, key->key_len);
    keyparam_len = key->header.length - (TLV_PAYLOADLEN(*key) - sizeof(key->key)); // key->key�������ʵ��С(�п��ܲ�����key->key_len)
    
    if (key->key_type_id == WIFI_KEYTYPE_WEP)
    {
      keyparam = (WiFi_KeyParam_WEP_v1 *)key->key;
      printf(", index=%d, is_default=%d\n", keyparam->wep_key_index, keyparam->is_default_tx_key);
      WiFi_LowLevel_Dump(keyparam->wep_key, key->key_len);
    }
    else
    {
      printf("\n");
      WiFi_LowLevel_Dump(key->key, keyparam_len);
    }
    
    // ת����һ����Կ+MAC��ַ���
    mac = (MrvlIETypes_StaMacAddr_t *)TLV_NEXT(key);
    key = (MrvlIETypes_KeyParamSet_t *)TLV_NEXT(mac);
  }
  
  if (key == &resp->keys)
    printf("No key materials!\n");
}

/* ͬ���ಥMAC��ַ�б� */
void WiFi_SynchronizeMulticastFilter(bss_t bss)
{
  WiFi_Cmd_MulticastAdr *cmd;
  
  if (wifi_mcast_filter.state != WIFI_MCASTFILTERSTATE_MODIFIED)
    return;
  if (WiFi_IsCommandBusy())
    return;
  
  cmd = (WiFi_Cmd_MulticastAdr *)WiFi_GetCommandBuffer(NULL, NULL);
  if (cmd == NULL)
    return;
  cmd->action = WIFI_ACT_SET;
  cmd->num_of_adrs = wifi_mcast_filter.num;
  if (wifi_mcast_filter.num != 0)
    memcpy(cmd->mac_list, wifi_mcast_filter.list, wifi_mcast_filter.num * WIFI_MACADDR_LEN);
  
  printf("Synchronizing multicast filter! num=%u\n", wifi_mcast_filter.num);
  wifi_mcast_filter.state = WIFI_MCASTFILTERSTATE_SUBMITTED;
  WiFi_SetCommandHeader(&cmd->header, bss, CMD_MAC_MULTICAST_ADR, cmd->mac_list[cmd->num_of_adrs] - (uint8_t *)cmd, 1);
  WiFi_SendCommand(WiFi_SynchronizeMulticastFilter_Callback, NULL);
}

static void WiFi_SynchronizeMulticastFilter_Callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
  {
    if (wifi_mcast_filter.state == WIFI_MCASTFILTERSTATE_SUBMITTED)
    {
      // ���û���µ��޸�, ��ô�ͱ��Ϊ�������
      wifi_mcast_filter.state = WIFI_MCASTFILTERSTATE_SYNCHRONIZED;
      printf("Multicast filter is synchronized! num=%u\n", wifi_mcast_filter.num);
    }
    else
      printf("Multicast filter will be synchronized again! num=%u\n", wifi_mcast_filter.num);
  }
  else
  {
    wifi_mcast_filter.state = WIFI_MCASTFILTERSTATE_MODIFIED;
    printf("Failed to synchronize multicast filter! status=%d\n", status);
  }
}
