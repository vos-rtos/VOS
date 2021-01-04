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

int kprintf(char* format, ...);

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
	int i = 0;
	u32 totals = 0;
	u32 bytes_1s = 0;

	u32 timemark = VOSGetTimeMs();
	u32 mark_1s = 0;

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
	  		u32 time_span = VOSGetTimeMs()-timemark;
	  		totals += ret;
	  		bytes_1s += ret;
	  		if (VOSGetTimeMs() - timemark > 1000) {
	  			kprintf("=====%d(KBps), totals=%d(KB) =====!\r\n", bytes_1s/1000, totals/1000);
	  			timemark = VOSGetTimeMs();
	  			bytes_1s = 0;
	  		}
		}
		if (ret <= 0) {
			VOSTaskDelay(5);
		}
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
