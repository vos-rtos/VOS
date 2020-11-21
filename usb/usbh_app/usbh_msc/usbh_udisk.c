#include "usbh_core.h"
#include "usbh_msc.h"

#include "usbd_uart.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "fatfs.h"
#include "usb_host.h"

static StUsbHostApp gUDiskApp;

extern USBH_HandleTypeDef hUsbHostFS;
extern char USBHPath[4];

void SystemClock_Config(void);
void Error_Handler(void);

void MSC_Application(void);

s32 usbh_udisk_do_status(s32 status)
{
	switch (status) {
		case APP_STATUS_IDLE:

			break;
		case APP_STATUS_START:
			f_mount(NULL, (TCHAR const*)"", 0);
			break;
		case APP_STATUS_READY:
			//MSC_Application();
			break;
		case APP_STATUS_DISCONNECT:
			break;
		default:
			break;
	}
	return status;
}

s32 usbh_udisk_init()
{
	StUsbHostApp *pUsbhApp = 0;
	s32 ret = 0;
	pUsbhApp = &gUDiskApp;

	pUsbhApp->status = APP_STATUS_IDLE;
	pUsbhApp->pfStateDo = usbh_udisk_do_status;
	pUsbhApp->pclass = USBH_MSC_CLASS;

	RegisterUsbhApp(pUsbhApp);

	return ret;
}

s32 usbh_udisk_status()
{
	StUsbHostApp *pUsbhApp = 0;
	s32 ret = 0;
	pUsbhApp = &gUDiskApp;
	return pUsbhApp->status;
}

StUsbHostApp *usbh_udisk_ptr()
{
	return &gUDiskApp;
}

