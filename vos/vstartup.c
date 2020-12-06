/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vstartup.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
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

//RAMϵͳ��1�ķ���ռ�
extern unsigned int _Heap_Begin;
extern unsigned int _Heap_Limit;
//CCRAMϵͳ��2
extern unsigned int _Heap_ccmram_Begin;
extern unsigned int _Heap_ccmram_Limit;

extern void misc_init ();
extern void main(void *param);
//long long main_stack[1024];

/********************************************************************************************************
* ������void init_data_and_bss();
* ����: data �� bss�γ�ʼ��
* ����: ��
* ���أ���
* ע�⣺��
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
* ������void __attribute__((weak)) _exit(int code __attribute__((unused)));
* ����: ld����ʱ��Ҫ����ú���
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void __attribute__((weak)) _exit(int code __attribute__((unused)))
{
  while (1) ;
}

/********************************************************************************************************
* ������void __attribute__((weak,noreturn)) abort(void);
* ����: ld����ʱ��Ҫ����ú���
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void __attribute__((weak,noreturn)) abort(void)
{
  _exit(1);
}

/********************************************************************************************************
* ������void __attribute__ ((section(".after_vectors"))) vos_start (void);
* ����: ������������λ���һ��ִ�еĺ���
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
extern void SystemClock_Config();
void __attribute__ ((section(".after_vectors")))
vos_start(void)
{
	int code = 0;
	u32 irq_save = 0;

	SystemInit();

	init_data_and_bss (); //�����������ȫ�ֱ��������ñ�������__vos_irq_save�������ʹ��ȫ�ֱ�������

	HAL_Init();
	SystemClock_Config();

	irq_save = __vos_irq_save();

	VHeapMgrInit();

	//����RAMϵͳ��
	struct StVMemHeap *pheap1 = VMemBuild((u8*)&_Heap_Begin, (u32)&_Heap_Limit-(u32)&_Heap_Begin,
			1024, 8, VHEAP_ATTR_SYS, "vos_sys_ram_heap", 1);//����slab������
	//����CCRAMϵͳ��
	struct StVMemHeap *pheap2 = VMemBuild((u8*)&_Heap_ccmram_Begin, (u32)&_Heap_ccmram_Limit-(u32)&_Heap_ccmram_Begin,
			1024, 8, VHEAP_ATTR_SYS, "vos_sys_ccram_heap", 1);//����slab������
#if USE_USB_FS
#if !defined(DATA_IN_ExtSRAM)
	ExSRamInit();
	struct StVMemHeap *pheap3 = VMemBuild(ExSRamGetBaseAddr(), ExSRamGetTotalSize(),
			1024, 8, VHEAP_ATTR_SYS, "vos_sys_exsram_heap", 1);//����slab������);
#endif
#endif
	VOSSemInit();
	VOSMutexInit();
	VOSMsgQueInit();
	VOSTaskInit();

	misc_init ();


	VOSTimerInit(); //��ʱ����ʼ���������ź����������������ܹ��ж���ִ�У���Ϊ������ʹ��svn�ж�

	VOSShellInit();
	void *main_stack = vmalloc(1024*8);
	if (main_stack) {
		code = VOSTaskCreate(main, 0, main_stack, 1024*8, TASK_PRIO_NORMAL, "main");
	}
	__vos_irq_restore(irq_save);

	VOSStarup();

	while(1) ;
}


