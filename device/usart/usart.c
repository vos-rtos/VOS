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

enum {
	EVENT_UART_RECV = 0,
};

typedef struct StMapEvtTask{
	s32 event;
	s32 task_id;
}StMapEvtTask;

typedef struct StUartComm {
	UART_HandleTypeDef handle;
	StVOSRingBuf *ring_rx; //接收环形缓冲
	s32 ring_len;
	u8 *dma_buf;
	s32 dma_len;
	s32 RecvIdx;
	DMA_HandleTypeDef hdma_tx;
	DMA_HandleTypeDef hdma_rx;
	StMapEvtTask notify;
}StUartComm;

struct StUartComm gUartComm[6];

/********************************************************************************************************
* 函数：void RegistUartEvent(s32 event, s32 task_id);
* 描述:   注册串口事件，vshell需要这个事件来处理用户输入命令
* 参数:
* [1] event: 事件号
* [2] task_id: 任务ID
* 返回：无
* 注意：无
*********************************************************************************************************/
void RegistUartEvent(s32 port, s32 event, s32 task_id)
{
	gUartComm[port].notify.event = event;
	gUartComm[port].notify.task_id = task_id;
}

struct StUartComm *UartCommPtr(USART_TypeDef *uartx)
{
	int i = 0;
	for (i=0; i<sizeof(gUartComm)/sizeof(gUartComm[0]); i++) {
		if (gUartComm[i].handle.Instance == uartx) {
			return &gUartComm[i];
		}
	}
	return 0;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	struct StUartComm *pUart = UartCommPtr(huart->Instance);
	if (huart->Instance == USART1) {
		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_USART1_CLK_ENABLE();
		__HAL_RCC_DMA2_CLK_ENABLE();
		GPIO_InitStruct.Pin       = GPIO_PIN_9;
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_PULLUP;
		GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
		HAL_GPIO_Init( GPIOA, &GPIO_InitStruct);
		GPIO_InitStruct.Pin = GPIO_PIN_10;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		pUart->hdma_tx.Instance                 = DMA2_Stream7;
		pUart->hdma_tx.Init.Channel             = DMA_CHANNEL_4;
		pUart->hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
		pUart->hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
		pUart->hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
		pUart->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		pUart->hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		pUart->hdma_tx.Init.Mode                = DMA_NORMAL;
		pUart->hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
		pUart->hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		pUart->hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
		pUart->hdma_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
		pUart->hdma_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

		HAL_DMA_Init(&pUart->hdma_tx);
		__HAL_LINKDMA(huart, hdmatx, pUart->hdma_tx);

		pUart->hdma_rx.Instance                 = DMA2_Stream2;

		pUart->hdma_rx.Init.Channel             = DMA_CHANNEL_4;
		pUart->hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
		pUart->hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
		pUart->hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
		pUart->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		pUart->hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		pUart->hdma_rx.Init.Mode                = DMA_CIRCULAR;
		pUart->hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
		pUart->hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		pUart->hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
		pUart->hdma_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
		pUart->hdma_rx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

		HAL_DMA_Init(&pUart->hdma_rx);

		__HAL_LINKDMA(huart, hdmarx, pUart->hdma_rx);

		HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 1);
		HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

		HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

		__HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
		HAL_NVIC_SetPriority(USART1_IRQn, 0, 2);
		HAL_NVIC_EnableIRQ(USART1_IRQn);
	}
	else if (huart->Instance == USART3) {
		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_USART3_CLK_ENABLE();
		__HAL_RCC_DMA1_CLK_ENABLE();

		GPIO_InitStruct.Pin       = GPIO_PIN_10;
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_PULLUP;
		GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART3;

		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_11;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART3;

		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		pUart->hdma_tx.Instance                 = DMA1_Stream3;

		pUart->hdma_tx.Init.Channel             = DMA_CHANNEL_4;
		pUart->hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
		pUart->hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
		pUart->hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
		pUart->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		pUart->hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		pUart->hdma_tx.Init.Mode                = DMA_NORMAL;
		pUart->hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
		pUart->hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		pUart->hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
		pUart->hdma_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
		pUart->hdma_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

		HAL_DMA_Init(&pUart->hdma_tx);

		__HAL_LINKDMA(huart, hdmatx, pUart->hdma_tx);

		pUart->hdma_rx.Instance                 = DMA1_Stream1;

		pUart->hdma_rx.Init.Channel             = DMA_CHANNEL_4;
		pUart->hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
		pUart->hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
		pUart->hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
		pUart->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		pUart->hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		pUart->hdma_rx.Init.Mode                = DMA_CIRCULAR;
		pUart->hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
		pUart->hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		pUart->hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
		pUart->hdma_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
		pUart->hdma_rx.Init.PeriphBurst         = DMA_PBURST_SINGLE;

		HAL_DMA_Init(&pUart->hdma_rx);

		__HAL_LINKDMA(huart, hdmarx, pUart->hdma_rx);

		HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 1);
		HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

		HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

		__HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);

		HAL_NVIC_SetPriority(USART3_IRQn, 0, 2);
		HAL_NVIC_EnableIRQ(USART3_IRQn);
	}
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
	struct StUartComm *pUart = UartCommPtr(huart->Instance);

	if (huart->Instance == USART1) {
		__HAL_RCC_USART1_FORCE_RESET();
		__HAL_RCC_USART1_RELEASE_RESET();

		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10);

		HAL_DMA_DeInit(&pUart->hdma_tx);
		HAL_DMA_DeInit(&pUart->hdma_rx);

		HAL_NVIC_DisableIRQ(DMA2_Stream7_IRQn);
		HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn);
	}
	if (huart->Instance == USART3) {
		__HAL_RCC_USART3_FORCE_RESET();
		__HAL_RCC_USART3_RELEASE_RESET();

		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);

		HAL_DMA_DeInit(&pUart->hdma_tx);
		HAL_DMA_DeInit(&pUart->hdma_rx);
		
		HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn);
		HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
	}
}


int UartxRecvStartup(struct StUartComm *pUart)
{
	if(HAL_UART_Receive_DMA(&pUart->handle, pUart->dma_buf, pUart->dma_len) != HAL_OK) {
		//kprintf("error: UartxRecvStartup->HAL_UART_Receive_DMA error!\r\n");
		return -1;
	}
	return 0;
}


//s32 USART1_DMA_Send(u8 *data, s32 len)
//{
//	s32 ret = 0;
//	s32 min = len < SEND_BUF_SIZE ? len : SEND_BUF_SIZE;
//
//	if(HAL_UART_Transmit_DMA(&USART1, (uint8_t*)data, len)!= HAL_OK)
//	{
//	}
//	return ret;
//}
//




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
	struct StUartComm *pUart = &gUartComm[STD_OUT];
	irq_save = __vos_irq_save();
	if (pUart && pUart->ring_rx) {
		ret = VOSRingBufGet(pUart->ring_rx, ch, 1);
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
	struct StUartComm *pUart = &gUartComm[STD_OUT];
	irq_save = __vos_irq_save();
	if (pUart && pUart->ring_rx) {
		ret = VOSRingBufGet(pUart->ring_rx, buf, len);
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
	struct StUartComm *pUart = &gUartComm[STD_OUT];
	irq_save = __vos_irq_save();
	if (pUart && pUart->ring_rx) {
		ret = VOSRingBufPeekGet(pUart->ring_rx, buf, len);
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
	struct StUartComm *pUart = &gUartComm[STD_OUT];
	for (i=0; i<len; i++) {
		HAL_UART_Transmit(&pUart->handle, (uint8_t *)&str[i], 1, 100);
	}
}

/********************************************************************************************************
* 函数：void dma_vputs(s8 *str, s32 len);
* 描述:   MA 输出到串口
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
s32 dma_vputs(s32 port, u8 *data, s32 len)
{
	s32 ret = 0;
	u32 irq_save;
	struct StUartComm *pUart = &gUartComm[port];
	if (pUart) {
		irq_save = __vos_irq_save();
		if(HAL_UART_Transmit_DMA(pUart->handle.Instance, data, len)!= HAL_OK)
		{
			//error
		}
		__vos_irq_restore(irq_save);
	}
}


/********************************************************************************************************
* 函数：int fputc(int ch, FILE *f);
* 描述:   阻塞输出到串
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
int vfputc(s32 port, int ch, FILE *f)
{
	struct StUartComm *pUart = &gUartComm[port];
	pUart->handle.Instance->SR = (uint16_t)~0x0040; //解决第一个字节发不出来问题
	while((pUart->handle.Instance->SR&0X40)==0);
	pUart->handle.Instance->DR = (u8) ch;
	return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	s32 ret = 0;
	struct StUartComm *pUart = UartCommPtr(huart->Instance);
	if (pUart && pUart->ring_rx && pUart->dma_buf && pUart->RecvIdx <= pUart->dma_len) {
		ret = VOSRingBufSet(pUart->ring_rx, pUart->dma_buf+pUart->RecvIdx, pUart->dma_len-pUart->RecvIdx);
		//if (ret != len) {
			//kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
		//}
		pUart->RecvIdx = 0;
	}
}



void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
	s32 ret = 0;
	s32 len = 0;
	struct StUartComm *pUart = UartCommPtr(huart->Instance);
	len = pUart->dma_len - huart->hdmarx->Instance->NDTR;
	pUart->RecvIdx = len;
	if (pUart && pUart->ring_rx && pUart->dma_buf && len > 0) {
		ret = VOSRingBufSet(pUart->ring_rx, pUart->dma_buf, len);
//		if (ret != len) {
//			kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
//		}
	}
}

/***************************
 *
 * UART1 中断处理
 *
 ***************************/

void DMA2_Stream2_IRQHandler(void)
{
	struct StUartComm *pUart = UartCommPtr(USART1);
	VOSIntEnter();

	HAL_DMA_IRQHandler(pUart->handle.hdmarx);

	VOSIntExit ();
}
void DMA2_Stream7_IRQHandler(void)
{
	struct StUartComm *pUart = UartCommPtr(USART1);
	VOSIntEnter();

	HAL_DMA_IRQHandler(pUart->handle.hdmatx);

	VOSIntExit ();
}
void USART1_IRQHandler(void)
{
	struct StUartComm *pUart = UartCommPtr(USART1);
	VOSIntEnter();

	UART_IDLE_Callback(pUart);
	HAL_UART_IRQHandler(&pUart->handle);

	VOSIntExit ();
}

/***************************
 *
 * UART3 中断处理
 *
 ***************************/
void DMA1_Stream1_IRQHandler(void)
{
	struct StUartComm *pUart = UartCommPtr(USART3);
	VOSIntEnter();
	if (pUart) {
		HAL_DMA_IRQHandler(pUart->handle.hdmarx);
	}
	VOSIntExit ();
}
void DMA1_Stream3_IRQHandler(void)
{
	struct StUartComm *pUart = UartCommPtr(USART3);
	VOSIntEnter();
	if (pUart) {
		HAL_DMA_IRQHandler(pUart->handle.hdmatx);
	}
	VOSIntExit ();
}
void USART3_IRQHandler(void)
{
	struct StUartComm *pUart = UartCommPtr(USART3);
	VOSIntEnter();
	if (pUart) {
		UART_IDLE_Callback(pUart);
		HAL_UART_IRQHandler(&pUart->handle);
	}
	VOSIntExit ();
}


void UART_IDLE_Callback(struct StUartComm *pUart)
{
	s32 ret = 0;
	s32 len = 0;
	uint32_t tmp1, tmp2;

	UART_HandleTypeDef *huart = &pUart->handle;

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

		__HAL_DMA_CLEAR_FLAG(huart->hdmarx,__HAL_DMA_GET_HT_FLAG_INDEX(huart->hdmarx));

		/* Process Unlocked */
		__HAL_UNLOCK(huart->hdmarx);

		if (pUart) {
			len = pUart->dma_len - huart->hdmarx->Instance->NDTR;
			if (pUart->ring_rx && len-pUart->RecvIdx > 0) {
				ret = VOSRingBufSet(pUart->ring_rx, pUart->dma_buf+pUart->RecvIdx, len-pUart->RecvIdx);
				if (ret != len) {
					//kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
				}
				if (pUart->notify.event != 0) {
					VOSEventSet(pUart->notify.task_id, pUart->notify.event);
				}
			}
		}
		pUart->RecvIdx = 0;
		//HAL_UART_Receive_DMA(&UartHandle, (uint8_t *)RecvBuff, RECV_BUF_SIZE);
		if (pUart) {
			UartxRecvStartup(pUart);
		}
		__HAL_DMA_ENABLE(huart->hdmarx);
	}
}

int uart_open(int port, int baudrate, int wordlength, char *parity, int stopbits)
{
	int err = 0;
	USART_TypeDef *pUartInst = 0;

	struct StUartComm *pUart = &gUartComm[port];
	//memset(pUart, 0, sizeof(struct StUartComm));

	switch (port) {
	case 0:
		pUartInst = USART1;
		break;
	case 1:
		pUartInst = USART2;
		break;
	case 2:
		pUartInst = USART3;
		break;
	case 3:
		pUartInst = UART4;
		break;
	case 4:
		pUartInst = UART5;
		break;
	case 5:
		pUartInst = USART6;
		break;
	default:
		pUartInst = 0;
		err = -1;
		goto END;
	}

	pUart->handle.Instance          = pUartInst;

	pUart->handle.Init.BaudRate     = baudrate;
	if (wordlength == 8) {
		pUart->handle.Init.WordLength   = UART_WORDLENGTH_8B;
	}
	else if (wordlength == 9) {
		pUart->handle.Init.WordLength   = UART_WORDLENGTH_9B;
	}
	if (strcmp(parity, "none")==0) {
		pUart->handle.Init.Parity = UART_PARITY_NONE;
	}
	else if (strcmp(parity, "even")==0) {
		pUart->handle.Init.Parity = UART_PARITY_EVEN;
	}
	else if (strcmp(parity, "odd")==0) {
		pUart->handle.Init.Parity = UART_PARITY_ODD;
	}
	else {
		err = -2;
		goto END;
	}
	if (stopbits == 1) {
		pUart->handle.Init.StopBits     = UART_STOPBITS_1;
	}
	else if (stopbits == 2) {
		pUart->handle.Init.StopBits     = UART_STOPBITS_2;
	}
	else {
		err = -3;
		goto END;
	}

	pUart->handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	pUart->handle.Init.Mode         = UART_MODE_TX_RX;
	pUart->handle.Init.OverSampling = UART_OVERSAMPLING_16;

	if(HAL_UART_Init(&pUart->handle) != HAL_OK) {
		err = -4;
		goto END;
	}

	pUart->ring_rx = VOSRingBufCreate(pUart->ring_len = (pUart->ring_len?pUart->ring_len:(1024+20)));
	if (pUart->ring_rx == 0) {
		err = -5;
		goto END;
	}

	pUart->dma_buf = vmalloc(pUart->dma_len = (pUart->dma_len ? pUart->dma_len : 512));
	if (pUart->dma_buf == 0) {
		err = -6;
		goto END;
	}

	err = UartxRecvStartup(pUart);
	if (err < 0) {
		err = -7;
		goto END;
	}

END:
	return err;
}

int uart_recvs(int port, unsigned char *buf, int len, unsigned int timeout_ms)
{
	s32 ret = 0;
	s32 irq_save;
	u32 mark_time=0;
	struct StUartComm *pUart = &gUartComm[port];
	if (pUart == 0) {
		return -1;
	}

	mark_time = VOSGetTimeMs();
	while (1) {
		irq_save = __vos_irq_save();
		if (pUart->ring_rx) {
			ret = VOSRingBufGet(pUart->ring_rx, buf, len);
		}
		__vos_irq_restore(irq_save);
		if (ret > 0) { //有数据立即跳出
			break;
		}
		if (VOSGetTimeMs() - mark_time > timeout_ms) {
			break;
		}
		VOSTaskDelay(20);
	}
	return ret;
}

int uart_sends(int port, unsigned char *buf, int len, unsigned int timeout_ms)
{
	s32 i = 0;
	struct StUartComm *pUart = &gUartComm[port];
	if (pUart == 0) return -1;
	HAL_UART_Transmit(&pUart->handle,  buf, len, timeout_ms);
	return 0;
}

void uart_close(int port)
{

}


