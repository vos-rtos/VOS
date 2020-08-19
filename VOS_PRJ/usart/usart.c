#include "sys.h"
#include "vos.h"
#include "usart.h"

#include "vringbuf.h"

int fputc(int ch, FILE *f)
{ 	
	USART1->SR = (uint16_t)~0x0040; //解决第一个字节发不出来问题
	while((USART1->SR&0X40)==0);
	USART1->DR = (u8) ch;      
	return ch;
}
#define SEND_BUF_SIZE 1024
#define RECV_BUF_SIZE 1024
u8 SendBuff[SEND_BUF_SIZE];
u8 RecvBuff[RECV_BUF_SIZE];

#define RX_RINGBUF_SIZE (1024+20)
u8 gRxRingBuf[RX_RINGBUF_SIZE];
StVOSRingBuf *gRingBuf = 0;

void uart_init(u32 bound){

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);


	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	USART_Cmd(USART1, ENABLE);
	
	USART_ClearFlag(USART1, USART_FLAG_TC);

	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//DMA
 	DMA_USART1_Tx_Init(SendBuff, SEND_BUF_SIZE);
 	DMA_USART1_Rx_Init(RecvBuff, RECV_BUF_SIZE);

 	//初始化RingBuf
 	gRingBuf = VOSRingBufFormat(gRxRingBuf, sizeof(gRxRingBuf));
}

//s32 USART1_DMA_Send(u8 *data, s32 len)
//{
//	s32 ret = 0;
//	s32 min = len < SEND_BUF_SIZE ? len : SEND_BUF_SIZE;
//	memcpy(SendBuff, data, min);
//
//
//	USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);
//    MYDMA_Enable(DMA2_Stream7,min);
//
//	while(1)
//	{
//		if(DMA_GetFlagStatus(DMA2_Stream7,DMA_FLAG_TCIF7)!=RESET)
//		{
//			DMA_ClearFlag(DMA2_Stream7,DMA_FLAG_TCIF7);
//			break;
//		}
//		ret = DMA_GetCurrDataCounter(DMA2_Stream7);
//	}
//	return ret;
//}


s32 USART1_DMA_Send(u8 *data, s32 len)
{
	s32 ret = 0;
	s32 min = len < SEND_BUF_SIZE ? len : SEND_BUF_SIZE;
	memcpy(SendBuff, data, min);

    /* 等待空闲 */
//    while (UART1_Use_DMA_Tx_Flag);
//    UART1_Use_DMA_Tx_Flag = 1;
    /* 复制数据 */
    memcpy(SendBuff,data,len);
    /* 设置传输数据长度 */
    DMA_SetCurrDataCounter(DMA2_Stream7,len);
    /* 打开DMA,开始发送 */
    DMA_Cmd(DMA2_Stream7,ENABLE);
    return ret;
}

//volatile s32 gUartRxCnts = 0;
//volatile s32 gUartRxWrIdx = 0;
//volatile s32 gUartRxRdIdx = 0;
//u8 gUart1Buf[1024];
//u8 markxx = -1;

//s32 RingBufSet(u8 *buf, s32 len)
//{
//	s32 free_size = sizeof(gUart1Buf)-gUartRxCnts;
//	s32 copy_len = len < free_size ? len : free_size;
//	s32 left_size = 0;
//	if (copy_len > 0) {
//		if (gUartRxWrIdx >= gUartRxRdIdx) {
//			left_size = sizeof(gUart1Buf)-gUartRxWrIdx;
//			if (copy_len > left_size) {
//				memcpy(&gUart1Buf[gUartRxWrIdx], buf, left_size);
//				gUartRxRdIdx = copy_len-left_size;
//				memcpy(gUart1Buf, &buf[left_size], gUartRxRdIdx);
//			}
//			else {
//				memcpy(&gUart1Buf[gUartRxWrIdx], buf, copy_len);
//				gUartRxRdIdx += copy_len;
//			}
//		}
//		else {
//			left_size = gUartRxRdIdx-gUartRxWrIdx;
//			copy_len = copy_len < left_size ? copy_len : left_size;
//			memcpy(&gUart1Buf[gUartRxWrIdx], buf, copy_len);
//			gUartRxWrIdx += copy_len;
//		}
//	}
//	gUartRxCnts += copy_len;
//	gUartRxWrIdx %= sizeof(gUart1Buf);
//	return 0;
//}



#define USART_RX_USE_DMA	1

void USART1_IRQHandler(void)
{
	s32 len = 0;
	s32 ret = 0;
	u32 data = 0;
	u32 irq_save = 0;
	u8 Res;
	VOSIntEnter();

#if USART_RX_USE_DMA
    /* 接收完成中断 */
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        /* 清各种标志 */
    	data = USART1->SR;
    	data = USART1->DR;

        DMA_Cmd(DMA2_Stream2,DISABLE);
        DMA_ClearFlag(DMA2_Stream2,DMA_FLAG_TCIF2);

        len = RECV_BUF_SIZE - DMA_GetCurrDataCounter(DMA2_Stream2);

        irq_save = __local_irq_save();
        if (gRingBuf) {
        	ret = VOSRingBufSet(gRingBuf, RecvBuff, len);
        	kprintf("%d\r\n", ret);
        	if (ret != len) {
        		kprintf("%s: ringbuf overflow!\r\n", __FUNCTION__);
        	}
        }
        __local_irq_restore(irq_save);
        //data_check(RecvBuff, len);

        DMA_SetCurrDataCounter(DMA2_Stream2,RECV_BUF_SIZE);
        DMA_Cmd(DMA2_Stream2,ENABLE);
    }
    else {
    	/* 清各种标志 */
    	data = USART1->SR;
        data = USART1->DR;
    }
#else
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		Res = USART_ReceiveData(USART1);
		irq_save = __local_irq_save();
        if (gRingBuf) {
        	ret = VOSRingBufSet(gRingBuf, Res, 1);
        	if (ret != len) {
        		kprintf("%s: ringbuf overflow!\r\n", __FUNCTION__);
        	}
        }
		__local_irq_restore(irq_save);
	}
	else {
		//读SR, 再读DR，清除ORE错误
		data = (uint16_t)(USART1->SR & (uint16_t)0x01FF);
		data = (uint16_t)(USART1->DR & (uint16_t)0x01FF);
	}
#endif
	VOSIntExit ();
} 

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

void vputs(s8 *str, s32 len)
{
	u32 irq_save;
	s32 i;
	//irq_save = __vos_irq_save();
	for (i=0; i<len; i++) {
		USART_SendData(USART1, (u8)str[i]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}
	//__vos_irq_restore(irq_save);
}

void dma_vputs(s8 *str, s32 len)
{
	u32 irq_save;
	s32 i;
	irq_save = __vos_irq_save();
	USART1_DMA_Send(str, len);
	__vos_irq_restore(irq_save);
}

 



