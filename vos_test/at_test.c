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

static int dumphex(const unsigned char *buf, int size)
{
	int i;
	for(i=0; i<size; i++)
		kprintf("%02x,%s", buf[i], (i+1)%16?"":"\r\n");
	if (i%16) kprintf("\r\n");
	return 0;
}



s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);
s32 CUSTOM_WriteMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);

#define MODEM_WRITE CUSTOM_WriteMODEM //__CUSTOM_DirectWriteAT
#define MODEM_READ CUSTOM_ReadMODEM //__CUSTOM_DirectReadAT
void task_modem(void *param)
{
	s32 at_done = 0;
	s32 ret = 0;
	u8 echo[100];
	u8 cmd[16*100];
	s32 mark = 0;
	int i = 0;
	s32 writed = 0;
	s32 readed = 0;
	while(1) {
		ret = peek_vgets(echo, sizeof(echo)-1);

		if (ret > 0 && at_done==0) { //echo
			echo[ret] = 0;
			kprintf("%s", &echo[mark]);
			mark = ret;
		}
		if (at_done==0 && ret > 0 && (echo[ret-1]=='\r'||echo[ret-1]=='\n')) {
			ret = vgets(cmd, sizeof(cmd)-1);
			if (strlen(cmd)) kprintf("\r\n");
			if (strncmp("quit", cmd, 4) == 0) goto END_UARTIN;
			for (i=0; i<strlen(cmd); i++) {
				if (cmd[i] == '\r' || cmd[i] == '\n') {
					if (i==0) break;
					cmd[i++] = '\r';
					cmd[i++] = '\n';
					cmd[i] = 0;
					writed = MODEM_WRITE(cmd, strlen(cmd), 2000);
					break;
				}
			}
			mark = 0;
			ret = 0;
		}
		else {//透穿二进制数据
			ret = vgets(cmd, sizeof(cmd)-1);
			if (ret > 0) {
				writed = MODEM_WRITE(cmd, ret, 2000);
			}
		}
		//这里有modem的通知信息，也能正常打印出来。
		memset(cmd, 0, sizeof(cmd));
		readed = MODEM_READ(cmd, sizeof(cmd)-1, 10);
		if (at_done == 0 && cmd[0]) {
			kprintf("%s", cmd);
			if (strstr(cmd, "CONNECT") || strstr(cmd, "connnect")) {
				at_done = 1;
			}
		}
		if (at_done && readed > 0) {
			kprintf("modem recv:\r\n");
			dumphex(cmd, readed);
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
