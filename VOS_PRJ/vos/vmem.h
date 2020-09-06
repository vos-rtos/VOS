/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vmem.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史:无
*********************************************************************************************************/

#ifndef __VMEM_H__
#define __VMEM_H__

#include "vtype.h"

struct StVMemHeap;

struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes);
void *VMemMalloc(struct StVMemHeap *pheap, u32 size);
void VMemFree (struct StVMemHeap *pheap, void *p);
void *VMemRealloc(struct StVMemHeap *pheap, void *p, u32 size);
#endif
