#ifndef __USBD_UART_H__
#define __USBD_UART_H__

#include "vos.h"

s32 usbd_uart_init();
s32 usbd_uart_app_getc(u8* c);
s32 usbd_uart_event_puts(u8* buf, s32 len);
s32 usbd_uart_app_gets(u8* buf, s32 size);
s32 usbd_uart_app_putc (u8 c);
s32 usbd_uart_app_puts(u8* buf, s32 len);
s32 usbd_uart_deinit();
#endif
