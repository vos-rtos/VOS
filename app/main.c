

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

#include "usbh_core.h"
#include "usbh_msc.h"

#include "usbd_uart.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

void MX_USB_DEVICE_Init(void);
void udisk_test()
{
	s32 ret = 0;
	s8 buf[100];
	//MX_USB_DEVICE_Init();
	usbd_uart_init();
	while (1)
	{
		ret = usbd_uart_app_gets(buf, sizeof(buf));
		if (ret > 0) {
			usbd_uart_app_puts(buf, ret);
		}
		else
		{
			VOSTaskDelay(100);
		}
	}
}


void HAL_Delay(uint32_t Delay)
{
#if 0
	VOSTaskDelay(Delay);
#else
	VOSDelayUs(Delay*1000);
#endif
}

void main(void *param)
{
	s32 res;

	void uart_init(u32 bound);
	uart_init(115200);
	kprintf("hello world!\r\n");
	dma_printf("=============%d============\r\n", 100);
	//TIM3_Init(5000,9000);
	//uart_test();

	kprintf("hello!\r\n");

	udisk_test();
#if 0
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
