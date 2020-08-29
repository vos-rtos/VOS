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

 	//初始化RingBuf
 	gRingBuf = VOSRingBufFormat(gRxRingBuf, sizeof(gRxRingBuf));
}

/********************************************************************************************************
* 函数： void DMA_USART1_Tx_Init(u8 *dma_buf, s32 len);
* 描述:   USART1 DMA发送初始化
* 参数:
* [1] dma_buf: dma buffer地址
* [2] len: dma buffer 长度
* 返回：无
* 注意：无
*********************************************************************************************************/
void DMA_USART1_Tx_Init(u8 *dma_buf, s32 len)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); // 使能DMA2时钟

    DMA_DeInit(DMA2_Stream7); // 配置使用DMA发送数据

    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               	// 配置DMA通道
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   	// 目的
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)dma_buf;             	// 源
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_MemoryToPeripheral;    	// 方向
    DMA_InitStructure.DMA_BufferSize          = len;                    		// 长度
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    	// 外设地址是否自增
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         	// 内存地址是否自增
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      	// 目的数据带宽
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      	// 源数据宽度
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;              	// 单次传输模式/循环传输模式
    DMA_InitStructure.DMA_Priority            = DMA_Priority_High;             	// DMA优先级
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          	// FIFO模式/直接模式
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; 	// FIFO大小
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       	// 单次传输
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;

    DMA_Init(DMA2_Stream7, &DMA_InitStructure); 	// 配置DMA
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE); 	// 使能DMA中断
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);  // 使能串口的DMA发送接口

    // 配置DMA中断优先级
    NVIC_InitStructure.NVIC_IRQChannel                   = DMA2_Stream7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 不使能DMA
    DMA_Cmd(DMA2_Stream7, DISABLE);
}

/********************************************************************************************************
* 函数： void DMA_USART1_Rx_Init(u8 *dma_buf, s32 len);
* 描述:   USART1 DMA接收初始化
* 参数:
* [1] dma_buf: dma buffer地址
* [2] len: dma buffer 长度
* 返回：无
* 注意：无
*********************************************************************************************************/
void DMA_USART1_Rx_Init(u8 *dma_buf, s32 len)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;


    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE); //使能DMA2时钟
    DMA_DeInit(DMA2_Stream2); //配置使用DMA接收数据

    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               	// 配置DMA通道
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   	// 源
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)dma_buf;             	// 目的
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralToMemory;    	// 方向
    DMA_InitStructure.DMA_BufferSize          = len;                    		// 长度
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    	// 外设地址是否自增
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         	// 内存地址是否自增
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      	// 目的数据带宽
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      	// 源数据宽度
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Circular;              // 单次传输模式/循环传输模式
    DMA_InitStructure.DMA_Priority            = DMA_Priority_VeryHigh;        	// DMA优先级
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          	// FIFO模式/直接模式
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; 	// FIFO大小
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       	// 单次传输
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;


    DMA_Init(DMA2_Stream2, &DMA_InitStructure); 	// 配置DMA
    DMA_ITConfig(DMA2_Stream2, DMA_IT_HT, ENABLE); 	// 使能DMA中断
    USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);  	// 使能串口的DMA接收

    // 配置DMA中断优先级
    NVIC_InitStructure.NVIC_IRQChannel                   = DMA2_Stream2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_Cmd(DMA2_Stream2,ENABLE);   // 使能DMA
}

/********************************************************************************************************
* 函数：void DMA2_Stream7_IRQHandler(void);
* 描述:   USART1 DMA 发送中断处理
* 参数:   无
* 返回：无
* 注意：无
*********************************************************************************************************/
void DMA2_Stream7_IRQHandler(void)
{
	VOSIntEnter();
    if(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7) != RESET)
    {
        // 清除标志位
        DMA_ClearFlag(DMA2_Stream7,DMA_IT_TCIF7);
        // 关闭DMA
        DMA_Cmd(DMA2_Stream7,DISABLE);
        /* 打开发送完成中断,确保最后一个字节发送成功 */
        //USART_ITConfig(USART1,USART_IT_TC,ENABLE);
    }
	VOSIntExit ();
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
	memcpy(SendBuff, data, min);

    /* 等待空闲 */
//    while (UART1_Use_DMA_Tx_Flag);
//    UART1_Use_DMA_Tx_Flag = 1;
    // 复制数据
    memcpy(SendBuff,data,len);
    // 设置传输数据长度
    DMA_SetCurrDataCounter(DMA2_Stream7,len);
    // 打开DMA,开始发送
    DMA_Cmd(DMA2_Stream7,ENABLE);
    return ret;
}

/********************************************************************************************************
* 函数：void USART1_IRQHandler(void);
* 描述:   USART1 接收中断处理， 接收线空闲状态监测中断
* 参数:   无
* 返回：无
* 注意：无
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
    // 接收完成中断
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        // 清各种标志
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
    	// 清各种标志
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

/********************************************************************************************************
* 函数：void DMA2_Stream2_IRQHandler(void);
* 描述:   USART1 DMA 接收中断处理
* 参数:   无
* 返回：无
* 注意：无
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
	//irq_save = __vos_irq_save();
	USART_GetFlagStatus(USART1, USART_FLAG_TC);
	for (i=0; i<len; i++) {
		USART_SendData(USART1, (u8)str[i]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}
	//__vos_irq_restore(irq_save);
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
	USART_GetFlagStatus(USART1, USART_FLAG_TC);
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

