#include "misc.h"
#include "vtype.h"
//#include "cmsis/stm32f4xx.h"
//#include "stm32f4-hal/stm32f4xx_hal.h"
#include "vos.h"


//extern unsigned int __vectors_start;


void SystemInit(void);
void hardware_early(void)
{
	SystemInit();
}

//UART_HandleTypeDef huart1;
//volatile s32 gUartRxCnts = 0;
//volatile s32 gUartRxWrIdx = 0;
//volatile s32 gUartRxRdIdx = 0;
//u8 gUart1Buf[1024];
//volatile u8 gRxOne;
///* USART1 init function */
//void MX_USART1_UART_Init(void)
//{
//
//  huart1.Instance = USART1;
//  huart1.Init.BaudRate = 115200;
//  huart1.Init.WordLength = UART_WORDLENGTH_8B;
//  huart1.Init.StopBits = UART_STOPBITS_1;
//  huart1.Init.Parity = UART_PARITY_NONE;
//  huart1.Init.Mode = UART_MODE_TX_RX;
//  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
//  HAL_UART_Init(&huart1);
//  //HAL_UART_Receive_IT(&huart1, &gRxOne, 1);
//}
//
//void HAL_UART_MspInit(UART_HandleTypeDef* huart)
//{
//  GPIO_InitTypeDef GPIO_InitStruct;
//  if(huart->Instance==USART1)
//  {
//	/* USER CODE BEGIN USART1_MspInit 0 */
//	__HAL_RCC_GPIOA_CLK_ENABLE();                        //使能GPIOA时钟
//	__HAL_RCC_USART1_CLK_ENABLE();
//	//PA9     ------> USART1_TX
//	//PA10     ------> USART1_RX
//    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
//    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//    GPIO_InitStruct.Pull = GPIO_PULLUP;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//    /*USART1 interrupt Init*/
//   // HAL_NVIC_SetPriority(USART1_IRQn, 0x0F, 0);//NVIC_PriorityGroup_4, 最后参数为0
//
//    //HAL_NVIC_EnableIRQ(USART1_IRQn);
//  }
//}
//
//
//void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
//{
//  /* Prevent unused argument(s) compilation warning */
//	if(huart->ErrorCode&HAL_UART_ERROR_ORE)
//	{
//		__HAL_UART_CLEAR_OREFLAG(huart);
//	}
//}
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *husart)
//{
//	u32 irq_save = 0;
//
//	irq_save = __local_irq_save();
//	if (gUartRxCnts == sizeof(gUart1Buf)) {
//		gUartRxCnts = 0; //覆盖，从头算起
//	}
//	gUart1Buf[gUartRxWrIdx++] = gRxOne;
//	gUartRxCnts++;
//	gUartRxWrIdx %= sizeof(gUart1Buf);
//	__local_irq_restore(irq_save);
//	HAL_StatusTypeDef ret = HAL_UART_Receive_IT(&huart1, &gRxOne, 1);
//	if (ret != HAL_OK) {
//		kprintf(".");
//	}
//}
//
//void vputs(s8 *str, s32 len)
//{
//	u32 irq_save;
//	irq_save = __local_irq_save();
//	HAL_UART_Transmit(&huart1, str, len, 100);
//	__local_irq_restore(irq_save);
//}
//
//s32 vgetc(u8 *ch)
//{
//
//	s32 ret = 0;
//
//	u32 irq_save;
//	irq_save = __local_irq_save();
//	if (gUartRxCnts > 0) {
//		*ch = gUart1Buf[gUartRxRdIdx++];
//		gUartRxCnts--;
//		gUartRxRdIdx %= sizeof(gUart1Buf);
//		ret = 1;
//	}
//	__local_irq_restore(irq_save);
//
//	return ret;
//
//}
//
//s32 vvgets(u8 *buf, s32 len)
//{
//	s32 ret = 0;
//	u32 irq_save = 0;
//	irq_save = __local_irq_save();
//	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, buf, len, 100);
//	__local_irq_restore(irq_save);
//	switch(status) {
//	case HAL_OK:
//		ret = len-huart1.RxXferCount;
//		break;
//	case HAL_ERROR:
//		ret = -1;
//		break;
//	case HAL_BUSY:
//		ret = -1;
//		break;
//	case HAL_TIMEOUT:
//		ret = 0;
//		break;
//	}
//
//	return ret;
//}
//
//static uint32_t SysTick_Config(uint32_t ticks)
//{
//  if (ticks > SysTick_LOAD_RELOAD_Msk)  return (1);       //ticks参数有效性检查
//
//  SysTick->LOAD  = (ticks & SysTick_LOAD_RELOAD_Msk) - 1; //设置重装载值
//                                                    //-1:装载时消耗掉一个Systick时钟周期
//
//  NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1); //配置NVIC
//
//  SysTick->VAL   = 0;    //初始化VAL=0,使能Systick后立刻进入重装载
//  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |      //选择时钟源
//                   SysTick_CTRL_TICKINT_Msk   |      //开启Systick中断
//                   SysTick_CTRL_ENABLE_Msk;          //使能Systick定时器
//  return (0);      /* Function successful */
//}

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
}

void SVC_Handler_C(unsigned int *svc_args)
{
	u8 svc_number;
	u32 irq_save;
	svc_number = ((char *)svc_args[6])[-2];
	switch(svc_number) {
	case 0://系统刚初始化完成，启动第一个任务
    	irq_save = __local_irq_save();
		TaskIdleBuild();//创建idle任务
		RunFirstTask(); //加载第一个任务，这时任务不一定是IDLE任务
		VOSSysTickSet();//设置tick时钟间隔
		__local_irq_restore(irq_save);
		break;
	case 1://用户任务主动调用切换到更高优先级任务，如果没有则继续用户任务
		VOSTaskSwitch(TASK_SWITCH_ACTIVE);
		//asm_ctx_switch();
		break;
	default:
		break;
	}
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



//
//
//void USART1_IRQHandler(void)
//{
//	VOSIntEnter();
//	u32 irq_save = 0;
//	irq_save = __local_irq_save();
//	HAL_UART_IRQHandler(&huart1);
//	__local_irq_restore(irq_save);
//	VOSIntExit ();
//}
