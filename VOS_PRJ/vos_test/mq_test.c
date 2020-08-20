#include "vos.h"

typedef struct StMsgBlock{
	u32 id;
}StMsgBlock;

StVOSMsgQueue *gMsgQuePtr;

#define MAX_MSG_NUM 10

StMsgBlock arr_msg[MAX_MSG_NUM];

static void task0(void *param)
{
	int cnts = 0;
	StMsgBlock tmp;
	kprintf("%s start ...\r\n", __FUNCTION__);
	s64 mark_time = VOSGetTimeMs()/1000;
	kprintf("mark_time: %d\r\n", (u32)mark_time);

	gMsgQuePtr = VOSMsgQueCreate(arr_msg, sizeof(arr_msg), sizeof(StMsgBlock), "msg test");

	while (1) {
		if (gMsgQuePtr) {
			tmp.id = cnts++;
			VOSMsgQuePut(gMsgQuePtr, &tmp, sizeof(tmp));
			VOSTaskDelay(5*1000);
		}
	}
}
static void task1(void *param)
{
	s32 ret = 0;
	StMsgBlock tmp;
	kprintf("%s start ...\r\n", __FUNCTION__);
	while(1) {
		if (gMsgQuePtr) {
			ret = VOSMsgQueGet(gMsgQuePtr, &tmp, sizeof(tmp), 1000);
			if (ret < 0) {
				kprintf("error: VOSMsgQueGet timeout!\r\n");
			}
			else {
				kprintf("info: VOSMsgQueGet tmp.id=%d!\r\n", tmp.id);
			}
		}
		else {
			VOSTaskDelay(1*1000);
		}
	}
}


static long long task0_stack[1024], task1_stack[1024];
void mq_test()
{
	kprintf("test msg queue!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), TASK_PRIO_NORMAL, "task0");
	task_id = VOSTaskCreate(task1, 0, task1_stack, sizeof(task1_stack), TASK_PRIO_HIGH, "task1");
	while (1) {
		VOSTaskDelay(1*1000);
	}
}

