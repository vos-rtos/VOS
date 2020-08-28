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

s32 TestExitFlagGet();

StVOSSemaphore *sem_hdl = 0;
static void task0(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	u32 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);

	sem_hdl = VOSSemCreate(1, 1, "sem_hdl");
	while (TestExitFlagGet() == 0) {
		//VOSTaskDelay(3000);
		//os_switch_next();
		if (sem_hdl) {
			VOSSemWait(sem_hdl, 1*1000);
		}
		kprintf("%s cnts=%d\r\n", __FUNCTION__, cnts++);
		VOSTaskDelay(1*1000);
	}
}
static void task1(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(TestExitFlagGet() == 0) {
		//VOSTaskDelay(3000);
		if (sem_hdl) {
			VOSSemWait(sem_hdl, 1*1000);

		}
		kprintf("%s cnts=%d\r\n", __FUNCTION__, cnts++);
		VOSTaskDelay(1*1000);
	}
}
static void task2(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(TestExitFlagGet() == 0) {
		//VOSTaskDelay(5000);
		if (sem_hdl) {
			VOSSemRelease(sem_hdl);
		}
		kprintf("%s cnts=%d\r\n", __FUNCTION__, cnts++);
		VOSTaskDelay(1*1000);
	}
}

StVOSSemaphore *sem_int = 0;
static void task0_int(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	u32 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);

	sem_int = VOSSemCreate(1, 1, "sem_int");
	while (TestExitFlagGet() == 0) {
		if (sem_int) {
			VOSSemWait(sem_int, 2000*1000);
			kprintf("%s cnts=%d\r\n", __FUNCTION__, cnts++);
		}
	}
}

void timer_hardware_process(){
	if (sem_int){
		VOSSemRelease(sem_int);
	}
}


static long long task0_stack[256], task1_stack[256], task2_stack[256], task0_stack_int[256];
void sem_test()
{
	kprintf("test sem!\r\n");

	s32 task_id;
	task_id = VOSTaskCreate(task0_int, 0, task0_stack_int, sizeof(task0_stack_int), TASK_PRIO_NORMAL, "task0_int");

//	s32 task_id;
//	task_id = VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), TASK_PRIO_NORMAL, "task0");
//	task_id = VOSTaskCreate(task1, 0, task1_stack, sizeof(task1_stack), TASK_PRIO_HIGH, "task1");
//	task_id = VOSTaskCreate(task2, 0, task2_stack, sizeof(task2_stack), TASK_PRIO_LOW, "task2");
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}


