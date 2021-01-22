#ifndef __SPI_H__
#define __SPI_H__
#include "common.h"
#include "vos.h"

enum {
	PARA_SPI_DIRECTION = 0,
	PARA_SPI_DATASIZE,
	PARA_SPI_CLK_POLARITY,
	PARA_SPI_CLK_PHASE,
	PARA_SPI_BAUDRATE_PRESCALER,
};

s32 spi_open(s32 port, s32 BaudRatePrescaler);
s32 spi_read_write(s32 port, u8 ch, u32 timeout);
s32 spi_close(s32 port);

#endif

