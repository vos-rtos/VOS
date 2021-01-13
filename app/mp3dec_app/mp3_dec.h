#ifndef __MP3_DEC__
#define __MP3_DEC__

#include "vos.h"

#define MP3_DEBUG 1

s32 mp3_dec_init();
s32 mp3_dec_play(u8 *buf, s32 len, u32 timeout);
s32 mp3_dec_file(s8 *path);
#endif
