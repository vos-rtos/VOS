#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

static uint32_t wifi_port; // ����ͨ���ļĴ�����ַ

/* �̼����� */
// WiFiģ�����Ҫ�й̼�������������, �̼�����Marvell��˾(WiFiģ���������)������
// �̼������Ǳ�����STM32��Ƭ����Flash�����, Flash���汣��������ڶϵ�ʱ���ᶪʧ
// ͨ��ʱ(��Ƭ����λ��), ��Ƭ���ѱ�����Flash����Ĺ̼�ͨ��SPI��SDIO�ӿڷ��͸�WiFiģ��
// �浽WiFiģ���SRAM(�ڴ�)��������, SRAM���������һ�ϵ��λ�ͻᶪʧ
// ����ÿ�ζϵ��λ��Ҫ�������ع̼�
void WiFi_DownloadFirmware(void)
{
  const uint8_t *data;
  int i, ret;
  uint16_t curr, retry;
  uint32_t len;
  
  if (WiFi_GetFirmwareStatus() == WIFI_FIRMWARESTATUS_OK)
  {
    // PDN����û����ȷ���ӵ���Ƭ����, ��Ƭ����λʱWiFiģ����޷����Ÿ�λ
    // �������ʱ����Ҫ�������ع̼�
    // WiFiģ��ԭ�е�״̬���� (���������ϵ��ȵ�, �ѷ��͵�δ���ջ�Ӧ������)
    // ���ʱ�����������ȥ, �ܿ��ܻ����
    printf("%s: Reset signal has no effect!\n", __FUNCTION__);
    printf("Please make sure PDN pin is connected to the MCU!\n");
    return;
  }
  
  // ���ع̼�
  data = WIFI_FIRMWARE_ADDR; // �õ�ַ������4�ֽڶ����, ���򽫻����������
  len = WIFI_FIRMWARE_SIZE;
  while (len)
  {
    // ��ȡ����Ӧ���ص��ֽ���
    // ÿ�ο��Է���n>=curr�ֽڵ�����, ֻ����һ��CMD53�����, WiFiģ��ֻ��ǰcurr�ֽڵ�����
    // 88W8801�������curr=0�����, �������ﲻ��Ҫwhile���
    curr = WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR0) | (WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR1) << 8);
#if WIFI_DISPLAY_FIRMWARE_DNLD
    printf("Required: %d bytes, Remaining: %d bytes\n", curr, len);
#endif
    
    // ���͹̼�����
    i = 0;
    do
    {
      ret = WiFi_LowLevel_WriteData(1, wifi_port, data, curr, 0, WIFI_RWDATA_ALLOWMULTIBYTE);
      if (ret == -2)
        break; // ��������
      
      ret = WiFi_Wait(WIFI_INTSTATUS_DNLD, 100); // �ȴ���Ӧ
      retry = WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR0);
      if (retry & 1)
      {
        ret = -1; // ����Ĵ�����ֵΪ����, ˵��ģ���յ��Ĺ̼���������, ��Ҫ�ش�
        retry |= WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR1) << 8;
        printf("%s: Data were not recognized! curr=%d, retry=%d, remaining=%d\n", __FUNCTION__, curr, retry, len);
      }
      
      i++;
    } while (ret == -1 && i < WIFI_LOWLEVEL_MAXRETRY); // ����ɹ�������ѭ��, ʧ�ܾ��ش�
    
    if (ret != 0)
    {
      printf("Failed to download firmware!\n");
      abort();
    }
    
    len -= curr;
    data += curr;
  }
  
  // �ȴ�Firmware����
  while (WiFi_GetFirmwareStatus() != WIFI_FIRMWARESTATUS_OK);
  printf("Firmware is successfully downloaded!\n");
}

/* ��ȡ�̼�״̬ */
uint16_t WiFi_GetFirmwareStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_0) | (WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_1) << 8);
}

/* ����ͨ������Ӧ��SDIO�Ĵ�����ַ */
uint32_t WiFi_GetPortAddress(port_t port)
{
  return wifi_port + port;
}

/* ��ʼ��WiFiģ�� */
void WiFi_Init(void)
{
  // ��ʼ���ײ�Ĵ���
  WiFi_LowLevel_Init();
  
  // ��ʾ�豸��Ϣ
  WiFi_LowLevel_SetBlockSize(0, 8);
  WiFi_ShowCIS();
  
  // ��ʼ��Function 1
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_IOEN, SDIO_CCCR_IOEN_IOE1); // IOE1=1 (Enable Function)
  while ((WiFi_LowLevel_ReadReg(0, SDIO_CCCR_IORDY) & SDIO_CCCR_IORDY_IOR1) == 0); // �ȴ�IOR1=1 (I/O Function Ready)
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_INTEN, SDIO_CCCR_INTEN_IENM | SDIO_CCCR_INTEN_IEN1); // ��SDIO�ж�����
  WiFi_LowLevel_WriteReg(1, WIFI_INTMASK, WIFI_INTMASK_HOSTINTMASK); // �����жϱ�־λ���ж��Ƿ�������Ҫ��ȡ, �ɿ��Ը���
  
  // ���ع̼�
  wifi_port = WiFi_LowLevel_ReadReg(1, WIFI_IOPORT0) | (WiFi_LowLevel_ReadReg(1, WIFI_IOPORT1) << 8) | (WiFi_LowLevel_ReadReg(1, WIFI_IOPORT2) << 16);
  WiFi_LowLevel_SetBlockSize(1, 32);
  WiFi_DownloadFirmware();
  WiFi_LowLevel_SetBlockSize(1, 256);
}

/* ��ʾWiFiģ����Ϣ */
void WiFi_ShowCIS(void)
{
  char *p;
  uint8_t data[255];
  uint8_t func, i, n, len;
  uint8_t tpl_code, tpl_link; // 16.2 Basic Tuple Format and Tuple Chain Structure
  uint32_t cis_ptr;
  
  // ע��: ���ع̼�֮ǰ, ��ò�Ҫִ��CMD53
  // ��������WIFI_SQREADBASEADDR0~1�Ĵ�����������Ϊ, ���¹̼��޷�����
  n = WiFi_LowLevel_GetFunctionNum();
  for (func = 0; func <= n; func++)
  {
    // ��ȡCIS�ĵ�ַ
    cis_ptr = (func << 8) | 0x9;
    cis_ptr  = WiFi_LowLevel_ReadReg(0, cis_ptr) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 1) << 8) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 2) << 16);
    printf("[CIS] func=%d, ptr=0x%08x\n", func, cis_ptr);
    
    // ����CIS, ֱ��β�ڵ�
    while ((tpl_code = WiFi_LowLevel_ReadReg(0, cis_ptr++)) != CISTPL_END)
    {
      if (tpl_code == CISTPL_NULL)
        continue;
      
      tpl_link = WiFi_LowLevel_ReadReg(0, cis_ptr++); // ��������ݵĴ�С
      for (i = 0; i < tpl_link; i++)
        data[i] = WiFi_LowLevel_ReadReg(0, cis_ptr + i);
      
      switch (tpl_code)
      {
        case CISTPL_VERS_1:
          printf("Product Information:");
          for (p = (char *)data + 2; p[0] != '\0'; p += len + 1)
          {
            // ���������ַ���
            len = strlen(p);
            printf(" %s", p);
          }
          printf("\n");
          break;
        case CISTPL_MANFID:
          // 16.6 CISTPL_MANFID: Manufacturer Identification String Tuple
          printf("Manufacturer Code: 0x%04x\n", *(uint16_t *)data); // TPLMID_MANF
          printf("Manufacturer Information: 0x%04x\n", *(uint16_t *)(data + 2)); // TPLMID_CARD
          break;
        case CISTPL_FUNCID:
          // 16.7.1 CISTPL_FUNCID: Function Identification Tuple
          printf("Card Function Code: 0x%02x\n", data[0]); // TPLFID_FUNCTION
          printf("System Initialization Bit Mask: 0x%02x\n", data[1]); // TPLFID_SYSINIT
          break;
        case CISTPL_FUNCE:
          // 16.7.2 CISTPL_FUNCE: Function Extension Tuple
          if (data[0] == 0)
          {
            // 16.7.3 CISTPL_FUNCE Tuple for Function 0 (Extended Data 00h)
            printf("Maximum Block Size: %d\n", *(uint16_t *)(data + 1));
            printf("Maximum Transfer Rate Code: 0x%02x\n", data[3]);
          }
          else
          {
            // 16.7.4 CISTPL_FUNCE Tuple for Function 1-7 (Extended Data 01h)
            printf("Maximum Block Size: %d\n", *(uint16_t *)(data + 0x0c)); // TPLFE_MAX_BLK_SIZE
          }
          break;
        default:
          printf("[CIS Tuple 0x%02x] addr=0x%08x size=%d\n", tpl_code, cis_ptr - 2, tpl_link);
          WiFi_LowLevel_Dump(data, tpl_link);
      }
      
      if (tpl_link == 0xff)
        break; // ��TPL_LINKΪ0xffʱ˵����ǰ���Ϊβ�ڵ�
      cis_ptr += tpl_link;
    }
  }
}
