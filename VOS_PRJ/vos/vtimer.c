/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vtimer.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

extern volatile u32  gVOSTicks;

static struct list_head gListTimerFree;//定时器链表闲链表
static struct list_head gListTimerRunning;//定时器正在运行的链表
static struct list_head gListTimerStop;//定时器暂停的
struct StVOSTimer gVOSTimer[MAX_VOS_TIEMR_NUM];

static StVOSSemaphore *gSemVosTimer = 0;

static StVOSMutex *gMutexVosTimer = 0; //互斥锁来保护资源


#define VOS_TIMER_LOCK() VOSMutexWait(gMutexVosTimer, TIMEOUT_INFINITY_U32)
#define VOS_TIMER_UNLOCK() VOSMutexRelease(gMutexVosTimer)

volatile u32 gVosTimerMarkNearest = TIMEOUT_INFINITY_U32; //记录延时时刻

static long long task_timer_stack[1024];

/********************************************************************************************************
* 函数：void VOSTimerSemPost();
* 描述: 唤醒定时任务
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSTimerSemPost()
{
	if (gSemVosTimer)
		VOSSemRelease(gSemVosTimer);
}

/********************************************************************************************************
* 函数：StVOSTimer *VOSTimerCreate(s32 type, u32 delay_ms, VOS_TIMER_CB callback, void *arg, s8 *name);
* 描述: 定时器创建
* 参数:
* [1] type: 一次性定时（VOS_TIMER_TYPE_ONE_SHOT），周期性定时（VOS_TIMER_TYPE_PERIODIC）
* [2] delay_ms: 定时时间
* [3] callback: 定时器执行例程
* [4] arg: 定时器例程参数
* [5] name: 定时器名
* 返回：定时器指针
* 注意：无
*********************************************************************************************************/
StVOSTimer *VOSTimerCreate(s32 type, u32 delay_ms, VOS_TIMER_CB callback, void *arg, s8 *name)
{
	StVOSTimer *pTimer = 0;
	VOS_TIMER_LOCK();
	if (!list_empty(&gListTimerFree)) {
		pTimer = list_entry(gListTimerFree.next, StVOSTimer, list);
		list_del(gListTimerFree.next);
		pTimer->type = type;
		pTimer->name = name;
		pTimer->status = VOS_TIMER_STA_STOPPED;
		pTimer->ticks_delay = MAKE_TICKS(delay_ms);
//		pTimer->ticks_start = gVOSTicks;
//		pTimer->ticks_alert = gVOSTicks + pTimer->ticks_delay;
		pTimer->callback = callback;
		pTimer->arg = arg;
		//添加到在暂停的链表
		list_add_tail(&pTimer->list, &gListTimerStop);
	}
	VOS_TIMER_UNLOCK();
	return  pTimer;
}

/********************************************************************************************************
* 函数：void VOSTimerDelete(StVOSTimer *pTimer);
* 描述: 定时器删除
* 参数:
* [1] pTimer: 指定定时器指针
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSTimerDelete(StVOSTimer *pTimer)
{
	VOS_TIMER_LOCK();
	if (pTimer && pTimer->status != VOS_TIMER_STA_FREE) {
		//断开当前的链表
		list_del(&pTimer->list);
		//添加到空闲链表
		pTimer->status = VOS_TIMER_STA_FREE;
		list_add_tail(&pTimer->list, &gListTimerFree);
	}
	VOS_TIMER_UNLOCK();
}

/********************************************************************************************************
* 函数：s32 VOSTimerStart(StVOSTimer *pTimer);
* 描述: 启动定时器
* 参数:
* [1] pTimer: 指定定时器指针
* 返回：无
* 注意：无
*********************************************************************************************************/
s32 VOSTimerStart(StVOSTimer *pTimer)
{
	s32 ret = VERROR_NO_ERROR;
	s32 notify = 0;//通知闹钟任务执行新的延时

	VOS_TIMER_LOCK();

	if (pTimer->status != VOS_TIMER_STA_STOPPED) {
		ret = VERROR_TIMER_RUNNING;
		goto END_TIMER_START;
	}
	//从暂停列表中删除
	list_del(&pTimer->list);
	//添加到运行列表
	pTimer->status = VOS_TIMER_STA_RUNNING;

	pTimer->ticks_start = gVOSTicks;
	pTimer->ticks_alert = gVOSTicks + pTimer->ticks_delay;

	//pTimer->ticks_alert < gVosTimerMarkNearest
	if (TICK_CMP(gVosTimerMarkNearest,pTimer->ticks_alert,pTimer->ticks_start)>0) {
		gVosTimerMarkNearest = pTimer->ticks_alert;
		notify = 1;
	}

	list_add_tail(&pTimer->list, &gListTimerRunning);


END_TIMER_START:

	VOS_TIMER_UNLOCK();

	if (notify) {//有更小的最近闹钟则通知任务执行新的延时
		VOSTimerSemPost();
	}
	return ret;
}

/********************************************************************************************************
* 函数：s32 VOSTimerStop(StVOSTimer *pTimer);
* 描述: 定时器暂停
* 参数:
* [1] pTimer: 指定定时器指针
* 返回：查看返回值
* 注意：无
*********************************************************************************************************/
s32 VOSTimerStop(StVOSTimer *pTimer)
{
	s32 ret = VERROR_NO_ERROR;
	VOS_TIMER_LOCK();
	if (pTimer->status != VOS_TIMER_STA_RUNNING) {
		ret = VERROR_TIMER_RUNNING;
		goto END_TIMER_STOP;
	}
	//从运行列表中删除
	list_del(&pTimer->list);
	//添加到暂停列表
	pTimer->status = VOS_TIMER_STA_STOPPED;
	list_add_tail(&pTimer->list, &gListTimerStop);

END_TIMER_STOP:
	VOS_TIMER_UNLOCK();
	return ret;

}

/********************************************************************************************************
* 函数：s32 VOSTimerGetStatus(StVOSTimer *pTimer);
* 描述: 获取定时器状态
* 参数:
* [1] pTimer: 指定定时器指针
* 返回：定时器状态
* 注意：无
*********************************************************************************************************/
s32 VOSTimerGetStatus(StVOSTimer *pTimer)
{
	s32 status = 0;
	VOS_TIMER_LOCK();
	status = pTimer->status;
	VOS_TIMER_UNLOCK();
	return status;
}

/********************************************************************************************************
* 函数：s32 VOSTimerGetLeftTime(StVOSTimer *pTimer);
* 描述: 获取闹钟响的剩余时间
* 参数:
* [1] pTimer: 指定定时器指针
* 返回：定时器剩余的时间，单位毫秒
* 注意：无
*********************************************************************************************************/
s32 VOSTimerGetLeftTime(StVOSTimer *pTimer)
{
	s32 ticks = 0;
	VOS_TIMER_LOCK();

	//pTimer->ticks_alert > gVOSTicks
	if (TICK_CMP(gVOSTicks,pTimer->ticks_alert,pTimer->ticks_start) < 0) {
		ticks = pTimer->ticks_alert - gVOSTicks;
	}
	VOS_TIMER_UNLOCK();
	return MAKE_TIME_MS(ticks);
}

/********************************************************************************************************
* 函数：void VOSTaskTimer(void *param);
* 描述: 定时器任务，实现多定时器执行操作
* 参数:
* [1] param: 暂时没用
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSTaskTimer(void *param)
{
	s32 ret = 0;
	struct list_head *list_timer;
	struct list_head *list_temp;
	StVOSTimer *pTimer;
	while(gSemVosTimer) {
		if (gVosTimerMarkNearest > gVOSTicks) {
			ret = VOSSemWait(gSemVosTimer, gVosTimerMarkNearest-gVOSTicks);//最小闹钟时间或者有信号发生就唤醒,注意这里要减去当前的时间才是延时的长度
		}
		VOS_TIMER_LOCK();

		gVosTimerMarkNearest = TIMEOUT_INFINITY_U32; //设置最大，下面更新最少的延时，然后延时处理
		list_for_each_safe(list_timer, list_temp, &gListTimerRunning) {
			pTimer = list_entry(list_timer, struct StVOSTimer, list);
			//gVOSTicks >= pTimer->ticks_alert
			if (TICK_CMP(gVOSTicks,pTimer->ticks_alert,pTimer->ticks_start) >= 0) { //闹钟到，执行相关任务
				if (pTimer->callback) pTimer->callback(pTimer, pTimer->arg);
				if (pTimer->type == VOS_TIMER_TYPE_ONE_SHOT) {
					//删除自己
					list_del(&pTimer->list);
					//添加到暂停队列
					pTimer->status = VOS_TIMER_STA_STOPPED;
					list_add_tail(&pTimer->list, &gListTimerStop);
				}
				else if (pTimer->type == VOS_TIMER_TYPE_PERIODIC){//更新闹钟时间，重新计时
					pTimer->ticks_start = gVOSTicks;
					pTimer->ticks_alert = gVOSTicks + pTimer->ticks_delay;
				}
				else {

				}
			}
			//都需要找到最近的闹钟
			if (pTimer->status == VOS_TIMER_STA_RUNNING &&
					TICK_CMP(gVosTimerMarkNearest,pTimer->ticks_alert,pTimer->ticks_start) > 0) { //pTimer->ticks_alert < gVosTimerMarkNearest
				gVosTimerMarkNearest = pTimer->ticks_alert;
			}
		}//ended ist_for_each(list_timer, &gListTimerRunning) {
		VOS_TIMER_UNLOCK();
	}
}

/********************************************************************************************************
* 函数：void VOSTimerInit();
* 描述: 定时器初始化（任务执行多个定时函数，如果某个阻塞，就影响到后面的定时器）
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSTimerInit()
{
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();

	INIT_LIST_HEAD(&gListTimerFree);
	INIT_LIST_HEAD(&gListTimerRunning);
	INIT_LIST_HEAD(&gListTimerStop);
	for (i=0; i<MAX_VOS_TIEMR_NUM; i++) {
		list_add_tail(&gVOSTimer[i].list, &gListTimerFree);
		gVOSTimer[i].status = VOS_TIMER_STA_FREE;
	}
	__local_irq_restore(irq_save);

	gMutexVosTimer = VOSMutexCreate("MutexVosTimer");

	gSemVosTimer = VOSSemCreate(1, 1, "SemVosTimer");//创建一个信号量，做延时和唤醒处理

	s32 task_id = VOSTaskCreate(VOSTaskTimer, 0, task_timer_stack, sizeof(task_timer_stack), VOS_TASK_TIMER_PRIO, "vtimer");

}

