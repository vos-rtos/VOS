/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: main.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "stm32f4xx.h"
#include "usart.h"
#include "vtype.h"
#include "vos.h"


//extern FATFS *fs[_VOLUMES];
char *text = "hello world!\r\n";
int kprintf(char* format, ...);
#define printf kprintf

void main(void *param)
{
	s32 res;
	void fatfs_test();
	fatfs_test();
#if 0
	W25QXX_Init();
	W25QXX_ReadID();
 	while(SD_Init())
	{
		VOSTaskDelay(500);
		VOSTaskDelay(500);
	}
 	exfuns_init();
  	f_mount(fs[0],"0:",1);
 	res=f_mount(fs[1],"1:",1);
	if(res==0X0D)
	{
		res=f_mkfs("1:",1,4096);
		if(res==0)
		{
			//f_setlabel((const TCHAR *)"1:mcudev");
		}
	}

	s32 total, free;
	while(exf_getfree("1:/",&total,&free))
	{
		kprintf("error!\r\n");
		VOSTaskDelay(500);
	}
#endif

//	kprintf("total: %d, free: %d!\r\n", total, free);
	dma_printf("hello world!\r\n");
	kprintf("main function!\r\n");
	while (1) {
		kprintf(".");
		VOSTaskDelay(1*1000);
	}
}
