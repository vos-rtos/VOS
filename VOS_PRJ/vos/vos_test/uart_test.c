#include "../vos/vtype.h"
#include "cmsis/stm32f4xx.h"
#include "stm32f4-hal/stm32f4xx_hal.h"
#include "../vos/vos.h"

s32 vgets(u8 *buf, s32 len)
{
	u8 ch;
	int i = 0;
	for (i=0; i<len; i++) {
		if (vgetc(&ch))
			buf[i] = ch;
		else
			break;
	}
	return i;
}
extern int aaa;
//static void task_uartin(void *param)
//{
//	s32 ret = 0;
//	u8 buf[100];
//
//	while(1) {
//		ret = vgets(buf, sizeof(buf));
//		if (ret > 0 && ret != sizeof(buf)){
//			buf[ret] = 0;
//			kprintf(".");
//			//kprintf("buf=%s\r\n", buf);
//		}
//
//		VOSTaskDelay(100);
//		if (aaa) {
//			VOSTaskPrtList(VOS_LIST_READY);
//			VOSTaskPrtList(VOS_LIST_BLOCK);
//		}
//	}
//}

s32 vvgets(u8 *buf, s32 len);
static void task_uartin(void *param)
{
	s32 ret = 0;
	u8 buf[100];

	while(1) {
		ret = vvgets(buf, sizeof(buf)-1);
		if (ret > 0){
			buf[ret] = 0;
			//kprintf(".");
			kprintf("buf=%s\r\n", buf);
		}

		VOSTaskDelay(10);
		if (aaa) {
			VOSTaskPrtList(VOS_LIST_READY);
			VOSTaskPrtList(VOS_LIST_BLOCK);
		}
	}
}





static long long task_uartin_stack[1024];
void uart_test()
{
	kprintf("uart_test!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task_uartin, 0, task_uartin_stack, sizeof(task_uartin_stack), TASK_PRIO_NORMAL, "task_uartin");
}
