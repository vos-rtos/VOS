

#include "stm32f4xx.h"
#include "../usart/usart.h"
#include "../vos/vtype.h"
#include "../vos/vos.h"
int kprintf(char* format, ...);
void sem_test();
void event_test();
void mq_test();
void mutex_test();
void delay_test();
void schedule_test();
void uart_test();
int aaa = 0;

void main(void *param)
{
	s32 len;
	s32 t;

	kprintf("main function!\r\n");
	//event_test();
	//sem_test();
	//mq_test();
	//mutex_test();
	//delay_test();
	//schedule_test();
	uart_test();

	while (1) {
		VOSTaskDelay(1*1000);
	}
}
