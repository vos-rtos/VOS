/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vshell.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

void RegistUartEvent(s32 event, s32 task_id);

//static long long task_vshell_stack[512];
long long stack_shell_bg[512];

typedef void (*VSHELL_FUN)(s8 **parr, s32 cnts);
extern unsigned int vshell_name_start;
extern unsigned int vshell_name_end;
extern unsigned int vshell_fun_start;
extern unsigned int vshell_fun_end;
extern unsigned int vshell_note_start;
extern unsigned int vshell_note_end;

typedef struct StTaskShellExeInfo{
	VSHELL_FUN fun;
	s8 **parr;
	s32 cnts;
}StTaskShellExeInfo;


/********************************************************************************************************
* 函数：void task_shell_bg(void *param);
* 描述: 后台执行，单独开任务执行
* 参数:
* [1] param: StTaskShellExeInfo指针，包括执行函数和参数
* 返回：无
* 注意：无
*********************************************************************************************************/
void task_shell_bg(void *param)
{
	StTaskShellExeInfo *p = (StTaskShellExeInfo*)param;
	if (p->fun) {
		p->fun(p->parr, p->cnts);
	}
}

/********************************************************************************************************
* 函数：void vshell_do(s8 **parr, s32 cnts, s32 is_bg);
* 描述: vshell执行例程
* 参数:
* [1] parr: 数组，函数参数
* [2] cnts: 数组元素个数
* [3] is_bg: 输出，是否后台运行，后台运行就是创建独立任务执行
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VOSTaskShell(void *param);
* 描述: 命令行字符串处理，分割空格和识别后台运行
* 参数:
* [1] line: 用户输入的命令行字符串
* [2] parr: 分割空格，然后输出到该数组
* [3] max: 数组元素个数
* [4] is_bg: 输出，是否后台运行，后台运行就是创建独立任务执行
* 返回：数组的有效个数
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VOSTaskShell(void *param);
* 描述: vshell执行例程
* 参数:
* [1] param: 暂时无用
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VOSShellInit();
* 描述: vshell初始化任务
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSShellInit()
{
	s32 i = 0;
	void *task_vshell_stack = vmalloc(1024*4);
	s32 task_id = VOSTaskCreate(VOSTaskShell, 0, task_vshell_stack, 1024*4, VOS_TASK_VSHELL_PRIO, "vshell");

	RegistUartEvent(EVENT_USART1_RECV, task_id);

}


/********************************************************************************************************
* 下面是具体shell命令定义函数
*********************************************************************************************************/
void sem_test();
void event_test();
void mq_test();
void mutex_test();
void delay_test();
void schedule_test();
void uart_test();
void timer_test();
void stack_test();

void CaluTasksCpuUsedRateStart();
s32 CaluTasksCpuUsedRateShow(struct StTaskInfo *arr, s32 cnts, s32 mode);
s32 GetTaskIdByName(u8 *name);
s32 test_exit_flag = 0;

/********************************************************************************************************
* 函数：s32 TestExitFlagGet();
* 描述: 测试标志获取
* 参数: 无
* 返回：测试标志
* 注意：无
*********************************************************************************************************/
s32 TestExitFlagGet()
{
	return test_exit_flag;
}

/********************************************************************************************************
* 函数：void TestExitFlagSet(s32 flag);
* 描述: 测试标志设置
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void TestExitFlagSet(s32 flag)
{
	test_exit_flag = flag;
}

#if VOS_SHELL_TEST


/********************************************************************************************************
* 函数：void VSHELL_FUN(quit)(s8 **parr, s32 cnts);
* 描述: 退出测试命令函数
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(quit)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(1);
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(sem_t)(s8 **parr, s32 cnts);
* 描述: sem_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(sem_t)(s8 **parr, s32 cnts)
{
//	s32 i = 0;
//	for (i=0; i<cnts; i++) {
//		kprintf("param[%d]:%s\r\n", i, parr[i]);
//	}
	TestExitFlagSet(0);
	sem_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(event_t)(s8 **parr, s32 cnts);
* 描述: event_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(event_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	event_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(mq_t)(s8 **parr, s32 cnts);
* 描述: mq_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(mq_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	mq_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(mutex_t)(s8 **parr, s32 cnts);
* 描述: mutex_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(mutex_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	mutex_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(delay_t)(s8 **parr, s32 cnts);
* 描述: delay_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(delay_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	delay_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(schedule_t)(s8 **parr, s32 cnts);
* 描述: schedule_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(schedule_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	schedule_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(uart_t)(s8 **parr, s32 cnts);
* 描述: uart_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VSHELL_FUN(timer_t)(s8 **parr, s32 cnts);
* 描述: timer_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(timer_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	timer_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(stack_t)(s8 **parr, s32 cnts);
* 描述: stack_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(stack_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	stack_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(tickcmp_t)(s8 **parr, s32 cnts);
* 描述: tickcmp_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(tickcmp_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	tickcmp_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(vmem_t)(s8 **parr, s32 cnts);
* 描述: vmem_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(vmem_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	vmem_test();
}

/********************************************************************************************************
* 函数：void VSHELL_FUN(vheap_t)(s8 **parr, s32 cnts);
* 描述: vheap_t 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSHELL_FUN(vheap_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	vheap_test();
}
#endif
/********************************************************************************************************
* 函数：void VSHELL_FUN(task)(s8 **parr, s32 cnts);
* 描述: task 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VSHELL_FUN_NOTE(heap, "list heap info")(s8 **parr, s32 cnts);
* 描述: task 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
void VHeapShellShow();
void VSHELL_FUN_NOTE(heap, "list heap info")(s8 **parr, s32 cnts)
{
	s32 ret = 0;
	VHeapShellShow();
}


/********************************************************************************************************
* 函数：void VSHELL_FUN(help)(s8 **parr, s32 cnts);
* 描述: help 命令
* 参数:
* [1] parr: 参数列表，第一个参数是命令字符串
* [2] cnts: 命令参数个数
* 返回：无
* 注意：无
*********************************************************************************************************/
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

void VSHELL_FUN_NOTE(modem, "4G modem: AT command")(s8 **parr, s32 cnts)
{
	u32 event = 0;
	s32 task_id = GetTaskIdByName("vshell");
	if (task_id != -1) {
		event = VOSEventGet(task_id);
		VOSEventDisable(task_id, event);//屏蔽接收某些位事件
	}
	TestExitFlagSet(0);
	at_test();
	VOSEventEnable(task_id, event);//恢复接收某些位事件
}
void usart_process(char ch);
void VSHELL_FUN_NOTE(wifi, "sd wifi 8081")(s8 **parr, s32 cnts)
{
	s32 i = 0;
	for (i=0; i<cnts; i++) {
		kprintf("param[%d]:%s\r\n", i, parr[i]);
	}
	usart_process(parr[1][0]);
}


