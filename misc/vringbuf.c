/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vingbuf.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/
#include "vringbuf.h"

/********************************************************************************************************
* 函数：StVOSRingBuf *VOSRingBufCreate(s32 len);
* 描述: 创建RingBuf, 这里需要malloc管理头+len大小
* 参数:
* [1] buf: 用户提供的buf
* [2] len: 用户提供的buf的大小
* 返回：返回RingBuf的对象指针，否则返回NULL
* 注意：无
*********************************************************************************************************/
StVOSRingBuf *VOSRingBufCreate(s32 len)
{
	StVOSRingBuf *pnew = (StVOSRingBuf *)vmalloc(sizeof(StVOSRingBuf)+len);
	if (pnew == 0) return 0;
	return VOSRingBufBuild(pnew, sizeof(StVOSRingBuf)+len);
}

/********************************************************************************************************
* 函数：void VOSRingBufDelete(StVOSRingBuf *pnew);
* 描述: 释放RingBuf, 这里对应VOSRingBufCreate。
* 参数:
* [1] pnew: VOSRingBufCreate创建的对象指针
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSRingBufDelete(StVOSRingBuf *pnew)
{
	if (pnew) {
		vfree(pnew);
		pnew = 0;
	}
}

/********************************************************************************************************
* 函数：StVOSRingBuf *VOSRingBufBuild(u8 *buf, s32 len);
* 描述: 不用malloc, 直接把用户提供的内存强制格式化出RingBuf头部，所以也不用调用VOSRingBufDelete删除。
* 参数:
* [1] buf: 用户提供的buf
* [2] len: 用户提供的buf的大小
* 返回：返回RingBuf的对象指针，否则返回NULL
* 注意：无
*********************************************************************************************************/
StVOSRingBuf *VOSRingBufBuild(u8 *buf, s32 len)
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

s32 VOSRingBufGetMaxSize(StVOSRingBuf *pRingBuf)
{
	if (pRingBuf == 0) return -1;
	return pRingBuf->max;
}


/********************************************************************************************************
* 函数：s32 VOSRingBufSet(StVOSRingBuf *ring, u8 *buf, s32 len);
* 描述: 添加数据到环形缓冲。
* 参数:
* [1] ring: 环形缓冲对象指针
* [2] buf: 用户添加的数据
* [3] len: 用户添加的数据大小
* 返回：实际添加到缓冲区的大小，如果缓冲区满，则返回0
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：s32 VOSRingBufGet(StVOSRingBuf *ring, u8 *buf, s32 len);
* 描述: 从环形缓冲区获取数据。
* 参数:
* [1] ring: 环形缓冲对象指针
* [2] buf: 用户提供地址
* [3] len: 用户提供读取的内存大小
* 返回：实际从缓冲区的读取的大小，如果缓冲区空，则返回0
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：s32 VOSRingBufPeekGet(StVOSRingBuf *ring, u8 *buf, s32 len);
* 描述: 从环形缓冲区窥视数据，不删除。
* 参数:
* [1] ring: 环形缓冲对象指针
* [2] buf: 用户提供地址
* [3] len: 用户提供读取的内存大小
* 返回：实际从缓冲区的读取的大小，如果缓冲区空，则返回0
* 注意：无
*********************************************************************************************************/
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
		}
		else {
			right_size = ring->max - ring->idx_rd;
			memcpy(buf, &ring->buf[ring->idx_rd], right_size);
			left_size = copy_len - right_size;
			memcpy(&buf[right_size], ring->buf, left_size);
		}
	}
	return copy_len;
}


s32 VOSRingBufIsEmpty(StVOSRingBuf *ring)
{
	return ring->cnts==0;
}

s32 VOSRingBufIsFull(StVOSRingBuf *ring)
{
	return (s32)(ring->cnts==ring->max);
}

s32 VOSRingBufGetMax(StVOSRingBuf *ring)
{
	return ring->max;
}
s32 VOSRingBufGetCnts(StVOSRingBuf *ring)
{
	return ring->cnts;
}

s32 VOSRingBufGetCurBytes(StVOSRingBuf *ring)
{
	return ring->cnts;
}

