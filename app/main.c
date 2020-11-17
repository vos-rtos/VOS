

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

#include "lcd_dev/lcd.h"
#include "tp_dev/touch.h"
void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);
 	POINT_COLOR=BLUE;
	LCD_ShowString(lcddev.width-24,0,200,16,16,"RST");
  	POINT_COLOR=RED;
}
void rtp_test(void)
{
	u8 key;
	u8 i=0;
	LCD_Init();
	tp_dev.init();
	while(1)
	{
		tp_dev.scan(0);
		if(1)
		{
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{
				if(tp_dev.x[0]>(lcddev.width-24)&&tp_dev.y[0]<16)Load_Drow_Dialog();
				else TP_Draw_Big_Point(tp_dev.x[0],tp_dev.y[0],RED);
			}
		}else delay_ms(10);
		if(1)
		{
			LCD_Clear(WHITE);
		    TP_Adjust();
			TP_Save_Adjdata();
			Load_Drow_Dialog();
		}
		i++;
	}
}

u64 xx_stack[1024];

void task_usbh_test(void *p)
{
	usbh_udisk_test();
}
int dumphex(const unsigned char *buf, int size);

s32 net_init();
s32 PppCheck();
s32 PppModemInit();
s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);
void LCD_Init();
void emWinTest();

void main(void *param)
{

	s32 res;
	s8 buf[100];
	void uart_init(u32 bound);
	uart_init(115200);
	kprintf("hello world!\r\n");
	//rtp_test();

	//dma_printf("=============%d============\r\n", 100);
	//TIM3_Init(5000,9000);
//	usbd_uart_init();
//	uart_test();
	//LCD_Init();
	//LCD_Clear(RED);
//	VOSTaskDelay(5000);
//	LCD_Clear(GREEN);
	//spiflash_test();
	emWinTest();
	while (1) {
		VOSTaskDelay(5*1000);
	}
	void fatfs_test();
	fatfs_test();
	sd_test_test();

	s32 xxxx = 0;
	while (1) {
		kprintf("---%d---\r\n", xxxx++);
		VOSTaskDelay(1000);
	}

	kprintf("hello!\r\n");
#if 0
	s32 task_id;
	task_id = VOSTaskCreate(task_usbh_test, 0, xx_stack, sizeof(xx_stack), TASK_PRIO_NORMAL, "task_usbh_test");
	VOSTaskDelay(5000);

	PppModemInit();
#endif

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
	if (0 == NetDhcpClient(30*1000)) {
		sock_tcp_test();
	}

	while (1) {
		VOSTaskDelay(5*1000);
		if (PppCheck()){
			kprintf("PppCheck OK!\r\n");
			void  sock_tcp_test();
			sock_tcp_test();
		}
		else {
			kprintf("PppCheck running!\r\n");
		}
	}

}
