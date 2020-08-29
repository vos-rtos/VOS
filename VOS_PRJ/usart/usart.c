/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: sart.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/

#include "usart.h"
#include "vos.h"
#include "vringbuf.h"

#define USART_RX_USE_DMA	1


#define SEND_BUF_SIZE 512
#define RECV_BUF_SIZE 512
u8 SendBuff[SEND_BUF_SIZE];
u8 RecvBuff[RECV_BUF_SIZE];

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
* ������void RegistUartEvent(s32 event, s32 task_id);
* ����:   ע�ᴮ���¼���vshell��Ҫ����¼��������û���������
* ����:
* [1] event: �¼���
* [2] task_id: ����ID
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void RegistUartEvent(s32 event, s32 task_id)
{
	gArrUart[0].event = event;
	gArrUart[0].task_id = task_id;
}

/********************************************************************************************************
* ������void uart_init(u32 bound);
* ����:   USART1 ��ʼ��
* ����:
* [1] bound: ������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void uart_init(u32 bound)
{

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

 	//��ʼ��RingBuf
 	gRingBuf = VOSRingBufFormat(gRxRingBuf, sizeof(gRxRingBuf));
}

/********************************************************************************************************
* ������ void DMA_USART1_Tx_Init(u8 *dma_buf, s32 len);
* ����:   USART1 DMA���ͳ�ʼ��
* ����:
* [1] dma_buf: dma buffer��ַ
* [2] len: dma buffer ����
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void DMA_USART1_Tx_Init(u8 *dma_buf, s32 len)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); // ʹ��DMA2ʱ��

    DMA_DeInit(DMA2_Stream7); // ����ʹ��DMA��������

    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               	// ����DMAͨ��
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   	// Ŀ��
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)dma_buf;             	// Դ
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_MemoryToPeripheral;    	// ����
    DMA_InitStructure.DMA_BufferSize          = len;                    		// ����
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    	// �����ַ�Ƿ�����
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         	// �ڴ��ַ�Ƿ�����
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      	// Ŀ�����ݴ���
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      	// Դ���ݿ��
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;              	// ���δ���ģʽ/ѭ������ģʽ
    DMA_InitStructure.DMA_Priority            = DMA_Priority_High;             	// DMA���ȼ�
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          	// FIFOģʽ/ֱ��ģʽ
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; 	// FIFO��С
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       	// ���δ���
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;

    DMA_Init(DMA2_Stream7, &DMA_InitStructure); 	// ����DMA
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE); 	// ʹ��DMA�ж�
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);  // ʹ�ܴ��ڵ�DMA���ͽӿ�

    // ����DMA�ж����ȼ�
    NVIC_InitStructure.NVIC_IRQChannel                   = DMA2_Stream7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // ��ʹ��DMA
    DMA_Cmd(DMA2_Stream7, DISABLE);
}

/********************************************************************************************************
* ������ void DMA_USART1_Rx_Init(u8 *dma_buf, s32 len);
* ����:   USART1 DMA���ճ�ʼ��
* ����:
* [1] dma_buf: dma buffer��ַ
* [2] len: dma buffer ����
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void DMA_USART1_Rx_Init(u8 *dma_buf, s32 len)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;


    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); //ʹ��DMA2ʱ��
    DMA_DeInit(DMA2_Stream2); //����ʹ��DMA��������

    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               	// ����DMAͨ��
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   	// Դ
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)dma_buf;             	// Ŀ��
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralToMemory;    	// ����
    DMA_InitStructure.DMA_BufferSize          = len;                    		// ����
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    	// �����ַ�Ƿ�����
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         	// �ڴ��ַ�Ƿ�����
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      	// Ŀ�����ݴ���
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      	// Դ���ݿ��
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Circular;              // ���δ���ģʽ/ѭ������ģʽ
    DMA_InitStructure.DMA_Priority            = DMA_Priority_VeryHigh;        	// DMA���ȼ�
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          	// FIFOģʽ/ֱ��ģʽ
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; 	// FIFO��С
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       	// ���δ���
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;


    DMA_Init(DMA2_Stream2, &DMA_InitStructure); 	// ����DMA
    DMA_ITConfig(DMA2_Stream2, DMA_IT_HT, ENABLE); 	// ʹ��DMA�ж�
    USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);  	// ʹ�ܴ��ڵ�DMA����

    // ����DMA�ж����ȼ�
    NVIC_InitStructure.NVIC_IRQChannel                   = DMA2_Stream2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_Cmd(DMA2_Stream2,ENABLE);   // ʹ��DMA
}

/********************************************************************************************************
* ������void DMA2_Stream7_IRQHandler(void);
* ����:   USART1 DMA �����жϴ���
* ����:   ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void DMA2_Stream7_IRQHandler(void)
{
	VOSIntEnter();
    if(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7) != RESET)
    {
        // �����־λ
        DMA_ClearFlag(DMA2_Stream7,DMA_IT_TCIF7);
        // �ر�DMA
        DMA_Cmd(DMA2_Stream7,DISABLE);
        /* �򿪷�������ж�,ȷ�����һ���ֽڷ��ͳɹ� */
        //USART_ITConfig(USART1,USART_IT_TC,ENABLE);
    }
	VOSIntExit ();
}

/********************************************************************************************************
* ������s32 USART1_DMA_Send(u8 *data, s32 len);
* ����:   MA ����
* ����:   ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
s32 USART1_DMA_Send(u8 *data, s32 len)
{
	s32 ret = 0;
	s32 min = len < SEND_BUF_SIZE ? len : SEND_BUF_SIZE;
	memcpy(SendBuff, data, min);

    /* �ȴ����� */
//    while (UART1_Use_DMA_Tx_Flag);
//    UART1_Use_DMA_Tx_Flag = 1;
    // ��������
    memcpy(SendBuff,data,len);
    // ���ô������ݳ���
    DMA_SetCurrDataCounter(DMA2_Stream7,len);
    // ��DMA,��ʼ����
    DMA_Cmd(DMA2_Stream7,ENABLE);
    return ret;
}

/********************************************************************************************************
* ������void USART1_IRQHandler(void);
* ����:   USART1 �����жϴ��� �����߿���״̬����ж�
* ����:   ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void USART1_IRQHandler(void)
{
	s32 len = 0;
	s32 ret = 0;
	u32 data = 0;
	u32 irq_save = 0;
	u8 Res;
	VOSIntEnter();

#if USART_RX_USE_DMA
    // ��������ж�
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        // ����ֱ�־
    	data = USART1->SR;
    	data = USART1->DR;

        irq_save = __local_irq_save();
        DMA_Cmd(DMA2_Stream2,DISABLE);
        //DMA_ClearFlag(DMA2_Stream2,DMA_FLAG_TCIF2);

        len = RECV_BUF_SIZE - DMA_GetCurrDataCounter(DMA2_Stream2);
        if (gRingBuf && len > 0) {
        	ret = VOSRingBufSet(gRingBuf, RecvBuff, len);
        	if (ret != len) {
        		kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
        	}
        }
        //data_check(RecvBuff, len);
        DMA_SetCurrDataCounter(DMA2_Stream2,RECV_BUF_SIZE);
        DMA_Cmd(DMA2_Stream2,ENABLE);
        if (gArrUart[0].event != 0) {
        	VOSEventSet(gArrUart[0].task_id, gArrUart[0].event);
        }
        __local_irq_restore(irq_save);
    }
    else {
    	// ����ֱ�־
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
		//��SR, �ٶ�DR�����ORE����
		data = (uint16_t)(USART1->SR & (uint16_t)0x01FF);
		data = (uint16_t)(USART1->DR & (uint16_t)0x01FF);
	}
#endif
	VOSIntExit ();
} 

/********************************************************************************************************
* ������void DMA2_Stream2_IRQHandler(void);
* ����:   USART1 DMA �����жϴ���
* ����:   ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void DMA2_Stream2_IRQHandler(void)
{
	s32 ret;
	s32 len;
	s32 irq_save;

	VOSIntEnter();
	if(DMA_GetFlagStatus(DMA2_Stream2,DMA_IT_HTIF2)!=RESET)
	{
        irq_save = __local_irq_save();
		DMA_Cmd(DMA2_Stream2,DISABLE);
        DMA_ClearFlag(DMA2_Stream2,DMA_IT_HTIF2);
        len = RECV_BUF_SIZE - DMA_GetCurrDataCounter(DMA2_Stream2);
        if (gRingBuf && len>0) {
        	ret = VOSRingBufSet(gRingBuf, RecvBuff, len);
        	if (ret != len) {
        		kprintf("warning: %s->VOSRingBufSet overflow!\r\n", __FUNCTION__);
        	}
        }
        DMA_SetCurrDataCounter(DMA2_Stream2,RECV_BUF_SIZE);
        DMA_Cmd(DMA2_Stream2,ENABLE);
		__local_irq_restore(irq_save);
	}
	VOSIntExit ();
}

/********************************************************************************************************
* ������s32 vgetc(u8 *ch);
* ����:   ����ͨ��vgetc��ȡһ���ַ�
* ����:   ��
* ���أ���
* ע�⣺��
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
* ������s32 vgets(u8 *buf, s32 len);
* ����:   ����ͨ��vget ��ȡһ���ַ�
* ����:   ��
* ���أ���
* ע�⣺��
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
* ������s32 peek_vgets(u8 *buf, s32 len);
* ����:   ����ͨ��peek_vgets�������ݣ�����ȡ��
* ����:   ��
* ���أ���
* ע�⣺��
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
* ������void vputs(s8 *str, s32 len);
* ����:   ����ͨ��vputs���������
* ����:   ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void vputs(s8 *str, s32 len)
{
	u32 irq_save;
	s32 i;
	//irq_save = __vos_irq_save();
	USART_GetFlagStatus(USART1, USART_FLAG_TC);
	for (i=0; i<len; i++) {
		USART_SendData(USART1, (u8)str[i]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}
	//__vos_irq_restore(irq_save);
}

/********************************************************************************************************
* ������void dma_vputs(s8 *str, s32 len);
* ����:   MA ���������
* ����:   ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void dma_vputs(s8 *str, s32 len)
{
	u32 irq_save;
	s32 i;
	USART_GetFlagStatus(USART1, USART_FLAG_TC);
	irq_save = __vos_irq_save();
	USART1_DMA_Send(str, len);
	__vos_irq_restore(irq_save);
}


/********************************************************************************************************
* ������int fputc(int ch, FILE *f);
* ����:   �����������
* ����:   ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
int fputc(int ch, FILE *f)
{
	USART1->SR = (uint16_t)~0x0040; //�����һ���ֽڷ�����������
	while((USART1->SR&0X40)==0);
	USART1->DR = (u8) ch;
	return ch;
}

