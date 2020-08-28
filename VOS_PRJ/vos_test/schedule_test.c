//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-15: initial by vincent.
//------------------------------------------------------
#include "vos.h"

extern void VOSDelayUs(u32 us);

static StVOSMutex *gMutexPtr;

//�������ȼ���ת����:
//�����ȼ������Ȼ����Դ�������ȼ��ȴ���Դ�������ȼ���ռ�����ȼ�����ִ��Ȩ��
//������Ϻ󵽵����ȼ����У���󵽸����ȼ�����ִ�С�
static void task_low(void *param)
{
	s32 ret = 0;
	s32 cnt = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	u32 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);

	gMutexPtr = VOSMutexCreate("mutex_schedule");

	while (TestExitFlagGet() == 0) {
		kprintf("%sks=%d!\r\n", __FUNCTION__, (u32)VOSGetTimeMs());
		if (gMutexPtr) {
			ret = VOSMutexWait(gMutexPtr, 60*1000);
			kprintf("%s Wait Mutex Success!\r\n", __FUNCTION__, ret);
			while (cnt < 10) {
				kprintf("%s working %d(s)!\r\n", __FUNCTION__, cnt);
				VOSDelayUs(1000*1000);
				cnt++;
				if (TestExitFlagGet()) break;
			}
			kprintf("%s Release Mutex Success!\r\n", __FUNCTION__);
			kprintf("%s working DONE!\r\n", __FUNCTION__);
			VOSMutexRelease(gMutexPtr);
		}
		while (TestExitFlagGet() == 0) VOSTaskDelay(1*1000);
	}
}
static void task_high(void *param)
{
	s32 cnt = 0;
	s32 ret = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
		if (gMutexPtr) {
			kprintf("%s try to get Mutex ...!\r\n", __FUNCTION__);
			ret = VOSMutexWait(gMutexPtr, 60*1000);
			kprintf("%s Wait Mutex Success, ret=%d!\r\n", __FUNCTION__, ret);
			while (cnt < 3) {
				kprintf("%s working %d(s)!\r\n", __FUNCTION__, cnt);
				VOSDelayUs(1000*1000);
				cnt++;
				if (TestExitFlagGet()) break;
			}
			kprintf("%s Release Mutex Success!\r\n", __FUNCTION__);
			kprintf("%s working DONE!\r\n", __FUNCTION__);
			VOSMutexRelease(gMutexPtr);
		}
		while (TestExitFlagGet() == 0) VOSTaskDelay(1*1000);
	}
}

static void task_normal(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(TestExitFlagGet() == 0) {
		VOSTaskDelay(2*1000);
		kprintf("%s start working ... \r\n", __FUNCTION__);
		while (cnts < 5) {
			kprintf("%s cnts=%d\r\n", __FUNCTION__, cnts++);
			VOSDelayUs(1000*1000);
			if (TestExitFlagGet()) break;
		}
		kprintf("%s ended working ... \r\n", __FUNCTION__);
		while (TestExitFlagGet() == 0) VOSTaskDelay(1*1000);
	}
}



static long long task_low_stack[1024], task_high_stack[256], task_normal_stack[256];
void schedule_test()
{
	kprintf("test sem!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task_low, 0, task_low_stack, sizeof(task_low_stack), TASK_PRIO_LOW, "task_low");
	task_id = VOSTaskCreate(task_high, 0, task_high_stack, sizeof(task_high_stack), TASK_PRIO_HIGH, "task_high");
	task_id = VOSTaskCreate(task_normal, 0, task_normal_stack, sizeof(task_normal_stack), TASK_PRIO_NORMAL, "task_normal");
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}

