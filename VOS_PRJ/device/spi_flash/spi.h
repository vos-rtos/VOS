#ifndef __SPI_H__
#define __SPI_H__
#include "stm32f4xx.h"

void SPI1_Init(void);
void SPI1_SetSpeed(u8 SpeedSet);
u8 SPI1_ReadWriteByte(u8 TxData);
		 
#endif

