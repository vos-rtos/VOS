#include "../vos/vtype.h"
#include "../vos/vos.h"


StVOSTimer *VOSTimerCreate(s32 type, u32 delay_ms, VOS_TIMER_CB callback, void *arg, s8 *name);
s32 VOSTimerDelete(StVOSTimer *pTimer);
s32 VOSTimerStart(StVOSTimer *pTimer);
s32 VOSTimerStop(StVOSTimer *pTimer);
s32 VOSTimerGetStatus(StVOSTimer *pTimer);
s32 VOSTimerGetLeftTime(StVOSTimer *pTimer);

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


static long long task_timer1_stack[1024];
static long long task_timer2_stack[1024];
static long long task_timer3_stack[1024];
static long long task_timer4_stack[1024];
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

}
