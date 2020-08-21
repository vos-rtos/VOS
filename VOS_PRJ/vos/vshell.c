//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-15: initial by vincent.
//------------------------------------------------------

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

SHELL_FUN(vincent)(s8 *fun_name)
{
	kprintf("vincent--%s--\r\n", fun_name);
}
SHELL_FUN(abc)(s8 *fun_name)
{
	kprintf("abc--%s--\r\n", fun_name);
}

SHELL_FUN(aaa)(s8 *fun_name)
{
	kprintf("aaa--%s--\r\n", fun_name);
}
SHELL_FUN(bbb)(s8 *fun_name)
{
	kprintf("bbb--%s--\r\n", fun_name);
}


typedef void (*SHELL_FUN)(u8*);
extern unsigned int shell_name_start;
extern unsigned int shell_name_end;
extern unsigned int shell_fun_start;
extern unsigned int shell_fun_end;

void shell_do(u8 *fun_name)
{
	u32 *pfname = 0;
	u32 *pfun = 0;
	u8 *name = 0;
	SHELL_FUN fun = 0;
	for (pfname = &shell_name_start, pfun = &shell_fun_start; pfname < &shell_name_end; pfname++, pfun++){
		name = (unsigned int *)(*pfname);
		fun = (SHELL_FUN)(*pfun);
		if (strcmp(name, fun_name)==0){
			fun(name);
			break;
		}
	}
	if (pfname == &shell_name_end) {
		kprintf("info: no command name \"%s\"!\r\n", fun_name);
	}
}
void RegistUartEvent(s32 event, s32 task_id);

void VOSTaskShell(void *param)
{
	s32 i =  0;
	s32 ret = 0;
	u8 buf[100];
	while(VOSEventWait(EVENT_USART1_RECV, TIMEOUT_INFINITY_U32)) {
		ret = peek_vgets(buf, sizeof(buf)-1);
		if (ret > 0 && (buf[ret-1]=='\r'||buf[ret-1]=='\n')) {
			ret = vgets(buf, sizeof(buf)-1);
			for (i=0; i<ret; i++) {
				if (buf[i]=='\r'||buf[i]=='\n') {
					buf[i]=0;
					break;
				}
			}
			if (i==ret) buf[ret] = 0;
			kprintf("VOSTaskShell Get Uart Data:");
			kprintf("\"%s\"\r\n", buf);
			shell_do(buf);
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
