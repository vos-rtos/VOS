/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: event_test.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "vos.h"
#include "vmem.h"

extern s32 VBoudaryCheck(struct StVMemHeap *pheap);
extern s32 VMemInfoDump(struct StVMemHeap *pheap);

/********************************************************************************************************
* 函数：void vmem_test_by_man();
* 描述: 手动测试, 手动设定参数，结合调试判断是否合法
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void vmem_test_by_man()
{
	/*****************************************************************************************************
	* 设定条件： MAX_PAGE_CLASS_MAX == 3
	* 测试buddy算法合并，分裂过程
	******************************************************************************************************/
	static u8 arr_heap[1024 * 10 + 10];
	u8 *p[10];
	struct StVMemHeap *pheap = 0;

	/******************************************************************************************************
	* 测试一：
	* 测试对象： 堆分裂和合并测试
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
	* 测试二：
	* 测试对象： 每次申请1页，耗尽测试,会内存泄漏
	*******************************************************************************************************/
	while (VMemMalloc(pheap, 1024)) {
		VBoudaryCheck(pheap);
		VMemInfoDump(pheap);
	}
	VBoudaryCheck(pheap);
	VMemInfoDump(pheap);

	/******************************************************************************************************
	* 测试三：
	* 测试对象：内存数据写越界测试
	*******************************************************************************************************/
#if VMEM_TRACE_DESTORY
	pheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 1024, 8, "test_heap");
	u8 *p1 = VMemMalloc(pheap, 1024 * 2 - 1);
	VMemInfoDump(pheap);
	memset(p1, 0, 1024 * 2 - 1);//p1未越界
	ASSERT_TEST(0==VMemTraceDestory(pheap), __FUNCTION__);	//没越界，通过
	memset(p1, 0, 1024 * 2);	//p1虽然没越界，但是已经超过自己设定的大小，再写入一个就写入到下一页中导致破坏别的数据
	ASSERT_TEST(-1==VMemTraceDestory(pheap), __FUNCTION__);	//虽然没越界，但是能抓到这个情况，assert
#endif
}

/********************************************************************************************************
* 函数：void vmem_test_random();
* 描述: 随机产生参数进行压力测试
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void vmem_test_random()
{
	/*****************************************************************************************************
	* 设定条件： MAX_PAGE_CLASS_MAX == 11
	* 测试buddy算法合并，分裂过程
	******************************************************************************************************/
	static u8 arr_heap[1024 * 50]; //50k空间
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
		//申请，释放各一次测试
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

		//申请耗尽，或者指针耗尽
		rand_num = rand() % 100;
		j = 0;
		while (1)
		{
			malloc_size = rand() % 10000;
			if (j < sizeof(p)/sizeof(p[0])) {
				p[j] = VMemMalloc(pheap, malloc_size);
				if (p[j] == 0) {//申请失败，跳出，但是还没耗尽
					VBoudaryCheck(pheap);
					break;
				}
				VBoudaryCheck(pheap);
				j++;
			}
			else {//只是p耗尽，释放后，继续申请直到mallo返回0
				for (j = 0; j < sizeof(p)/sizeof(p[0]); j++) {
					VMemFree(pheap, p[j]);
					VBoudaryCheck(pheap);
				}
				j = 0;
			}
		}
		//打印耗尽情况
		VMemInfoDump(pheap);
		//重新释放全部
		for (j = 0; j < sizeof(p) / sizeof(p[0]); j++) {
			VMemFree(pheap, p[j]);
			VBoudaryCheck(pheap);
		}
		//全部回收
		VMemInfoDump(pheap);

		/***************************************************/

		//申请随机次，释放随机次测试
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
* 函数：void vmem_test();
* 描述:  测试
* 参数:  无
* 返回：无
* 注意：无
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

