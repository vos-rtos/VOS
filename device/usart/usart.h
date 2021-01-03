#ifndef __USART_H
#define __USART_H
#include "stdio.h"
#include "vos.h"
//#include "stm32f4xx_conf.h"

void uart_init(u32 bound);
int fputc(int ch, FILE *f);
s32 dma_vputs(s32 port, u8 *data, s32 len);
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
void RegistUartEvent(s32 port, s32 event, s32 task_id);
//void RegistUartEvent(s32 event, s32 task_id);

int uart_recvs(int port, unsigned char *buf, int len, unsigned int timeout_ms);
int uart_sends(int port, unsigned char *buf, int len, unsigned int timeout_ms);
void uart_close(int port);
int uart_open(int port, int baudrate, int wordlength, char *parity, int stopbits);

#define STD_OUT	2 //port 0 (uart1) print out

#endif


