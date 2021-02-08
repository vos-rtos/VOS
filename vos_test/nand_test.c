#include "vos.h"
#include "ffconf.h"
#include "ff.h"
#include "nand_diskio.h"

char nand_path[4];

void fatfs_nand_test()
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    UINT num = 0;

	s32 res;

	if(FATFS_LinkDriver(&NAND_Driver, nand_path) != 0) {
		return;
	}
	if(f_mount(&fs, (TCHAR const*)nand_path, 0) != FR_OK)
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

}
