#include "vos.h"
#include "usbh_diskio.h"
char USBHPath[4];   /* USBH logical drive path */
FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */


static long long speex_stack[6*1024];

static void task_speex_codec(void *param)
{
	//wm8978_test();

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
#if 0
	int faad_main(int argc, char *argv[]);
	char *argv[3] = {
			"xxx",
			"-o0:/xxx.wav",
//			"0:/aaa.aac",
			"0:/ccc.aac",
//			"-w 0:/xxx.wav",
//			"0:/aaa.m4a",
	};
	faad_main(3, argv);
#endif
#if 0
	char *argv[3] = {
			"xxx",
			"0:/aaa.wav",
			"0:/aaa.mp3",
	};
	c_main(3, argv);
#endif
#if 1
	char *argv[3] = {
			"xxx",
			"0:/aaa.wav",
			"0:/aaa.aac",
	};
	int faac_main(int argc, char *argv[]);
	faac_main(3, argv);
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
//	wm8978_test();
//	s32 mp3_dec_file(s8 *path);
//	mp3_dec_file("0:/386.mp3");
	void mic_test();
	mic_test();
#if 0
	s32 task_id;

	task_id = VOSTaskCreate(task_speex_codec, 0, speex_stack,
			sizeof(speex_stack), TASK_PRIO_REAL, "speex_codec");
//	void *pxxxx = vmalloc(256*1024);
//	task_id = VOSTaskCreate(task_speex_codec, 0, pxxxx/*speex_stack*/,
//			256*1024/*sizeof(speex_stack)*/, TASK_PRIO_REAL, "speex_codec");
	while (1)  {VOSTaskDelay(5*1000);}
#endif
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
    		if (totals >= 20*1024*1024) {
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
    		if (totals >= 20*1024*1024) {
    			kprintf("info: fatfs read speed: %dK(B)!\r\n", totals / (VOSGetTimeMs()-timemark));
    			break;
    		}
    	}
    }
    /* Close the file */
    f_close(&fil);

}
