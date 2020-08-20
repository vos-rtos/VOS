//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-20: initial by vincent.
//------------------------------------------------------
#include "vringbuf.h"

StVOSRingBuf *VOSRingBufCreate(u8 *buf, s32 len)
{
	StVOSRingBuf *pnew = (StVOSRingBuf *)malloc(sizeof(StVOSRingBuf));
	if (pnew) {
		pnew->buf = buf;
		pnew->max = len;
		pnew->cnts = 0;
		pnew->idx_wr = 0;
		pnew->idx_rd = 0;
	}
	return pnew;
}

void VOSRingBufDelete(StVOSRingBuf *pnew)
{
	if (pnew) {
		free(pnew);
		pnew = 0;
	}
}

//强制格式化出头部
StVOSRingBuf *VOSRingBufFormat(u8 *buf, s32 len)
{
	if (len < sizeof(StVOSRingBuf)) return 0;
	StVOSRingBuf *phead = (StVOSRingBuf*)buf;

	phead->buf = phead+1;
	phead->max = len-sizeof(StVOSRingBuf);
	phead->cnts = 0;
	phead->idx_wr = 0;
	phead->idx_rd = 0;
	return phead;
}

s32 VOSRingBufSet(StVOSRingBuf *ring, u8 *buf, s32 len)
{
	s32 right_size = 0;
	s32 left_size = 0;
	s32 copy_len = 0;

	if (ring==0) return -1;

	copy_len = len < (ring->max-ring->cnts) ? len : (ring->max-ring->cnts);


	if (copy_len > 0) {
		if (ring->max - ring->idx_wr >= copy_len) {
			memcpy(&ring->buf[ring->idx_wr], buf, copy_len);
			ring->idx_wr += copy_len;
		}
		else {
			right_size = ring->max - ring->idx_wr;
			memcpy(&ring->buf[ring->idx_wr], buf, right_size);
			left_size = copy_len - right_size;
			memcpy(ring->buf, &buf[right_size], left_size);
			ring->idx_wr = left_size;
		}
		ring->idx_wr %= ring->max;
		ring->cnts += copy_len;
	}
	return copy_len;
}

s32 VOSRingBufGet(StVOSRingBuf *ring, u8 *buf, s32 len)
{
	s32 right_size = 0;
	s32 left_size = 0;
	s32 copy_len = 0;

	if (ring==0) return -1;

	copy_len = len < ring->cnts ? len : ring->cnts;

	if (copy_len > 0) {
		if (ring->max - ring->idx_rd >= copy_len) {
			memcpy(buf, &ring->buf[ring->idx_rd], copy_len);
			ring->idx_rd += copy_len;
		}
		else {
			right_size = ring->max - ring->idx_rd;
			memcpy(buf, &ring->buf[ring->idx_rd], right_size);
			left_size = copy_len - right_size;
			memcpy(&buf[right_size], ring->buf, left_size);
			ring->idx_rd = left_size;
		}
		ring->idx_rd %= ring->max;
		ring->cnts -= copy_len;
	}
	return copy_len;
}

s32 VOSRingBufPeekGet(StVOSRingBuf *ring, u8 *buf, s32 len)
{
	s32 right_size = 0;
	s32 left_size = 0;
	s32 copy_len = 0;

	if (ring==0) return -1;

	copy_len = len < ring->cnts ? len : ring->cnts;

	if (copy_len > 0) {
		if (ring->max - ring->idx_rd >= copy_len) {
			memcpy(buf, &ring->buf[ring->idx_rd], copy_len);
			//ring->idx_rd += copy_len;
		}
		else {
			right_size = ring->max - ring->idx_rd;
			memcpy(buf, &ring->buf[ring->idx_rd], right_size);
			left_size = copy_len - right_size;
			memcpy(&buf[right_size], ring->buf, left_size);
			//ring->idx_rd = left_size;
		}
		//ring->idx_rd %= ring->max;
		//ring->cnts -= copy_len;
	}
	return copy_len;
}
