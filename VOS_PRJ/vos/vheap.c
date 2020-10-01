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
#include "vslab.h"


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
	kprintf("+---------------------------------------------------------------------------------+\r\n");
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		VMemGetHeapInfo(pheap, &heap_info);
		kprintf(" buddy分配器信息：\r\n");
		kprintf(" 编号[%d]: \"%s\", 分配地址范围: 0x%08x ~ 0x%08x\r\n",
				i, pheap->name ? pheap->name : "", (u32)pheap->page_base, (u32)pheap->mem_end);
		kprintf(" 对齐字节: 0x%08x, 页属性: %s, 页大小: 0x%08x\r\n"
				" 空闲页数: 0x%08x, 已分配页数：0x%08x, 页总数: 0x%08x\r\n"
				" 当前可最大分配大小： 0x%08x\r\n",
				pheap->align_bytes,
				pheap->heap_attr==VHEAP_ATTR_SYS ? "通用堆":"专用堆",
				pheap->page_size,
				heap_info.free_page_num,
				heap_info.used_page_num,
				heap_info.page_counts,
				heap_info.cur_max_size);

		if (pheap->slab_ptr) {
			VSlabInfohow(pheap->slab_ptr);
		}

		i++;
		kprintf("+---------------------------------------------------------------------------------+\r\n");
	}
	if (list_empty(&gVheapMgr.heap)) {
		kprintf("没堆内存分配空间\r\n");
		kprintf("+---------------------------------------------------------------------------------+\r\n");
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
* [2] size: 重新分配的尺寸（总尺寸）
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

/********************************************************************************************************
* 函数：void *vcalloc(u32 nitems, u32 size);
* 描述: 通用堆里申请内存，并初始化为0
* 参数:
* [1] size: 指定申请空间大小，单位字节
* 返回：无
* 注意：vrealloc为了区分库的realloc冲突
*********************************************************************************************************/
void *vcalloc(u32 nitems, u32 size)
{
	void *ptr_tmp = 0;
	ptr_tmp = vmalloc(nitems*size);
	if (ptr_tmp) {
		memset(ptr_tmp, 0, nitems*size);
	}
	return ptr_tmp;
}

/********************************************************************************************************
* 函数：void *VHeapMgrGetPageBaseAddr(void *any_addr, u8 *heap_name, s32 len);
* 描述: 根据用户提供的任何地址，找出对应的堆和返回对应的页基地址
* 参数:
* [1] any_addr: 指定任何地址
* [2] heap_name: 获取堆名字，可以为0，则不输出名字
* [3] len: heap_name的内存长度
* 返回：页基地址
* 注意：提供给slab分配器使用
*********************************************************************************************************/
void *VHeapMgrGetPageBaseAddr(void *any_addr, u8 *heap_name, s32 len)
{
	void *ptr = 0;
	s32 min = 0;
	struct list_head *list;
	struct StVMemHeap *pheap = 0;
	VHEAP_LOCK();
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		ptr = VMemGetPageBaseAddr(pheap, any_addr);
		if (ptr) {
			if (pheap->name && len > 0) {
				min = strlen(pheap->name) - 1;
				min = min < len ? min : len;
				memcpy(heap_name, pheap->name, min);
				heap_name[min] = 0;
			}
			break;
		}
	}
	VHEAP_UNLOCK();
	return ptr;
}

