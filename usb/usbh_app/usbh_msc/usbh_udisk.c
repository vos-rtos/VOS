#include "usbh_core.h"
#include "usbh_msc.h"

#include "usbd_uart.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "fatfs.h"
#include "usb_host.h"

static StUsbHostApp gUDiskApp;

void udisk_test()
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



extern USBH_HandleTypeDef hUsbHostFS;
extern char USBHPath[4];

void SystemClock_Config(void);
void Error_Handler(void);

void MSC_Application(void);


//FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */
//FIL MyFile;                   /* File object */
//
//void MSC_Application(void)
//{
//    FRESULT res;                                          /* FatFs function common result code */
//    uint32_t byteswritten;                   /* File write/read counts */
//    uint8_t wtext[] = "The site is STM32cube.com working with FatFs"; /* File write buffer */
////  uint8_t rtext[100];                                   /* File read buffer */
//
//    /* Register the file system object to the FatFs module */
//    if(f_mount(&USBDISKFatFs, (TCHAR const*)USBHPath, 0) != FR_OK)
//    {
//        /* FatFs Initialization Error */
//        Error_Handler();
//    }
//    else
//    {
//        /* Create and Open a new text file object with write access */
//        if(f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
//        {
//            /* 'STM32.TXT' file Open for write Error */
//            Error_Handler();
//        }
//        else
//        {
//            /* Write data to the text file */
//            res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);
//
//            if((byteswritten == 0) || (res != FR_OK))
//            {
//                /* 'STM32.TXT' file Write or EOF Error */
//                Error_Handler();
//            }
//            else
//            {
//                /* Close the open text file */
//                f_close(&MyFile);
//            }
//        }
//    }
//}

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

