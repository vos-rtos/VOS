/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vshell.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828������ע��
*********************************************************************************************************/

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

void RegistUartEvent(s32 event, s32 task_id);

static long long task_vshell_stack[1024];
long long stack_shell_bg[1024];

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
* ������void task_shell_bg(void *param);
* ����: ��ִ̨�У�����������ִ��
* ����:
* [1] param: StTaskShellExeInfoָ�룬����ִ�к����Ͳ���
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void task_shell_bg(void *param)
{
	StTaskShellExeInfo *p = (StTaskShellExeInfo*)param;
	if (p->fun) {
		p->fun(p->parr, p->cnts);
	}
}

/********************************************************************************************************
* ������void vshell_do(s8 **parr, s32 cnts, s32 is_bg);
* ����: vshellִ������
* ����:
* [1] parr: ���飬��������
* [2] cnts: ����Ԫ�ظ���
* [3] is_bg: ������Ƿ��̨���У���̨���о��Ǵ�����������ִ��
* ���أ���
* ע�⣺��
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
			if (strcmp(&name[strlen(name)-2], "_t")==0) is_bg = 1; //��������ȫ�����������������ԡ�
			if (is_bg == 1) {
				temp.parr = parr;
				temp.cnts = cnts;
				temp.fun = fun;
				VOSTaskCreate(task_shell_bg, &temp, stack_shell_bg, sizeof(stack_shell_bg), TASK_PRIO_NORMAL, name);
			}
			else {//shell ����ִ��
				fun(parr, cnts);
			}
			break;
		}
	}
}

/********************************************************************************************************
* ������void VOSTaskShell(void *param);
* ����: �������ַ����������ָ�ո��ʶ���̨����
* ����:
* [1] line: �û�������������ַ���
* [2] parr: �ָ�ո�Ȼ�������������
* [3] max: ����Ԫ�ظ���
* [4] is_bg: ������Ƿ��̨���У���̨���о��Ǵ�����������ִ��
* ���أ��������Ч����
* ע�⣺��
*********************************************************************************************************/
s32 VOSShellPaserLine(s8 *line, s8 **parr, s32 max, s32 *is_bg)
{
	s32 i = 0;
	s8 *p = line;
	s32 cnts = 0;
	s32 len = strlen(line);

	for (i=0; i<len; i++) {
		if (line[i]=='\r'||line[i]=='\n'||line[i]=='&') { // & ��̨����������ִ�иú�����
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
		if (line[i]==' '){//�ո�������
			line[i] = 0;
			if (cnts >= max) {//��������̫�ֱ࣬�ӷ���
				return cnts;
			}
			parr[cnts] = p;
			cnts++;
			while (line[i+1] == ' ') { //�Ե������Ŀո�
				i++;
			}
			p = &line[i+1];
		}
	}
	return cnts;
}

/********************************************************************************************************
* ������void VOSTaskShell(void *param);
* ����: vshellִ������
* ����:
* [1] param: ��ʱ����
* ���أ���
* ע�⣺��
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
			//ִ��������
			vshell_do(parr, cnts, is_bg); //ֱ��shell����ִ��

			mark = 0;
			ret = 0;

			kprintf("\r\n%s", VSHELL_PROMPT);
		}
	}
}

/********************************************************************************************************
* ������void VOSShellInit();
* ����: vshell��ʼ������
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSShellInit()
{
	s32 i = 0;

	s32 task_id = VOSTaskCreate(VOSTaskShell, 0, task_vshell_stack, sizeof(task_vshell_stack), VOS_TASK_VSHELL_PRIO, "vshell");

	RegistUartEvent(EVENT_USART1_RECV, task_id);

}


/********************************************************************************************************
* �����Ǿ���shell����庯��
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
* ������s32 TestExitFlagGet();
* ����: ���Ա�־��ȡ
* ����: ��
* ���أ����Ա�־
* ע�⣺��
*********************************************************************************************************/
s32 TestExitFlagGet()
{
	return test_exit_flag;
}

/********************************************************************************************************
* ������void TestExitFlagSet(s32 flag);
* ����: ���Ա�־����
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void TestExitFlagSet(s32 flag)
{
	test_exit_flag = flag;
}

#if VOS_SHELL_TEST


/********************************************************************************************************
* ������void VSHELL_FUN(quit)(s8 **parr, s32 cnts);
* ����: �˳����������
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(quit)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(1);
}

/********************************************************************************************************
* ������void VSHELL_FUN(sem_t)(s8 **parr, s32 cnts);
* ����: sem_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
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
* ������void VSHELL_FUN(event_t)(s8 **parr, s32 cnts);
* ����: event_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(event_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	event_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(mq_t)(s8 **parr, s32 cnts);
* ����: mq_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(mq_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	mq_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(mutex_t)(s8 **parr, s32 cnts);
* ����: mutex_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(mutex_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	mutex_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(delay_t)(s8 **parr, s32 cnts);
* ����: delay_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(delay_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	delay_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(schedule_t)(s8 **parr, s32 cnts);
* ����: schedule_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(schedule_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	schedule_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(uart_t)(s8 **parr, s32 cnts);
* ����: uart_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(uart_t)(s8 **parr, s32 cnts)
{
	u32 event = 0;
	s32 task_id = GetTaskIdByName("vshell");
	if (task_id != -1) {
		event = VOSEventGet(task_id);
		VOSEventDisable(task_id, event);//���ν���ĳЩλ�¼�
	}
	TestExitFlagSet(0);
	uart_test();
	VOSEventEnable(task_id, event);//�ָ�����ĳЩλ�¼�
}

/********************************************************************************************************
* ������void VSHELL_FUN(timer_t)(s8 **parr, s32 cnts);
* ����: timer_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(timer_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	timer_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(stack_t)(s8 **parr, s32 cnts);
* ����: stack_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(stack_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	stack_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(tickcmp_t)(s8 **parr, s32 cnts);
* ����: tickcmp_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(tickcmp_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	tickcmp_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(vmem_t)(s8 **parr, s32 cnts);
* ����: vmem_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(vmem_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	vmem_test();
}

/********************************************************************************************************
* ������void VSHELL_FUN(vheap_t)(s8 **parr, s32 cnts);
* ����: vheap_t ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN(vheap_t)(s8 **parr, s32 cnts)
{
	TestExitFlagSet(0);
	vheap_test();
}
#endif
/********************************************************************************************************
* ������void VSHELL_FUN(task)(s8 **parr, s32 cnts);
* ����: task ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSHELL_FUN_NOTE(task, "list tasks infomation: task [time_ms]")(s8 **parr, s32 cnts)
{
	s32 ret = 0;
	s32 mode = 0;
	s32 i = 0;
	s32 timeout = 1000;//�ɼ�ʱ��Ϊ1��
	struct StTaskInfo arr[MAX_VOSTASK_NUM];

	if (cnts > 1) {
		timeout = atoi(parr[1]);//��һ�������Ǳ�ʾ�ɼ�ʱ�䣬Ĭ��һ�룬
						  //ע�⣺������������0����һֱ�ɼ�����������
	}
	CaluTasksCpuUsedRateStart();

	if (timeout==0) {
		mode = 1; //һֱ�ɼ�������һֱ��ʾ
	}
	else {
		VOSTaskDelay(timeout);
	}
	ret = CaluTasksCpuUsedRateShow(arr, MAX_VOSTASK_NUM, mode);
}

/********************************************************************************************************
* ������void VSHELL_FUN_NOTE(heap, "list heap info")(s8 **parr, s32 cnts);
* ����: task ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VHeapShellShow();
void VSHELL_FUN_NOTE(heap, "list heap info")(s8 **parr, s32 cnts)
{
	s32 ret = 0;
	VHeapShellShow();
}


/********************************************************************************************************
* ������void VSHELL_FUN(help)(s8 **parr, s32 cnts);
* ����: help ����
* ����:
* [1] parr: �����б�����һ�������������ַ���
* [2] cnts: �����������
* ���أ���
* ע�⣺��
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
