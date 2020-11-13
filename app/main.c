

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

u64 xx_stack[1024];

void task_usbh_test(void *p)
{
	usbh_udisk_test();
}

int dumphex(const unsigned char *buf, int size)
{
	int i;
	for(i=0; i<size; i++)
		printf("%02x,%s", buf[i], (i+1)%16?"":"\r\n");
	return 0;
}

s32 PppCheck();
s32 PppModemInit();
s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);
void main(void *param)
{
	s32 res;
	s8 buf[100];
	void uart_init(u32 bound);
	uart_init(115200);
	kprintf("hello world!\r\n");
	//dma_printf("=============%d============\r\n", 100);
	//TIM3_Init(5000,9000);
//	usbd_uart_init();
//	uart_test();

	kprintf("hello!\r\n");

	s32 task_id;
	task_id = VOSTaskCreate(task_usbh_test, 0, xx_stack, sizeof(xx_stack), TASK_PRIO_NORMAL, "task_usbh_test");
	VOSTaskDelay(5000);

	PppModemInit();


#if 0
	//udisk_test();

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
		VOSTaskDelay(5*1000);
		if (PppCheck()){
			kprintf("PppCheck OK!\r\n");
		}
		else {
			kprintf("PppCheck running!\r\n");
		}
	}

}
