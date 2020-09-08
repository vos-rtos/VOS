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

#define VMEM_BELONG_TO_NONE (0xFF)  //指向空闲任务，或者ISR上下文，或系统未启动

#define AUTO_TEST_RANDOM  0 //AUTO_TEST_RANDOM == 1 在pc上使用vs2017随机测试

#define VMEM_TRACER_ENABLE	1 //检查内存是否越界

#define VMEM_TRACE_DESTORY 0 //检查内存是否写破坏

#define ASSERT_TEST(t,s) \
	do {\
		if(!t) {\
			kprintf("error[%d]: %s test failed!!!\r\n", __LINE__, s); while(1);\
		}\
	}while(0)


struct StVMemHeap;
struct StVMemCtrlBlock;

typedef struct StVMemHeapInfo {
	s32 free_page_num; //剩下空闲页的块数
	s32 used_page_num; //已经分配的页的块数
	s32 cur_max_size; //当前最大可分配的尺寸，单位字节
	s32 max_page_class; //堆最大分区尺寸，单位字节
	s32 align_bytes; //几字节对齐
	s32 page_counts; //暂时没什么用
	u32 page_size; //每页大小，单位字节，通常1024,2048等等
}StVMemHeapInfo;

struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes, s8 *name);
void *VMemMalloc(struct StVMemHeap *pheap, u32 size);
void VMemFree (struct StVMemHeap *pheap, void *p);
void *VMemRealloc(struct StVMemHeap *pheap, void *p, u32 size);

s32 VMemGetHeapInfo(struct StVMemHeap *pheap, struct StVMemHeapInfo *pheadinfo);

#endif
