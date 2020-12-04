#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

static uint32_t wifi_port; // 命令通道的寄存器地址

/* 固件下载 */
// WiFi模块必须要有固件才能正常工作, 固件是由Marvell公司(WiFi模块的制造商)开发的
// 固件内容是保存在STM32单片机的Flash里面的, Flash里面保存的内容在断电时不会丢失
// 通电时(或单片机复位后), 单片机把保存在Flash里面的固件通过SPI或SDIO接口发送给WiFi模块
// 存到WiFi模块的SRAM(内存)里面运行, SRAM里面的内容一断电或复位就会丢失
// 所以每次断电或复位后都要重新下载固件
void WiFi_DownloadFirmware(void)
{
  const uint8_t *data;
  int i, ret;
  uint16_t curr, retry;
  uint32_t len;
  
  if (WiFi_GetFirmwareStatus() == WIFI_FIRMWARESTATUS_OK)
  {
    // PDN引脚没有正确连接到单片机上, 单片机复位时WiFi模块就无法跟着复位
    // 所以这个时候不需要重新下载固件
    // WiFi模块原有的状态不变 (比如已连上的热点, 已发送但未接收回应的命令)
    // 这个时候继续运行下去, 很可能会出错
    printf("%s: Reset signal has no effect!\n", __FUNCTION__);
    printf("Please make sure PDN pin is connected to the MCU!\n");
    return;
  }
  
  // 下载固件
  data = WIFI_FIRMWARE_ADDR; // 该地址必须是4字节对齐的, 否则将会引起传输错误
  len = WIFI_FIRMWARE_SIZE;
  while (len)
  {
    // 获取本次应下载的字节数
    // 每次可以发送n>=curr字节的数据, 只能用一个CMD53命令发送, WiFi模块只认前curr字节的数据
    // 88W8801不会出现curr=0的情况, 所以这里不需要while语句
    curr = WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR0) | (WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR1) << 8);
#if WIFI_DISPLAY_FIRMWARE_DNLD
    printf("Required: %d bytes, Remaining: %d bytes\n", curr, len);
#endif
    
    // 发送固件数据
    i = 0;
    do
    {
      ret = WiFi_LowLevel_WriteData(1, wifi_port, data, curr, 0, WIFI_RWDATA_ALLOWMULTIBYTE);
      if (ret == -2)
        break; // 参数有误
      
      ret = WiFi_Wait(WIFI_INTSTATUS_DNLD, 100); // 等待响应
      retry = WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR0);
      if (retry & 1)
      {
        ret = -1; // 如果寄存器的值为奇数, 说明模块收到的固件内容有误, 需要重传
        retry |= WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR1) << 8;
        printf("%s: Data were not recognized! curr=%d, retry=%d, remaining=%d\n", __FUNCTION__, curr, retry, len);
      }
      
      i++;
    } while (ret == -1 && i < WIFI_LOWLEVEL_MAXRETRY); // 如果成功就跳出循环, 失败就重传
    
    if (ret != 0)
    {
      printf("Failed to download firmware!\n");
      abort();
    }
    
    len -= curr;
    data += curr;
  }
  
  // 等待Firmware启动
  while (WiFi_GetFirmwareStatus() != WIFI_FIRMWARESTATUS_OK);
  printf("Firmware is successfully downloaded!\n");
}

/* 获取固件状态 */
uint16_t WiFi_GetFirmwareStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_0) | (WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_1) << 8);
}

/* 计算通道所对应的SDIO寄存器地址 */
uint32_t WiFi_GetPortAddress(port_t port)
{
  return wifi_port + port;
}

/* 初始化WiFi模块 */
void WiFi_Init(void)
{
  // 初始化底层寄存器
  WiFi_LowLevel_Init();
  
  // 显示设备信息
  WiFi_LowLevel_SetBlockSize(0, 8);
  WiFi_ShowCIS();
  
  // 初始化Function 1
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_IOEN, SDIO_CCCR_IOEN_IOE1); // IOE1=1 (Enable Function)
  while ((WiFi_LowLevel_ReadReg(0, SDIO_CCCR_IORDY) & SDIO_CCCR_IORDY_IOR1) == 0); // 等待IOR1=1 (I/O Function Ready)
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_INTEN, SDIO_CCCR_INTEN_IENM | SDIO_CCCR_INTEN_IEN1); // 打开SDIO中断请求
  WiFi_LowLevel_WriteReg(1, WIFI_INTMASK, WIFI_INTMASK_HOSTINTMASK); // 利用中断标志位来判定是否有数据要读取, 可靠性更高
  
  // 下载固件
  wifi_port = WiFi_LowLevel_ReadReg(1, WIFI_IOPORT0) | (WiFi_LowLevel_ReadReg(1, WIFI_IOPORT1) << 8) | (WiFi_LowLevel_ReadReg(1, WIFI_IOPORT2) << 16);
  WiFi_LowLevel_SetBlockSize(1, 32);
  WiFi_DownloadFirmware();
  WiFi_LowLevel_SetBlockSize(1, 256);
}

/* 显示WiFi模块信息 */
void WiFi_ShowCIS(void)
{
  char *p;
  uint8_t data[255];
  uint8_t func, i, n, len;
  uint8_t tpl_code, tpl_link; // 16.2 Basic Tuple Format and Tuple Chain Structure
  uint32_t cis_ptr;
  
  // 注意: 下载固件之前, 最好不要执行CMD53
  // 否则会干扰WIFI_SQREADBASEADDR0~1寄存器的正常行为, 导致固件无法下载
  n = WiFi_LowLevel_GetFunctionNum();
  for (func = 0; func <= n; func++)
  {
    // 获取CIS的地址
    cis_ptr = (func << 8) | 0x9;
    cis_ptr  = WiFi_LowLevel_ReadReg(0, cis_ptr) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 1) << 8) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 2) << 16);
    printf("[CIS] func=%d, ptr=0x%08x\n", func, cis_ptr);
    
    // 遍历CIS, 直到尾节点
    while ((tpl_code = WiFi_LowLevel_ReadReg(0, cis_ptr++)) != CISTPL_END)
    {
      if (tpl_code == CISTPL_NULL)
        continue;
      
      tpl_link = WiFi_LowLevel_ReadReg(0, cis_ptr++); // 本结点数据的大小
      for (i = 0; i < tpl_link; i++)
        data[i] = WiFi_LowLevel_ReadReg(0, cis_ptr + i);
      
      switch (tpl_code)
      {
        case CISTPL_VERS_1:
          printf("Product Information:");
          for (p = (char *)data + 2; p[0] != '\0'; p += len + 1)
          {
            // 遍历所有字符串
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
        break; // 当TPL_LINK为0xff时说明当前结点为尾节点
      cis_ptr += tpl_link;
    }
  }
}
