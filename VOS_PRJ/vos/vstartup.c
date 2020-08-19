//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "../vos/vtype.h"
#include "../vos/vos.h"

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

void misc_init ();
void __attribute__ ((section(".after_vectors")))
vos_start (void)
{
	int code = 0;
	u32 irq_save = 0;

	SystemInit();
	init_data_and_bss (); //这里由于清除全局变量，不用被包含到__vos_irq_save里，里面有使用全局变量计数

	VOSSysTickSet();//设置tick时钟间隔

	irq_save = __vos_irq_save();
	VOSSemInit();
	VOSMutexInit();
	VOSMsgQueInit();
	VOSTaskInit();
	misc_init ();

	//VOSTimerInit(); //定时器初始化，依赖信号量，互斥量，不能关中断里执行，因为里面有使用svn中断

	code = VOSTaskCreate(main, 0, main_stack, sizeof(main_stack), TASK_PRIO_NORMAL, "main");

	__vos_irq_restore(irq_save);


	VOSStarup();

	_exit (code);
	while(1) ;
}


