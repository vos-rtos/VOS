#include "vos.h"

extern void VOSDelayUs(u32 us);

static void task_normal1(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(1) {
		VOSDelayUs(1000*1000);
	}
}
static void task_normal2(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(1) {
		VOSDelayUs(1000*1000);
	}
}


static long long task_normal1_stack[1024], task_normal2_stack[1024];
void shell_test()
{
	kprintf("test sem!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task_normal1, 0, task_normal1_stack, sizeof(task_normal1_stack), TASK_PRIO_NORMAL, "task_normal1");
	task_id = VOSTaskCreate(task_normal2, 0, task_normal2_stack, sizeof(task_normal2_stack), TASK_PRIO_NORMAL, "task_normal2");
	while (1) {
		VOSTaskDelay(1*1000);
	}
}

