//#include "usbd_cdc_uart.h"
#include "vtype.h"
#include "vos.h"


void usb2uart_test()
{
	s32 ret = 0;
	s8 buf[100];

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
