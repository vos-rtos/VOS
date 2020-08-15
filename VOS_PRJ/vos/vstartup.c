
//#include "cmsis/stm32f407xx.h"
#include "../vos/vtype.h"
#include "../vos/vos.h"

//#include "stm32f4xx_hal.h"
//#include "stm32f4xx_hal_uart.h"



extern unsigned int __data_rw_array_start;
extern unsigned int __data_rw_array_end;
extern unsigned int __bss_zero_array_start;
extern unsigned int __bss_zero_array_end;


extern void main(void *param);
long long main_stack[1024];


void init_data_and_bss ()
{
    unsigned int *to_begin = 0;
    unsigned int *to_end = 0;
    unsigned int *from = 0;
    unsigned int *p = 0;
    // Copy the data sections from flash to SRAM.
	for (p = &__data_rw_array_start; p < &__data_rw_array_end;) {
		from = (unsigned int *) (*p++);
		to_begin = (unsigned int *) (*p++);
		to_end = (unsigned int *) (*p++);
		while (to_begin < to_end) *to_begin++ = *from++;
	}

	// Zero fill all bss segments
	for (p = &__bss_zero_array_start; p < &__bss_zero_array_end;) {
		to_begin = (unsigned int*) (*p++);
		to_end = (unsigned int*) (*p++);
		while (to_begin < to_end) *to_begin++ = 0;
	}
}

void
__attribute__((weak))
_exit(int code __attribute__((unused)))
{
  // TODO: write on trace
  while (1)
    ;
}

void
__attribute__((weak,noreturn))
abort(void)
{
  //trace_puts("abort(), exiting...");

  _exit(1);
}


void __attribute__ ((section(".after_vectors")))
vos_start (void)
{
	int code = 0;
	local_irq_disable();
	//hardware_early();
	SystemInit();
	init_data_and_bss ();
	//hw_init_clock();

	VOSSemInit();
	VOSMutexInit();
	VOSMsgQueInit();
	VOSTaskInit();

	VOSTimerInit(); //定时器初始化，依赖信号量

	//SCB->CCR |= SCB_CCR_STKALIGN_Msk;
	//MX_USART1_UART_Init();

	//HAL_NVIC_SetPriority(SVCall_IRQn, 2, 0U);
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	//uart_init(115200);
	void misc_init ();
	misc_init ();
	local_irq_enable();

	code = VOSTaskCreate(main, 0, main_stack, sizeof(main_stack), TASK_PRIO_NORMAL, "main");
	VOSStarup();

	_exit (code);
	while(1) ;
}


