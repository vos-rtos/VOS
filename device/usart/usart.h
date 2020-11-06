#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "vos.h"
//#include "stm32f4xx_conf.h"

void uart_init(u32 bound);
int fputc(int ch, FILE *f);
void dma_vputs(s8 *str, s32 len);
void vputs(s8 *str, s32 len);
s32 peek_vgets(u8 *buf, s32 len);
s32 vgets(u8 *buf, s32 len);
s32 vgetc(u8 *ch);
void DMA2_Stream2_IRQHandler(void);
void USART1_IRQHandler(void);
s32 USART1_DMA_Send(u8 *data, s32 len);
void DMA2_Stream7_IRQHandler(void);
void DMA_USART1_Rx_Init(u8 *dma_buf, s32 len);
void DMA_USART1_Tx_Init(u8 *dma_buf, s32 len);

void uart_init(u32 bound);
void RegistUartEvent(s32 event, s32 task_id);


#endif


