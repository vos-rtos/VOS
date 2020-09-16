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

int kprintf(char* format, ...);
#define printf kprintf

void main(void *param)
{
	s32 res;
//	kprintf("total: %d, free: %d!\r\n", total, free);
	dma_printf("hello world!\r\n");
	kprintf("main function!\r\n");
	while (1) {
		kprintf(".");
		VOSTaskDelay(1*1000);
	}
}
