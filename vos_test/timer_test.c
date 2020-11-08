/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: timer_test.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828������ע��
*********************************************************************************************************/

#include "vtype.h"
#include "vos.h"




StVOSTimer *timer1 =0;
StVOSTimer *timer2 =0;
StVOSTimer *timer3 =0;
StVOSTimer *timer4 =0;


void func_timer1(void *ptimer, void *parg)
{
	static s32 count = 0;
	StVOSTimer *p = (StVOSTimer*)ptimer;
	kprintf("%s->%s count=%d running!\r\n", __FUNCTION__, p->name, count++);
}

void func_timer2(void *ptimer, void *parg)
{
	static s32 count = 0;
	StVOSTimer *p = (StVOSTimer*)ptimer;
	kprintf("%s->%s count=%d running!\r\n", __FUNCTION__, p->name, count++);
}

void func_timer3(void *ptimer, void *parg)
{
	static s32 count = 0;
	StVOSTimer *p = (StVOSTimer*)ptimer;
	kprintf("%s->%s count=%d running!\r\n", __FUNCTION__, p->name, count++);
}

void func_timer4(void *ptimer, void *parg)
{
	static s32 count = 0;
	StVOSTimer *p = (StVOSTimer*)ptimer;
	kprintf("%s->%s count=%d running!\r\n", __FUNCTION__, p->name, count++);
}


static long long task_timer1_stack[256];
static long long task_timer2_stack[256];
static long long task_timer3_stack[256];
static long long task_timer4_stack[256];
void timer_test()
{
	kprintf("timer_test!\r\n");
	timer1 = VOSTimerCreate(VOS_TIMER_TYPE_ONE_SHOT, 5000, func_timer1, 0, "timer1");
	timer2 = VOSTimerCreate(VOS_TIMER_TYPE_ONE_SHOT, 10000, func_timer2, 0, "timer2");
	timer3 = VOSTimerCreate(VOS_TIMER_TYPE_PERIODIC, 1000, func_timer3, 0, "timer3");
	timer4 = VOSTimerCreate(VOS_TIMER_TYPE_PERIODIC, 8000, func_timer4, 0, "timer4");

	VOSTimerStart(timer1);
	VOSTimerStart(timer2);
	VOSTimerStart(timer3);
	VOSTimerStart(timer4);

	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
	VOSTimerStop(timer1);
	VOSTimerStop(timer2);
	VOSTimerStop(timer3);
	VOSTimerStop(timer4);

}