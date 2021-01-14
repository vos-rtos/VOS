#ifndef __MIC_H__
#define __MIC_H__
#include "vos.h"

s32 mic_open(s32 port);

s32 mic_recvs(s32 port, u8 *buf, s32 len, u32 timeout_ms);

s32 mic_ctrl(s32 port, s32 option, void *value, s32 len);

s32 mic_close(s32 port);

#endif
