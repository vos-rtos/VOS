
#include "ffconf.h"
#include "ff.h"

#include "spi.h"
#include "w25qxx.h"
#include "sdio_sdcard.h"


#include "sd_diskio.h"
char SDPath[4];

#define SD_CARD_FS_TEST 1
FATFS fs;
void fatfs_sd_card()
{
	if(FATFS_LinkDriver(&SD_Driver, SDPath) != 0) {
		return ;
	}
	if(f_mount(&fs, (TCHAR const*)SDPath, 0) != FR_OK)
	{
		return;
	}
}


#if 1
void fatfs_test()
{
    FRESULT fr;
    FATFS fs;
    FIL fil;
    UINT num = 0;

	s32 res;

#if SD_CARD_FS_TEST
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


static long long speex_stack[8*1024];//__attribute__ ((section(".bss.CCMRAM")));

static void task_speex_codec(void *param)
{
#if 0
	int test_enc(int argc, char **argv);
	char *argv[4] = {
				"xxx",
				"0:/aaa.wav",
				"0:/aaa.data",
				"0:/aaa.spx",
		};
	test_enc(4, argv);
#endif
#if 0
	int speexenc_test(int argc, char **argv);
	char *argv[3] = {
			"xxx",
			"0:/aaa.wav",
			"0:/aaa.spx",
	};
	speexenc_test(3, argv);
#endif
#if 1
	int faad_main(int argc, char *argv[]);
	char *argv[3] = {
			"xxx",
			"-o0:/xxx.wav",
			"0:/aaa.aac",
	};
	faad_main(3, argv);
#endif


#if 0
	int speex_dec(int argc, char **argv);
	char *argv_dec[3] = {
			"xxx",
			"0:/kkk.spx",
			"0:/zzz.wav",
	};
	speex_dec(3, argv_dec);
#endif
	while (1)  {VOSTaskDelay(5*1000);}
}

static u8 buf[512*4] __attribute__ ((aligned (4)));
void fatfs_sddisk_test()
{
	s32 i = 0;
	u32 totals = 0;
    FRESULT fr;
    FATFS fs;
    FIL fil;
    UINT num = 0;

	s32 res;
	u32 timemark;

	if(FATFS_LinkDriver(&SD_Driver, SDPath) != 0) {
		return ;
	}
	if(f_mount(&fs, (TCHAR const*)SDPath, 0) != FR_OK)
	{
		return;
	}
#if 1
	s32 task_id;
	task_id = VOSTaskCreate(task_speex_codec, 0, speex_stack, sizeof(speex_stack), TASK_PRIO_REAL, "speex_codec");
	while (1)  {VOSTaskDelay(5*1000);}
#endif
	//s32 mp3_dec_file(s8 *path);
	//mp3_dec_file("0:/386.mp3");
	//wm8978_test();
//	void mic_test();
//	mic_test();

#if 1
 	char *arg[3] = {
 			"test",
 			"0:/abc.bmp",
 			"0:/bbb.bmp",
 	};

 	//minutia_main(3, arg);
 	//binary_main(3, arg);
 	//thinner_main(3, arg);
 	//create_main(3, arg);
 	//direction_main(3, arg);
 	//enhancer_main(3, arg);
 	//mask_main(3, arg);
#else
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
#endif
}



