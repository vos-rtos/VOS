/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vheap.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 内	    容: 把所有不同的内存堆连接一起管理
* 历    史: 无
*********************************************************************************************************/
#include "vtype.h"
#include "vos.h"
#include "vmem.h"
#include "vlist.h"


#define VHEAP_LOCK() 	gVheapMgr.irq_save = __vos_irq_save()
#define VHEAP_UNLOCK()   __vos_irq_restore(gVheapMgr.irq_save)

typedef struct StVHeapMgr  {
	s32 irq_save;//中断状态存储
	struct list_head heap; //把多个堆连接一起
}StVMemHeapMgr;


static struct StVHeapMgr gVheapMgr;

/********************************************************************************************************
* 函数：void VHeapMgrInit();
* 描述: 初始化堆管理对象
* 参数: 无
* 返回：无
* 注意：在刚启动时先初始，就可以直接使用
*********************************************************************************************************/
void VHeapMgrInit()
{
	VHEAP_LOCK();
	INIT_LIST_HEAD(&gVheapMgr.heap);
	VHEAP_UNLOCK();
}

/********************************************************************************************************
* 函数：void VHeapMgrAdd(struct StVMemHeap *pheap);
* 描述: 把单个堆添加到堆管理对象中
* 参数:
* [1] pheap: 要添加的堆
* 返回：无
* 注意：无
*********************************************************************************************************/
void VHeapMgrAdd(struct StVMemHeap *pheap)
{
	if (pheap) {
		VHEAP_LOCK();
		list_add_tail(&pheap->list_heap, &gVheapMgr.heap);
		VHEAP_UNLOCK();
	}
}

/********************************************************************************************************
* 函数：void VHeapMgrDel(struct StVMemHeap *pheap);
* 描述: 把单个堆从堆管理对象中删除
* 参数:
* [1] pheap: 要删除的堆
* 返回：无
* 注意：无
*********************************************************************************************************/
void VHeapMgrDel(struct StVMemHeap *pheap)
{
	if (pheap) {
		VHEAP_LOCK();
		list_del(&pheap->list_heap);
		VHEAP_UNLOCK();
	}
}

/********************************************************************************************************
* 函数：struct StVMemHeap *VHeapFindByName(s8 *name);
* 描述: 从堆管理者中查找名字为name的指定堆
* 参数:
* [1] name: 指定堆的名字
* 返回：指定堆的指针
* 注意：无
*********************************************************************************************************/
struct StVMemHeap *VHeapFindByName(s8 *name)
{
	struct list_head *list;
	struct StVMemHeap *pheap = 0;
	if (name == 0) return 0;
	VHEAP_LOCK();
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		if (strcmp(pheap->name, name) == 0) {
			break;
		}
	}
	VHEAP_UNLOCK();
	return pheap;
}

/********************************************************************************************************
* 函数：void VHeapShellShow();
* 描述: shell命令显示heap的信息
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void VHeapShellShow()
{
	s32 i = 0;
	struct list_head *list;
	struct StVMemHeap *pheap = 0;
	StVMemHeapInfo heap_info;
	VHEAP_LOCK();
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		VMemGetHeapInfo(pheap, &heap_info);
		kprintf("==============\r\n[%d]堆名字: %s\r\n", i, pheap->name ? pheap->name : "");
		kprintf("对齐字节: 0x%08x, 页属性: %s, 页大小: 0x%08x, 当前可最大分配大小： 0x%08x\r\n"
				"空闲页数: 0x%08x, 已分配页数：0x%08x, 页总数: 0x%08x\r\n",
				pheap->align_bytes, pheap->heap_attr==VHEAP_ATTR_SYS ? "通用堆":"专用堆", pheap->page_size,
				heap_info.cur_max_size, heap_info.free_page_num, heap_info.used_page_num, heap_info.page_counts);
		i++;
	}
	VHEAP_UNLOCK();
}

/********************************************************************************************************
* 函数：void *vmalloc(u32 size);
* 描述: 通用堆里申请内存
* 参数:
* [1] size: 指定申请空间大小，单位字节
* 返回：已分配内存的地址
* 注意：vmalloc为了区分库的malloc冲突
*********************************************************************************************************/
void *vmalloc(u32 size)
{
	void *ptr = 0;
	struct list_head *list;
	struct StVMemHeap *pheap = 0;
	VHEAP_LOCK();
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		if (pheap->heap_attr == VHEAP_ATTR_SYS &&
				(ptr = VMemMalloc(pheap, size))) {//通用堆里分配
			break;
		}
	}
	VHEAP_UNLOCK();
	return ptr;
}

/********************************************************************************************************
* 函数：void vfree(void *ptr);
* 描述: 释放通用堆的内存
* 参数:
* [1] ptr: 要释放的内存指针
* 返回：无
* 注意：vfree为了区分库的free冲突
*********************************************************************************************************/
void vfree(void *ptr)
{
	struct list_head *list;
	struct StVMemHeap *pheap = 0;
	VHEAP_LOCK();
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		if (pheap->heap_attr == VHEAP_ATTR_SYS) {//通用堆里分配
			VMemFree(pheap, ptr);
			break;
		}
	}
	VHEAP_UNLOCK();
}

/********************************************************************************************************
* 函数：void *vrealloc(void *ptr, u32 size);
* 描述: 通用堆里重新申请内存
* 参数:
* [1] ptr: 要释放的内存指针
* 返回：无
* 注意：vrealloc为了区分库的realloc冲突
*********************************************************************************************************/
void *vrealloc(void *ptr, u32 size)
{
	void *ptr_tmp = 0;
	struct list_head *list;
	struct StVMemHeap *pheap = 0;
	VHEAP_LOCK();
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		if (pheap->heap_attr == VHEAP_ATTR_SYS &&
				(ptr_tmp = VMemRealloc(pheap, ptr, size))) {//通用堆里分配
			break;
		}
	}
	VHEAP_UNLOCK();
	return ptr_tmp;
}



