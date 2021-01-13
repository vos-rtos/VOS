#ifndef __AUDIO_H__
#define __AUDIO_H__
#include "vos.h"

enum {
	AUDIO_ADC_16BIT = 0,
	AUDIO_ADC_24BIT,
};

enum {
	AUDIO_OPT_AUDIO_SAMPLE = 0,
};

typedef enum
{
    audio_sample_rate_11025 = 11025,
    audio_sample_rate_22050 = 22050,
    audio_sample_rate_44100 = 44100,
    audio_sample_rate_12000 = 12000,
    audio_sample_rate_24000 = 24000,
    audio_sample_rate_48000 = 48000,
    audio_sample_rate_8000  = 8000,
    audio_sample_rate_16000 = 16000,
    audio_sample_rate_32000 = 32000,
};

s32 audio_open(s32 port, s32 data_bits);

s32 audio_sends(s32 port, u8 *buf, s32 len, u32 timeout_ms);

s32 audio_ctrl(s32 port, s32 option, void *value, s32 len);

s32 audio_close(s32 port);


#endif
