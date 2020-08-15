//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "vconf.h"
#include "vos.h"
#include "list.h"



#ifndef MAX_VOS_SEMAPHONRE_NUM
#define MAX_VOS_SEMAPHONRE_NUM  2
#endif

#ifndef MAX_VOS_MUTEX_NUM
#define MAX_VOS_MUTEX_NUM   2
#endif

#ifndef MAX_VOS_MSG_QUE_NUM
#define MAX_VOS_MSG_QUE_NUM   2
#endif

extern struct StVosTask *pRunningTask;
extern volatile s64  gVOSTicks;
extern volatile s64 gMarkTicksNearest;

static struct list_head gListSemaphore;//�����ź�������

static struct list_head gListMutex;//���л���������

static struct list_head gListMsgQue;//������Ϣ��������

StVOSSemaphore gVOSSemaphore[MAX_VOS_SEMAPHONRE_NUM];

StVOSMutex gVOSMutex[MAX_VOS_MUTEX_NUM];

StVOSMsgQueue gVOSMsgQue[MAX_VOS_MSG_QUE_NUM];

extern volatile u32 VOSIntNesting;

void VOSSemInit()
{
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//��ʼ���ź�������
	INIT_LIST_HEAD(&gListSemaphore);
	//�������������ӵ������ź���������
	for (i=0; i<MAX_VOS_SEMAPHONRE_NUM; i++) {
		list_add_tail(&gVOSSemaphore[i].list, &gListSemaphore);
	}
	__local_irq_restore(irq_save);
}

StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name)
{
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_CREATE,
			.u32param0 = max_sems,
			.u32param1 = init_sems,
			.u32param2 = name,
	};
	vos_sys_call(&sa);
	return (StVOSSemaphore *)sa.u32param0; //return;
}
StVOSSemaphore *SysCallVOSSemCreate(StVosSysCallParam *psa)
{
	s32 max_sems = (s32)psa->u32param0;
	s32 init_sems = (s32)psa->u32param1;
	s8 *name = (s32)psa->u32param2;

	StVOSSemaphore *pSemaphore = 0;
	if (!list_empty(&gListSemaphore)) {
		pSemaphore = list_entry(gListSemaphore.next, StVOSSemaphore, list); //��ȡ��һ�������ź���
		list_del(gListSemaphore.next);
		pSemaphore->name = name;
		pSemaphore->max = max_sems;
		if (init_sems > max_sems) {
			init_sems = max_sems;
		}
		pSemaphore->left = init_sems; //��ʼ���ź���Ϊ��
		memset(pSemaphore->bitmap, 0, sizeof(pSemaphore->bitmap)); //���λͼ��λͼָ��ռ�ø��ź���������id.
	}
	return  pSemaphore;
}

s32 VOSSemTryWait(StVOSSemaphore *pSem)
{
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_TRY_WAIT,
			.u32param0 = (u32)pSem,
	};
	vos_sys_call(&sa);
	return (s32)sa.u32param0; //return;
}

s32 SysCallVOSSemTryWait(StVosSysCallParam *psa)
{
	StVOSSemaphore *pSem = (StVOSSemaphore *)psa->u32param0;
	s32 ret = -1;
	if (pSem->max == 0) return -1; //�ź������ܲ����ڻ��߱��ͷ�
	if (pSem->left > 0) {
		pSem->left--;
		ret = 1;
	}
	return ret;
}

s32 VOSSemWait(StVOSSemaphore *pSem, u64 timeout_ms)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_WAIT,
			.u32param0 = (u32)pSem,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret==0) { //û�ź�����������������
		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ����ź�������
		case VOS_WAKEUP_FROM_DELAY:
			ret = 0;
			break;
		case VOS_WAKEUP_FROM_SEM:
			ret = 1;
			break;
		case VOS_WAKEUP_FROM_SEM_DEL:
			ret = -1;
			break;
		default:
			ret = 0;
			break;
		}
	}
	return ret;
}

s32 SysCallVOSSemWait(StVosSysCallParam *psa)
{
	StVOSSemaphore *pSem = (StVOSSemaphore*)psa->u32param0;
	u64 timeout_ms = (u64)psa->u64param0;

	s32 ret = 0;

	if (pSem->max == 0) return -1; //�ź������ܲ����ڻ��߱��ͷ�
	if (VOSIntNesting != 0) return -1;

	if (pSem->left > 0) {
		pSem->left--;
		ret = 1;
	}
	else {//�ѵ�ǰ�����л�����������
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
		//�ź�����������
		pRunningTask->block_type |= VOS_BLOCK_SEMP;//�ź�������
		pRunningTask->psyn = pSem;
		//ͬʱ�ǳ�ʱʱ������
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
	}
	return ret;
}

s32 VOSSemRelease(StVOSSemaphore *pSem)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_RELEASE,
			.u32param0 = (u32)pSem,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}
s32 SysCallVOSSemRelease(StVosSysCallParam *psa)
{
	StVOSSemaphore *pSem = (StVOSSemaphore*)psa->u32param0;
	s32 ret = 0;
	u32 irq_save = 0;
	if (pSem->max == 0) return -1; //�ź������ܲ����ڻ��߱��ͷ�
	if (VOSIntNesting != 0) return -1;

	if (pSem->left < pSem->max) {
		pSem->left++; //�ͷ��ź���
		VOSTaskBlockWaveUp(); //���������������������ĵȴ����ź���������
		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
		//bitmap_clear(pRunningTask->id, pSem->bitmap); //��������ţ����ͬ�������λ�ȡ��ͬ�ź�������δ���
		ret = 1;
	}
	return ret;
}

s32 VOSSemDelete(StVOSSemaphore *pSem)
{

	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_DELETE,
			.u32param0 = (u32)pSem,
	};
	vos_sys_call(&sa);

	return (s32)sa.u32param0; //return;
}
s32 SysCallVOSSemDelete(StVosSysCallParam *psa)
{
	StVOSSemaphore *pSem = (StVOSSemaphore*)psa->u32param0;
	s32 ret = -1;
	u32 irq_save = 0;

	if (VOSIntNesting != 0) return -1;

	if (!list_empty(&gListSemaphore)) {

		if (pSem->max == pSem->left) {//�������񶼹黹�ź�������ʱ�����ͷš�
			list_del(pSem);
			list_add_tail(&pSem->list, &gListSemaphore);
			pSem->max = 0;
			pSem->name = 0;
			pSem->left = 0;
			//pSem->distory = 0;
			ret = 0;
		}
	}
	return ret;
}

void VOSMutexInit()
{
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//��ʼ���ź�������
	INIT_LIST_HEAD(&gListMutex);
	//�������������ӵ������ź���������
	for (i=0; i<MAX_VOS_MUTEX_NUM; i++) {
		list_add_tail(&gVOSMutex[i].list, &gListMutex);
		gVOSMutex[i].counter = -1; //��ʼ��Ϊ-1����ʾ��Ч
		gVOSMutex[i].ptask_owner = 0;
	}
	__local_irq_restore(irq_save);
}
//�����ǿ���������֮�ⴴ��
StVOSMutex *VOSMutexCreate(s8 *name)
{
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_CREAT,
			.u32param0 = (u32)name,
	};
	vos_sys_call(&sa);

	return (StVOSMutex *)sa.u32param0; //return;
}
//�����ǿ���������֮�ⴴ��
StVOSMutex *SysCallVOSMutexCreate(StVosSysCallParam *psa)
{
	s8 *name = (s8*)psa->u32param0;

	StVOSMutex *pMutex = 0;
	if (!list_empty(&gListMutex)) {
		pMutex = list_entry(gListMutex.next, StVOSMutex, list); //��ȡ��һ�����л�����
		list_del(gListMutex.next);
		pMutex->name = name;
		pMutex->counter = 1;
		pMutex->ptask_owner =  0;
	}
	return  pMutex;
}


s32 VOSMutexWait(StVOSMutex *pMutex, s64 timeout_ms)
{

	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_WAIT,
			.u32param0 = (u32)pMutex,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret==0) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = -1;
			break;
		case VOS_WAKEUP_FROM_MUTEX:
			ret = 1;
			break;
		case VOS_WAKEUP_FROM_MUTEX_DEL:
			ret = -1;
			break;
		default:
			ret = 0;
			//printf info here
			break;
		}
	}
	return ret;
}

s32 SysCallVOSMutexWait(StVosSysCallParam *psa)
{
	StVOSMutex *pMutex = (StVOSMutex *)psa->u32param0;
	s64 timeout_ms = (s64)psa->u64param0;

	s32 ret = 0;

	if (pRunningTask == 0) return -1; //��Ҫ���û�������ִ��
	if (pMutex->counter == -1) return -1; //�ź������ܲ����ڻ��߱��ͷ�

	if (VOSIntNesting != 0) return -1;

	if (pMutex->counter > 1) pMutex->counter = 1;

	if (pMutex->counter > 0) {
		pMutex->counter--;
		pMutex->ptask_owner = pRunningTask;
		//pRunningTask->psyn = pMutex; //ҲҪ����ָ����Դ��ָ�룬���ȼ���ת��Ҫ�õ�
		ret = 1;
	}
	else {//�ѵ�ǰ�����л�����������
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������

		//�ź�����������
		pRunningTask->block_type |= VOS_BLOCK_MUTEX;//����������
		pRunningTask->psyn = pMutex;
		//ͬʱ�ǳ�ʱʱ������
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		VOSTaskRaisePrioBeforeBlock(pMutex->ptask_owner); //����ǰ�������ȼ���ת���⣬����������������������ȼ�
	}
	return ret;
}


s32 VOSMutexRelease(StVOSMutex *pMutex)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_RELEASE,
			.u32param0 = (u32)pMutex,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}

s32 SysCallVOSMutexRelease(StVosSysCallParam *psa)
{
	StVOSMutex *pMutex = (StVOSMutex *)psa->u32param0;
	s32 ret = 0;
	if (pMutex->counter == -1) return -1; //���������ܲ����ڻ��߱��ͷ�
	if (pMutex->counter > 1) pMutex->counter = 1;

	if (VOSIntNesting != 0) return -1;

	if (pMutex->counter < 1 && pMutex->ptask_owner == pRunningTask) {//����������Ҫ��ӵ���߲����ͷ�
		pMutex->counter++; //�ͷŻ�����
		VOSTaskBlockWaveUp(); //���������������������ĵȴ��û�����������
		VOSTaskRestorePrioBeforeRelease();//�ָ���������ȼ�
		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
		pMutex->ptask_owner = 0; //���ӵ����
		ret = 1;
	}
	return ret;
}

s32 VOSMutexDelete(StVOSMutex *pMutex)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_DELETE,
			.u32param0 = (u32)pMutex,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	return ret;
}

s32 SysCallVOSMutexDelete(StVosSysCallParam *psa)
{
	StVOSMutex *pMutex = (StVOSMutex *)psa->u32param0;
	s32 ret = -1;
	if (VOSIntNesting != 0) return -1;

	if (!list_empty(&gListSemaphore)) {
		if (pMutex->ptask_owner == 0 && pMutex->counter == 1) {//������û�����ã�ֱ���ͷ�
			//ɾ���Լ�
			list_del(pMutex);
			list_add_tail(&pMutex->list, &gListMutex);
			pMutex->name = 0;
			pMutex->counter = -1;
			ret = 0;
		}
	}
	return ret;
}


s32 VOSEventWait(u32 event_mask, u64 timeout_ms)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_WAIT,
			.u32param0 = (u32)event_mask,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	if (ret==0) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = -1;
			break;
		case VOS_WAKEUP_FROM_EVENT:
			ret = 1;
			break;
		default:
			ret = -1;
			//printf info here
			break;
		}
	}
	return ret;
}

s32 SysCallVOSEventWait(StVosSysCallParam *psa)
{
	u32 event_mask = (u32)psa->u32param0;
	u64 timeout_ms = (u64)psa->u64param0;
	s32 ret = 0;

	if (VOSIntNesting != 0) return -1;

	//�ѵ�ǰ�����л�����������
	pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������

	//�ź�����������
	pRunningTask->block_type |= VOS_WAKEUP_FROM_EVENT;//�¼�����
	pRunningTask->psyn = 0;
	pRunningTask->event_mask = event_mask;
	//ͬʱ�ǳ�ʱʱ������
	pRunningTask->ticks_start = gVOSTicks;
	pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
	if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
		gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
	}
	pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

	return ret;
}


s32 VOSEventSet(s32 task_id, u32 event)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_SET,
			.u32param0 = (u32)task_id,
			.u32param1 = (u32)event,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}

s32 SysCallVOSEventSet(StVosSysCallParam *psa)
{
	s32 task_id = (s32)psa->u32param0;
	u32 event = (u32)psa->u32param1;
	s32 ret = -1;

	if (VOSIntNesting != 0) return -1;

	StVosTask *pTask = VOSGetTaskFromId(task_id);
	if (pTask) {
		pTask->event = event;
		VOSTaskBlockWaveUp(); //���������������������ĵȴ����¼�������
		ret = 1;
	}
	return ret;
}


u32 VOSEventGet(s32 task_id)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_GET,
			.u32param0 = (u32)task_id,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	return ret;
}

u32 SysCallVOSEventGet(StVosSysCallParam *psa)
{
	s32 task_id = (s32)psa->u32param0;
	u32 event_mask = 0;

	if (VOSIntNesting != 0) return -1;
	StVosTask *pTask = VOSGetTaskFromId(task_id);
	if (pTask) {
		event_mask = pTask->event_mask;
	}
	return event_mask;
}


s32 VOSEventClear(s32 task_id, u32 event)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_CLEAR,
			.u32param0 = (u32)task_id,
			.u32param1 = (u32)event,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	return ret;
}

s32 SysCallVOSEventClear(StVosSysCallParam *psa)
{
	s32 task_id = (s32)psa->u32param0;
	u32 event = (u32)psa->u32param1;
	u32 mask = 0;
	if (VOSIntNesting != 0) return -1;

	StVosTask *pTask = VOSGetTaskFromId(task_id);
	if (pTask) {
		pTask->event_mask &= (~event);
		mask = pTask->event_mask;
	}
	return mask;
}



void VOSMsgQueInit()
{
	s32 i = 0;
	u32 irq_save = 0;

	irq_save = __local_irq_save();
	//��ʼ���ź�������
	INIT_LIST_HEAD(&gListMsgQue);
	//�������������ӵ������ź���������
	for (i=0; i<MAX_VOS_MSG_QUE_NUM; i++) {
		list_add_tail(&gVOSMsgQue[i].list, &gListMsgQue);
	}
	__local_irq_restore(irq_save);
}

StVOSMsgQueue *VOSMsgQueCreate(s8 *pRingBuf, s32 length, s32 msg_size, s8 *name)
{
	StVOSMsgQueue * ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_CREAT,
			.u32param0 = (u32)pRingBuf,
			.u32param1 = (u32)length,
			.u32param2 = (u32)msg_size,
			.u32param3 = (u32)name,
	};
	vos_sys_call(&sa);

	ret = (StVOSMsgQueue *)sa.u32param0; //return;
	return ret;
}

StVOSMsgQueue *SysCallVOSMsgQueCreate(StVosSysCallParam *psa)
{
	s8 *pRingBuf = (s8*)psa->u32param0;
	s32 length = (s32)psa->u32param1;
	s32 msg_size = (s32)psa->u32param2;
	s8 *name = (s8*)psa->u32param3;

	StVOSMsgQueue *pMsgQue = 0;

	if (VOSIntNesting != 0) return -1;

	if (!list_empty(&gListMsgQue)) {
		pMsgQue = list_entry(gListMsgQue.next, StVOSMsgQueue, list);
		list_del(gListMsgQue.next);
		pMsgQue->name = name;
		pMsgQue->pdata = pRingBuf;
		pMsgQue->length = length;
		pMsgQue->pos_head = 0;
		pMsgQue->pos_tail = 0;
		pMsgQue->msg_cnts = 0;
		pMsgQue->msg_maxs = length/msg_size;
		pMsgQue->msg_size = msg_size;
	}
	return  pMsgQue;
}

//������ӵĸ������ɹ�����1����ʾ���һ����Ϣ�ɹ����������0����ʾ��������
s32 VOSMsgQuePut(StVOSMsgQueue *pMQ, void *pmsg, s32 len)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_PUT,
			.u32param0 = (u32)pMQ,
			.u32param1 = (u32)pmsg,
			.u32param2 = (u32)len,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}

s32 SysCallVOSMsgQuePut(StVosSysCallParam *psa)
{
	StVOSMsgQueue *pMQ = (StVOSMsgQueue *)psa->u32param0;
	void *pmsg = psa->u32param1;
	s32 len = (s32)psa->u32param2;

	s32 ret = 0;
	u8 *ptail = 0;

	if (pMQ->pos_tail != pMQ->pos_head || //ͷ������β�������������Ϣ
		pMQ->msg_cnts == 0) { //����Ϊ�գ������������Ϣ
		ptail = pMQ->pdata + pMQ->pos_tail * pMQ->msg_size;
		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
		memcpy(ptail, pmsg, len);
		pMQ->pos_tail++;
		pMQ->pos_tail = pMQ->pos_tail % pMQ->msg_maxs;
		pMQ->msg_cnts++;
		VOSTaskBlockWaveUp(); //���������������������ĵȴ����ź���������
		//VOSTaskRestorePrioBeforeRelease(pRunningTask);//�ָ���������ȼ�
		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
		ret = 1;
	}
	return ret;
}

//������ӵĸ���
s32 VOSMsgQueGet(StVOSMsgQueue *pMQ, void *pmsg, s32 len, s64 timeout_ms)
{
	u8 *phead = 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_GET,
			.u32param0 = (u32)pMQ,
			.u32param1 = (u32)pmsg,
			.u32param2 = (u32)len,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret==0) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = -1;
			break;
		case VOS_WAKEUP_FROM_MSGQUE: //����������Ϣ����ȡ��Ϣ
			phead = pMQ->pdata + pMQ->pos_head * pMQ->msg_size;
			len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
			memcpy(pmsg, phead, len);
			pMQ->pos_head++;
			pMQ->pos_head = pMQ->pos_head % pMQ->msg_maxs;
			pMQ->msg_cnts--;
			ret = 1;
			break;
		case VOS_WAKEUP_FROM_MSGQUE_DEL:
			ret = -1;
			break;
		default:
			ret = 0;
			//printf info here
			break;
		}
	}

	return ret;
}

s32 SysCallVOSMsgQueGet(StVosSysCallParam *psa)
{
	StVOSMsgQueue *pMQ = (StVOSMsgQueue *)psa->u32param0;
	void *pmsg = (void*)psa->u32param1;
	s32 len = (s32)psa->u32param2;
	s64 timeout_ms = (s64)psa->u64param0;
	s32 ret = 0;
	u8 *phead = 0;

	if (VOSIntNesting != 0) return -1;

	if (pMQ->msg_cnts > 0) {//����Ϣ
		phead = pMQ->pdata + pMQ->pos_head * pMQ->msg_size;
		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
		memcpy(pmsg, phead, len);
		pMQ->pos_head++;
		pMQ->pos_head = pMQ->pos_head % pMQ->msg_maxs;
		pMQ->msg_cnts--;

		//pRunningTask->psyn = pMQ; //ҲҪ����ָ����Դ��ָ�룬���ȼ���ת��Ҫ�õ�
		ret = 1;
	}
	else {//û��Ϣ�������������
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������

		//�ź�����������
		pRunningTask->block_type |= VOS_BLOCK_MSGQUE;//��Ϣ��������
		pRunningTask->psyn = pMQ;
		//ͬʱ�ǳ�ʱʱ������
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		//VOSTaskRaisePrioBeforeBlock(pRunningTask); //����ǰ�������ȼ���ת���⣬����������������������ȼ�
	}
	return ret;
}


s32 VOSMsgQueFree(StVOSMsgQueue *pMQ)
{
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_GET,
			.u32param0 = (u32)pMQ,
	};
	vos_sys_call(&sa);
	ret = (s32)sa.u32param0; //return;
	return ret;
}

s32 SysCallVOSMsgQueFree(StVosSysCallParam *psa)
{
	StVOSMsgQueue *pMQ = (StVOSMsgQueue *)psa->u32param0;
	s32 ret = -1;

	if (!list_empty(&gListMsgQue)) {

		if (pMQ->msg_cnts == 0) {//��ϢΪ�գ��ͷ�
//			//�����Ϣ���У��Ƿ���Ҫ�Ѿ�������������еȴ����ź���������������ӵ���������
//			pMQ->distory = 1;
//			VOSTaskBlockWaveUp(); //���������������������ĵȴ����ź���������

			list_del(pMQ);
			list_add_tail(&pMQ->list, &gListMsgQue);
			pMQ->distory = 0;
			pMQ->name = 0;
			pMQ->pdata = 0;
			pMQ->length = 0;
			pMQ->pos_head = 0;
			pMQ->pos_tail = 0;
			pMQ->msg_cnts = 0;
			pMQ->msg_maxs = 0;
			pMQ->msg_size = 0;
			ret = 0;
		}
	}
	return ret;
}
/* ��ʱ�������ܷŵ����Ч��̫�� */
//u32 VOSTaskDelay(u32 ms)
//{
//	u32 ret = 0;
//	StVosSysCallParam sa = {
//			.call_num = VOS_SYSCALL_TASK_DELAY,
//			.u32param0 = (u32)ms,
//	};
//	vos_sys_call(&sa);
//	ret = (u32)sa.u32param0; //return;
//	if (ret == 0) VOSTaskSchedule();
//	return ret;
//}
//
//u32 SysCallVOSTaskDelay(StVosSysCallParam *psa)
//{
//	u32 ms = (u32)psa->u32param0;
//	if (ms) {//������ʱ����
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
//			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
//		}
//		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
//	}
//	return ms;
//}
u32 VOSTaskDelay(u32 ms)
{
	//����жϱ��رգ�ϵͳ��������ȣ���ֱ��Ӳ��ʱ
	//todo
	//����������ϵͳ��������ʱ
	if (ms) {//������ʱ����
		__asm volatile ("svc 3\n"); //VOS_SVC_NUM_DELAY
	}
	//msΪ0���л�����, ����VOS_STA_BLOCK_DELAY��־����������
	VOSTaskSchedule();
	return 0;
}


#if 0 //λͼ����
#define MAX_TASK_NUM 10;
u8 bitmap[10];
int bitmap_prinf(u8 *bitmap, s32 num)
{
	s32 line_num = 0;
	u32 n = 0;
	bitmap_for_each(n, num)
	{
		//printf("%d ", bitmap_get(n, bitmap) ? 1 : 0);
		printf("%d ", bitmap_get(n, bitmap));
		line_num++;
		if (line_num % 8 == 0) printf("\r\n");
	}
	printf("\r\n");
}

int bitmap_test() {
	u32 n = 0;
	memset(bitmap, 0, sizeof(bitmap));
	bitmap_set(2, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_clear(2, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(7, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(8, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(15, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(16, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));

#if 0
	bitmap_for_each(n, bitmap) {
		if (bitmap_check(n, bitmap)) {

		}
	}
#endif
}
#endif

