#include "vos.h"

extern void VOSDelayUs(u32 us);

s32 TestExitFlagGet();

static void task0(void *param)
{
	int cnts = 0;
	kprintf("%s start ...\r\n", __FUNCTION__);
	s64 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);
#if 1
	while (TestExitFlagGet() == 0) {
		kprintf("delay %d(s) ticks: %d\r\n", ++cnts, (u32)VOSGetTimeMs());
		VOSTaskDelay(1000*cnts);
		kprintf("---------------------\r\n");
	}
#else
	while(TestExitFlagGet() == 0) {
		VOSDelayUs(1000*1000);
		kprintf("delay %d(s) ticks: %d\r\n", ++cnts, (u32)VOSGetTimeMs());
	}
#endif
}

static long long task0_stack[256];
void delay_test()
{
	kprintf("test delay!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), TASK_PRIO_NORMAL, "task0");
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}
