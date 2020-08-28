/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vos.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/
#include "vos.h"

static StVOSMutex *gMutexPtr;

static void task0(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	u32 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);

	gMutexPtr = VOSMutexCreate("mutex test");

	while (TestExitFlagGet() == 0) {
		if (gMutexPtr) {
			VOSMutexRelease(gMutexPtr);
			VOSTaskDelay(5*1000);
		}
	}
}
static void task1(void *param)
{
	s32 ret = 0;

	kprintf("%s start ...\r\n", __FUNCTION__);
	while(TestExitFlagGet() == 0) {
		if (gMutexPtr) {
			ret = VOSMutexWait(gMutexPtr, 1000);
			if (ret == VERROR_NO_ERROR) {
				kprintf("info: VOSMutexWait OK!\r\n");
			}
			else if (ret == VERROR_TIMEOUT) {
				kprintf("error: VOSMutexWait timeout!\r\n");
			}
			else {

			}
		}
		else {
			VOSTaskDelay(1*1000);
		}
	}
}


static long long task0_stack[256], task1_stack[256];
void mutex_test()
{
	kprintf("test mutex!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), TASK_PRIO_NORMAL, "task0");
	task_id = VOSTaskCreate(task1, 0, task1_stack, sizeof(task1_stack), TASK_PRIO_HIGH, "task1");
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}
