#include "../vos/vtype.h"
#include "../vos/vos.h"

//s32 vgets(u8 *buf, s32 len)
//{
//	u8 ch;
//	int i = 0;
//	for (i=0; i<len; i++) {
//		if (vgetc(&ch))
//			buf[i] = ch;
//		else
//			break;
//	}
//	return i;
//}



s32 data_check(s8 *buf, s32 len)
{
	s32 i = 0;
	static s32 gindex = -1;
	if (gindex == -1) {
		gindex = buf[0]-'a';
	}
	for (i=0; i < len; i++) {
		if (buf[i] != 'a'+gindex%26) {
			kprintf("ERROR: data error!\r\n");
			while (1);
		}
		gindex++;
	}
}

s32 vvgets(u8 *buf, s32 len);
static void task_uartin(void *param)
{
	s32 ret = 0;
	u8 buf[1000];
	u32 counts = 0;
	u32 mark_cnts = 0;
	s64 mark_ms = 0;

	s32 time_spend = 0;
	mark_ms = VOSGetTimeMs();
	kprintf("statistics:\r\n");
	while(1) {
		ret = vgets(buf, sizeof(buf)-1);
		if (ret > 0){
			//data_check(buf, ret);
			buf[ret] = 0;
			counts += ret;
			if (counts > mark_cnts+5000){
				mark_cnts = counts;
				time_spend = (s32)(VOSGetTimeMs()-mark_ms);
				kprintf("speed[%08dms:%08dB:%05dBps]\r\n", time_spend, counts, (s32)((u64)(counts)*1000/time_spend));
			}
		}
		VOSTaskDelay(10);
	}
}





static long long task_uartin_stack[1024];
void uart_test()
{
	kprintf("uart_test!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task_uartin, 0, task_uartin_stack, sizeof(task_uartin_stack), TASK_PRIO_NORMAL, "task_uartin");
}
