/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vheap.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��	    ��: �����в�ͬ���ڴ������һ�����
* ��    ʷ: ��
*********************************************************************************************************/
#include "vtype.h"
#include "vos.h"
#include "vmem.h"
#include "vlist.h"
#include "vslab.h"


#define VHEAP_LOCK() 	gVheapMgr.irq_save = __vos_irq_save()
#define VHEAP_UNLOCK()   __vos_irq_restore(gVheapMgr.irq_save)

typedef struct StVHeapMgr  {
	s32 irq_save;//�ж�״̬�洢
	struct list_head heap; //�Ѷ��������һ��
}StVMemHeapMgr;


static struct StVHeapMgr gVheapMgr;

/********************************************************************************************************
* ������void VHeapMgrInit();
* ����: ��ʼ���ѹ������
* ����: ��
* ���أ���
* ע�⣺�ڸ�����ʱ�ȳ�ʼ���Ϳ���ֱ��ʹ��
*********************************************************************************************************/
void VHeapMgrInit()
{
	VHEAP_LOCK();
	INIT_LIST_HEAD(&gVheapMgr.heap);
	VHEAP_UNLOCK();
}

/********************************************************************************************************
* ������void VHeapMgrAdd(struct StVMemHeap *pheap);
* ����: �ѵ�������ӵ��ѹ��������
* ����:
* [1] pheap: Ҫ��ӵĶ�
* ���أ���
* ע�⣺��
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
* ������void VHeapMgrDel(struct StVMemHeap *pheap);
* ����: �ѵ����ѴӶѹ��������ɾ��
* ����:
* [1] pheap: Ҫɾ���Ķ�
* ���أ���
* ע�⣺��
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
* ������struct StVMemHeap *VHeapFindByName(s8 *name);
* ����: �Ӷѹ������в�������Ϊname��ָ����
* ����:
* [1] name: ָ���ѵ�����
* ���أ�ָ���ѵ�ָ��
* ע�⣺��
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
* ������void VHeapShellShow();
* ����: shell������ʾheap����Ϣ
* ����: ��
* ���أ���
* ע�⣺��
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
		kprintf(" buddy��������Ϣ��\r\n");
		kprintf(" ���[%d]: \"%s\", �����ַ��Χ: 0x%08x ~ 0x%08x\r\n",
				i, pheap->name ? pheap->name : "", (u32)pheap->page_base, (u32)pheap->mem_end);
		kprintf(" �����ֽ�: 0x%08x, ҳ����: %s, ҳ��С: 0x%08x\r\n"
				" ����ҳ��: 0x%08x, �ѷ���ҳ����0x%08x, ҳ����: 0x%08x\r\n"
				" ��ǰ���������С�� 0x%08x\r\n",
				pheap->align_bytes,
				pheap->heap_attr==VHEAP_ATTR_SYS ? "ͨ�ö�":"ר�ö�",
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
		kprintf("û���ڴ����ռ�\r\n");
		kprintf("+---------------------------------------------------------------------------------+\r\n");
	}
	VHEAP_UNLOCK();
}

/********************************************************************************************************
* ������void *vmalloc(u32 size);
* ����: ͨ�ö��������ڴ�
* ����:
* [1] size: ָ������ռ��С����λ�ֽ�
* ���أ��ѷ����ڴ�ĵ�ַ
* ע�⣺vmallocΪ�����ֿ��malloc��ͻ
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
				(ptr = VMemMalloc(pheap, size))) {//ͨ�ö������
			break;
		}
	}
	VHEAP_UNLOCK();
	return ptr;
}

/********************************************************************************************************
* ������void vfree(void *ptr);
* ����: �ͷ�ͨ�öѵ��ڴ�
* ����:
* [1] ptr: Ҫ�ͷŵ��ڴ�ָ��
* ���أ���
* ע�⣺vfreeΪ�����ֿ��free��ͻ
*********************************************************************************************************/
void vfree(void *ptr)
{
	struct list_head *list;
	struct StVMemHeap *pheap = 0;
	VHEAP_LOCK();
	list_for_each(list, &gVheapMgr.heap) {
		pheap = list_entry(list, struct StVMemHeap, list_heap);
		if (pheap->heap_attr == VHEAP_ATTR_SYS) {//ͨ�ö������
			VMemFree(pheap, ptr);
			break;
		}
	}
	VHEAP_UNLOCK();
}

/********************************************************************************************************
* ������void *vrealloc(void *ptr, u32 size);
* ����: ͨ�ö������������ڴ�
* ����:
* [1] ptr: Ҫ�ͷŵ��ڴ�ָ��
* [2] size: ���·���ĳߴ磨�ܳߴ磩
* ���أ���
* ע�⣺vreallocΪ�����ֿ��realloc��ͻ
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
				(ptr_tmp = VMemRealloc(pheap, ptr, size))) {//ͨ�ö������
			break;
		}
	}
	VHEAP_UNLOCK();
	return ptr_tmp;
}

/********************************************************************************************************
* ������void *vcalloc(u32 nitems, u32 size);
* ����: ͨ�ö��������ڴ棬����ʼ��Ϊ0
* ����:
* [1] size: ָ������ռ��С����λ�ֽ�
* ���أ���
* ע�⣺vreallocΪ�����ֿ��realloc��ͻ
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
* ������void *VHeapMgrGetPageBaseAddr(void *any_addr, u8 *heap_name, s32 len);
* ����: �����û��ṩ���κε�ַ���ҳ���Ӧ�ĶѺͷ��ض�Ӧ��ҳ����ַ
* ����:
* [1] any_addr: ָ���κε�ַ
* [2] heap_name: ��ȡ�����֣�����Ϊ0�����������
* [3] len: heap_name���ڴ泤��
* ���أ�ҳ����ַ
* ע�⣺�ṩ��slab������ʹ��
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

