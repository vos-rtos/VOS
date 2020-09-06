/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vringbuf.h
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
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
