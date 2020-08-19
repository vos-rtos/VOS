#include "sys.h"
#include "vos.h"
#include "usart.h"

void MYDMA_Config(DMA_Stream_TypeDef *DMA_Streamx,u32 chx,u32 par,u32 mar,u16 ndtr)
{
	DMA_InitTypeDef  DMA_InitStructure;

	if((u32)DMA_Streamx>(u32)DMA2)
	{
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);

	}else
	{
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);
	}
	DMA_DeInit(DMA_Streamx);

	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}

  DMA_InitStructure.DMA_Channel = chx;
  DMA_InitStructure.DMA_PeripheralBaseAddr = par;
  DMA_InitStructure.DMA_Memory0BaseAddr = mar;
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_BufferSize = ndtr;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA_Streamx, &DMA_InitStructure);
}

//void DMA_USART1_Config(DMA_Stream_TypeDef *DMA_Streamx,u32 chx,u32 par,u32 mar,u16 ndtr)
//{
//	DMA_InitTypeDef  DMA_InitStructure;
//
//	if((u32)DMA_Streamx>(u32)DMA2)
//	{
//	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);
//
//	}else
//	{
//	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);
//	}
//	DMA_DeInit(DMA_Streamx);
//
//	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}
//
//  DMA_InitStructure.DMA_Channel = chx;
//  DMA_InitStructure.DMA_PeripheralBaseAddr = par;
//  DMA_InitStructure.DMA_Memory0BaseAddr = mar;
//  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
//  DMA_InitStructure.DMA_BufferSize = ndtr;
//  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
//  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
//  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
//  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
//  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
//  DMA_Init(DMA_Streamx, &DMA_InitStructure);
//}

void DMA_USART1_Tx_Init(u8 *dma_buf, s32 len)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;


    /* 1.使能DMA2时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    /* 2.配置使用DMA发送数据 */
    DMA_DeInit(DMA2_Stream7);

    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               /* 配置DMA通道 */
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   /* 目的 */
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)dma_buf;             /* 源 */
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_MemoryToPeripheral;    /* 方向 */
    DMA_InitStructure.DMA_BufferSize          = len;                    		/* 长度 */
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    /* 外设地址是否自增 */
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         /* 内存地址是否自增 */
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      /* 目的数据带宽 */
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      /* 源数据宽度 */
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;              /* 单次传输模式/循环传输模式 */
    DMA_InitStructure.DMA_Priority            = DMA_Priority_High;             /* DMA优先级 */
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          /* FIFO模式/直接模式 */
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; /* FIFO大小 */
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       /* 单次传输 */
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;

    /* 3. 配置DMA */
    DMA_Init(DMA2_Stream7, &DMA_InitStructure);

    /* 4.使能DMA中断 */
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);

    /* 5.使能串口的DMA发送接口 */
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);

    /* 6. 配置DMA中断优先级 */
    NVIC_InitStructure.NVIC_IRQChannel                   = DMA2_Stream7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 7.不使能DMA */
    DMA_Cmd(DMA2_Stream7, DISABLE);
}

void DMA_USART1_Rx_Init(u8 *dma_buf, s32 len)
{
    DMA_InitTypeDef DMA_InitStructure;

    /* 1.使能DMA2时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    /* 2.配置使用DMA接收数据 */
    DMA_DeInit(DMA2_Stream2);

    DMA_InitStructure.DMA_Channel             = DMA_Channel_4;               	/* 配置DMA通道 */
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)(&(USART1->DR));   	/* 源 */
    DMA_InitStructure.DMA_Memory0BaseAddr     = (uint32_t)dma_buf;             	/* 目的 */
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralToMemory;    	/* 方向 */
    DMA_InitStructure.DMA_BufferSize          = len;                    		/* 长度 */
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    	/* 外设地址是否自增 */
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;         	/* 内存地址是否自增 */
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_MemoryDataSize_Byte;      	/* 目的数据带宽 */
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      	/* 源数据宽度 */
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Normal;              	/* 单次传输模式/循环传输模式 */
    DMA_InitStructure.DMA_Priority            = DMA_Priority_VeryHigh;        	/* DMA优先级 */
    DMA_InitStructure.DMA_FIFOMode            = DMA_FIFOMode_Disable;          	/* FIFO模式/直接模式 */
    DMA_InitStructure.DMA_FIFOThreshold       = DMA_FIFOThreshold_HalfFull; 	/* FIFO大小 */
    DMA_InitStructure.DMA_MemoryBurst         = DMA_MemoryBurst_Single;       	/* 单次传输 */
    DMA_InitStructure.DMA_PeripheralBurst     = DMA_PeripheralBurst_Single;

    /* 3. 配置DMA */
    DMA_Init(DMA2_Stream2, &DMA_InitStructure);

    /* 4.由于接收不需要DMA中断，故不设置DMA中断 */

    /* 5.使能串口的DMA接收 */
    USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);

    /* 6. 由于接收不需要DMA中断，故不能配置DMA中断优先级 */

    /* 7.使能DMA */
    DMA_Cmd(DMA2_Stream2,ENABLE);
}

void DMA2_Stream7_IRQHandler(void)
{
	VOSIntEnter();
    if(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7) != RESET)
    {
        /* 清除标志位 */
        DMA_ClearFlag(DMA2_Stream7,DMA_IT_TCIF7);
        /* 关闭DMA */
        DMA_Cmd(DMA2_Stream7,DISABLE);
        /* 打开发送完成中断,确保最后一个字节发送成功 */
        //USART_ITConfig(USART1,USART_IT_TC,ENABLE);
    }
	VOSIntExit ();
}

void MYDMA_Enable(DMA_Stream_TypeDef *DMA_Streamx,u16 ndtr)
{

	DMA_Cmd(DMA_Streamx, DISABLE);

	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}

	DMA_SetCurrDataCounter(DMA_Streamx,ndtr);

	DMA_Cmd(DMA_Streamx, ENABLE);
}






