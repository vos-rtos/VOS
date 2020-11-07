#ifndef __USBD_UART_H__
#define __USBD_UART_H__

#include "vos.h"

enum {
	STATUS_USBD_UART_OK = 0,
	STATUS_USBD_UART_CONNECTED,
	STATUS_USBD_UART_DIS_CONNECTED,
	STATUS_USBD_UART_SUSPENDED,
	STATUS_USBD_UART_RESUMED,
	STATUS_USBD_UART_ERROR,
};

s32 usbd_uart_init();
s32 usbd_uart_app_getc(u8* c);
s32 usbd_uart_event_puts(u8* buf, s32 len);
s32 usbd_uart_app_gets(u8* buf, s32 size);
s32 usbd_uart_app_putc (u8 c);
s32 usbd_uart_app_puts(u8* buf, s32 len);
s32 usbd_uart_status_get();
void usbd_uart_status_set(s32 status);
#endif
