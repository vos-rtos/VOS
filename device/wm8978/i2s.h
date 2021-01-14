#ifndef __I2S_H__
#define __I2S_H__

#include "vos.h"

enum {
	MODE_I2S_PLAYER = 0,
	MODE_I2S_RECORDER,
};
void i2s_mode_set(s32 port, s32 mode);
s32 i2s_open(s32 port, u32 I2S_Standard, u32 I2S_Mode, u32 I2S_Clock_Polarity, u32 I2S_DataFormat);
s32 i2s_recvs(s32 port, u8 *buf, s32 len, u32 timeout_ms);
s32 i2s_sends(s32 port, u8 *buf, s32 len, u32 timeout_ms);
void i2s_close(s32 port);

void i2s_tx_dma_start(s32 port);
void i2s_tx_dma_stop(s32 port);

void i2s_rx_dma_start(s32 port);
void i2s_rx_dma_stop(s32 port);
#endif
