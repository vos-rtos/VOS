#include "../vos.h"

StVOSSemaphore *sem_hdl = 0;
static void task0(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	s64 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);

	sem_hdl = VOSSemCreate(1, 1, "sem_hdl");
	while (1) {
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
	while(1) {
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
	while(1) {
		//VOSTaskDelay(5000);
		if (sem_hdl) {
			VOSSemRelease(sem_hdl);
		}
		kprintf("%s cnts=%d\r\n", __FUNCTION__, cnts++);
		VOSTaskDelay(1*1000);
	}
}

static long long task0_stack[1024], task1_stack[1024], task2_stack[1024];
void sem_test()
{
	kprintf("test sem!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), TASK_PRIO_NORMAL, "task0");
	task_id = VOSTaskCreate(task1, 0, task1_stack, sizeof(task1_stack), TASK_PRIO_HIGH, "task1");
	task_id = VOSTaskCreate(task2, 0, task2_stack, sizeof(task2_stack), TASK_PRIO_LOW, "task2");
	while (1) {
		VOSTaskDelay(1*1000);
	}
}

