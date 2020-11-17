#include "sd_card/sdio_sdcard.h"
#include "vmisc.h"


void fatfs_test();
void SDIO_SHOW_REGS();
int kprintf(char* format, ...);
#define PRT_REG(x) kprintf(#x"=0x%08x\r\n", x)
void SDIO_SHOW_REGS()
{

  PRT_REG(SDIO->POWER);
  PRT_REG(SDIO->CLKCR);
  PRT_REG(SDIO->ARG);
  PRT_REG(SDIO->CMD);
  PRT_REG(SDIO->RESPCMD);
  PRT_REG(SDIO->RESP1);
  PRT_REG(SDIO->RESP2);
  PRT_REG(SDIO->RESP3);
  PRT_REG(SDIO->RESP4);
  PRT_REG(SDIO->DTIMER);
	PRT_REG(SDIO->DLEN);
  PRT_REG(SDIO->DCTRL);
  PRT_REG(SDIO->DCOUNT);
  PRT_REG(SDIO->STA);
	PRT_REG(SDIO->ICR);
  PRT_REG(SDIO->MASK);
  //PRT_REG(SDIO->RESERVED0[2]);
  PRT_REG(SDIO->FIFOCNT);
  //PRT_REG(SDIO->RESERVED1[13]);
  PRT_REG(SDIO->FIFO);

}


void sd_test_test()
{
 	while(BSP_SD_Init())
	{
 		VOSTaskDelay(1*1000);
	}
	show_sdcard_info();
	SDIO_SHOW_REGS();
	u8 *buf=vmalloc(512);
	if(SD_ReadDisk(buf,0,1)==0)
	{
		s32 sd_size = 0;
		for(sd_size=0;sd_size<512;sd_size++) kprintf("%x ",buf[sd_size]);
	}
	vfree(0,buf);
}

void show_sdcard_info(void)
{
	HAL_SD_CardInfoTypeDef CardInfo;
	memset(&CardInfo, 0, sizeof(CardInfo));
	BSP_SD_GetCardInfo(&CardInfo);
}
#if 0
int fatfs_sd_test(void)
{
  FRESULT res;                                          /* FatFs function common result code */
  uint32_t byteswritten, bytesread;                     /* File write/read counts */
  uint8_t wtext[] = "This is STM32 working with FatFs"; /* File write buffer */
  uint8_t rtext[100];                                   /* File read buffer */

  /*##-1- Link the micro SD disk I/O driver ##################################*/
  if(FATFS_LinkDriver(&SD_Driver, SDPath) == 0)
  {
    /*##-2- Register the file system object to the FatFs module ##############*/
    if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
    {
      /* FatFs Initialization Error */
      Error_Handler();
    }
    else
    {
      /*##-3- Create a FAT file system (format) on the logical drive #########*/
      /* WARNING: Formatting the uSD card will delete all content on the device */
      if(f_mkfs((TCHAR const*)SDPath, FM_ANY, 0, buffer, sizeof(buffer)) != FR_OK)
      {
        /* FatFs Format Error */
        Error_Handler();
      }
      else
      {
        /*##-4- Create and Open a new text file object with write access #####*/
        if(f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        {
          /* 'STM32.TXT' file Open for write Error */
          Error_Handler();
        }
        else
        {
          /*##-5- Write data to the text file ################################*/
          res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);

          if((byteswritten == 0) || (res != FR_OK))
          {
            /* 'STM32.TXT' file Write or EOF Error */
            Error_Handler();
          }
          else
          {
            /*##-6- Close the open text file #################################*/
            f_close(&MyFile);

            /*##-7- Open the text file object with read access ###############*/
            if(f_open(&MyFile, "STM32.TXT", FA_READ) != FR_OK)
            {
              /* 'STM32.TXT' file Open for read Error */
              Error_Handler();
            }
            else
            {
              /*##-8- Read data from the text file ###########################*/
              res = f_read(&MyFile, rtext, sizeof(rtext), (UINT*)&bytesread);

              if((bytesread == 0) || (res != FR_OK))
              {
                /* 'STM32.TXT' file Read or EOF Error */
                Error_Handler();
              }
              else
              {
                /*##-9- Close the open text file #############################*/
                f_close(&MyFile);

                /*##-10- Compare read data with the expected data ############*/
                if((bytesread != byteswritten))
                {
                  /* Read data is different from the expected data */
                  Error_Handler();
                }
                else
                {
                  /* Success of the demo: no error occurrence */
                  BSP_LED_On(LED1);
                }
              }
            }
          }
        }
      }
    }
  }

  /*##-11- Unlink the RAM disk I/O driver ####################################*/
  FATFS_UnLinkDriver(SDPath);

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

