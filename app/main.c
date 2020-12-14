

#include "stm32f4xx.h"
#include "vtype.h"
#include "usart.h"
#include "vos.h"
#include "lwip/ip_addr.h"
#include "usb_host.h"
#include "usbh_udisk.h"


int kprintf(char* format, ...);
#define printf kprintf

void lwip_test();

s32 NetDhcpClient(u32 timeout);
void emWinTest();
int usb2uart_test();

void usbh_udisk_test();

int dumphex(const unsigned char *buf, int size);

s32 net_init();
s32 PppCheck();
s32 PppModemInit();
s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);
void LCD_Init();
void emWinTest();



void MX_GPIO_Init()
{

  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}


#define DEF_SD_WIFI 1
//#define DEF_ETH 1
//#define DEF_4G_PPP 1
//#define DEF_SD_FATFS 1
//#define DEF_USB_FATFS 1
//#define DEF_GUI 1
void main(void *param)
{
	s32 res;
	s8 buf[100];
	void uart_init(u32 bound);
 	uart_init(115200);
	kprintf("hello world!\r\n");

#if DEF_SD_WIFI
	int wifi_test();
	wifi_test();
	VOSTaskDelay(20*1000);
	sock_tcp_test();
#endif

#if !USE_USB_FS
	MX_GPIO_Init();
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

#if DEF_USB_FATFS
	usb_host_init();
	s32 usbh_udisk_init();
	usbh_udisk_init();
	while (usbh_udisk_status() != APP_STATUS_READY) {
		VOSTaskDelay(1000);
	}
	void fatfs_udisk_test();
	fatfs_udisk_test();
#endif

#if DEF_SD_FATFS
	//sd_test_test();
	void fatfs_sddisk_test();
	fatfs_sddisk_test();
#endif

#if 0
	udisk_test();

#endif

#if DEF_ETH
	SetNetWorkInfo ("192.168.2.101", "255.255.255.0", "192.168.2.100");
	//if (0 == NetDhcpClient(30*1000))
	if (1) {
		ip_addr_t perf_server_ip;
		IP_ADDR4(&perf_server_ip, 192, 168, 2, 101);
		while(1) {
			lwiperf_start_tcp_server(&perf_server_ip, 9527, NULL, NULL);
			VOSTaskDelay(10);;
		}
	}
	else
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
