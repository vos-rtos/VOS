/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vingbuf.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/
#include "vringbuf.h"

/********************************************************************************************************
* ������StVOSRingBuf *VOSRingBufCreate(s32 len);
* ����: ����RingBuf, ������Ҫmalloc����ͷ+len��С
* ����:
* [1] buf: �û��ṩ��buf
* [2] len: �û��ṩ��buf�Ĵ�С
* ���أ�����RingBuf�Ķ���ָ�룬���򷵻�NULL
* ע�⣺��
*********************************************************************************************************/
StVOSRingBuf *VOSRingBufCreate(s32 len)
{
	StVOSRingBuf *pnew = (StVOSRingBuf *)vmalloc(sizeof(StVOSRingBuf)+len);
	if (pnew == 0) return 0;
	return VOSRingBufBuild(pnew, sizeof(StVOSRingBuf)+len);
}

/********************************************************************************************************
* ������void VOSRingBufDelete(StVOSRingBuf *pnew);
* ����: �ͷ�RingBuf, �����ӦVOSRingBufCreate��
* ����:
* [1] pnew: VOSRingBufCreate�����Ķ���ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSRingBufDelete(StVOSRingBuf *pnew)
{
	if (pnew) {
		vfree(pnew);
		pnew = 0;
	}
}

/********************************************************************************************************
* ������StVOSRingBuf *VOSRingBufBuild(u8 *buf, s32 len);
* ����: ����malloc, ֱ�Ӱ��û��ṩ���ڴ�ǿ�Ƹ�ʽ����RingBufͷ��������Ҳ���õ���VOSRingBufDeleteɾ����
* ����:
* [1] buf: �û��ṩ��buf
* [2] len: �û��ṩ��buf�Ĵ�С
* ���أ�����RingBuf�Ķ���ָ�룬���򷵻�NULL
* ע�⣺��
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
* ������s32 VOSRingBufSet(StVOSRingBuf *ring, u8 *buf, s32 len);
* ����: ������ݵ����λ��塣
* ����:
* [1] ring: ���λ������ָ��
* [2] buf: �û���ӵ�����
* [3] len: �û���ӵ����ݴ�С
* ���أ�ʵ����ӵ��������Ĵ�С����������������򷵻�0
* ע�⣺��
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
* ������s32 VOSRingBufGet(StVOSRingBuf *ring, u8 *buf, s32 len);
* ����: �ӻ��λ�������ȡ���ݡ�
* ����:
* [1] ring: ���λ������ָ��
* [2] buf: �û��ṩ��ַ
* [3] len: �û��ṩ��ȡ���ڴ��С
* ���أ�ʵ�ʴӻ������Ķ�ȡ�Ĵ�С������������գ��򷵻�0
* ע�⣺��
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
* ������s32 VOSRingBufPeekGet(StVOSRingBuf *ring, u8 *buf, s32 len);
* ����: �ӻ��λ������������ݣ���ɾ����
* ����:
* [1] ring: ���λ������ָ��
* [2] buf: �û��ṩ��ַ
* [3] len: �û��ṩ��ȡ���ڴ��С
* ���أ�ʵ�ʴӻ������Ķ�ȡ�Ĵ�С������������գ��򷵻�0
* ע�⣺��
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

