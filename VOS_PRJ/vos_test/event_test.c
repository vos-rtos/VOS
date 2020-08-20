#include "vos.h"

s32 VOSEventWait(u32 event_mask, u64 timeout_ms);

u32 task0_id = -1;

static void task0(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	s64 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);


	while (1) {
		kprintf("%s begin VOSEventWait ticks=%d ...\r\n", __FUNCTION__, (u32)VOSGetTimeMs());
		VOSEventWait(0x01, 10000);
		kprintf("%s Done VOSEventWait ticks=%d ...\r\n", __FUNCTION__, (u32)VOSGetTimeMs());
		VOSTaskDelay(1*1000);
	}
}
static void task1(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(1) {
		VOSTaskDelay(5*1000);

		if (task0_id > 0) {
			kprintf("%s VOSEventSet ticks=%d ...\r\n", __FUNCTION__, (u32)VOSGetTimeMs());
			VOSEventSet(task0_id, 0x001);
		}

		VOSTaskDelay(1*1000);
	}
}


static long long task0_stack[1024], task1_stack[1024];
void event_test()
{
	kprintf("test sem!\r\n");
	s32 task_id;
	task0_id = VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), TASK_PRIO_NORMAL, "task0");
	task_id = VOSTaskCreate(task1, 0, task1_stack, sizeof(task1_stack), TASK_PRIO_HIGH, "task1");
	while (1) {
		VOSTaskDelay(1*1000);
	}
}
