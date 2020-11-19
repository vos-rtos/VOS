

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

//#include "lcd_dev/lcd.h"
//#include "tp_dev/touch.h"

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

#include "usb_host.h"
#include "usbh_udisk.h"

//#define DEF_ETH 1
//#define DEF_4G_PPP 1
//#define DEF_USBH_UDISK 1
#define DEF_SD_FATFS 1
void main(void *param)
{

	s32 res;
	s8 buf[100];
	void uart_init(u32 bound);
	uart_init(115200);
	kprintf("hello world!\r\n");

#if DEF_USBH_UDISK
	usb_host_init();
	usbh_udisk_init();

	while (usbh_udisk_status() != APP_STATUS_READY) {
		VOSTaskDelay(1000);
	}
//	void MSC_Application(void);
//	MSC_Application();
	void fatfs_bandmark_test();
	fatfs_bandmark_test();
	while (1) {
		VOSTaskDelay(5*1000);
	}
#endif

#if DEF_UART_USB
	//usb2uart_test();
	usbd_uart_init();
	uart_test();
#endif

#if DEF_SPI_FLASH
	spiflash_test();
#endif

#if DEF_GUI
	emWinTest();
#endif

#if DEF_SD_FATFS
	void fatfs_test();
	fatfs_test();
	//sd_test_test();
#endif

#if 0
	udisk_test();

#endif

#if DEF_ETH
	SetNetWorkInfo ("192.168.2.101", "255.255.255.0", "192.168.2.100");
	//if (0 == NetDhcpClient(30*1000))
	{
		sock_tcp_test();
	}
#endif
#if DEF_4G_PPP

	usb_host_init();
	usbh_modem_init();

	while (usbh_modem_status() != APP_STATUS_READY) {
		VOSTaskDelay(1000);
	}

	PppModemInit();
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
#endif
	while (1) {
		VOSTaskDelay(5*1000);
	}

}
