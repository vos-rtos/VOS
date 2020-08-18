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
void hardware_early(void)
{
	SystemInit();
}
#define OS_TICKS_PER_SEC       	100u   /* 10ms tick space, Set the number of ticks in one second                        */
void systick_init(u8 SYSCLK)
{
	u32 reload;

 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

	reload=SYSCLK/8;
	//reload*=1000000/OS_TICKS_PER_SEC;
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD=reload;
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;
}


void misc_init ()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uart_init(115200);
 	TIM3_Int_Init(5000-1,8400-1);
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



