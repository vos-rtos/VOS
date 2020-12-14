#ifndef __SPI_H__
#define __SPI_H__
#include "common.h"

void SPI1_Init(void);
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler);
u8 SPI1_ReadWriteByte(u8 TxData);

#endif

