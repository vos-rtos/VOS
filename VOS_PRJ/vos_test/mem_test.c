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
#include "vmem.h"

extern s32 VBoudaryCheck(struct StVMemHeap *pheap);
extern s32 VMemInfoDump(struct StVMemHeap *pheap);

/********************************************************************************************************
* ������void vmem_test_by_man();
* ����: �ֶ�����, �ֶ��趨��������ϵ����ж��Ƿ�Ϸ�
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void vmem_test_by_man()
{
	/*****************************************************************************************************
	* �趨������ MAX_PAGE_CLASS_MAX == 3
	* ����buddy�㷨�ϲ������ѹ���
	******************************************************************************************************/
	static u8 arr_heap[1024 * 10 + 10];
	u8 *p[10];
	struct StVMemHeap *pheap = 0;

	/******************************************************************************************************
	* ����һ��
	* ���Զ��� �ѷ��Ѻͺϲ�����
	*******************************************************************************************************/
	pheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 1024, 8, VHEAP_ATTR_SYS, "test_heap", 1);
	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	p[0] = (u8*)VMemMalloc(pheap, 1024 * 1 + 1);
	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	p[1] = (u8*)VMemMalloc(pheap, 1024 * 1 + 1);

	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	VMemFree(pheap, p[0]);

	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	VMemFree(pheap, p[1]);

	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	/******************************************************************************************************
	* ���Զ���
	* ���Զ��� ÿ������1ҳ���ľ�����,���ڴ�й©
	*******************************************************************************************************/
	while (VMemMalloc(pheap, 1024)) {
		VBoudaryCheck(pheap);
		VMemInfoDump(pheap);
	}
	VBoudaryCheck(pheap);
	VMemInfoDump(pheap);

	/******************************************************************************************************
	* ��������
	* ���Զ����ڴ�����дԽ�����
	*******************************************************************************************************/
#if VMEM_TRACE_DESTORY
	pheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 1024, 8, "test_heap");
	u8 *p1 = VMemMalloc(pheap, 1024 * 2 - 1);
	VMemInfoDump(pheap);
	memset(p1, 0, 1024 * 2 - 1);//p1δԽ��
	ASSERT_TEST(0==VMemTraceDestory(pheap), __FUNCTION__);	//ûԽ�磬ͨ��
	memset(p1, 0, 1024 * 2);	//p1��ȻûԽ�磬�����Ѿ������Լ��趨�Ĵ�С����д��һ����д�뵽��һҳ�е����ƻ��������
	ASSERT_TEST(-1==VMemTraceDestory(pheap), __FUNCTION__);	//��ȻûԽ�磬������ץ����������assert
#endif
}

/********************************************************************************************************
* ������void vmem_test_random();
* ����: ���������������ѹ������
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void vmem_test_random()
{
	/*****************************************************************************************************
	* �趨������ MAX_PAGE_CLASS_MAX == 11
	* ����buddy�㷨�ϲ������ѹ���
	******************************************************************************************************/
	static u8 arr_heap[1024 * 50]; //50k�ռ�
	u8 *p[100];
	s32 i = 0;
	s32 j = 0;
	struct StVMemHeap *pheap = 0;
	s32 rand_num;
	s32 malloc_size = 0;
	s32 counter = 10000;
	pheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 512, 8, VHEAP_ATTR_SYS, "test_heap", 1);
	VMemInfoDump(pheap);
	while (1) {
		//���룬�ͷŸ�һ�β���
		counter = 100;
		while (counter--) {
			rand_num = rand() % 1000;
			for (i = 0; i < rand_num; i++) {
				malloc_size = rand() % 10000;
				p[0] = VMemMalloc(pheap, malloc_size);
				VBoudaryCheck(pheap);
				VMemFree(pheap, p[0]);
				VBoudaryCheck(pheap);
			}
		}
		VMemInfoDump(pheap);

		/***************************************************/

		//����ľ�������ָ��ľ�
		rand_num = rand() % 100;
		j = 0;
		while (1)
		{
			malloc_size = rand() % 10000;
			if (j < sizeof(p)/sizeof(p[0])) {
				p[j] = VMemMalloc(pheap, malloc_size);
				if (p[j] == 0) {//����ʧ�ܣ����������ǻ�û�ľ�
					VBoudaryCheck(pheap);
					break;
				}
				VBoudaryCheck(pheap);
				j++;
			}
			else {//ֻ��p�ľ����ͷź󣬼�������ֱ��mallo����0
				for (j = 0; j < sizeof(p)/sizeof(p[0]); j++) {
					VMemFree(pheap, p[j]);
					VBoudaryCheck(pheap);
				}
				j = 0;
			}
		}
		//��ӡ�ľ����
		VMemInfoDump(pheap);
		//�����ͷ�ȫ��
		for (j = 0; j < sizeof(p) / sizeof(p[0]); j++) {
			VMemFree(pheap, p[j]);
			VBoudaryCheck(pheap);
		}
		//ȫ������
		VMemInfoDump(pheap);

		/***************************************************/

		//��������Σ��ͷ�����β���
		counter = 10000;
		j = 0;
		while (counter--) {
			j = rand() % (sizeof(p)/sizeof(p[0]));
			for (i = 0; i < j; i++) {
				malloc_size = rand() % 10000;
				p[i] = VMemMalloc(pheap, malloc_size);
				VBoudaryCheck(pheap);
			}
			for (i = 0; i < j; i++) {
				VMemFree(pheap, p[i]);
				VBoudaryCheck(pheap);
			}
		}
		VMemInfoDump(pheap);
		if (TestExitFlagGet()) break;
	}
}

/********************************************************************************************************
* ������void vmem_test();
* ����:  ����
* ����:  ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
s32 TestExitFlagGet();
void vmem_test()
{
#if AUTO_TEST_RANDOM
	vmem_test_random();
#else
	vmem_test_by_man();
#endif
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}

