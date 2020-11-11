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

static s32 is_quit(u8 ch)
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

void task_modem(void *param)
{
	s32 ret = 0;
	u8 echo[100];
	u8 cmd[100];
	s32 mark = 0;
	int i = 0;

	while(1) {
		ret = peek_vgets(echo, sizeof(echo)-1);
		if (ret > 0) { //echo
			echo[ret] = 0;
			kprintf("%s", &echo[mark]);
			mark = ret;
		}
		if (ret > 0 && (echo[ret-1]=='\r'||echo[ret-1]=='\n')) {
			ret = vgets(cmd, sizeof(cmd)-1);
			if (strlen(cmd)) kprintf("\r\n");
			if (strncmp("quit", cmd, 4) == 0) goto END_UARTIN;
			for (i=0; i<strlen(cmd); i++) {
				if (cmd[i] == '\r' || cmd[i] == '\n') {
					if (i==0) break;
					cmd[i++] = '\r';
					cmd[i++] = '\n';
					cmd[i] = 0;
					__CUSTOM_DirectWriteAT(cmd, strlen(cmd), 2000);
					break;
				}
			}
			mark = 0;
			ret = 0;
		}
		//这里有modem的通知信息，也能正常打印出来。
		memset(cmd, 0, sizeof(cmd));
		__CUSTOM_DirectReadAT(cmd, sizeof(cmd)-1, 10);
		if (cmd[0]) {
			kprintf("%s", cmd);
		}
		VOSTaskDelay(1);
	}
END_UARTIN:
	kprintf("AT command EXIT!\r\n");
	TestExitFlagSet(1);
	return;
}





static long long task_modem_stack[1024];
void at_test()
{
	kprintf("4G Modem AT command TEST!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(task_modem, 0, task_modem_stack, sizeof(task_modem_stack), TASK_PRIO_NORMAL, "task_modem");
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}
