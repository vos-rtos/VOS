#include "../vos.h"


static void task0(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	s64 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);
	while (1) {
		kprintf("delay %d(s) ticks: %d\r\n", ++cnts, (u32)VOSGetTimeMs());
		VOSTaskDelay(1000*cnts);
		kprintf("---------------------\r\n");
	}
}

static long long task0_stack[1024];
void delay_test()
{
	kprintf("test delay!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), TASK_PRIO_NORMAL, "task0");
	while (1) {
		VOSTaskDelay(1*1000);
	}
}
