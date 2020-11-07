#include "usbd_conf.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_uart.h"

#include "vringbuf.h"

USBD_HandleTypeDef hUsbDeviceFS;

StVOSRingBuf *VOSRingBufBuild(u8 *buf, s32 len);

#define USBD_UART_RECV_MAX 1024

typedef struct StUsbdUart {
	StVOSRingBuf *pRingBuf;
	u8 buf[sizeof(StVOSRingBuf)+USBD_UART_RECV_MAX];
	s32 status;
	s32 init_flag;
}StUsbdUart;

static struct StUsbdUart gUsbdUart;

s32 usbd_uart_init()
{
	StUsbdUart *pUsbdUart = &gUsbdUart;
	if (pUsbdUart->init_flag == 1) return 1;

	/* Init Device Library, add supported class and start the library. */
	USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);

	USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);

	USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);

	USBD_Start(&hUsbDeviceFS);

	pUsbdUart->init_flag = 1;

	pUsbdUart->pRingBuf = VOSRingBufBuild(pUsbdUart->buf, sizeof(pUsbdUart->buf));

	pUsbdUart->init_flag = 1;

	pUsbdUart->status = STATUS_USBD_UART_OK;

	/* Return OK */
	return 0;
}

s32 usbd_uart_app_getc(u8* c)
{
	s32 ret = 0;
	StUsbdUart *pUsbdUart = &gUsbdUart;
	ret = VOSRingBufGet(pUsbdUart->pRingBuf, c, 1);
	return ret;
}

s32 usbd_uart_event_puts(u8* buf, s32 len)
{
	s32 ret = 0;
	StUsbdUart *pUsbdUart = &gUsbdUart;
	ret = VOSRingBufSet(pUsbdUart->pRingBuf, buf, len);
	return ret;
}



s32 usbd_uart_app_gets(u8* buf, s32 size)
{
	s32 ret = 0;
	StUsbdUart *pUsbdUart = &gUsbdUart;
	ret = VOSRingBufGet(pUsbdUart->pRingBuf, buf, size);
	return ret;
}

s32 usbd_uart_app_putc (u8 c)
{
	CDC_Transmit_FS(&c, 1);
	return USBD_OK;
}

s32 usbd_uart_app_puts(u8* buf, s32 len)
{
	CDC_Transmit_FS(buf, len);
	return USBD_OK;
}

s32 usbd_uart_status_get()
{
	StUsbdUart *pUsbdUart = &gUsbdUart;
	if (pUsbdUart->init_flag == 0) {
		pUsbdUart->status = STATUS_USBD_UART_ERROR;
	}
	return pUsbdUart->status;
}

void usbd_uart_status_set(s32 status)
{
	StUsbdUart *pUsbdUart = &gUsbdUart;
	if (pUsbdUart->init_flag == 0) {
		pUsbdUart->status = STATUS_USBD_UART_ERROR;
	}
	pUsbdUart->status = status;
}

