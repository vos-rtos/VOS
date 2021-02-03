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

#define VOS_RUNTIME_BOUDARY_CHECK (1) //运行时边界检查

#define VOS_SLAB_ENABLE		(1) //打开slab分配器开关
#define VOS_SLAB_STEP_SIZE 	(8) //slab分配器步长

#define VMEM_BELONG_TO_NONE (0xFF)  //指向空闲任务，或者ISR上下文，或系统未启动


#define AUTO_TEST_RANDOM  0 //AUTO_TEST_RANDOM == 1 在pc上使用vs2017随机测试
#define AUTO_TEST_BY_MAN  0

#define VMEM_TRACER_ENABLE	1 //检查内存是否越界

#define VMEM_TRACE_DESTORY 0 //检查内存是否写破坏

#define VHEAP_ATTR_SYS		0 //通用堆（系统堆）, 系统堆就是正常的malloc和free申请的堆
#define VHEAP_ATTR_PRIV		1 //专用堆（私有堆），必须从名字获取专用堆指针使用

#define ASSERT_TEST(t,s) \
	do {\
		if(!t) {\
			kprintf("error[%d]: %s test failed!!!\r\n", __LINE__, s); while(1);\
		}\
	}while(0)


struct StVMemHeap;
struct StVMemCtrlBlock;

typedef struct StVMemHeapInfo {
	s32 free_page_bytes; //剩下空闲页的总字节数
	s32 used_page_bytes; //已经分配的页的总字节数
	s32 cur_max_size; //当前最大可分配的尺寸，单位字节
	s32 max_page_class; //堆最大分区尺寸，单位字节
	s32 align_bytes; //几字节对齐
	s32 page_counts; //暂时没什么用
	u32 page_size; //每页大小，单位字节，通常1024,2048等等
}StVMemHeapInfo;


//#define kprintf printf

#define BOUNDARY_ERROR() \
	do { \
		kprintf("BOUNDARY CHECK ERROR: code(%d)\r\n", __LINE__);\
		goto ERROR_RET;\
	} while(0)

#define VMEM_STATUS_UNKNOWN 0
#define VMEM_STATUS_USED 	1
#define VMEM_STATUS_FREE 	2

#if AUTO_TEST_RANDOM
	#define MAX_PAGE_CLASS_MAX 7
#elif AUTO_TEST_BY_MAN
	#define MAX_PAGE_CLASS_MAX 3
#else
	#define MAX_PAGE_CLASS_MAX 13//11
#endif

#define VMEM_MALLOC_PAD		0xAC //剩余空间填充字符

struct StVMemCtrlBlock;
struct StVSlabMgr;

enum {
	BLOCK_OWN_NONE = 0,
	BLOCK_OWN_SLAB,
	BLOCK_OWN_BUDDY,
};

typedef struct StVMemHeap {
	s8 *name; //堆名字
	u8 *mem_end; //内存对齐后的结束地址，用来判断释放是否越界
	struct StVSlabMgr *slab_ptr; //指向slab分配器
	s32 irq_save;//中断状态存储
	s32 heap_attr; //堆属性有两种，一种是专用堆，另一种是通用堆（系统堆）
	s32 align_bytes; //几字节对齐
	s32 page_counts; //暂时没什么用
	u32 page_size; //每页大小，单位字节，通常1024,2048等等
	struct StVMemCtrlBlock *pMCB_base; //内存控制块基地址，与page_base页地址是平行数组，偏移量一致
	u8 *page_base; //数据页基地址，与pMCB_base控制块是平行数组，偏移量一致
	struct list_head page_class[MAX_PAGE_CLASS_MAX]; //page_class[0] 指向2^0个page的链表；page_class[1]指向2^1个page的链表；
	struct list_head mcb_used; //把所有使用的内存连接到这个链表中，方便调试和排查问题，同时做压力测试可以使用；
	struct list_head list_heap; //堆链表
}StVMemHeap;

//每个页对应着一个内存控制块，例如1个页1k字节，估计花销就是sizeof(StVMemCtrlBlock)/1k
//这里StVMemCtrlBlock数组和所有页是平行数组，偏移值一致，这样设计是为了malloc的输入，写穿后不影响管理书籍
//平行数组也是为了加速释放内存时，合并相邻的内存，不用遍历链表
typedef struct StVMemCtrlBlock {
	struct list_head list;
#if VMEM_TRACER_ENABLE
	u32 used_num; //当前使用了多少字节，用来调试是否越界写入，用法就是剩余空闲去添加指定数据，检查是否越界写入
#endif
	u32 flag_who; //	BLOCK_OWN_NONE, BLOCK_OWN_SLAB, BLOCK_OWN_BUDDY
	u16 page_max; //最大分配了多少页, 2^n
	s8 status; //VMEM_STATUS_FREE,VMEM_STATUS_USED,VMEM_STATUS_UNKNOWN
	//已分配内存属于的任务id,注意：如果空闲块，这里设置为0xFF(空闲任务), 但是空闲任务不申请内存（特殊处理）
	//在中断上下文或者系统任务还没启动前，都设定VMEM_BELONG_TO_NONE, 代码不在任务上下文使用
	u8 task_id;

}StVMemCtrlBlock;

#define ALIGN_UP(mem, align) 	((u32)(mem) & ~((align)-1))
#define ALIGN_DOWN(mem, align) 	(((u32)(mem)+(align)-1) & ~((align)-1))


struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes,
												s32 heap_attr, s8 *name, s32 enable_slab);
void *VMemMalloc(struct StVMemHeap *pheap, u32 size, s32 is_slab);
s32 VMemFree (struct StVMemHeap *pheap, void *p, s32 is_slab);
void *VMemExpAlloc(struct StVMemHeap *pheap, void *p, u32 size);
void *VMemGetPageBaseAddr(struct StVMemHeap *pheap, void *any_addr);
s32 VMemGetHeapInfo(struct StVMemHeap *pheap, struct StVMemHeapInfo *pheadinfo);

#endif
