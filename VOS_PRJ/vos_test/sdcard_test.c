#include "sd_card/sdio_sdcard.h"
#include "vmisc.h"


void fatfs_test();
void SDIO_SHOW_REGS();

void sd_test_test()
{
 	while(SD_Init())
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
	switch(SDCardInfo.CardType)
	{
		case SDIO_STD_CAPACITY_SD_CARD_V1_1:printf("Card Type:SDSC V1.1\r\n");break;
		case SDIO_STD_CAPACITY_SD_CARD_V2_0:printf("Card Type:SDSC V2.0\r\n");break;
		case SDIO_HIGH_CAPACITY_SD_CARD:printf("Card Type:SDHC V2.0\r\n");break;
		case SDIO_MULTIMEDIA_CARD:printf("Card Type:MMC Card\r\n");break;
	}
  	printf("Card ManufacturerID:%d\r\n",SDCardInfo.SD_cid.ManufacturerID);
 	printf("Card RCA:%d\r\n",SDCardInfo.RCA);
	printf("Card Capacity:%d MB\r\n",(u32)(SDCardInfo.CardCapacity>>20));
 	printf("Card BlockSize:%d\r\n\r\n",SDCardInfo.CardBlockSize);
}
