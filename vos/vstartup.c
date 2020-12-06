/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vstartup.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "vtype.h"
#include "ex_sram.h"
#include "vos.h"
#include "vheap.h"
#include "vmem.h"


extern unsigned int __data_rw_array_start;
extern unsigned int __data_rw_array_end;
extern unsigned int __bss_zero_array_start;
extern unsigned int __bss_zero_array_end;

//RAM系统堆1的分配空间
extern unsigned int _Heap_Begin;
extern unsigned int _Heap_Limit;
//CCRAM系统堆2
extern unsigned int _Heap_ccmram_Begin;
extern unsigned int _Heap_ccmram_Limit;

extern void misc_init ();
extern void main(void *param);
//long long main_stack[1024];

/********************************************************************************************************
* 函数：void init_data_and_bss();
* 描述: data 和 bss段初始化
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void init_data_and_bss()
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

/********************************************************************************************************
* 函数：void __attribute__((weak)) _exit(int code __attribute__((unused)));
* 描述: ld链接时需要定义该函数
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void __attribute__((weak)) _exit(int code __attribute__((unused)))
{
  while (1) ;
}

/********************************************************************************************************
* 函数：void __attribute__((weak,noreturn)) abort(void);
* 描述: ld链接时需要定义该函数
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void __attribute__((weak,noreturn)) abort(void)
{
  _exit(1);
}

/********************************************************************************************************
* 函数：void __attribute__ ((section(".after_vectors"))) vos_start (void);
* 描述: 启动函数，复位后第一个执行的函数
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
extern void SystemClock_Config();
void __attribute__ ((section(".after_vectors")))
vos_start(void)
{
	int code = 0;
	u32 irq_save = 0;

	SystemInit();

	init_data_and_bss (); //这里由于清除全局变量，不用被包含到__vos_irq_save里，里面有使用全局变量计数

	HAL_Init();
	SystemClock_Config();

	irq_save = __vos_irq_save();

	VHeapMgrInit();

	//创建RAM系统堆
	struct StVMemHeap *pheap1 = VMemBuild((u8*)&_Heap_Begin, (u32)&_Heap_Limit-(u32)&_Heap_Begin,
			1024, 8, VHEAP_ATTR_SYS, "vos_sys_ram_heap", 1);//启动slab分配器
	//创建CCRAM系统堆
	struct StVMemHeap *pheap2 = VMemBuild((u8*)&_Heap_ccmram_Begin, (u32)&_Heap_ccmram_Limit-(u32)&_Heap_ccmram_Begin,
			1024, 8, VHEAP_ATTR_SYS, "vos_sys_ccram_heap", 1);//启动slab分配器
#if USE_USB_FS
#if !defined(DATA_IN_ExtSRAM)
	ExSRamInit();
	struct StVMemHeap *pheap3 = VMemBuild(ExSRamGetBaseAddr(), ExSRamGetTotalSize(),
			1024, 8, VHEAP_ATTR_SYS, "vos_sys_exsram_heap", 1);//启动slab分配器);
#endif
#endif
	VOSSemInit();
	VOSMutexInit();
	VOSMsgQueInit();
	VOSTaskInit();

	misc_init ();


	VOSTimerInit(); //定时器初始化，依赖信号量，互斥量，不能关中断里执行，因为里面有使用svn中断

	VOSShellInit();
	void *main_stack = vmalloc(1024*8);
	if (main_stack) {
		code = VOSTaskCreate(main, 0, main_stack, 1024*8, TASK_PRIO_NORMAL, "main");
	}
	__vos_irq_restore(irq_save);

	VOSStarup();

	while(1) ;
}


