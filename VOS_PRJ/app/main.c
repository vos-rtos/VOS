

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
