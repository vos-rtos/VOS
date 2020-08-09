#include "vtype.h"
#include "cmsis/stm32f4xx.h"
#include "stm32f4-hal/stm32f4xx_hal.h"
#include "vos.h"

extern unsigned int __vectors_start;

/**
 * @brief  System Clock Configuration
 * @param  None
 * @retval None
 */
void
__attribute__((weak))
SystemClock_Config(void)
{
  // Enable Power Control clock
  __PWR_CLK_ENABLE();

  // The voltage scaling allows optimizing the power consumption when the
  // device is clocked below the maximum system frequency, to update the
  // voltage scaling value regarding system frequency refer to product
  // datasheet.
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

#warning "Please check if the SystemClock_Config() settings match your board!"
  // Comment out the warning after checking and updating.

  RCC_OscInitTypeDef RCC_OscInitStruct;

#if defined(HSE_VALUE) && (HSE_VALUE != 0)
  // Enable HSE Oscillator and activate PLL with HSE as source.
  // This is tuned for STM32F4-DISCOVERY; update it for your board.
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  // This assumes the HSE_VALUE is a multiple of 1 MHz. If this is not
  // your case, you have to recompute these PLL constants.
  RCC_OscInitStruct.PLL.PLLM = (HSE_VALUE/1000000u);
#else
  // Use HSI and activate PLL with HSI as source.
  // This is tuned for NUCLEO-F411; update it for your board.
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  // 16 is the average calibration value, adjust for your own board.
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  // This assumes the HSI_VALUE is a multiple of 1 MHz. If this is not
  // your case, you have to recompute these PLL constants.
  RCC_OscInitStruct.PLL.PLLM = (HSI_VALUE/1000000u);
#endif

  RCC_OscInitStruct.PLL.PLLN = 336;
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE)
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4; /* 84 MHz */
#elif defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; /* 168 MHz */
#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx)
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; /* 168 MHz */
#elif defined(STM32F446xx)
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; /* 168 MHz */
#else
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4; /* 84 MHz, conservative */
#endif
  RCC_OscInitStruct.PLL.PLLQ = 7; /* To make USB work. */
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
  // clocks dividers
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE)
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#else
  // This is expected to work for most large cores.
  // Check and update it for your own configuration.
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
#endif

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
}

void hw_init_clock(void)
{
  // Initialise the HAL Library; it must be the first function
  // to be executed before the call of any HAL function.
  HAL_Init();

  // Enable HSE Oscillator and activate PLL with HSE as source
  SystemClock_Config();

  // Call the CSMSIS system clock routine to store the clock frequency
  // in the SystemCoreClock global RAM location.
  SystemCoreClockUpdate();
}

#define VECT_TAB_OFFSET 0
void SysInit(void)
{
  /* FPU settings ------------------------------------------------------------*/
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x24003010;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;

#if defined (DATA_IN_ExtSRAM) || defined (DATA_IN_ExtSDRAM)
  SystemInit_ExtMemCtl();
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */

  /* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

void hardware_early(void)
{
  // Call the CSMSIS system initialisation routine.
	SysInit();

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  // Set VTOR to the actual address, provided by the linker script.
  // Override the manual, possibly wrong, SystemInit() setting.
  SCB->VTOR = (uint32_t)(&__vectors_start);
#endif

}

UART_HandleTypeDef huart1;
volatile s32 gUartRxCnts = 0;
volatile s32 gUartRxWrIdx = 0;
volatile s32 gUartRxRdIdx = 0;
u8 gUart1Buf[1024];
volatile u8 gRxOne;
/* USART1 init function */
void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart1);
  //HAL_UART_Receive_IT(&huart1, &gRxOne, 1);
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(huart->Instance==USART1)
  {
	/* USER CODE BEGIN USART1_MspInit 0 */
	__HAL_RCC_GPIOA_CLK_ENABLE();                        //使能GPIOA时钟
	__HAL_RCC_USART1_CLK_ENABLE();
	//PA9     ------> USART1_TX
	//PA10     ------> USART1_RX
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*USART1 interrupt Init*/
   // HAL_NVIC_SetPriority(USART1_IRQn, 0x0F, 0);//NVIC_PriorityGroup_4, 最后参数为0

    //HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
	if(huart->ErrorCode&HAL_UART_ERROR_ORE)
	{
		__HAL_UART_CLEAR_OREFLAG(huart);
	}
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *husart)
{
	u32 irq_save = 0;

	irq_save = __local_irq_save();
	if (gUartRxCnts == sizeof(gUart1Buf)) {
		gUartRxCnts = 0; //覆盖，从头算起
		//kprintf(".");
	}
	gUart1Buf[gUartRxWrIdx++] = gRxOne;
	gUartRxCnts++;
	gUartRxWrIdx %= sizeof(gUart1Buf);
	__local_irq_restore(irq_save);

	HAL_UART_Receive_IT(&huart1, &gRxOne, 1);
}

void vputs(s8 *str, s32 len)
{
	HAL_UART_Transmit(&huart1, str, len, 100);
}

s32 vgetc(u8 *ch)
{

	s32 ret = 0;

	u32 irq_save;
	irq_save = __local_irq_save();
	if (gUartRxCnts > 0) {
		*ch = gUart1Buf[gUartRxRdIdx++];
		gUartRxCnts--;
		gUartRxRdIdx %= sizeof(gUart1Buf);
		ret = 1;
	}
	__local_irq_restore(irq_save);

	return ret;

}

s32 vvgets(u8 *buf, s32 len)
{
	s32 ret = 0;
	HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, buf, len, 100);
	switch(status) {
	case HAL_OK:
		ret = len-huart1.RxXferCount;
		break;
	case HAL_ERROR:
		ret = -1;
		break;
	case HAL_BUSY:
		ret = -1;
		break;
	case HAL_TIMEOUT:
		ret = 0;
		break;
	}
	return ret;
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
	SysTick_Config(168000);
}

void HAL_IncTick(void);
void __attribute__ ((section(".after_vectors")))
SysTick_Handler()
{
	HAL_IncTick(); //stm32串口驱动超时要用到

	VOSIntEnter();
	VOSSysTick();
	VOSIntExit ();
}





void USART1_IRQHandler(void)
{
	//VOSIntEnter();
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	HAL_UART_IRQHandler(&huart1);
	__local_irq_restore(irq_save);
	//VOSIntExit ();
}
