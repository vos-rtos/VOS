

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

#include "mongoose.h"

// The very first web page in history
static const char *s_url = "http://info.cern.ch";

// Print HTTP response and signal that we're done
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_CONNECT) {
    // Connected to server
    struct mg_str host = mg_url_host(s_url);

    if (mg_url_is_ssl(s_url)) {
      // If s_url is https://, tell client connection to use TLS
      struct mg_tls_opts opts = {.ca = "ca.pem"};
      mg_tls_init(c, &opts);
    }
    // Send request
    mg_printf(c, "GET %s HTTP/1.0\r\nHost: %.*s\r\n\r\n", mg_url_uri(s_url),
              (int) host.len, host.ptr);
  } else if (ev == MG_EV_HTTP_MSG) {
    // Response is received. Print it
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    printf("%.*s", (int) hm->message.len, hm->message.ptr);
    c->is_closing = 1;         // Tell mongoose to close this connection
    *(bool *) fn_data = true;  // Tell event loop to stop
  }
}

int test_mg_http()
{
  struct mg_mgr mgr;                        // Event manager
  bool done = false;                        // Event handler flips it to true
  mg_log_set("3");                          // Set to 0 to disable debug
  mg_mgr_init(&mgr);                        // Initialise event manager
  mg_http_connect(&mgr, s_url, fn, &done);  // Create client connection
  while (!done) {
	  mg_mgr_poll(&mgr, 1000);    // Infinite event loop
  }
  mg_mgr_free(&mgr);                        // Free resources
  return 0;
}


//#define DEF_SD_WIFI 1
#define DEF_ETH 1
//#define DEF_4G_PPP 1
//#define DEF_SD_FATFS 1
//#define DEF_USB_FATFS 1
//#define DEF_GUI 1
#undef printf
void main(void *param)
{
	s32 res;
	s8 buf[100];
	void uart_init(u32 bound);
 	uart_init(115200);

#if DEF_SD_WIFI
	int wifi_test();
	wifi_test();
	VOSTaskDelay(20*1000);
	//sock_tcp_test();
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
	//SetNetWorkInfo ("192.168.2.101", "255.255.255.0", "192.168.2.100");
	if (0 == NetDhcpClient(30*1000))
	if (0) {
		ip_addr_t perf_server_ip;
		IP_ADDR4(&perf_server_ip, 192, 168, 2, 101);
		while(1) {
			lwiperf_start_tcp_server(&perf_server_ip, 9527, NULL, NULL);
			VOSTaskDelay(10);
		}
	}
	else
	{
		test_mg_http();
		//sock_tcp_test();
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
//			void  sock_tcp_test();
//			sock_tcp_test();
			test_mg_http();
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
