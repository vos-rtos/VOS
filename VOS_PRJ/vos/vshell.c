//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-21: initial by vincent.
//------------------------------------------------------

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

void sem_test();
void event_test();
void mq_test();
void mutex_test();
void delay_test();
void schedule_test();
void uart_test();
void timer_test();
void stack_test();

s32 test_exit_flag = 0;

s32 TestExitFlagGet()
{
	return test_exit_flag;
}

void TestExitFlagSet(s32 flag)
{
	test_exit_flag = flag;
}


void VSHELL_FUN(quit)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(1);
}


void VSHELL_FUN(sem_t)(s8 **parr, s32 cnts)
{
//	s32 i = 0;
//	for (i=0; i<cnts; i++) {
//		kprintf("param[%d]:%s\r\n", i, parr[i]);
//	}
	TestExitFlagSet(0);
	sem_test();
}

void VSHELL_FUN(event_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	event_test();
}
//
void VSHELL_FUN(mq_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	mq_test();
}
void VSHELL_FUN(mutex_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	mutex_test();
}
void VSHELL_FUN(delay_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	delay_test();
}
void VSHELL_FUN(schedule_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	schedule_test();
}
s32 GetTaskIdByName(u8 *name);
void VSHELL_FUN(uart_t)(s8 **parr, s32 cnts)
{
	u32 event = 0;
	s32 task_id = GetTaskIdByName("vshell");
	if (task_id != -1) {
		event = VOSEventGet(task_id);
		VOSEventDisable(task_id, event);//屏蔽接收某些位事件
	}
	TestExitFlagSet(0);
	uart_test();
	VOSEventEnable(task_id, event);//恢复接收某些位事件
}

void VSHELL_FUN(timer_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	timer_test();
}

void VSHELL_FUN(stack_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	stack_test();
}
void VSHELL_FUN(tickcmp_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	tickcmp_test();
}

void CaluTasksCpuUsedRateStart();
s32 CaluTasksCpuUsedRateShow(struct StTaskInfo *arr, s32 cnts, s32 mode);
void VSHELL_FUN_NOTE(task, "list tasks infomation: task [time_ms]")(s8 **parr, s32 cnts)
{
	s32 ret = 0;
	s32 mode = 0;
	s32 i = 0;
	s32 timeout = 1000;//采集时间为1秒
	struct StTaskInfo arr[MAX_VOSTASK_NUM];

	if (cnts > 1) {
		timeout = atoi(parr[1]);//第一个参数是表示采集时间，默认一秒，
						  //注意：这个参数如果是0，就一直采集，不结束。
	}
	CaluTasksCpuUsedRateStart();

	if (timeout==0) {
		mode = 1; //一直采集，可以一直显示
	}
	else {
		VOSTaskDelay(timeout);
	}
	ret = CaluTasksCpuUsedRateShow(arr, MAX_VOSTASK_NUM, mode);
}

typedef void (*VSHELL_FUN)(s8 **parr, s32 cnts);
extern unsigned int vshell_name_start;
extern unsigned int vshell_name_end;
extern unsigned int vshell_fun_start;
extern unsigned int vshell_fun_end;
extern unsigned int vshell_note_start;
extern unsigned int vshell_note_end;


void VSHELL_FUN(help)(s8 **parr, s32 cnts)
{
	s32 i = 0;
	u32 *pfname = 0;
	u32 *pfnote = 0;
	s8 *name = 0;
	s8 *note = 0;
	kprintf("\r\ncmd name: %s\r\n", parr[0]);

	for (pfname = &vshell_name_start, pfnote = &vshell_note_start; pfname < &vshell_name_end; pfname++,pfnote++){
		name = (unsigned int *)(*pfname);
		note = (unsigned int *)(*pfnote);

		kprintf("%02d. %s\t%s\r\n", i++, name, note?note:"");
	}
}


typedef struct StTaskShellExeInfo{
	VSHELL_FUN fun;
	s8 **parr;
	s32 cnts;
}StTaskShellExeInfo;

void task_shell_bg(void *param)
{
	StTaskShellExeInfo *p = (StTaskShellExeInfo*)param;
	if (p->fun) {
		p->fun(p->parr, p->cnts);
	}
}


long long stack_shell_bg[1024];
void vshell_do(s8 **parr, s32 cnts, s32 is_bg)
{
	u32 *pfname = 0;
	u32 *pfun = 0;
	s8 *name = 0;
	VSHELL_FUN fun = 0;
	static StTaskShellExeInfo temp;
	for (pfname = &vshell_name_start, pfun = &vshell_fun_start; pfname < &vshell_name_end; pfname++, pfun++){
		name = (unsigned int *)(*pfname);
		if (strcmp(name, parr[0])==0){
			fun = (VSHELL_FUN)(*pfun);
			if (strcmp(&name[strlen(name)-2], "_t")==0) is_bg = 1; //测试任务全部独立开任务来测试。
			if (is_bg == 1) {
				temp.parr = parr;
				temp.cnts = cnts;
				temp.fun = fun;
				VOSTaskCreate(task_shell_bg, &temp, stack_shell_bg, sizeof(stack_shell_bg), TASK_PRIO_NORMAL, name);
			}
			else {//shell 任务执行
				fun(parr, cnts);
			}
			break;
		}
	}
}
void RegistUartEvent(s32 event, s32 task_id);
//is_bg, 获取是否后台运行
s32 VOSShellPaserLine(s8 *line, s8 **parr, s32 max, s32 *is_bg)
{
	s32 i = 0;
	s8 *p = line;
	s32 cnts = 0;
	s32 len = strlen(line);

	for (i=0; i<len; i++) {
		if (line[i]=='\r'||line[i]=='\n'||line[i]=='&') { // & 后台开个新任务执行该函数。
			if (line[i]=='&' && is_bg) {
				*is_bg = 1;
			}
			line[i] = 0;
			if (*p) {
				parr[cnts] = p;
				cnts++;
			}
			break;
		}
		if (line[i]==' '){//空格分离参数
			line[i] = 0;
			if (cnts >= max) {//参数个数太多，直接返回
				return cnts;
			}
			parr[cnts] = p;
			cnts++;
			while (line[i+1] == ' ') { //吃掉连续的空格
				i++;
			}
			p = &line[i+1];
		}
	}
	return cnts;
}

void VOSTaskShell(void *param)
{
	s32 i =  0;
	s32 is_bg = 0;
	s8 *parr[10] = {0};
	s32 ret = 0;
	s32 cnts = 0;
	u8  cmd[100];
	u8 echo[100];
	s32 mark = 0;

	while(VERROR_NO_ERROR == VOSEventWait(EVENT_USART1_RECV, TIMEOUT_INFINITY_U32)) {
		ret = peek_vgets(echo, sizeof(echo)-1);
		if (ret > 0) { //echo
			echo[ret] = 0;
			kprintf("%s", &echo[mark]);
			mark = ret;
		}
		if (ret > 0 && (echo[ret-1]=='\r'||echo[ret-1]=='\n')) {
			ret = vgets(cmd, sizeof(cmd)-1);
			if (strlen(cmd)) kprintf("\r\n");
			is_bg = 0;
			cnts = VOSShellPaserLine(cmd, parr, sizeof(parr)/sizeof(parr[0]), &is_bg);
			//执行命令行
			vshell_do(parr, cnts, is_bg); //直接shell任务执行

			mark = 0;
			ret = 0;

			kprintf("\r\n%s", VSHELL_PROMPT);
		}
	}
}


static long long task_vshell_stack[1024];
void VOSShellInit()
{
	s32 i = 0;

	s32 task_id = VOSTaskCreate(VOSTaskShell, 0, task_vshell_stack, sizeof(task_vshell_stack), VOS_TASK_VSHELL_PRIO, "vshell");

	RegistUartEvent(EVENT_USART1_RECV, task_id);

}
