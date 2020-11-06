#include "stm32f4xx_hal.h"

#include "stm32f4xx.h"
#include "vos.h"


TIM_HandleTypeDef TIM3_Handler;


void TIM3_Init(u16 arr,u16 psc)
{
    TIM3_Handler.Instance=TIM3;
    TIM3_Handler.Init.Prescaler=psc;
    TIM3_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;
    TIM3_Handler.Init.Period=arr;
    TIM3_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&TIM3_Handler);

    HAL_TIM_Base_Start_IT(&TIM3_Handler);
}


void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if(htim->Instance==TIM3)
	{
		__HAL_RCC_TIM3_CLK_ENABLE();
		HAL_NVIC_SetPriority(TIM3_IRQn,1,3);
		HAL_NVIC_EnableIRQ(TIM3_IRQn);
	}
}


void timer_hardware_process();
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim==(&TIM3_Handler))
    {
    	//timer_hardware_process();
    	kprintf("---timer up---!\r\n");
    }
}

//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	VOSIntEnter();
	HAL_TIM_IRQHandler(&TIM3_Handler);
	VOSIntExit ();
}
