#ifndef __SAI_H
#define __SAI_H
#include "vos.h"
#include "common.h"

enum {
	MODE_SAI_PLAYER = 0,
	MODE_SAI_RECORDER,
};
void sai_mode_set(s32 port, s32 mode);
s32 sai_open(s32 port, u32 I2S_Mode, u32 I2S_Clock_Polarity, u32 I2S_DataFormat);
s32 sai_recvs(s32 port, u8 *buf, s32 len, u32 timeout_ms);
s32 sai_sends(s32 port, u8 *buf, s32 len, u32 timeout_ms);
void sai_close(s32 port);

void sai_tx_dma_start(s32 port);
void sai_tx_dma_stop(s32 port);

void sai_rx_dma_start(s32 port);
void sai_rx_dma_stop(s32 port);

//extern SAI_HandleTypeDef SAI1A_Handler;        //SAI1 Block A句柄
//extern SAI_HandleTypeDef SAI1B_Handler;        //SAI1 Block B句柄
//extern DMA_HandleTypeDef SAI1_TXDMA_Handler;   //DMA发送句柄
//extern DMA_HandleTypeDef SAI1_RXDMA_Handler;   //DMA接收句柄
//
//extern void (*sai_tx_callback)(void);		    //sai tx回调函数指针
//extern void (*sai_rx_callback)(void);		    //sai rx回调函数
//
//void SAIA_Init(u32 mode,u32 cpol,u32 datalen);
//void SAIB_Init(u32 mode,u32 cpol,u32 datalen);
//void SAIA_DMA_Enable(void);
//void SAIB_DMA_Enable(void);
//u8 SAIA_SampleRate_Set(u32 samplerate);
//void SAIA_TX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width);
//void SAIA_RX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width);
//void SAI_Play_Start(void);
//void SAI_Play_Stop(void);
//void SAI_Rec_Start(void);
//void SAI_Rec_Stop(void);
#endif





















