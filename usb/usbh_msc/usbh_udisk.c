#include "usbh_core.h"
#include "usbh_msc.h"

#include "usbd_uart.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "fatfs.h"
#include "usb_host.h"

void MX_USB_DEVICE_Init(void);
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



void MX_USB_HOST_Process(void);

extern ApplicationTypeDef Appli_state;
extern USBH_HandleTypeDef hUsbHostFS;
extern char USBHPath[4];

void SystemClock_Config(void);
void Error_Handler(void);
void MX_USB_HOST_Process(void);


static void MSC_Application(void);


FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */
FIL MyFile;                   /* File object */

static void MSC_Application(void)
{
    FRESULT res;                                          /* FatFs function common result code */
    uint32_t byteswritten;                   /* File write/read counts */
    uint8_t wtext[] = "The site is STM32cube.com working with FatFs"; /* File write buffer */
//  uint8_t rtext[100];                                   /* File read buffer */

    /* Register the file system object to the FatFs module */
    if(f_mount(&USBDISKFatFs, (TCHAR const*)USBHPath, 0) != FR_OK)
    {
        /* FatFs Initialization Error */
        Error_Handler();
    }
    else
    {
        /* Create and Open a new text file object with write access */
        if(f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        {
            /* 'STM32.TXT' file Open for write Error */
            Error_Handler();
        }
        else
        {
            /* Write data to the text file */
            res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);

            if((byteswritten == 0) || (res != FR_OK))
            {
                /* 'STM32.TXT' file Write or EOF Error */
                Error_Handler();
            }
            else
            {
                /* Close the open text file */
                f_close(&MyFile);
            }
        }
    }
}


u32 __CUSTOM_DirectWriteAT(u8 *pBuf, u32 dwLen, u32 dwTimeout);
u32 __CUSTOM_DirectReadAT(u8 *pBuf, u32 dwLen, u32 dwTimeout);


void usbh_udisk_test()
{
	  MX_USB_HOST_Init();
	  //MX_FATFS_Init();
	  while (1)
	  {
	    /* USER CODE END WHILE */
	    MX_USB_HOST_Process();

	    /* USER CODE BEGIN 3 */
		switch(Appli_state)
		{
		case APPLICATION_READY:
			//MSC_Application();

			//Appli_state = APPLICATION_START;
			break;

		case APPLICATION_START:
			f_mount(NULL, (TCHAR const*)"", 0);
			break;
		case APPLICATION_DISCONNECT:
			Appli_state = APPLICATION_IDLE;
			break;
		default:
			break;
		}
	  }
}
