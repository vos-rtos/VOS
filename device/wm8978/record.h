#ifndef __RECORD_H__
#define __RECORD_H__

#include "vos.h"
#include "wavplay.h"
#include "ff.h"

#define I2S_RX_DMA_BUF_SIZE    	4096		//定义RX DMA 数组大小

void rec_i2s_dma_rx_callback(void);
void recoder_enter_rec_mode(void);
void recoder_wav_init(__WaveHeader* wavhead);
void recoder_msg_show(u32 tsec,u32 kbps);
void recoder_remindmsg_show(u8 mode);
void recoder_new_pathname(u8 *pname, s32 len);
void wav_recorder(void);

#endif
