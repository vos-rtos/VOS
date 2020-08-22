#include "vos.h"



static void task_stack_test(void *param)
{
	u8 buf[18*4];
	memset(buf, 0x55 , sizeof(buf));
	while(1) {
		VOSDelayUs(1000*1000);
	}
}

static u32 temp[50];//���ƻ��ĵط�
static long long task_stack[12]; //���ٵ�18��u32

void stack_test()
{
	memset(temp, 0x44, sizeof(temp));
	kprintf("test sem!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task_stack_test, 0, task_stack, sizeof(task_stack), TASK_PRIO_NORMAL, "task_stack_test");
	while (1) {
		VOSTaskDelay(1*1000);
	}
}
