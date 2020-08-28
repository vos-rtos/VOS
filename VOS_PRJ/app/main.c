//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-15: initial by vincent.
//------------------------------------------------------

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
