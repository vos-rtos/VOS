#include "cmsis/stm32f407xx.h"
#include "../vos/vtype.h"
#include "../vos/vos.h"

int kprintf(char* format, ...);
void sem_test();
void main(void *param)
{
	kprintf("main function!\r\n");
	sem_test();
	while (1) {
		VOSTaskDelay(1*1000);
	}
}
