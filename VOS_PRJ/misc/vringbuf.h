//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-20: initial by vincent.
//------------------------------------------------------
#ifndef __V_RING_BUF_H__
#define __V_RING_BUF_H__

#include "vtype.h"

typedef struct StVOSRingBuf {
	u8 *buf;
	s32 max;
	volatile s32 cnts;
	volatile s32 idx_wr;
	volatile s32 idx_rd;
}StVOSRingBuf;

StVOSRingBuf *VOSRingBufCreate(u8 *buf, s32 len);
void VOSRingBufDelete(StVOSRingBuf *pnew);
StVOSRingBuf *VOSRingBufFormat(u8 *buf, s32 len);
s32 VOSRingBufSet(StVOSRingBuf *ring, u8 *buf, s32 len);
s32 VOSRingBufGet(StVOSRingBuf *ring, u8 *buf, s32 len);

#endif
