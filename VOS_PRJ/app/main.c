

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
void timer_test();

//u32 Mode2AdminSwitch()
//{
//	__asm volatile ("svc 6\n");
//}
//
//void Mode2AdminRestore(u32 save)
//{
//	__asm volatile ("svc 7\n");
//}

void main(void *param)
{
	s32 len;
	s32 t;


//	u32 irq_save = 0;
//
//	u32 sv = Mode2AdminSwitch();
//	irq_save = __local_irq_save();
//
//	__local_irq_restore(irq_save);
//	Mode2AdminRestore(sv);

	kprintf("main function!\r\n");
	//event_test();
	//sem_test();
	//mq_test();
	//mutex_test();
	//delay_test();
	//schedule_test();
	uart_test();
	//timer_test();
	while (1) {
		VOSTaskDelay(1*1000);
	}
}
