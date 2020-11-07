

#include "vmisc.h"
#include "vtype.h"
#include "vos.h"

//#include "stm32f4xx_hal_pwr_ex.h"

#include "stm32f4xx_hal.h"

void SystemInit(void);
void HAL_IncTick(void);
void VOSExceptHandler(u32 *sp, s32 is_psp);


static void SystemClock_Config(void)
{
#if 1
	  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	  /**Configure the main internal regulator output voltage
	  */
	  __HAL_RCC_PWR_CLK_ENABLE();
	  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	  /**Initializes the CPU, AHB and APB busses clocks
	  */
	  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	  RCC_OscInitStruct.PLL.PLLM = 8;
	  RCC_OscInitStruct.PLL.PLLN = 336;
	  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	  RCC_OscInitStruct.PLL.PLLQ = 7;
	  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	  {
	    Error_Handler();
	  }
	  /**Initializes the CPU, AHB and APB busses clocks
	  */
	  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
	                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	  {
	    Error_Handler();
	  }
#endif
}


void misc_init()
{
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	//uart_init(115200);
 	//TIM3_Int_Init(5000-1,8400-1);
	HAL_Init();
	VOSSysTickSet();//…Ë÷√tick ±÷”º‰∏Ù
}


void VOSSysTickSet()
{
	SystemCoreClockUpdate();
	//SysTick_Config(168000);
	SystemClock_Config();
}


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
		offset = 1;
	}
	irq_save = __local_irq_save();
	svc_number = ((char *)svc_args[6+offset])[-2];
	switch(svc_number) {
	case VOS_SVC_NUM_SYSCALL:
//		psa = (StVosSysCallParam *)svc_args[0+offset];
//		VOSSysCall(psa);
		break;

	case VOS_SVC_PRIVILEGED_MODE:
		svc_args[0+offset] = __switch_privileged_mode();
		break;

	default:
		kprintf("ERROR: SVC_Handler_C!\r\n");
		while (1) ;
		break;
	}

	__local_irq_restore(irq_save);
	VOSIntExit ();
}

void __attribute__ ((section(".after_vectors"),noreturn))
Reset_Handler ()
{
	vos_start ();
}

void __attribute__ ((section(".after_vectors"),weak))
HardFault_Handler ()
{
	asm volatile(
		  " push 	{lr}    \n"
		  " tst 	lr, #4  \n"
		  " ite 	eq      \n"
		  " mrseq 	r0, msp \n"
		  " mrsne 	r0, psp \n"
		  " mrs 	r1, msp \n"
		  " sub 	r1, r0  \n"
		  " bl 		VOSExceptHandler		\n"
		  " b		. 		\n"
		  " pop		{pc}	\n"
	);
}


void __attribute__ ((section(".after_vectors"),weak))
MemManage_Handler ()
{
	asm volatile(
		  " push 	{lr}    \n"
		  " tst 	lr, #4  \n"
		  " ite 	eq      \n"
		  " mrseq 	r0, msp \n"
		  " mrsne 	r0, psp \n"
		  " mrs 	r1, msp \n"
		  " sub 	r1, r0  \n"
		  " bl 		VOSExceptHandler		\n"
		  " b		. 		\n"
		  " pop		{pc}	\n"
	);
}


void __attribute__ ((section(".after_vectors"),weak,naked))
BusFault_Handler ()
{
	asm volatile(
		  " push 	{lr}    \n"
		  " tst 	lr, #4  \n"
		  " ite 	eq      \n"
		  " mrseq 	r0, msp \n"
		  " mrsne 	r0, psp \n"
		  " mrs 	r1, msp \n"
		  " sub 	r1, r0  \n"
		  " bl 		VOSExceptHandler		\n"
		  " b		. 		\n"
		  " pop		{pc}	\n"
	);
}

void __attribute__ ((section(".after_vectors"),weak,naked))
UsageFault_Handler ()
{
	asm volatile(
		  " push 	{lr}    \n"
		  " tst 	lr, #4  \n"
		  " ite 	eq      \n"
		  " mrseq 	r0, msp \n"
		  " mrsne 	r0, psp \n"
		  " mrs 	r1, msp \n"
		  " sub 	r1, r0  \n"
		  " bl 		VOSExceptHandler		\n"
		  " b		. 		\n"
		  " pop		{pc}	\n"
	);
}


void __attribute__ ((section(".after_vectors"),weak))
DebugMon_Handler ()
{
	asm volatile(
		  " push 	{lr}    \n"
		  " tst 	lr, #4  \n"
		  " ite 	eq      \n"
		  " mrseq 	r0, msp \n"
		  " mrsne 	r0, psp \n"
		  " mrs 	r1, msp \n"
		  " sub 	r1, r0  \n"
		  " bl 		VOSExceptHandler		\n"
		  " b		. 		\n"
		  " pop		{pc}	\n"
	);
}


void __attribute__ ((section(".after_vectors"),weak))
NMI_Handler ()
{
	asm volatile(
		  " push 	{lr}    \n"
		  " tst 	lr, #4  \n"
		  " ite 	eq      \n"
		  " mrseq 	r0, msp \n"
		  " mrsne 	r0, psp \n"
		  " mrs 	r1, msp \n"
		  " sub 	r1, r0  \n"
		  " bl 		VOSExceptHandler		\n"
		  " b		. 		\n"
		  " pop		{pc}	\n"
	);
}






