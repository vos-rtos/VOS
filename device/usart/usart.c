/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: sart.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "usart.h"
#include "vos.h"
#include "vringbuf.h"
#include "stm32f4xx_hal.h"

UART_HandleTypeDef UartHandle;

#if USE_USB_FS

#define USARTx                           USART1
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE();
#define DMAx_CLK_ENABLE()                __HAL_RCC_DMA2_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART1_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART1_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_9
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART1
#define USARTx_RX_PIN                    GPIO_PIN_10
#define USARTx_RX_GPIO_PORT              GPIOA
#define USARTx_RX_AF                     GPIO_AF7_USART1

/* Definition for USARTx's DMA */
#define USARTx_TX_DMA_CHANNEL            DMA_CHANNEL_4
#define USARTx_TX_DMA_STREAM             DMA2_Stream7
#define USARTx_RX_DMA_CHANNEL            DMA_CHANNEL_4
#define USARTx_RX_DMA_STREAM             DMA2_Stream2

/* Definition for USARTx's NVIC */
#define USARTx_DMA_TX_IRQn               DMA2_Stream7_IRQn
#define USARTx_DMA_RX_IRQn               DMA2_Stream2_IRQn
#define USARTx_DMA_TX_IRQHandler         DMA2_Stream7_IRQHandler
#define USARTx_DMA_RX_IRQHandler         DMA2_Stream2_IRQHandler
#define USARTx_IRQn                      USART1_IRQn
#define USARTx_IRQHandler                USART1_IRQHandler
#else

#define USARTx                           USART3
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART3_CLK_ENABLE();
#define DMAx_CLK_ENABLE()                __HAL_RCC_DMA1_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART3_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART3_RELEASE_RESET()


#define USARTx_TX_PIN                    GPIO_PIN_10
#define USARTx_TX_GPIO_PORT              GPIOC
#define USARTx_TX_AF                     GPIO_AF7_USART3
#define USARTx_RX_PIN                    GPIO_PIN_11
#define USARTx_RX_GPIO_PORT              GPIOC
#define USARTx_RX_AF                     GPIO_AF7_USART3

/* Definition for USARTx's DMA */
#define USARTx_TX_DMA_CHANNEL            DMA_CHANNEL_4
#define USARTx_TX_DMA_STREAM             DMA1_Stream3
#define USARTx_RX_DMA_CHANNEL            DMA_CHANNEL_4
#define USARTx_RX_DMA_STREAM             DMA1_Stream1

/* Definition for USARTx's NVIC */
#define USARTx_DMA_TX_IRQn               DMA1_Stream3_IRQn
#define USARTx_DMA_RX_IRQn               DMA1_Stream1_IRQn
#define USARTx_DMA_TX_IRQHandler         DMA1_Stream3_IRQHandler
#define USARTx_DMA_RX_IRQHandler         DMA1_Stream1_IRQHandler
#define USARTx_IRQn                      USART3_IRQn
#define USARTx_IRQHandler                USART3_IRQHandler
#endif

#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

#define USART_RX_USE_DMA	1


#define SEND_BUF_SIZE 512
#define RECV_BUF_SIZE 512
u8 SendBuff[SEND_BUF_SIZE];
static u8 RecvBuff[RECV_BUF_SIZE];
static s32 RecvIdx = 0;

#define RX_RINGBUF_SIZE (1024+20)
u8 gRxRingBuf[RX_RINGBUF_SIZE];
StVOSRingBuf *gRingBuf = 0;


enum {
	EVENT_UART_RECV = 0,
};

typedef struct StMapEvtTask{
	s32 event;
	s32 task_id;
}StMapEvtTask;

StMapEvtTask gArrUart[1];


/********************************************************************************************************
* 函数：void RegistUartEvent(s32 event, s32 task_id);
* 描述:   注册串口事件，vshell需要这个事件来处理用户输入命令
* 参数:
* [1] event: 事件号
* [2] task_id: 任务ID
* 返回：无
* 注意：无
*********************************************************************************************************/
void RegistUartEvent(s32 event, s32 task_id)
{
	gArrUart[0].event = event;
	gArrUart[0].task_id = task_id;
}

/********************************************************************************************************
* 函数：void uart_init(u32 bound);
* 描述:   USART1 初始化
* 参数:
* [1] bound: 波特率
* 返回：无
* 注意：无
*********************************************************************************************************/
void Error_Handler()
{

}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  static DMA_HandleTypeDef hdma_tx;
  static DMA_HandleTypeDef hdma_rx;

  GPIO_InitTypeDef  GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO clock */
  USARTx_TX_GPIO_CLK_ENABLE();
  USARTx_RX_GPIO_CLK_ENABLE();
  /* Enable USARTx clock */
  USARTx_CLK_ENABLE();
  /* Enable DMA2 clock */
  DMAx_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = USARTx_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
  GPIO_InitStruct.Alternate = USARTx_TX_AF;

  HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);

  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = USARTx_RX_PIN;
  GPIO_InitStruct.Alternate = USARTx_RX_AF;

  HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);

  /*##-3- Configure the DMA streams ##########################################*/
  /* Configure the DMA handler for Transmission process */
  hdma_tx.Instance                 = USARTx_TX_DMA_STREAM;

  hdma_tx.Init.Channel             = USARTx_TX_DMA_CHANNEL;
  hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_tx.Init.Mode                = DMA_NORMAL;
  hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
  hdma_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
  hdma_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdma_tx);

  /* Associate the initialized DMA handle to the UART handle */
  __HAL_LINKDMA(huart, hdmatx, hdma_tx);

  /* Configure the DMA handler for reception process */
  hdma_rx.Instance                 = USARTx_RX_DMA_STREAM;

  hdma_rx.Init.Channel             = USARTx_RX_DMA_CHANNEL;
  hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_rx.Init.Mode                = DMA_CIRCULAR;
  hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
  hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
  hdma_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
  hdma_rx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

  HAL_DMA_Init(&hdma_rx);

  /* Associate the initialized DMA handle to the the UART handle */
  __HAL_LINKDMA(huart, hdmarx, hdma_rx);

  /*##-4- Configure the NVIC for DMA #########################################*/
  /* NVIC configuration for DMA transfer complete interrupt (USARTx_TX) */
  HAL_NVIC_SetPriority(USARTx_DMA_TX_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(USARTx_DMA_TX_IRQn);

  /* NVIC configuration for DMA transfer complete interrupt (USARTx_RX) */
  HAL_NVIC_SetPriority(USARTx_DMA_RX_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USARTx_DMA_RX_IRQn);

  /* NVIC configuration for USART TC interrupt */
  //huart->Instance->CR1 |= USART_CR1_IDLEIE;//enable idle int

  __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);

  HAL_NVIC_SetPriority(USARTx_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USARTx_IRQn);
}

/**
  * @brief UART MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO, DMA and NVIC configuration to their default state
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{

  static DMA_HandleTypeDef hdma_tx;
  static DMA_HandleTypeDef hdma_rx;

  /*##-1- Reset peripherals ##################################################*/
  USARTx_FORCE_RESET();
  USARTx_RELEASE_RESET();

  /*##-2- Disable peripherals and GPIO Clocks #################################*/
  /* Configure UART Tx as alternate function  */
  HAL_GPIO_DeInit(USARTx_TX_GPIO_PORT, USARTx_TX_PIN);
  /* Configure UART Rx as alternate function  */
  HAL_GPIO_DeInit(USARTx_RX_GPIO_PORT, USARTx_RX_PIN);

  /*##-3- Disable the DMA Streams ############################################*/
  /* De-Initialize the DMA Stream associate to transmission process */
  HAL_DMA_DeInit(&hdma_tx);
  /* De-Initialize the DMA Stream associate to reception process */
  HAL_DMA_DeInit(&hdma_rx);

  /*##-4- Disable the NVIC for DMA ###########################################*/
  HAL_NVIC_DisableIRQ(USARTx_DMA_TX_IRQn);
  HAL_NVIC_DisableIRQ(USARTx_DMA_RX_IRQn);
}


void UartxRecvStartup()
{
	  if(HAL_UART_Receive_DMA(&UartHandle, (uint8_t *)RecvBuff, RECV_BUF_SIZE) != HAL_OK)
	  {
		  /* Transfer error in reception process */
	      //while(1);
	  }
}

void HAL_UART1_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART1_RxHalfCpltCallback(UART_HandleTypeDef *huart);

void uart_init(u32 bound)
{

	UartHandle.Instance          = USARTx;

	UartHandle.Init.BaudRate     = bound;
	UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
	UartHandle.Init.StopBits     = UART_STOPBITS_1;
	UartHandle.Init.Parity       = UART_PARITY_NONE;
	UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	UartHandle.Init.Mode         = UART_MODE_TX_RX;
	UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

	if(HAL_UART_Init(&UartHandle) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

//	if (HAL_OK == HAL_UART_RegisterCallback(&UartHandle, HAL_UART_RX_COMPLETE_CB_ID, HAL_UART1_RxCpltCallback)) {
//
//	}
//	if (HAL_OK == HAL_UART_RegisterCallback(&UartHandle, HAL_UART_RX_HALFCOMPLETE_CB_ID, HAL_UART1_RxHalfCpltCallback)) {
//
//	}
	UartxRecvStartup();



 	//初始化RingBuf
 	gRingBuf = VOSRingBufBuild(gRxRingBuf, sizeof(gRxRingBuf));
}



/********************************************************************************************************
* 函数：s32 USART1_DMA_Send(u8 *data, s32 len);
* 描述:   MA 发送
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
s32 USART1_DMA_Send(u8 *data, s32 len)
{
	s32 ret = 0;
	s32 min = len < SEND_BUF_SIZE ? len : SEND_BUF_SIZE;

  if(HAL_UART_Transmit_DMA(&UartHandle, (uint8_t*)data, len)!= HAL_OK)
  {
  }
    return ret;
}





/********************************************************************************************************
* 函数：s32 vgetc(u8 *ch);
* 描述:   任务通过vgetc获取一个字符
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
s32 vgetc(u8 *ch)
{
	s32 ret = 0;
	u32 irq_save;
	irq_save = __vos_irq_save();
	if (gRingBuf) {
		ret = VOSRingBufGet(gRingBuf, ch, 1);
	}
	__vos_irq_restore(irq_save);

	return ret;

}

/********************************************************************************************************
* 函数：s32 vgets(u8 *buf, s32 len);
* 描述:   任务通过vget 获取一串字符
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
s32 vgets(u8 *buf, s32 len)
{
	s32 ret = 0;
	s32 irq_save;
	irq_save = __vos_irq_save();
	if (gRingBuf) {
		ret = VOSRingBufGet(gRingBuf, buf, len);
	}
	__vos_irq_restore(irq_save);
	return ret;
}

/********************************************************************************************************
* 函数：s32 peek_vgets(u8 *buf, s32 len);
* 描述:   任务通过peek_vgets窥视数据，但不取出
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
s32 peek_vgets(u8 *buf, s32 len)
{
	s32 ret = 0;
	s32 irq_save;
	irq_save = __vos_irq_save();
	if (gRingBuf) {
		ret = VOSRingBufPeekGet(gRingBuf, buf, len);
	}
	__vos_irq_restore(irq_save);
	return ret;
}

/********************************************************************************************************
* 函数：void vputs(s8 *str, s32 len);
* 描述:   任务通过vputs输出到串口
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
void vputs(s8 *str, s32 len)
{
	u32 irq_save;
	s32 i;
	for (i=0; i<len; i++) {
		HAL_UART_Transmit(&UartHandle, (uint8_t *)&str[i], 1, 100);
	}
}

/********************************************************************************************************
* 函数：void dma_vputs(s8 *str, s32 len);
* 描述:   MA 输出到串口
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
void dma_vputs(s8 *str, s32 len)
{
	u32 irq_save;
	s32 i;
	//USART_GetFlagStatus(USART1, USART_FLAG_TC);
	irq_save = __vos_irq_save();
	USART1_DMA_Send(str, len);
	__vos_irq_restore(irq_save);
}


/********************************************************************************************************
* 函数：int fputc(int ch, FILE *f);
* 描述:   阻塞输出到串
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
int fputc(int ch, FILE *f)
{
	USART1->SR = (uint16_t)~0x0040; //解决第一个字节发不出来问题
	while((USART1->SR&0X40)==0);
	USART1->DR = (u8) ch;
	return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	s32 ret = 0;
	//s32 len = 0;
	//len = RECV_BUF_SIZE - huart->hdmarx->Instance->NDTR;
	if (gRingBuf && RecvIdx <= RECV_BUF_SIZE) {
		ret = VOSRingBufSet(gRingBuf, RecvBuff+RecvIdx, RECV_BUF_SIZE-RecvIdx);
		//if (ret != len) {
			//kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
		//}
		RecvIdx = 0;
	}
}


void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
	s32 ret = 0;
	s32 len = 0;
	len = RECV_BUF_SIZE - huart->hdmarx->Instance->NDTR;
	RecvIdx = len;
	if (gRingBuf && len > 0) {
		ret = VOSRingBufSet(gRingBuf, RecvBuff, len);
		if (ret != len) {
			//kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
		}
	}
}


/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles DMA RX interrupt request.
  * @param  None
  * @retval None
  */
void USARTx_DMA_RX_IRQHandler(void)
{
	VOSIntEnter();
	HAL_DMA_IRQHandler(UartHandle.hdmarx);
	VOSIntExit ();
}

/**
  * @brief  This function handles DMA TX interrupt request.
  * @param  None
  * @retval None
  */
void USARTx_DMA_TX_IRQHandler(void)
{
	VOSIntEnter();
	HAL_DMA_IRQHandler(UartHandle.hdmatx);
	VOSIntExit ();
}

/**
  * @brief  This function handles USARTx interrupt request.
  * @param  None
  * @retval None
  */


void UART_IDLE_Callback(UART_HandleTypeDef *huart)
{
	s32 ret = 0;
	s32 len = 0;
	uint32_t tmp1, tmp2;

	tmp1 = __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_IDLE);

	if((tmp1 != RESET) && (tmp2 != RESET))
	{
		__HAL_UART_CLEAR_IDLEFLAG(huart);
		/* set uart state  ready*/
		huart->gState = HAL_UART_STATE_READY;
		/* Disable the rx  DMA peripheral */
		__HAL_DMA_DISABLE(huart->hdmarx);
		/*Clear the DMA Stream pending flags.*/
		__HAL_DMA_CLEAR_FLAG(huart->hdmarx,__HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmarx));

		/* Process Unlocked */
		__HAL_UNLOCK(huart->hdmarx);

		len = RECV_BUF_SIZE - huart->hdmarx->Instance->NDTR;

		if (gRingBuf && len-RecvIdx > 0) {
			ret = VOSRingBufSet(gRingBuf, RecvBuff+RecvIdx, len-RecvIdx);
			if (ret != len) {
				//kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
			}
	        if (gArrUart[0].event != 0) {
	        	VOSEventSet(gArrUart[0].task_id, gArrUart[0].event);
	        }
		}
		RecvIdx = 0;
		HAL_UART_Receive_DMA(&UartHandle, (uint8_t *)RecvBuff, RECV_BUF_SIZE);
		__HAL_DMA_ENABLE(huart->hdmarx);
	}
}

void USARTx_IRQHandler(void)
{
	VOSIntEnter();
	UART_IDLE_Callback(&UartHandle);
	HAL_UART_IRQHandler(&UartHandle);
	VOSIntExit ();
}

