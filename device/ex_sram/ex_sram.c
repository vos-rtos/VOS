/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: ex_sram.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200915�������ļ�
*********************************************************************************************************/
#include "stm32f4xx_hal.h"
#include "ex_sram.h"

#define FONT_SIZE  0// 859464 //ʸ���ֿ�ܴ�ֱ�ӻ��ֵ�exsram��

/* Exported constants --------------------------------------------------------*/

#define FMC_BANK3_BASE  ((uint32_t)(0x60000000 | 0x08000000))

#define SRAM_BANK_ADDR                 FMC_BANK3_BASE

/* #define SRAM_MEMORY_WIDTH            FSMC_NORSRAM_MEM_BUS_WIDTH_8  */
#define SRAM_MEMORY_WIDTH               FSMC_NORSRAM_MEM_BUS_WIDTH_16
/* #define SRAM_MEMORY_WIDTH            FSMC_NORSRAM_MEM_BUS_WIDTH_32 */

/**
  * @brief SRAM MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param hsram: SRAM handle pointer
  * @retval None
  */
void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
{
  GPIO_InitTypeDef GPIO_Init_Structure;

  /* Enable FSMC clock */
  __HAL_RCC_FSMC_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* Common GPIO configuration */
  GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init_Structure.Pull      = GPIO_NOPULL;
  GPIO_Init_Structure.Speed     = GPIO_SPEED_HIGH;
  GPIO_Init_Structure.Alternate = GPIO_AF12_FSMC;

  /* GPIOD configuration */
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8     |\
                              GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |\
                              GPIO_PIN_14 | GPIO_PIN_15;

  HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);

  /* GPIOE configuration */
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3| GPIO_PIN_4 | GPIO_PIN_7     |\
                              GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |\
                              GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &GPIO_Init_Structure);

  /* GPIOF configuration */
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4     |\
                              GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOF, &GPIO_Init_Structure);

  /* GPIOG configuration */
  GPIO_Init_Structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4     |\
                              GPIO_PIN_5 | GPIO_PIN_10;
  HAL_GPIO_Init(GPIOG, &GPIO_Init_Structure);

}

/**
  * @brief SRAM MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO configuration to their default state
  * @param hsram: SRAM handle pointer
  * @retval None
  */
void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef *hsram)
{
  /*## Disable peripherals and GPIO Clocks ###################################*/
  /* Configure FSMC as alternate function  */
  HAL_GPIO_DeInit(GPIOD, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3| GPIO_PIN_4 | GPIO_PIN_5     |\
                         GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |\
                         GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

  HAL_GPIO_DeInit(GPIOE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3| GPIO_PIN_4 | GPIO_PIN_7     |\
                         GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |\
                         GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

  HAL_GPIO_DeInit(GPIOF, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4     |\
                         GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

  HAL_GPIO_DeInit(GPIOG, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 |\
                         GPIO_PIN_5 | GPIO_PIN_10);
}



/********************************************************************************************************
* ������void ExSRamInit();
* ����:  ExSRamInit, FSMC���ڷ���
* ����:  ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/

void ExSRamInit()
{	
	SRAM_HandleTypeDef hsram;
	FSMC_NORSRAM_TimingTypeDef SRAM_Timing;

	memset(&hsram, 0, sizeof(SRAM_HandleTypeDef));
	memset(&SRAM_Timing, 0, sizeof(FSMC_NORSRAM_TimingTypeDef));

	/*##-1- Configure the SRAM device ##########################################*/
	/* SRAM device configuration */
	hsram.Instance  = FSMC_NORSRAM_DEVICE;
	hsram.Extended  = FSMC_NORSRAM_EXTENDED_DEVICE;

	SRAM_Timing.AddressSetupTime       = 2;
	SRAM_Timing.AddressHoldTime        = 1;
	SRAM_Timing.DataSetupTime          = 2;
	SRAM_Timing.BusTurnAroundDuration  = 1;
	SRAM_Timing.CLKDivision            = 2;
	SRAM_Timing.DataLatency            = 2;
	SRAM_Timing.AccessMode             = FSMC_ACCESS_MODE_A;

	hsram.Init.NSBank             = FSMC_NORSRAM_BANK3;
	hsram.Init.DataAddressMux     = FSMC_DATA_ADDRESS_MUX_DISABLE;
	hsram.Init.MemoryType         = FSMC_MEMORY_TYPE_SRAM;
	hsram.Init.MemoryDataWidth    = SRAM_MEMORY_WIDTH;
	hsram.Init.BurstAccessMode    = FSMC_BURST_ACCESS_MODE_DISABLE;
	hsram.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
	hsram.Init.WrapMode           = FSMC_WRAP_MODE_DISABLE;
	hsram.Init.WaitSignalActive   = FSMC_WAIT_TIMING_BEFORE_WS;
	hsram.Init.WriteOperation     = FSMC_WRITE_OPERATION_ENABLE;
	hsram.Init.WaitSignal         = FSMC_WAIT_SIGNAL_DISABLE;
	hsram.Init.ExtendedMode       = FSMC_EXTENDED_MODE_DISABLE;
	hsram.Init.AsynchronousWait   = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
	hsram.Init.WriteBurst         = FSMC_WRITE_BURST_DISABLE;

	/* Initialize the SRAM controller */
	if(HAL_SRAM_Init(&hsram, &SRAM_Timing, &SRAM_Timing) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}
}

/********************************************************************************************************
* ������u32 ExSRamGetBaseAddr();
* ����:  ��ȡ��ʼ���ʵ�ַ
* ����:  ��
* ���أ�������ʼ���ʵ�ַ
* ע�⣺��
*********************************************************************************************************/
u32 ExSRamGetBaseAddr()
{
#if FONT_SIZE
	return SRAM_BANK_ADDR+FONT_SIZE;
#else
	return SRAM_BANK_ADDR;
#endif
}

/********************************************************************************************************
* ������u32 ExSRamGetTotalSize();
* ����:  ��ȡ��չ�ڴ���ܴ�С
* ����:  ��
* ���أ������ڴ��С����λ���ֽ�
* ע�⣺��
*********************************************************************************************************/
u32 ExSRamGetTotalSize()
{
#if FONT_SIZE
	return 1024*1024-FONT_SIZE;
#else
	return 1024*1024;
#endif
}

u32 TtfFontAddr()
{
#if FONT_SIZE
	return SRAM_BANK_ADDR;
#else
	return 0;
#endif
}

u32 TtfFontSize()
{
#if FONT_SIZE
	return FONT_SIZE;
#else
	return 0;
#endif
}









