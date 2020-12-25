#include "random_hw.h"
#include "stm32f4xx_hal.h"

static RNG_HandleTypeDef RNG_Handler;


void HwRandomInit()
{
    u16 cnts = 0;
    RNG_Handler.Instance = RNG;
    HAL_RNG_Init(&RNG_Handler);
    while (__HAL_RNG_GET_FLAG(&RNG_Handler, RNG_FLAG_DRDY) == RESET &&
    		cnts < 10000) {
    	cnts++;
        VOSTaskDelay(1);
    }
}
void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng)
{
     __HAL_RCC_RNG_CLK_ENABLE();
}

u32 HwRandomBuild()
{
    return HAL_RNG_GetRandomNumber(&RNG_Handler);
}

u32 HwRandomBuildRang(u32 from, u32 to)
{ 
   return HAL_RNG_GetRandomNumber(&RNG_Handler) % (to - from + 1) + from;
}




