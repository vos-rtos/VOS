

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

void usbh_udisk_test();

void main(void *param)
{
	s32 res;

	void uart_init(u32 bound);
	uart_init(115200);
	kprintf("hello world!\r\n");
	//dma_printf("=============%d============\r\n", 100);
	//TIM3_Init(5000,9000);
//	usbd_uart_init();
//	uart_test();

	kprintf("hello!\r\n");

	usbh_udisk_test();


	//udisk_test();
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
