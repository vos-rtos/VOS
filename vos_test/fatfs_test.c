
#include "ffconf.h"
#include "ff.h"

#include "spi.h"
#include "w25qxx.h"
#include "sdio_sdcard.h"
#define SD_CARD_FS_TEST 1
FATFS fs;
void fatfs_sd_card()
{

    FIL fil;
    UINT num = 0;

	s32 res;

	while(SD_Init()) {
		kprintf("SD_Init failed!\r\n");
	}
    /* Open or create a log file and ready to append */
    f_mount(&fs, "0:", 0);
}


#include "sd_diskio.h"
char SDPath[4];
#if 1
void fatfs_test()
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    UINT num = 0;

	s32 res;

#if SD_CARD_FS_TEST
	while(SD_Init()) {
		kprintf("SD_Init failed!\r\n");
	}
	if(FATFS_LinkDriver(&SD_Driver, SDPath) != 0) {
		return ;
	}
	if(f_mount(&fs, (TCHAR const*)SDPath, 0) != FR_OK)
	{
		return;
	}
    /* Open or create a log file and ready to append */
//    f_mount(&fs, "0:", 0);
    u8 buf[] = "hello world!";
    fr = f_open(&fil, "0:/logfile.txt", FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
    	num = 0;
		fr = f_write (&fil, buf, strlen(buf), &num);
    }
    /* Close the file */
    f_close(&fil);
    u8 buf_rd[100] = "";
    memset(buf_rd, 0, sizeof(buf_rd));
    fr = f_open(&fil, "0:/logfile.txt", FA_READ | FA_OPEN_EXISTING);
    if (fr == FR_OK) {
    	num = 0;
		fr = f_read (&fil, buf_rd, sizeof(buf_rd), &num);
    }
    /* Close the file */
    f_close(&fil);
#else
	W25QXX_Init();
	W25QXX_ReadID();

    /* Open or create a log file and ready to append */
    f_mount(&fs, "1:", 0);
    u8 buf[] = "hello world!";
    fr = f_open(&fil, "1:/logfile.txt", FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
    	num = 0;
		fr = f_write (&fil, buf, strlen(buf), &num);
    }
    /* Close the file */
    f_close(&fil);
    u8 buf_rd[100] = "";
    memset(buf_rd, 0, sizeof(buf_rd));
    fr = f_open(&fil, "1:/logfile.txt", FA_READ | FA_OPEN_EXISTING);
    if (fr == FR_OK) {
    	num = 0;
		fr = f_read (&fil, buf_rd, sizeof(buf_rd), &num);
    }
    /* Close the file */
    f_close(&fil);
#endif
}
#endif
#include "usbh_diskio.h"
char USBHPath[4];   /* USBH logical drive path */
FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */
void fatfs_bandmark_test()
{
	s32 i = 0;
	s32 totals = 0;
    FRESULT fr;
    FATFS fs;
    FIL fil;
    UINT num = 0;
    static u8 buf[512*4];
	s32 res;
	u32 timemark;

	if(FATFS_LinkDriver(&USBH_Driver, USBHPath) != 0) {
		return ;
	}
	if(f_mount(&fs, (TCHAR const*)USBHPath, 0) != FR_OK)
	{
		return;
	}
    for (i=0; i<sizeof(buf); i++) {
    	buf[i] = 'a'+i%26;
    }
    fr = f_open(&fil, "0:/fatfs_test.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
    if (fr == FR_OK) {
    	totals = 0;
    	timemark = VOSGetTimeMs();
    	while (1) {
    		num = 0;
    		fr = f_write (&fil, buf, sizeof(buf), &num);
    		if (FR_OK==fr && num > 0) {
    			totals += num;
    		}
    		if (totals >= 1024*1024) {
    			kprintf("info: fatfs write speed: %dK(B)!\r\n", totals / (VOSGetTimeMs()-timemark));
    			break;
    		}
    	}
    	totals = 0;
    	timemark = VOSGetTimeMs();
    	fr = f_lseek (&fil, 0);
    	while (1) {
    		num = 0;
    		fr = f_read (&fil, buf, sizeof(buf), &num);
    		if (FR_OK==fr && num > 0) {
    			totals += num;
    		}
    		if (totals >= 1024*1024) {
    			kprintf("info: fatfs read speed: %dK(B)!\r\n", totals / (VOSGetTimeMs()-timemark));
    			break;
    		}
    	}
    }
    /* Close the file */
    f_close(&fil);
}



