//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-15: initial by vincent.
//------------------------------------------------------

//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-21: initial by vincent.
//------------------------------------------------------

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

void SHELL_FUN(vincent)(s8 *fun_name)
{
	kprintf("vincent--%s--\r\n", fun_name);
}
void SHELL_FUN(abc)(s8 *fun_name)
{
	kprintf("abc--%s--\r\n", fun_name);
}

void SHELL_FUN(aaa)(s8 *fun_name)
{
	kprintf("aaa--%s--\r\n", fun_name);
}
//
void SHELL_FUN_NOTE(bbb, "this is function for test!")(s8 *fun_name)
{
	kprintf("bbb--%s--\r\n", fun_name);
}

typedef void (*SHELL_FUN)(s8*);
extern unsigned int shell_name_start;
extern unsigned int shell_name_end;
extern unsigned int shell_fun_start;
extern unsigned int shell_fun_end;
extern unsigned int shell_note_start;
extern unsigned int shell_note_end;


void SHELL_FUN(help)(s8 *fun_name)
{
	s32 i = 0;
	u32 *pfname = 0;
	u32 *pfnote = 0;
	s8 *name = 0;
	s8 *note = 0;
	kprintf("cmd name: %s\r\n", fun_name);

	for (pfname = &shell_name_start, pfnote = &shell_note_start; pfname < &shell_name_end; pfname++,pfnote++){
		name = (unsigned int *)(*pfname);
		note = (unsigned int *)(*pfnote);

		kprintf("%02d. %s\t\t%s\r\n", i++, name, note?note:"");
	}
}


void shell_do(s8 *fun_name)
{
	u32 *pfname = 0;
	u32 *pfun = 0;
	s8 *name = 0;
	SHELL_FUN fun = 0;
	for (pfname = &shell_name_start, pfun = &shell_fun_start; pfname < &shell_name_end; pfname++, pfun++){
		name = (unsigned int *)(*pfname);
		if (strcmp(name, fun_name)==0){
			fun = (SHELL_FUN)(*pfun);
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
	u8  cmd[100];
	while(VOSEventWait(EVENT_USART1_RECV, TIMEOUT_INFINITY_U32)) {
		ret = peek_vgets(cmd, sizeof(cmd)-1);
		if (ret > 0 && (cmd[ret-1]=='\r'||cmd[ret-1]=='\n')) {
			ret = vgets(cmd, sizeof(cmd)-1);
			for (i=0; i<ret; i++) {
				if (cmd[i]=='\r'||cmd[i]=='\n') {
					cmd[i]=0;
					break;
				}
			}
			if (i==ret) cmd[ret] = 0;
			kprintf("VOSTaskShell Get Uart Data:");
			kprintf("\"%s\"\r\n", cmd);
			shell_do(cmd);
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
