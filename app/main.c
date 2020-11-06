/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: main.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "stm32f4xx.h"
#include "vtype.h"
#include "usart.h"
#include "vos.h"

int kprintf(char* format, ...);
#define printf kprintf

void lwip_test();

s32 NetDhcpClient(u32 timeout);
void emWinTest();
int usb2uart_test();
#if 0
#include "usbh_core.h"
#include "usbh_msc.h"
typedef enum {
  APPLICATION_IDLE = 0,
  APPLICATION_READY,
  APPLICATION_DISCONNECT,
}MSC_ApplicationTypeDef;

//FRESULT Explore_Disk(char *path, uint8_t recu_level);

USBH_HandleTypeDef hUSBHost;
MSC_ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/**
  * @brief  User Process
  * @param  phost: Host Handle
  * @param  id: Host Library user message ID
  * @retval None
  */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  switch(id)
  {
  case HOST_USER_SELECT_CONFIGURATION:
    break;

  case HOST_USER_DISCONNECTION:
//    Appli_state = APPLICATION_DISCONNECT;
//    if (FATFS_UnLinkDriver(USBDISKPath) == 0)
//    {
//      if(f_mount(NULL, "", 0) != FR_OK)
//      {
//        LCD_ErrLog("ERROR : Cannot DeInitialize FatFs! \n");
//      }
//    }
    break;

  case HOST_USER_CLASS_ACTIVE:
    Appli_state = APPLICATION_READY;
    break;

  case HOST_USER_CONNECTION:
//    if (FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
//    {
//      if (f_mount(&USBH_fatfs, "", 0) != FR_OK)
//      {
//        LCD_ErrLog("ERROR : Cannot Initialize FatFs! \n");
//      }
//    }
    break;

  default:
    break;
  }
}
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"
USBD_HandleTypeDef  USBD_Device;
void udisk_test()
{

	  /* Init Device Library */
	  USBD_Init(&USBD_Device, &VCP_Desc, 0);

	  /* Add Supported Class */
	  USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);

	  /* Add CDC Interface Class */
	  USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);

	  /* Start Device Process */
	  USBD_Start(&USBD_Device);

	  /* Run Application (Interrupt mode) */
	  while (1)
	  {
		  VOSTaskDelay(100);
	  }
#if 0
	  //HAL_Init();

	  /* Configure the system clock to 168 MHz */
	  //SystemClock_Config();

	  /* Init MSC Application */
	  //MSC_InitApplication();

	  /* Init Host Library */
	  USBH_Init(&hUSBHost, USBH_UserProcess, 0);

	  /* Add Supported Class */
	  USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS);

	  /* Start Host Process */
	  USBH_Start(&hUSBHost);

	  /* Run Application (Blocking mode) */
	  while (1)
	  {
	    /* USB Host Background task */
	    USBH_Process(&hUSBHost);

	    /* MSC Menu Process */
	    //MSC_MenuProcess();
	  }
#endif
}


void HAL_Delay(uint32_t Delay)
{
	VOSTaskDelay(Delay);
}
#endif
void main(void *param)
{
	s32 res;

	void uart_init(u32 bound);
	uart_init(115200);
	kprintf("hello world!\r\n");
	dma_printf("=============%d============\r\n", 100);
	TIM3_Init(5000,9000);
	uart_test();
#if 0
	kprintf("hello!\r\n");

	udisk_test();

	usb2uart_test();

	emWinTest();

	NetDhcpClient(30*1000);
	//SetNetWorkInfo ("192.168.2.150", "255.255.255.0", "192.168.2.101");
	lwip_inner_test();
	//sock_tcp_test();

//	kprintf("total: %d, free: %d!\r\n", total, free);
	dma_printf("hello world!\r\n");
	kprintf("main function!\r\n");
#endif
	while (1) {
		VOSTaskDelay(1*1000);
	}

}
