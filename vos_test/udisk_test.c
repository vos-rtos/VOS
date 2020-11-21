#include "vos.h"
#include "usbh_diskio.h"
char USBHPath[4];   /* USBH logical drive path */
FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */
static u8 buf[512*4] __attribute__ ((aligned (4)));
void fatfs_udisk_test()
{
	s32 i = 0;
	u32 totals = 0;
    FRESULT fr;
    FATFS fs;
    FIL fil;
    UINT num = 0;

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
    		if (totals >= 1*1024*1024) {
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
    		if (totals >= 1*1024*1024) {
    			kprintf("info: fatfs read speed: %dK(B)!\r\n", totals / (VOSGetTimeMs()-timemark));
    			break;
    		}
    	}
    }
    /* Close the file */
    f_close(&fil);
}
