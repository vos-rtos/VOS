#include "sd_card/sdio_sdcard.h"
#include "vmisc.h"

#define printf kprintf
void fatfs_test();
void SDIO_SHOW_REGS();
int kprintf(char* format, ...);
#define PRT_REG(x) kprintf(#x"=0x%08x\r\n", x)
void SDIO_SHOW_REGS()
{
	//SDIO->CLKCR = 0x00000100;
	//SDIO->ARG = 0;
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
 	while(SD_Init())
	{
 		VOSTaskDelay(1*1000);
	}
	show_sdcard_info();
	SDIO_SHOW_REGS();
	u8 *buf=vmalloc(512);
	memset(buf, 0, 512);
	if(SD_ReadDisk(buf,0,1)==0)
	{
		dumphex(buf, 512);
	}
	vfree(0,buf);
}

void show_sdcard_info(void)
{
	uint64_t CardCap;
	HAL_SD_CardCIDTypeDef SDCard_CID;

	HAL_SD_GetCardCID(&SDCARD_Handler,&SDCard_CID);
	SD_GetCardInfo(&SDCardInfo);
	switch(SDCardInfo.CardType)
	{
		case CARD_SDSC:
		{
			if(SDCardInfo.CardVersion == CARD_V1_X)
				printf("Card Type:SDSC V1\r\n");
			else if(SDCardInfo.CardVersion == CARD_V2_X)
				printf("Card Type:SDSC V2\r\n");
		}
		break;
		case CARD_SDHC_SDXC:printf("Card Type:SDHC\r\n");break;
	}
	CardCap=(uint64_t)(SDCardInfo.LogBlockNbr)*(uint64_t)(SDCardInfo.LogBlockSize);
  	printf("Card ManufacturerID:%d\r\n",SDCard_CID.ManufacturerID);
 	printf("Card RCA:%d\r\n",SDCardInfo.RelCardAdd);
	printf("LogBlockNbr:%d \r\n",(u32)(SDCardInfo.LogBlockNbr));
	printf("LogBlockSize:%d \r\n",(u32)(SDCardInfo.LogBlockSize));
	printf("Card Capacity:%d MB\r\n",(u32)(CardCap>>20));
 	printf("Card BlockSize:%d\r\n\r\n",SDCardInfo.BlockSize);
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

