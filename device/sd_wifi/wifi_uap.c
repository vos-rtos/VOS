#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

static void WiFi_ShowAPClientList_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_ShowAPSysInfo_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_StartMicroAP_Callback(void *arg, void *data, WiFi_Status status);

/* 强制断开客户端的连接 */
void WiFi_DeauthenticateClient(bss_t bss, uint8_t mac_addr[WIFI_MACADDR_LEN], uint16_t reason, WiFi_Callback callback, void *arg)
{
  WiFi_APCmd_STADeauth *cmd;
  
  cmd = (WiFi_APCmd_STADeauth *)WiFi_GetCommandBuffer(callback, arg);
  if (cmd == NULL)
    return;
  memcpy(cmd->sta_mac_address, mac_addr, WIFI_MACADDR_LEN);
  cmd->reason = reason;
  
  WiFi_SetCommandHeader(&cmd->header, bss, APCMD_STA_DEAUTH, sizeof(WiFi_APCmd_STADeauth), 1);
  WiFi_SendCommand(callback, arg);
}

/* 显示客户端列表 */
void WiFi_ShowAPClientList(bss_t bss, WiFi_Callback callback, void *arg)
{
  WiFi_ArgBase *p;
  WiFi_CommandHeader *cmd;
  
  p = WiFi_AllocateArgument(sizeof(WiFi_ArgBase), __FUNCTION__, bss, callback, arg);
  if (p == NULL)
    return;
  
  cmd = WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  
  WiFi_SetCommandHeader(cmd, bss, APCMD_STA_LIST, sizeof(WiFi_CommandHeader), 1);
  WiFi_SendCommand(WiFi_ShowAPClientList_Callback, p);
}

static void WiFi_ShowAPClientList_Callback(void *arg, void *data, WiFi_Status status)
{
  int i;
  WiFi_APCmdResponse_STAList *resp = data;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_ShowAPClientList failed! status=%d\n", status);
    goto end;
  }
  
  printf("[AP STAList] count=%u\n", resp->sta_count);
  for (i = 0; i < resp->sta_count; i++)
  {
    printf("Client %02X:%02X:%02X:%02X:%02X:%02X", resp->sta_list[i].mac_address[0], resp->sta_list[i].mac_address[1],
      resp->sta_list[i].mac_address[2], resp->sta_list[i].mac_address[3], resp->sta_list[i].mac_address[4], 
      resp->sta_list[i].mac_address[5]);
    
    printf(", RSSI %d", resp->sta_list[i].rssi);
    if (resp->sta_list[i].power_mgt_status == 1)
      printf(" (Power Save)");
    printf("\n");
  }
  
end:
  WiFi_FreeArgument(&arg, data, status);
}

/* 显示uAP系统信息 */
void WiFi_ShowAPSysInfo(bss_t bss, WiFi_Callback callback, void *arg)
{
  WiFi_ArgBase *p;
  WiFi_CommandHeader *cmd;
  
  p = WiFi_AllocateArgument(sizeof(WiFi_ArgBase), __FUNCTION__, bss, callback, arg);
  if (p == NULL)
    return;
  
  cmd = WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  
  WiFi_SetCommandHeader(cmd, bss, APCMD_SYS_INFO, sizeof(WiFi_CommandHeader), 1);
  WiFi_SendCommand(WiFi_ShowAPSysInfo_Callback, p);
}

static void WiFi_ShowAPSysInfo_Callback(void *arg, void *data, WiFi_Status status)
{
  int len;
  WiFi_APCmdResponse_SysInfo *resp = data;
  
  if (status == WIFI_STATUS_OK)
  {
    len = resp->header.header.length - sizeof(WiFi_CommandHeader); // 根据命令回应的大小算出字符串长度 (字符串不含\0)
    printf("[AP SysInfo] len=%d\n", len);
    printf("%.*s\n", len, resp->sys_info);
  }
  else
    printf("WiFi_ShowAPSysInfo failed! status=%d\n", status);
  
  WiFi_FreeArgument(&arg, data, status);
}

/* 创建热点 */
void WiFi_StartMicroAP(const WiFi_AccessPoint *ap, uint32_t flags, WiFi_Callback callback, void *arg)
{
  uint8_t *end;
  WiFi_ArgBase *p;
  WiFi_APCmd_SysConfigure *cmd;
  MrvlIETypes_ApBCast_SSID_Ctrl_t *apbcast;
  MrvlIETypes_SSIDParamSet_t *ssid;
  MrvlIETypes_WPA_Passphrase_t *wpa_passphrase;
  MrvlIETypes_EncryptionProtocol_t *encryption;
  MrvlIETypes_AKMP_t *akmp;
  MrvlIETypes_PTK_Cipher_t *ptk_cipher;
  MrvlIETypes_GTK_Cipher_t *gtk_cipher;
  
  if (ap->sec_type == WIFI_SECURITYTYPE_WEP)
  {
    printf("%s: WEP is not supported in AP mode!\n", __FUNCTION__);
    if (callback != NULL)
      callback(arg, NULL, WIFI_STATUS_INVALID);
    return;
  }
  
  p = WiFi_AllocateArgument(sizeof(WiFi_ArgBase), __FUNCTION__, ap->bss, callback, arg);
  if (p == NULL)
    return;
  
  cmd = (WiFi_APCmd_SysConfigure *)WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  cmd->action = WIFI_ACT_SET;
  
  apbcast = (MrvlIETypes_ApBCast_SSID_Ctrl_t *)(cmd + 1);
  apbcast->header.type = WIFI_MRVLIETYPES_APBCASTSSIDCTRL;
  apbcast->header.length = TLV_PAYLOADLEN(*apbcast);
  if (flags & WIFI_MICROAP_BROADCASTSSID)
    apbcast->bcast_ssid_ctl = 1;
  else
    apbcast->bcast_ssid_ctl = 0;
  
  end = TLV_NEXT(apbcast);
  if ((flags & WIFI_MICROAP_USEDEFAULTSSID) == 0)
  {
    ssid = (MrvlIETypes_SSIDParamSet_t *)end;
    ssid->header.type = WIFI_MRVLIETYPES_SSIDPARAMSET;
    ssid->header.length = strlen(ap->ssid);
    memcpy(ssid->ssid, ap->ssid, ssid->header.length);
    end = TLV_NEXT(ssid);
  }
  
  switch (ap->sec_type)
  {
    case WIFI_SECURITYTYPE_WPA:
    case WIFI_SECURITYTYPE_WPA2:
      wpa_passphrase = (MrvlIETypes_WPA_Passphrase_t *)end;
      wpa_passphrase->header.type = WIFI_MRVLIETYPES_WPAPASSPHRASE;
      wpa_passphrase->header.length = strlen(ap->password.wpa);
      memcpy(wpa_passphrase->passphrase, ap->password.wpa, wpa_passphrase->header.length);
      
      encryption = (MrvlIETypes_EncryptionProtocol_t *)TLV_NEXT(wpa_passphrase);
      encryption->header.type = WIFI_MRVLIETYPES_ENCRYPTIONPROTOCOL;
      encryption->header.length = TLV_PAYLOADLEN(*encryption);
      if (ap->sec_type == WIFI_SECURITYTYPE_WPA)
        encryption->protocol = WIFI_ENCRYPTIONPROTOCOL_WPA;
      else if (ap->sec_type == WIFI_SECURITYTYPE_WPA2)
        encryption->protocol = WIFI_ENCRYPTIONPROTOCOL_WPA2;
      
      akmp = (MrvlIETypes_AKMP_t *)TLV_NEXT(encryption);
      akmp->header.type = WIFI_MRVLIETYPES_AKMP;
      akmp->header.length = TLV_PAYLOADLEN(*akmp);
      akmp->key_mgmt = WIFI_KEY_MGMT_PSK;
      akmp->operation = 0;
      
      ptk_cipher = (MrvlIETypes_PTK_Cipher_t *)akmp;
      ptk_cipher->header.type = WIFI_MRVLIETYPES_PTKCIPHER;
      ptk_cipher->header.length = TLV_PAYLOADLEN(*ptk_cipher);
      ptk_cipher->protocol = encryption->protocol;
      ptk_cipher->pairwise_cipher = WIFI_WPA_CIPHER_CCMP;
      ptk_cipher->reserved = 0;
      
      gtk_cipher = (MrvlIETypes_GTK_Cipher_t *)TLV_NEXT(ptk_cipher);
      gtk_cipher->header.type = WIFI_MRVLIETYPES_GTKCIPHER;
      gtk_cipher->header.length = TLV_PAYLOADLEN(*gtk_cipher);
      gtk_cipher->groupwise_cipher = WIFI_WPA_CIPHER_CCMP;
      gtk_cipher->reserved = 0;
      end = TLV_NEXT(gtk_cipher);
      break;
    default:
      break;
  }
  
  WiFi_SetCommandHeader(&cmd->header, ap->bss, APCMD_SYS_CONFIGURE, end - (uint8_t *)cmd, 1);
  WiFi_SendCommand(WiFi_StartMicroAP_Callback, p);
}

static void WiFi_StartMicroAP_Callback(void *arg, void *data, WiFi_Status status)
{
  uint16_t code;
  WiFi_ArgBase *p = arg;
  WiFi_CommandHeader *cmd;
  
  if (status != WIFI_STATUS_OK)
    goto end;
  
  code = WiFi_GetCommandCode(data);
  if (code == APCMD_BSS_START)
    goto end;
  
  cmd = WiFi_GetSubCommandBuffer((void **)&p);
  if (cmd == NULL)
    return;
  WiFi_SetCommandHeader(cmd, p->bss, APCMD_BSS_START, sizeof(WiFi_CommandHeader), 1);
  WiFi_SendCommand(WiFi_StartMicroAP_Callback, p);
  return;
  
end:
  WiFi_FreeArgument((void **)&p, data, status);
}

/* 关闭热点 */
void WiFi_StopMicroAP(bss_t bss, WiFi_Callback callback, void *arg)
{
  WiFi_CommandHeader *cmd;
  
  cmd = WiFi_GetCommandBuffer(callback, arg);
  if (cmd == NULL)
    return;
  
  WiFi_SetCommandHeader(cmd, bss, APCMD_BSS_STOP, sizeof(WiFi_CommandHeader), 1);
  WiFi_SendCommand(callback, arg);
}
