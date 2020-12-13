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
static WiFi_STAInfo wifi_sta_info; // 当前连接的热点信息

/* 关联一个热点并输入密码 */
// 连接WPA型的热点时, security成员直接赋值WIFI_SECURITYTYPE_WPA即可, 不需要明确指出WPA版本号
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
  p->retry = max_retry; // 最大尝试重新连接的次数, -1表示无限次数, 0表示不重试
  
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
    // 关联命令执行完毕并收到了回应, 现在需要检查是否关联成功
    resp = (WiFi_CmdResponse_Associate *)data;
#if WIFI_DISPLAY_ASSOCIATE_RESULT
    printf("capability=0x%04x, status_code=0x%04x, aid=0x%04x\n", resp->capability, resp->status_code, resp->association_id);
#endif
    
    real_sec_type = WiFi_GetSecurityType(&wifi_sta_info.info);
    if (resp->association_id == 0xffff)
      status = WIFI_STATUS_FAIL; // 关联失败 (在回调函数的data中检查resp->capability和resp->status_code的值可获得详细原因)
    else if (real_sec_type == WIFI_SECURITYTYPE_WPA || real_sec_type == WIFI_SECURITYTYPE_WPA2)
      status = WIFI_STATUS_INPROGRESS; // 等待认证
  }
  
  if ((cmd_code == CMD_802_11_ASSOCIATE && status != WIFI_STATUS_OK && status != WIFI_STATUS_INPROGRESS)
      || (cmd_code == CMD_802_11_SCAN && status != WIFI_STATUS_OK))
  {
    // 关联失败, 重试
    if (p->retry != 0)
    {
      if (p->retry != -1)
        p->retry--;
      
      // 回退
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
    // 关联成功
    wifi_sta_info.state = WIFI_STASTATE_CONNECTED;
    WiFi_FreeArgument((void **)&p, data, status);
    return;
  }
  
  if (status != WIFI_STATUS_OK)
  {
    // 关联失败
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
        // WPA网络必须在命令中加入Vendor参数才能成功连接
        vendor = (MrvlIETypes_VendorParamSet_t *)(auth + 1);
        memcpy(vendor, &wifi_sta_info.info.wpa, TLV_STRUCTLEN(wifi_sta_info.info.wpa));
        cmd_size += TLV_STRUCTLEN(wifi_sta_info.info.wpa);
      }
      else if (real_sec_type == WIFI_SECURITYTYPE_WPA2)
      {
        // WPA2网络必须在命令中加入RSN参数才能成功连接
        rsn = (MrvlIETypes_RsnParamSet_t *)(auth + 1);
        memcpy(rsn, &wifi_sta_info.info.rsn, TLV_STRUCTLEN(wifi_sta_info.info.rsn));
        cmd_size += TLV_STRUCTLEN(wifi_sta_info.info.rsn);
      }
      
      WiFi_SetCommandHeader(&cmd->header, p->base.bss, CMD_802_11_ASSOCIATE, cmd_size, 1);
      WiFi_SendCommand(WiFi_Associate_Callback, arg);
      // 保留arg内存, 等关联成功后再释放
      break;
  }
}

/* 检查输入的WEP密钥是否合法, 并将十六进制字符串密钥转换为二进制密钥 */
// dest不会以\0结束, 返回值为密钥的真正长度
int WiFi_CheckWEPKey(uint8_t *dest, const char *src)
{
  int i, len = 0;
  uint32_t temp;
  
  if (src != NULL)
  {
    len = strlen(src);
    if (len == 5 || len == 13)
      memcpy(dest, src, len); // 5个或13个ASCII密钥字符
    else if (len == 10 || len == 26)
    {
      // 10个或26个16进制密钥字符
      for (i = 0; i < len; i += 2)
      {
        if (!isxdigit(src[i]) || !isxdigit(src[i + 1]))
          return -1; // 密钥中含有非法字符
        
        sscanf(src + i, "%02x", &temp);
        dest[i / 2] = temp;
      }
      len /= 2;
    }
    else
      len = 0; // 长度不合法
  }
  return len;
}

/* 意外断开连接 */
void WiFi_ConnectionLost(void)
{
  wifi_sta_info.state = WIFI_STASTATE_IDLE;
}

/* 与热点断开连接 */
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

/* 获取所连热点的MAC地址 */
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

/* 获取热点的认证类型 */
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

/* 获取连接状态 */
WiFi_STAState WiFi_GetSTAState(void)
{
  return wifi_sta_info.state;
}

/* 加入多播组 */
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

/* 离开多播组 */
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

/* 获取或设置MAC地址 */
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

/* 配置MAC */
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

/* 显示速率列表 */
void WiFi_PrintRates(uint16_t rates)
{
  // 速率列表在固件API手册的3.1 Receive Packet Descriptor一节里面的RxRate字段描述里面
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

/* 显示数据帧的发送情况 */
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

/* 扫描全部热点 (仅显示) */
void WiFi_Scan(WiFi_Callback callback, void *arg)
{
  int i;
  WiFi_ArgBase *p;
  WiFi_CmdRequest_Scan *cmd; // 要发送的命令
  MrvlIETypes_ChanListParamSet_t *chanlist;

  p = WiFi_AllocateArgument(sizeof(WiFi_ArgBase), __FUNCTION__, 0, callback, arg);
  if (p == NULL)
    return;
  
  cmd = (WiFi_CmdRequest_Scan *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  
  cmd->bss_type = WIFI_BSS_ANY;
  memset(cmd->bss_id, 0, sizeof(cmd->bss_id));
  
  // 通道的基本参数
  chanlist = (MrvlIETypes_ChanListParamSet_t *)(cmd + 1); // 这里的+1指的是前进sizeof(指针类型)个地址单元, 而非只前进1个地址单元
  chanlist->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chanlist->header.length = 4 * sizeof(chanlist->channels);
  for (i = 0; i < 4; i++) // 先扫描前4个通道 (i作下标, i+1才是通道号)
  {
    chanlist->channels[i].band_config_type = 0; // 2.4GHz band, 20MHz channel width
    chanlist->channels[i].chan_number = i + 1; // 通道号
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
  
  // 发送扫描接下来的4个通道的命令
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
  
  // 显示本次扫描结果, num_of_set为热点数
  if (resp->num_of_set > 0)
  {
    bss_desc_set = (WiFi_BssDescSet *)(resp + 1);
    
#if WIFI_DISPLAY_SCAN_DETAILS
    // resp->buf_size就是bss_desc_set的总大小
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
            // SSID名称
            len = ie_params->header.length;
            if (len > WIFI_MAX_SSIDLEN)
              len = WIFI_MAX_SSIDLEN;
            memcpy(ssid, ie_params->data, len);
            ssid[len] = '\0';
            break;
#if WIFI_DISPLAY_SCAN_DETAILS
          case WIFI_MRVLIETYPES_RATESPARAMSET:
            // 速率
            rates = ie_params;
            break;
#endif
          case WIFI_MRVLIETYPES_PHYPARAMDSSET:
            // 通道号
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
      
      printf("SSID '%s', ", ssid); // 热点名称
      printf("MAC %02X:%02X:%02X:%02X:%02X:%02X, ", bss_desc_set->bssid[0], bss_desc_set->bssid[1], bss_desc_set->bssid[2], bss_desc_set->bssid[3], bss_desc_set->bssid[4], bss_desc_set->bssid[5]); // MAC地址
      printf("RSSI %d, Channel %d\n", bss_desc_set->rssi, channel); // 信号强度和通道号
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
        // 关于速率值的表示方法, 请参阅802.11-2016.pdf的9.4.2.3 Supported Rates and BSS Membership Selectors element一节的内容
        printf("  Rates:");
        for (j = 0; j < rates->header.length; j++)
          printf(" %.1fMbps", (rates->data[j] & 0x7f) * 0.5);
        printf("\n");
      }
#endif
      
      // 转向下一个热点信息
      bss_desc_set = (WiFi_BssDescSet *)((uint8_t *)bss_desc_set + sizeof(bss_desc_set->ie_length) + bss_desc_set->ie_length);
    }
  }
  
  // 扫描完毕时调用回调函数
  if (n == 0)
    WiFi_FreeArgument((void **)&p, data, status);
}

/* 获取指定名称的热点的详细信息 */
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
  memset(info, 0, sizeof(WiFi_SSIDInfo)); // 将整个info结构体清零
  
  cmd = (WiFi_CmdRequest_Scan *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  cmd->bss_type = WIFI_BSS_ANY;
  memset(cmd->bss_id, 0, sizeof(cmd->bss_id));
  
  // 给info->ssid成员赋值
  info->ssid.header.type = WIFI_MRVLIETYPES_SSIDPARAMSET;
  info->ssid.header.length = strlen(ssid);
  memcpy(info->ssid.ssid, ssid, info->ssid.header.length);
  memcpy(cmd + 1, &info->ssid, TLV_STRUCTLEN(info->ssid)); // 把info->ssid复制到待发送的命令内容中
  
  chan_list = (MrvlIETypes_ChanListParamSet_t *)((uint8_t *)(cmd + 1) + TLV_STRUCTLEN(info->ssid));
  chan_list->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chan_list->header.length = 14 * sizeof(chan_list->channels); // 一次性扫描14个通道
  for (i = 0; i < 14; i++) // i作下标, i+1才是通道号
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
    // 未找到指定的AP热点, 此时info结构体中除了ssid成员外, 其余的成员均为0
    // resp中的内容到了num_of_set成员后就没有了
    status = WIFI_STATUS_NOTFOUND;
    printf("SSID not found!\n");
    goto end;
  }
  
  // bss_desc_set以扫描到的第一个信息项为准
  memcpy(p->info->mac_addr, bss_desc_set->bssid, sizeof(p->info->mac_addr));
  p->info->cap_info = bss_desc_set->cap_info;
  p->info->bcn_period = bss_desc_set->bcn_interval;
  
  // 若info->xxx.header.type=0, 则表明没有该项的信息 (除SSID结构体外, 因为SSID的type=WIFI_MRVLIETYPES_SSIDPARAMSET=0)
  ie_params = &bss_desc_set->ie_parameters;
  ie_size = bss_desc_set->ie_length - (sizeof(WiFi_BssDescSet) - sizeof(bss_desc_set->ie_length) - sizeof(bss_desc_set->ie_parameters)); // 所有IEEE_Type数据的总大小
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
        // 速率
        WiFi_TranslateTLV((MrvlIEType *)&p->info->rates, ie_params, sizeof(p->info->rates.rates));
        break;
      case WIFI_MRVLIETYPES_PHYPARAMDSSET:
        // 通道号
        p->info->channel = ie_params->data[0];
        break;
      case WIFI_MRVLIETYPES_RSNPARAMSET:
        // 通常只有一个RSN信息 (与WPA2相关)
        WiFi_TranslateTLV((MrvlIEType *)&p->info->rsn, ie_params, sizeof(p->info->rsn.rsn));
        break;
      case WIFI_MRVLIETYPES_VENDORPARAMSET:
        // 通常会有多项VENDOR信息 (与WPA相关)
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
              if (ie_params->header.length == 24) // 合法大小
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
    
    // 转向下一个TLV
    ie_size -= TLV_STRUCTLEN(*ie_params);
    ie_params = (IEEEType *)TLV_NEXT(ie_params);
  }
  
end:
  WiFi_FreeArgument((void **)&p, data, status);
}

/* 使用CMD_802_11_KEY_MATERIAL命令设置WEP密钥 */
// 仅支持单个密钥
void WiFi_SetWEP(bss_t bss, const WiFi_WEPKey *key, WiFi_Callback callback, void *arg)
{
  static int last = -1;
  int len;
  WiFi_Arg_SetWEP *p;
  WiFi_Cmd_KeyMaterial *cmd;
  MrvlIETypes_KeyParamSet_v2_t *keyparamset; // 设置密钥时必须使用V2版本的TLV参数
  WiFi_KeyParam_WEP_v2 *keyparam;
  
  cmd = (WiFi_Cmd_KeyMaterial *)WiFi_GetCommandBuffer(callback, arg);
  if (cmd == NULL)
    return;
  cmd->action = WIFI_ACT_SET;
  
  keyparamset = (MrvlIETypes_KeyParamSet_v2_t *)&cmd->keys;
  keyparamset->header.type = WIFI_MRVLIETYPES_KEYPARAMSETV2;
  keyparamset->header.length = TLV_PAYLOADLEN(*keyparamset);
  memset(keyparamset->mac_address, 0, sizeof(keyparamset->mac_address)); // 如果MAC地址不为0, 则必须要连接了热点之后才能设置成功
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
  
  // 待删除密钥的序号
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
  // 移除旧密钥
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

/* 设置WPA密码 */
// 命令中的三个TLV参数缺一不可, 这样固件才能在关联热点之前就把费时的PTK密钥计算好
// 如果只有passphrase的TLV, 则PTK密钥的计算必须推迟到关联热点之后, 有可能导致路由器端认证超时
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
  cmd->cache_result = 0; // 命令回应中, 0表示成功, 1表示失败
  
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

/* 显示密钥 */
void WiFi_ShowKeyMaterials(bss_t bss)
{
  WiFi_Cmd_KeyMaterial *cmd;
  
  cmd = (WiFi_Cmd_KeyMaterial *)WiFi_GetCommandBuffer(NULL, NULL);
  if (cmd == NULL)
    return;
  
  cmd->action = WIFI_ACT_GET;
  cmd->keys.header.type = WIFI_MRVLIETYPES_KEYPARAMSET;
  cmd->keys.header.length = 6; // 这个TLV参数只能包含下面三个字段
  cmd->keys.key_type_id = 0;
  cmd->keys.key_info = WIFI_KEYINFO_UNICASTKEY | WIFI_KEYINFO_MULTICASTKEY; // 同时获取PTK和GTK密钥
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
  
  // 输出所有获取到的密钥
  // info=0x6为单播密钥(PTK), info=0x5为广播密钥(GTK)
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
    keyparam_len = key->header.length - (TLV_PAYLOADLEN(*key) - sizeof(key->key)); // key->key数组的真实大小(有可能不等于key->key_len)
    
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
    
    // 转到下一对密钥+MAC地址组合
    mac = (MrvlIETypes_StaMacAddr_t *)TLV_NEXT(key);
    key = (MrvlIETypes_KeyParamSet_t *)TLV_NEXT(mac);
  }
  
  if (key == &resp->keys)
    printf("No key materials!\n");
}

/* 同步多播MAC地址列表 */
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
      // 如果没有新的修改, 那么就标记为更新完毕
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
