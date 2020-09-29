/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vslab. 
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��	    ��: slab��Ҫ�������Ǵ���С�ڴ���䣬�ͷŲ��Ի��ӳ٣���Ƶ����������Ƚϴ󣬻����ڴ��˷��кô�
* ��    ʷ��
* --20200925�������ļ�, ʵ��slab�ڴ�ط����㷨���������������ڴ����obj_size_max��С��ֱ�Ӵ�buddy����
*
*********************************************************************************************************/
#ifndef __VSLAB_H__
#define __VSLAB_H__

#define VSLAB_FREE_PAGES_THREHOLD   (3U) //ÿ�ֳߴ磬�������ҳ�ﵽ��������ż�������Ľ����ͷ�

#define bitmap_get(n, bitmap)		(!!(((u8*)(bitmap))[(n)>>3] & 1<<((n)&0x07)))
#define bitmap_clr(n, bitmap)		(((u8*)(bitmap))[(n)>>3] &= ~(1<<((n)&0x07)))
#define bitmap_set(n, bitmap)		(((u8*)(bitmap))[(n)>>3] |= 1<<((n)&0x07))

enum {
	SLAB_BITMAP_UNKNOWN = 0,  	//����״̬����ʼ��״̬
	SLAB_BITMAP_FREE,			//����״̬��λͼ��ǰ��ȫ��Ϊ���п�
	SLAB_BITMAP_PARTIAL,		//����״̬��λͼ��ǰ�ǲ����ѷ����
	SLAB_BITMAP_FULL,			//����״̬��λͼ��ǰ��ȫ���ѷ����
};

struct StVSlabMgr;
struct StVMemHeap;

struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size,
		s32 align_bytes, s32 step_size, s8 *name, struct StVMemHeap *pheap);
void *VSlabBlockAlloc(struct StVSlabMgr *pSlabMgr, s32 size);
s32 VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr);

#endif
