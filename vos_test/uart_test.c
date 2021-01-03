/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: uart_test.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "vtype.h"
#include "vos.h"

s32 usbd_uart_app_gets(u8* buf, s32 size);
int usb_printf(char* format, ...);
//#define vgets usbd_uart_app_gets
//#define kprintf usb_printf

s32 data_check(s8 *buf, s32 len)
{
	s32 i = 0;
	static s32 gindex = -1;
	if (gindex == -1) {
		gindex = buf[0]-'a';
	}
	for (i=0; i < len; i++) {
		if (buf[i] != 'a'+gindex%26) {
			gindex = -1;
			kprintf("#");
			break;
		}
		gindex++;
	}
}

s32 is_quit(u8 ch)
{
	s32 ret = 0;
	static s32 index = 0;

	switch (index) {
	case 0:
		ch == 'q' ? index++ : (index = 0);
		break;
	case 1:
		ch == 'u' ? index++ : (index = 0);
		break;
	case 2:
		ch == 'i' ? index++ : (index = 0);
		break;
	case 3:
		ch == 't' ? (ret = 1, index = 0) : (index = 0);
		break;
	}
	return ret;
}

void task_uartin(void *param)
{
	s32 ret = 0;
	u8 buf[512];
	u32 counts = 0;
	u32 mark_cnts = 0;
	u32 mark_ms = 0;
	int i = 0;

	s32 time_spend = 0;
	mark_ms = VOSGetTimeMs();
	kprintf("statistics:\r\n");
	while(1) {
		ret = vgets(buf, sizeof(buf)-1);
		if (ret > 0){
			for (i=0; i<ret; i++) {
				if (is_quit(buf[i])) {
					goto END_UARTIN;
				}
			}
			data_check(buf, ret);
			buf[ret] = 0;
			counts += ret;
			if (counts > mark_cnts+5000){
				mark_cnts = counts;
				time_spend = (s32)(VOSGetTimeMs()-mark_ms);
				kprintf("speed[%08d(ms):%08d(B):%05d(KBps)]\r\n", time_spend, counts, (s32)((u32)(counts)/time_spend));
			}
		}
		VOSTaskDelay(1);
	}
END_UARTIN:
	TestExitFlagSet(1);
	return;
}





static long long task_uartin_stack[256];
void uart_test()
{
	kprintf("uart_test!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task_uartin, 0, task_uartin_stack, sizeof(task_uartin_stack), TASK_PRIO_NORMAL, "task_uartin");
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}
