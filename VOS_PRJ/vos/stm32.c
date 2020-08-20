//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "misc.h"
#include "vtype.h"
#include "vos.h"

void SystemInit(void);
void misc_init ()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uart_init(115200);
 	//TIM3_Int_Init(5000-1,8400-1);
}


void VOSSysTickSet()
{
	SystemCoreClockUpdate();
	SysTick_Config(168000);
}

void HAL_IncTick(void);
void __attribute__ ((section(".after_vectors")))
SysTick_Handler()
{
	VOSIntEnter();
	VOSSysTick();
	VOSIntExit ();
}

void SVC_Handler_C(u32 *svc_args, s32 is_psp)
{

	VOSIntEnter();
	StVosSysCallParam *psa;
	u8 svc_number;
	u32 irq_save;
	u32 offset = 0;
	if (!is_psp) {
		offset = 1;//+1是汇编里把lr也push一个到msp，所以这里要加1
	}
	irq_save = __local_irq_save();
	svc_number = ((char *)svc_args[6+offset])[-2]; //+1是汇编里把lr也push一个，所以这里要加1
	switch(svc_number) {
	case VOS_SVC_NUM_SYSCALL: //系统调用
//		psa = (StVosSysCallParam *)svc_args[0+offset];
//		VOSSysCall(psa);
		break;

	case VOS_SVC_PRIVILEGED_MODE: //svc 6 切换到特权并关中断
		svc_args[0+offset] = __switch_privileged_mode();//返回切换前的control[0]状态
		break;

	default:
		kprintf("ERROR: SVC_Handler_C!\r\n");
		while (1) ;
		break;
	}

	__local_irq_restore(irq_save);
	VOSIntExit ();
}




