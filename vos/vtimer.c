/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vtimer.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

extern volatile u32  gVOSTicks;

static struct list_head gListTimerFree;//��ʱ������������
static struct list_head gListTimerRunning;//��ʱ���������е�����
static struct list_head gListTimerStop;//��ʱ����ͣ��
struct StVOSTimer gVOSTimer[MAX_VOS_TIEMR_NUM];

static StVOSSemaphore *gSemVosTimer = 0;

static StVOSMutex *gMutexVosTimer = 0; //��������������Դ


#define VOS_TIMER_LOCK() VOSMutexWait(gMutexVosTimer, TIMEOUT_INFINITY_U32)
#define VOS_TIMER_UNLOCK() VOSMutexRelease(gMutexVosTimer)

volatile u32 gVosTimerMarkNearest = TIMEOUT_INFINITY_U32; //��¼��ʱʱ��

static long long task_timer_stack[1024];

/********************************************************************************************************
* ������void VOSTimerSemPost();
* ����: ���Ѷ�ʱ����
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTimerSemPost()
{
	if (gSemVosTimer)
		VOSSemRelease(gSemVosTimer);
}

/********************************************************************************************************
* ������StVOSTimer *VOSTimerCreate(s32 type, u32 delay_ms, VOS_TIMER_CB callback, void *arg, s8 *name);
* ����: ��ʱ������
* ����:
* [1] type: һ���Զ�ʱ��VOS_TIMER_TYPE_ONE_SHOT���������Զ�ʱ��VOS_TIMER_TYPE_PERIODIC��
* [2] delay_ms: ��ʱʱ��
* [3] callback: ��ʱ��ִ������
* [4] arg: ��ʱ�����̲���
* [5] name: ��ʱ����
* ���أ���ʱ��ָ��
* ע�⣺��
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
		//��ӵ�����ͣ������
		list_add_tail(&pTimer->list, &gListTimerStop);
	}
	VOS_TIMER_UNLOCK();
	return  pTimer;
}

/********************************************************************************************************
* ������void VOSTimerDelete(StVOSTimer *pTimer);
* ����: ��ʱ��ɾ��
* ����:
* [1] pTimer: ָ����ʱ��ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTimerDelete(StVOSTimer *pTimer)
{
	VOS_TIMER_LOCK();
	if (pTimer && pTimer->status != VOS_TIMER_STA_FREE) {
		//�Ͽ���ǰ������
		list_del(&pTimer->list);
		//��ӵ���������
		pTimer->status = VOS_TIMER_STA_FREE;
		list_add_tail(&pTimer->list, &gListTimerFree);
	}
	VOS_TIMER_UNLOCK();
}

/********************************************************************************************************
* ������s32 VOSTimerStart(StVOSTimer *pTimer);
* ����: ������ʱ��
* ����:
* [1] pTimer: ָ����ʱ��ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
s32 VOSTimerStart(StVOSTimer *pTimer)
{
	s32 ret = VERROR_NO_ERROR;
	s32 notify = 0;//֪ͨ��������ִ���µ���ʱ

	VOS_TIMER_LOCK();

	if (pTimer->status != VOS_TIMER_STA_STOPPED) {
		ret = VERROR_TIMER_RUNNING;
		goto END_TIMER_START;
	}
	//����ͣ�б���ɾ��
	list_del(&pTimer->list);
	//��ӵ������б�
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

	if (notify) {//�и�С�����������֪ͨ����ִ���µ���ʱ
		VOSTimerSemPost();
	}
	return ret;
}

/********************************************************************************************************
* ������s32 VOSTimerStop(StVOSTimer *pTimer);
* ����: ��ʱ����ͣ
* ����:
* [1] pTimer: ָ����ʱ��ָ��
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSTimerStop(StVOSTimer *pTimer)
{
	s32 ret = VERROR_NO_ERROR;
	VOS_TIMER_LOCK();
	if (pTimer->status != VOS_TIMER_STA_RUNNING) {
		ret = VERROR_TIMER_RUNNING;
		goto END_TIMER_STOP;
	}
	//�������б���ɾ��
	list_del(&pTimer->list);
	//��ӵ���ͣ�б�
	pTimer->status = VOS_TIMER_STA_STOPPED;
	list_add_tail(&pTimer->list, &gListTimerStop);

END_TIMER_STOP:
	VOS_TIMER_UNLOCK();
	return ret;

}

/********************************************************************************************************
* ������s32 VOSTimerGetStatus(StVOSTimer *pTimer);
* ����: ��ȡ��ʱ��״̬
* ����:
* [1] pTimer: ָ����ʱ��ָ��
* ���أ���ʱ��״̬
* ע�⣺��
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
* ������s32 VOSTimerGetLeftTime(StVOSTimer *pTimer);
* ����: ��ȡ�������ʣ��ʱ��
* ����:
* [1] pTimer: ָ����ʱ��ָ��
* ���أ���ʱ��ʣ���ʱ�䣬��λ����
* ע�⣺��
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
* ������void VOSTaskTimer(void *param);
* ����: ��ʱ������ʵ�ֶඨʱ��ִ�в���
* ����:
* [1] param: ��ʱû��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTaskTimer(void *param)
{
	s32 ret = 0;
	struct list_head *list_timer;
	struct list_head *list_temp;
	StVOSTimer *pTimer;
	while(gSemVosTimer) {
		if (gVosTimerMarkNearest > gVOSTicks) {
			ret = VOSSemWait(gSemVosTimer, gVosTimerMarkNearest-gVOSTicks);//��С����ʱ��������źŷ����ͻ���,ע������Ҫ��ȥ��ǰ��ʱ�������ʱ�ĳ���
		}
		VOS_TIMER_LOCK();

		gVosTimerMarkNearest = TIMEOUT_INFINITY_U32; //�����������������ٵ���ʱ��Ȼ����ʱ����
		list_for_each_safe(list_timer, list_temp, &gListTimerRunning) {
			pTimer = list_entry(list_timer, struct StVOSTimer, list);
			//gVOSTicks >= pTimer->ticks_alert
			if (TICK_CMP(gVOSTicks,pTimer->ticks_alert,pTimer->ticks_start) >= 0) { //���ӵ���ִ���������
				if (pTimer->callback) pTimer->callback(pTimer, pTimer->arg);
				if (pTimer->type == VOS_TIMER_TYPE_ONE_SHOT) {
					//ɾ���Լ�
					list_del(&pTimer->list);
					//��ӵ���ͣ����
					pTimer->status = VOS_TIMER_STA_STOPPED;
					list_add_tail(&pTimer->list, &gListTimerStop);
				}
				else if (pTimer->type == VOS_TIMER_TYPE_PERIODIC){//��������ʱ�䣬���¼�ʱ
					pTimer->ticks_start = gVOSTicks;
					pTimer->ticks_alert = gVOSTicks + pTimer->ticks_delay;
				}
				else {

				}
			}
			//����Ҫ�ҵ����������
			if (pTimer->status == VOS_TIMER_STA_RUNNING &&
					TICK_CMP(gVosTimerMarkNearest,pTimer->ticks_alert,pTimer->ticks_start) > 0) { //pTimer->ticks_alert < gVosTimerMarkNearest
				gVosTimerMarkNearest = pTimer->ticks_alert;
			}
		}//ended ist_for_each(list_timer, &gListTimerRunning) {
		VOS_TIMER_UNLOCK();
	}
}

/********************************************************************************************************
* ������void VOSTimerInit();
* ����: ��ʱ����ʼ��������ִ�ж����ʱ���������ĳ����������Ӱ�쵽����Ķ�ʱ����
* ����: ��
* ���أ���
* ע�⣺��
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

	gSemVosTimer = VOSSemCreate(1, 1, "SemVosTimer");//����һ���ź���������ʱ�ͻ��Ѵ���

	s32 task_id = VOSTaskCreate(VOSTaskTimer, 0, task_timer_stack, sizeof(task_timer_stack), VOS_TASK_TIMER_PRIO, "vtimer");

}

