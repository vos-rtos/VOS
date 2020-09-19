/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vheap.h
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史:无
*********************************************************************************************************/

#ifndef __VHeap_H__
#define __VHeap_H__

#include "vtype.h"

void VHeapMgrInit();
void VHeapMgrAdd(struct StVMemHeap *pheap);
void VHeapMgrDel(struct StVMemHeap *pheap);
struct StVMemHeap *VHeapFindByName(s8 *name);

void *vmalloc(u32 size);
void vfree(void *ptr);
void *vrealloc(void *ptr, u32 size);
void *vcalloc(u32 size);

#endif
