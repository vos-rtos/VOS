//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-15: initial by vincent.
//------------------------------------------------------

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

void RegistUartEvent(s32 event, s32 task_id);

void VOSTaskShell(void *param)
{
	s32 ret = 0;
	u8 buf[100];
	while(VOSEventWait(EVENT_USART1_RECV, TIMEOUT_INFINITY_U32)) {
		ret = peek_vgets(buf, sizeof(buf)-1);
		if (ret > 0) {
			buf[ret] = 0;
			if (buf[ret-1]=='\r'||buf[ret-1]=='\n') {
				ret = vgets(buf, sizeof(buf)-1);
				while (ret > 0 && (buf[ret-1]=='\r'||buf[ret-1]=='\n')) {
					ret--;
					buf[ret]=0;
				}
				if (ret > 0) buf[ret+1] = 0;
				kprintf("VOSTaskShell Get Uart Data:");
				kprintf("\"%s\"\r\n", buf);
			}
		}
	}
}


static long long task_vshell_stack[1024];
void VOSShellInit()
{
	s32 i = 0;

	s32 task_id = VOSTaskCreate(VOSTaskShell, 0, task_vshell_stack, sizeof(task_vshell_stack), VOS_TASK_VSHELL_PRIO, "vos_shell");

	RegistUartEvent(EVENT_USART1_RECV, task_id);

}
