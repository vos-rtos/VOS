/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vslab. 
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 内	    容: slab主要的意义是处理小内存分配，释放策略会延迟，对频繁分配意义比较大，还有内存浪费有好处
* 历    史：
* --20200925：创建文件, 实现slab内存池分配算法，这里如果申请的内存大于obj_size_max大小，直接从buddy分配
*
*********************************************************************************************************/
#ifndef __VSLAB_H__
#define __VSLAB_H__

#define VSLAB_FREE_PAGES_THREHOLD   (3U) //每种尺寸，如果空闲页达到超过这个门槛，多出的将被释放

#define bitmap_get(n, bitmap)		(!!(((u8*)(bitmap))[(n)>>3] & 1<<((n)&0x07)))
#define bitmap_clr(n, bitmap)		(((u8*)(bitmap))[(n)>>3] &= ~(1<<((n)&0x07)))
#define bitmap_set(n, bitmap)		(((u8*)(bitmap))[(n)>>3] |= 1<<((n)&0x07))

enum {
	SLAB_BITMAP_UNKNOWN = 0,  	//返回状态，初始化状态
	SLAB_BITMAP_FREE,			//返回状态，位图当前是全部为空闲块
	SLAB_BITMAP_PARTIAL,		//返回状态，位图当前是部分已分配块
	SLAB_BITMAP_FULL,			//返回状态，位图当前是全部已分配块
};

struct StVSlabMgr;
struct StVMemHeap;

struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size,
		s32 align_bytes, s32 step_size, s8 *name, struct StVMemHeap *pheap);
void *VSlabBlockAlloc(struct StVSlabMgr *pSlabMgr, s32 size);
s32 VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr);

#endif
