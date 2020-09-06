/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vringbuf.h
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/
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
StVOSRingBuf *VOSRingBufBuild(u8 *buf, s32 len);
s32 VOSRingBufSet(StVOSRingBuf *ring, u8 *buf, s32 len);
s32 VOSRingBufGet(StVOSRingBuf *ring, u8 *buf, s32 len);
s32 VOSRingBufPeekGet(StVOSRingBuf *ring, u8 *buf, s32 len);
#endif
