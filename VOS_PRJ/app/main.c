#include "cmsis/stm32f407xx.h"
#include "../vos/vtype.h"
#include "../vos/vos.h"

int kprintf(char* format, ...);
void sem_test();
void event_test();
void mq_test();
void mutex_test();
void delay_test();
void main(void *param)
{
	kprintf("main function!\r\n");
	//event_test();
	//sem_test();
	//mq_test();
	//mutex_test();
	delay_test();
	while (1) {
		VOSTaskDelay(1*1000);
	}
}
