/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: event_test.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/

#include "vos.h"
#include "vheap.h"
#include "vmem.h"

static long long task_vheap_stack1[256], task_vheap_stack2[256];


static void task_vheap1(void *param)
{
	void *p = 0;
	s32 size = 0;
	while (TestExitFlagGet() == 0) {
		size = rand() % 10000;
		p = vmalloc(size);
		if (p) vfree(p);
		VOSTaskDelay(500);
		kprintf("%s test ok!\r\n", __FUNCTION__);
	}
}

static void task_vheap2(void *param)
{
	void *p = 0;
	s32 size = 0;
	while (TestExitFlagGet() == 0) {
		size = 0;
		p = vmalloc(size);
		if (p) vfree(p);
		VOSTaskDelay(500);
		kprintf("%s test ok!\r\n", __FUNCTION__);
	}
}

void vheap_test()
{
	kprintf("test vheap!\r\n");
	static u8 arr_heap[11*1024];
	//�ȴ���һ��ͨ�ö�
	struct StVMemHeap *pheap = 0;
	pheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 1024, 8, VHEAP_ATTR_SYS, "test_heap", 1);

	s32 task_id;
	task_id = VOSTaskCreate(task_vheap1, 0, task_vheap_stack1, sizeof(task_vheap_stack1), TASK_PRIO_NORMAL, "task_vheap1");
	task_id = VOSTaskCreate(task_vheap2, 0, task_vheap_stack2, sizeof(task_vheap_stack2), TASK_PRIO_NORMAL, "task_vheap2");
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}
