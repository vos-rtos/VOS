//#include "usbd_cdc_uart.h"
#include "vtype.h"
#include "vos.h"


int usb2uart_test()
{
#if 0
    uint8_t c;

    usbd_uart_init();

    while (1) {
        if (usbd_uart_status_get() == STATUS_USBD_UART_CONNECTED) {
            if (usbd_uart_app_getc(&c)>0) {
            	usbd_uart_app_putc(c);
            }
        } else {
            /* USB not OK */
        	VOSTaskDelay(1000);
        }
    }
#endif
}
