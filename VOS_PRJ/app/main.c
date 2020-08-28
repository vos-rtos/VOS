/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vos.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "stm32f4xx.h"
#include "../usart/usart.h"
#include "../vos/vtype.h"
#include "../vos/vos.h"
int kprintf(char* format, ...);
void main(void *param)
{
	u32 save = 0;
	dma_printf("hello world!\r\n");
	kprintf("main function!\r\n");

	while (1) {
		VOSTaskDelay(1*1000);
	}
}
