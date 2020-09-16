
#include "ffconf.h"
#include "ff.h"

#include "spi.h"
#include "w25qxx.h"

void fatfs_test()
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    UINT num = 0;

	s32 res;

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
}
