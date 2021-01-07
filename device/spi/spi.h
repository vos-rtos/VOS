#ifndef __SPI_H__
#define __SPI_H__
#include "common.h"
#include "vos.h"

s32 spi_open(s32 port, s32 BaudRatePrescaler);
s32 spi_read_write(s32 port, u8 ch, u32 timeout);
s32 spi_close(s32 port);

#endif

