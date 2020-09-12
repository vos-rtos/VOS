/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: ipc.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/

#include "vconf.h"
#include "vos.h"
#include "vlist.h"

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
extern volatile u32  gVOSTicks;
extern volatile u32 gMarkTicksNearest;

static struct list_head gListSemaphore;//�����ź�������

static struct list_head gListMutex;//���л���������

static struct list_head gListMsgQue;//������Ϣ��������

StVOSSemaphore gVOSSemaphore[MAX_VOS_SEMAPHONRE_NUM];

StVOSMutex gVOSMutex[MAX_VOS_MUTEX_NUM];

StVOSMsgQueue gVOSMsgQue[MAX_VOS_MSG_QUE_NUM];

extern volatile u32 VOSIntNesting;
extern volatile u32 VOSRunning;

/********************************************************************************************************
* ������void VOSSemInit();
* ����: �ź������ϳ�ʼ��
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSSemInit()
{
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	//��ʼ���ź�������
	INIT_LIST_HEAD(&gListSemaphore);
	//�������������ӵ������ź���������
	for (i=0; i<MAX_VOS_SEMAPHONRE_NUM; i++) {
		list_add_tail(&gVOSSemaphore[i].list, &gListSemaphore);
	}
	__vos_irq_restore(irq_save);
}


/********************************************************************************************************
* ������StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name);
* ����: �����ź���
* ����:
* [1] max_sems: �ź�����Դ���ֵ�������źŵ�
* [2] init_sems: ��ʼ���ź�����Ŀǰ�ж��ٸ�����
* [3] name: �ź�����
* ���أ�NULL��ʾ����ʧ�ܣ���0��ʾ�����ɹ�
* ע�⣺��
*********************************************************************************************************/
StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name)
{
	StVOSSemaphore *pSemaphore = 0;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	if (!list_empty(&gListSemaphore)) {
		pSemaphore = list_entry(gListSemaphore.next, StVOSSemaphore, list); //��ȡ��һ�������ź���
		list_del(gListSemaphore.next);
		pSemaphore->name = name;
		pSemaphore->max = max_sems;
		if (init_sems > max_sems) {
			init_sems = max_sems;
		}
		pSemaphore->left = init_sems; //��ʼ���ź���Ϊ��
		INIT_LIST_HEAD(&pSemaphore->list_task);
		memset(pSemaphore->bitmap, 0, sizeof(pSemaphore->bitmap)); //���λͼ��λͼָ��ռ�ø��ź���������id.
	}
	__vos_irq_restore(irq_save);
	return  pSemaphore;
}

/********************************************************************************************************
* ������s32 VOSSemTryWait(StVOSSemaphore *pSem);
* ����: ���Ի�ȡĳ���ź������������VERROR_NO_ERROR,���Ѿ���ȡ���ź������������صȴ�VERROR_WAIT_RESOURCES
* ����:
* [1] pSem���ź���ָ��
* ���أ���ȡ�ɹ�����VERROR_NO_ERROR�� ��ȡʧ�ܷ���VERROR_WAIT_RESOURCES���������ء�
* ע�⣺��
*********************************************************************************************************/
s32 VOSSemTryWait(StVOSSemaphore *pSem)
{
	s32 ret = VERROR_NO_RESOURCES;
	u32 irq_save = 0;

	if (VOSRunning == 0) return VERROR_NO_SCHEDULE;
	if (pSem==0) return VERROR_PARAMETER; //�ź������ܲ����ڻ��߱��ͷ�

	irq_save = __vos_irq_save();

	if (pSem->left > 0) {
		pSem->left--;
		ret = VERROR_NO_ERROR;
	}
	__vos_irq_restore(irq_save);
	return ret;
}

/********************************************************************************************************
* ������s32 VOSSemWait(StVOSSemaphore *pSem, u32 timeout_ms);
* ����: ��ȡĳ���ź��������ָ���ź���û�������ͽ������״̬��������������
* ����:
* [1] pSem���ź���ָ��
* [2] timeout_ms����ʱʱ�䣬��λ����
* ���أ��鿴����ֵ��
* ע�⣺��
*********************************************************************************************************/
s32 VOSSemWait(StVOSSemaphore *pSem, u32 timeout_ms)
{
	s32 ret = VERROR_NO_ERROR;
	s32 need_sche = 0;
	u32 irq_save = 0;

	if (VOSRunning == 0) return VERROR_NO_SCHEDULE;
	if (pSem==0) return VERROR_PARAMETER; //�ź������ܲ����ڻ��߱��ͷ�

	irq_save = __vos_irq_save();

	if (pSem->left > 0) {
		pSem->left--;
	}
	else if (VOSIntNesting==0) {
		//�ѵ�ǰ�����л�����������
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������

		//�ź�����������
		pRunningTask->block_type |= VOS_BLOCK_SEMP;//�ź�������
		pRunningTask->psyn = pSem;
		//ͬʱ�ǳ�ʱʱ������
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
		//gMarkTicksNearest > pRunningTask->ticks_alert ����Ҫ����ʱ��
		if (TICK_CMP(gMarkTicksNearest,pRunningTask->ticks_alert,pRunningTask->ticks_start)>0) {
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		//���뵽��ʱ���У�Ҳͬʱ���뵽��������
		VOSTaskDelayListInsert(pRunningTask);
		VOSTaskBlockListInsert(pRunningTask, &pSem->list_task);

		need_sche = 1;

	}
	if (VOSIntNesting) {//���ж���������,ֱ�ӷ���-1
		ret = VERROR_INT_CORTEX;
	}
	__vos_irq_restore(irq_save);

	if (need_sche) { //û�ź�����������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ����ź�������
		case VOS_WAKEUP_FROM_DELAY:
			ret = VERROR_TIMEOUT;
			break;
		case VOS_WAKEUP_FROM_SEM:
			ret = VERROR_NO_ERROR;
			break;
		case VOS_WAKEUP_FROM_SEM_DEL:
			ret = VERROR_DELETE_RESOURCES;
			break;
		default:
			ret = VERROR_UNKNOWN;
			break;
		}
	}
	return ret;
}

/********************************************************************************************************
* ������s32 VOSSemRelease(StVOSSemaphore *pSem);
* ����: �ͷ�ĳ���ź���
* ����:
* [1] pSem���ź���ָ��
* ���أ��鿴����ֵ��
* ע�⣺��
*********************************************************************************************************/
s32 VOSSemRelease(StVOSSemaphore *pSem)
{
	s32 ret = VERROR_NO_ERROR;
	s32 need_sche = 0;
	struct StVosTask *pBlockTask = 0;
	u32 irq_save = 0;

	if (VOSRunning == 0) return VERROR_NO_SCHEDULE;
	if (pSem == 0) return VERROR_PARAMETER; //�ź������ܲ����ڻ��߱��ͷ�

	irq_save = __vos_irq_save();

	if (pSem->left < pSem->max) {
		if (!list_empty(&pSem->list_task)) {
			pBlockTask = list_entry(pSem->list_task.next, struct StVosTask, list);
			VOSTaskBlockListRelease(pBlockTask);//���������������������ĵȴ����ź���������,ÿ�ζ��������ȼ���ߵ��Ǹ�
			//�޸�״̬
			pBlockTask->status = VOS_STA_READY;
			pBlockTask->block_type = 0;
			pBlockTask->ticks_start = 0;
			pBlockTask->ticks_alert = 0;
			pBlockTask->psyn = 0;
			if (pSem->distory == 0) {
				pBlockTask->wakeup_from = VOS_WAKEUP_FROM_SEM;
			}
			else {
				pBlockTask->wakeup_from = VOS_WAKEUP_FROM_SEM_DEL;
			}
			need_sche = 1; //��Ҫ
		}
		else {//û������������ֱ�ӼӼ��ź���
			pSem->left++; //�ͷ��ź���
		}

		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
		ret = VERROR_NO_ERROR;
	}
	__vos_irq_restore(irq_save);
	if (VOSIntNesting == 0 && need_sche) { //������ж������ĵ���(VOSIntNesting!=0),������������
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}

/********************************************************************************************************
* ������s32 VOSSemDelete(StVOSSemaphore *pSem);
* ����: ɾ��ĳ���ź���
* ����:
* [1] pSem���ź���ָ��
* ���أ��鿴����ֵ��
* ע�⣺��
*********************************************************************************************************/
s32 VOSSemDelete(StVOSSemaphore *pSem)
{
	s32 ret = VERROR_NO_ERROR;
	u32 irq_save = 0;

	if (pSem==0) return VERROR_PARAMETER; //�ź������ܲ����ڻ��߱��ͷ�

	irq_save = __vos_irq_save();
	if (!list_empty(&gListSemaphore)) {

		if (pSem->max == pSem->left) {//�������񶼹黹�ź�������ʱ�����ͷš�
			list_del(pSem);
			list_add_tail(&pSem->list, &gListSemaphore);
			pSem->max = 0;
			pSem->name = 0;
			pSem->left = 0;
			//pSem->distory = 0;
			ret = VERROR_NO_ERROR;
		}
	}
	else {
		ret = VERROR_DELETE_RESOURCES;
	}
	__vos_irq_restore(irq_save);
	return ret;
}

/********************************************************************************************************
* ������void VOSMutexInit();
* ����: ��ʼ��������
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSMutexInit()
{
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	//��ʼ���ź�������
	INIT_LIST_HEAD(&gListMutex);
	//�������������ӵ������ź���������
	for (i=0; i<MAX_VOS_MUTEX_NUM; i++) {
		list_add_tail(&gVOSMutex[i].list, &gListMutex);
		gVOSMutex[i].counter = -1; //��ʼ��Ϊ-1����ʾ��Ч
		gVOSMutex[i].ptask_owner = 0;
	}
	__vos_irq_restore(irq_save);
}

/********************************************************************************************************
* ������StVOSMutex *VOSMutexCreate(s8 *name);
* ����: ����������
* ����:
* [1] name����������
* ���أ�������ָ��
* ע�⣺��
*********************************************************************************************************/
StVOSMutex *VOSMutexCreate(s8 *name)
{
	StVOSMutex *pMutex = 0;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	if (!list_empty(&gListMutex)) {
		pMutex = list_entry(gListMutex.next, StVOSMutex, list); //��ȡ��һ�����л�����
		list_del(gListMutex.next);
		pMutex->name = name;
		pMutex->counter = 1;
		pMutex->ptask_owner =  0;
		INIT_LIST_HEAD(&pMutex->list_task);
	}
	__vos_irq_restore(irq_save);
	return  pMutex;
}

/********************************************************************************************************
* ������s32 VOSMutexWait(StVOSMutex *pMutex, u32 timeout_ms);
* ����: �������ȴ������û�����������أ�����ȴ����������ͷź���ܷ��سɹ�
* ����:
* [1] pMutex��������ָ��
* [2] timeout_ms: ��ʱʱ�䣬��λ����
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSMutexWait(StVOSMutex *pMutex, u32 timeout_ms)
{
	s32 ret = VERROR_NO_ERROR;
	s32 need_sche = 0;
	u32 irq_save = 0;


	if (VOSIntNesting != 0) return VERROR_INT_CORTEX;

	if (VOSRunning == 0) return VERROR_NO_SCHEDULE;

	if (pMutex->counter > 1) pMutex->counter = 1;

	irq_save = __vos_irq_save();

	if (pMutex->counter > 0) {
		pMutex->counter--;
		pMutex->ptask_owner = pRunningTask;
		ret = VERROR_NO_ERROR;
	}
	else {//�ѵ�ǰ�����л�����������
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������

		//�ź�����������
		pRunningTask->block_type |= VOS_BLOCK_MUTEX;//����������
		pRunningTask->psyn = pMutex;
		//ͬʱ�ǳ�ʱʱ������
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);

		//gMarkTicksNearest > pRunningTask->ticks_alert ����Ҫ����ʱ��
		if (TICK_CMP(gMarkTicksNearest,pRunningTask->ticks_alert,pRunningTask->ticks_start) > 0) {
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		VOSTaskRaisePrioBeforeBlock(pMutex); //����ǰ�������ȼ���ת���⣬����������������������ȼ�
		//���뵽��ʱ���У�Ҳͬʱ���뵽��������
		VOSTaskDelayListInsert(pRunningTask);
		VOSTaskBlockListInsert(pRunningTask, &pMutex->list_task);
		need_sche = 1;
	}

	__vos_irq_restore(irq_save);

	if (need_sche) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = VERROR_TIMEOUT;
			break;
		case VOS_WAKEUP_FROM_MUTEX:
			ret = VERROR_NO_ERROR;
			break;
		case VOS_WAKEUP_FROM_MUTEX_DEL:
			ret = VERROR_DELETE_RESOURCES;
			break;
		default:
			ret = VERROR_UNKNOWN;
			break;
		}
	}
	return ret;
}

/********************************************************************************************************
* ������s32 VOSMutexRelease(StVOSMutex *pMutex);
* ����: �������ͷ�
* ����:
* [1] pMutex��������ָ��
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSMutexRelease(StVOSMutex *pMutex)
{
	s32 ret = 0;
	s32 need_sche = 0;
	struct StVosTask *pBlockTask = 0;

	u32 irq_save = 0;

	if (VOSIntNesting != 0) return VERROR_INT_CORTEX;
	if (VOSRunning == 0) return VERROR_NO_SCHEDULE;
	if (pMutex == 0) return VERROR_PARAMETER;

	if (pMutex->counter < 0) return VERROR_UNKNOWN; //���������ܲ����ڻ��߱��ͷ�

	if (pMutex->counter > 1) pMutex->counter = 1;

	irq_save = __vos_irq_save();

	if (pMutex->ptask_owner != pRunningTask)
		ret = VERROR_MUTEX_RELEASE;

	if (pMutex->counter < 1 && pMutex->ptask_owner == pRunningTask) {//����������Ҫ��ӵ���߲����ͷ�
		if (!list_empty(&pMutex->list_task)) {
			pBlockTask = list_entry(pMutex->list_task.next, struct StVosTask, list);
			pMutex->ptask_owner = pBlockTask;
			VOSTaskBlockListRelease(pBlockTask);//���ѵ�һ������
			//�޸�״̬
			pBlockTask->status = VOS_STA_READY;
			pBlockTask->block_type = 0;
			pBlockTask->ticks_start = 0;
			pBlockTask->ticks_alert = 0;
			if (pMutex->distory == 0) {
				pBlockTask->wakeup_from = VOS_WAKEUP_FROM_MUTEX;
			}
			else {
				pBlockTask->wakeup_from = VOS_WAKEUP_FROM_MUTEX_DEL;
			}
			need_sche = 1;
		}
		else {//�ͷŻ�����, û�ȴ���������ֱ���ͷ�
			pMutex->counter++;
		}
		VOSTaskRestorePrioBeforeRelease();//�ָ���������ȼ�
		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
		pMutex->ptask_owner = 0; //���ӵ����
		ret = VERROR_NO_ERROR;
	}

	__vos_irq_restore(irq_save);
	if (need_sche) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}

/********************************************************************************************************
* ������s32 VOSMutexDelete(StVOSMutex *pMutex);
* ����: ������ɾ��
* ����:
* [1] pMutex��������ָ��
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSMutexDelete(StVOSMutex *pMutex)
{
	s32 ret = VERROR_NO_ERROR;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	if (!list_empty(&gListSemaphore)) {

		if (pMutex->ptask_owner == 0 && pMutex->counter == 1) {//������û�����ã�ֱ���ͷ�
			//ɾ���Լ�
			list_del(pMutex);
			list_add_tail(&pMutex->list, &gListMutex);
			pMutex->name = 0;
			pMutex->counter = -1;
			//pMutex->distory = 0;
			ret = 0;
		}
	}
	else {
		ret = VERROR_DELETE_RESOURCES;
	}
	__vos_irq_restore(irq_save);
	return ret;
}

/********************************************************************************************************
* ������s32 VOSEventWait(u32 event, u32 timeout_ms);
* ����: �¼��ȴ�
* ����:
* [1] event��32λ�¼������֧��32���¼����ͣ��û��Զ���
* [2] timeout_ms: ��ʱʱ�䣬��λ����
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSEventWait(u32 event, u32 timeout_ms)
{
	s32 ret = 0;
	s32 need_sche = 0;
	u32 irq_save = 0;

	if (VOSIntNesting != 0) return VERROR_INT_CORTEX;
	if (VOSRunning == 0) return VERROR_NO_SCHEDULE;

	irq_save = __vos_irq_save();

	if (event & pRunningTask->event_mask) {
		pRunningTask->event_mask &= (~event); //�¼�match����ֱ���˳�
	}
	else {
		//�ѵ�ǰ�����л�����������
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������

		//�ź�����������
		pRunningTask->block_type |= VOS_BLOCK_EVENT;//�¼�����
		pRunningTask->psyn = 0;
		//pRunningTask->event_req = event; //��¼������¼�
		pRunningTask->event_mask |= event;
		//ͬʱ�ǳ�ʱʱ������
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
		//gMarkTicksNearest > pRunningTask->ticks_alert ����Ҫ����ʱ��
		if (TICK_CMP(gMarkTicksNearest,pRunningTask->ticks_alert,pRunningTask->ticks_start) > 0) {
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		//ֱ�ӹ�����ʱȫ���������Ѿ�ֱ����ȫ����ʱ���ѯ
		VOSTaskDelayListInsert(pRunningTask);
		need_sche = 1;
	}

	__vos_irq_restore(irq_save);

	if (need_sche) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = VERROR_TIMEOUT;
			break;
		case VOS_WAKEUP_FROM_EVENT:
			ret = VERROR_NO_ERROR;
			break;
		default:
			ret = VERROR_UNKNOWN;
			break;
		}
	}
	return ret;
}
/********************************************************************************************************
* ������s32 VOSEventSet(s32 task_id, u32 event);
* ����: �¼����ã��ỽ�ѵȴ����¼�����������
* ����:
* [1] task_id: ����ĳ�������������ID
* [2] event��32λ�¼������֧��32���¼����ͣ��û��Զ���
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSEventSet(s32 task_id, u32 event)
{
	s32 ret = -1;
	s32 need_sche = 0;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	StVosTask *pTask = VOSGetTaskFromId(task_id);
	if (pTask && pTask->block_type == (VOS_BLOCK_DELAY|VOS_BLOCK_EVENT)) {

		if ((pTask->event_stop & event) == 0 && //���ν���ָ���ź�
				pTask->event_mask & event) { //�������õ��¼���mask���ʹ�������

			VOSTaskBlockListRelease(pTask);//���������������������ĵȴ����ź���������,ÿ�ζ��������ȼ���ߵ��Ǹ�
			//�޸�״̬
			pTask->status = VOS_STA_READY;
			pTask->block_type = 0;
			pTask->ticks_start = 0;
			pTask->ticks_alert = 0;
			//pTask->event_req = 0;
			pTask->event_mask &= (~event);//����¼�λ�������������Ѿ���ȡ���¼�֪ͨ
			pTask->wakeup_from = VOS_WAKEUP_FROM_EVENT;
			need_sche = 1;
		}
		else {
			pTask->event_mask |= event;
		}
	}
	else {//���ø��¼�λ��Ч
		pTask->event_mask |= event;
	}
	__vos_irq_restore(irq_save);

	if (VOSIntNesting == 0 && need_sche) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}
/********************************************************************************************************
* ������u32 VOSEventGet(s32 task_id);
* ����: ��ȡĳ��������¼����롣
* ����:
* [1] task_id: ����ĳ�������������ID
* ���أ��¼�����
* ע�⣺��
*********************************************************************************************************/
u32 VOSEventGet(s32 task_id)
{
	u32 event_mask = 0;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	StVosTask *pTask = VOSGetTaskFromId(task_id);
	if (pTask) {
		event_mask = pTask->event_mask;
	}
	__vos_irq_restore(irq_save);

	return event_mask;
}

/********************************************************************************************************
* ������s32 VOSEventDisable(s32 task_id, u32 event);
* ����: ��ֹĳ�������ĳЩ�¼����ѡ�
* ����:
* [1] task_id: ����ĳ�������������ID
* [2] event��32λ�¼������֧��32���¼����ͣ��û��Զ���
* ���أ��¼���ֹλ
* ע�⣺��
*********************************************************************************************************/
s32 VOSEventDisable(s32 task_id, u32 event)
{
	u32 mask = 0;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	StVosTask *pTask = VOSGetTaskFromId(task_id);
	if (pTask) {
		pTask->event_stop |= event;
		mask = pTask->event_stop;
	}
	__vos_irq_restore(irq_save);

	return mask;
}

/********************************************************************************************************
* ������s32 VOSEventEnable(s32 task_id, u32 event);
* ����: ���¼���ĳ�������ĳЩ�¼����ѡ�
* ����:
* [1] task_id: ����ĳ�������������ID
* [2] event��32λ�¼������֧��32���¼����ͣ��û��Զ���
* ���أ��¼�����λ
* ע�⣺��
*********************************************************************************************************/
s32 VOSEventEnable(s32 task_id, u32 event)
{
	u32 mask = 0;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	StVosTask *pTask = VOSGetTaskFromId(task_id);
	if (pTask) {
		pTask->event_stop &= (~event);
		mask = pTask->event_stop;
	}
	__vos_irq_restore(irq_save);

	return mask;
}

/**************************************
 * ��Ϣ���к�����ֱ��ʹ���ź���ʵ��
 **************************************/

/********************************************************************************************************
* ������void VOSMsgQueInit();
* ����: ��Ϣ���г�ʼ����
* ����:
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSMsgQueInit()
{
	s32 i = 0;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	//��ʼ���ź�������
	INIT_LIST_HEAD(&gListMsgQue);
	//�������������ӵ������ź���������
	for (i=0; i<MAX_VOS_MSG_QUE_NUM; i++) {
		list_add_tail(&gVOSMsgQue[i].list, &gListMsgQue);
	}
	__vos_irq_restore(irq_save);
}
/********************************************************************************************************
* ������StVOSMsgQueue *VOSMsgQueCreate(s8 *pRingBuf, s32 length, s32 msg_size, s8 *name);
* ����: ��Ϣ���д���
* ����:
* [1] pRingBuf���û��ṩ�ڴ��ַ
* [2] length���û��ṩ���ڴ��С����λ�ֽ�
* [3] msg_size���û�ָ��ÿ����Ϣ��С
* [4] name����Ϣ������
* ���أ���Ϣ����ָ��
* ע�⣺��
*********************************************************************************************************/
StVOSMsgQueue *VOSMsgQueCreate(s8 *pRingBuf, s32 length, s32 msg_size, s8 *name)
{
	StVOSMsgQueue *pMsgQue = 0;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();

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
		pMsgQue->psem = VOSSemCreate(pMsgQue->msg_maxs, 0, "mq_vos");
	}
	__vos_irq_restore(irq_save);
	return  pMsgQue;
}

/********************************************************************************************************
* ������s32 VOSMsgQuePut(StVOSMsgQueue *pMQ, void *pmsg, s32 len);
* ����: ��һ����Ϣ�ŵ���Ϣ����
* ����:
* [1] pMQ����Ϣ����ָ��
* [2] pmsg��ÿ����Ϣָ��[N]���û��ṩ
* [3] len���û��ṩ����Ϣ��С
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSMsgQuePut(StVOSMsgQueue *pMQ, void *pmsg, s32 len)
{
	s32 ret = VERROR_NO_ERROR;
	u8 *ptail = 0;

	if (pMQ->psem == 0) return VERROR_PARAMETER;

	if (pMQ->pos_tail != pMQ->pos_head || //ͷ������β�������������Ϣ
		pMQ->msg_cnts == 0) { //����Ϊ�գ������������Ϣ
		ptail = pMQ->pdata + pMQ->pos_tail * pMQ->msg_size;
		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
		memcpy(ptail, pmsg, len);
		pMQ->pos_tail++;
		pMQ->pos_tail = pMQ->pos_tail % pMQ->msg_maxs;
		pMQ->msg_cnts++;

		VOSSemRelease(pMQ->psem);
	}
	else {
		ret = VERROR_FULL_RESOURCES;
	}
	return ret;
}
/********************************************************************************************************
* ������s32 VOSMsgQueGet(StVOSMsgQueue *pMQ, void *pmsg, s32 len, u32 timeout_ms);
* ����: ����Ϣ�������ȡһ����Ϣ��
* ����:
* [1] pMQ����Ϣ����ָ��
* [2] pmsg��ÿ����Ϣָ��[out]���û��ṩ
* [3] len���û��ṩ����Ϣ��С
* [4] timeout_ms����ʱʱ�䣬��λ����
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSMsgQueGet(StVOSMsgQueue *pMQ, void *pmsg, s32 len, u32 timeout_ms)
{
	s32 ret = VERROR_UNKNOWN;
	u8 *phead = 0;

	if (pMQ->psem == 0) return VERROR_PARAMETER;

	if (VERROR_NO_ERROR == (ret = VOSSemWait(pMQ->psem, timeout_ms))) {
		phead = pMQ->pdata + pMQ->pos_head * pMQ->msg_size;
		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
		memcpy(pmsg, phead, len);
		pMQ->pos_head++;
		pMQ->pos_head = pMQ->pos_head % pMQ->msg_maxs;
		pMQ->msg_cnts--;
	}
	return ret;
}
/********************************************************************************************************
* ������s32 VOSMsgQueFree(StVOSMsgQueue *pMQ);
* ����: ɾ��һ����Ϣ����
* ����:
* [1] pMQ����Ϣ����ָ��
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
s32 VOSMsgQueFree(StVOSMsgQueue *pMQ)
{
	s32 ret = VERROR_NO_ERROR;
	u32 irq_save = 0;

	irq_save = __vos_irq_save();
	if (pMQ->msg_cnts == 0) {//��ϢΪ�գ��ͷ�
		list_del(pMQ);
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
	else {
		ret = VERROR_DELETE_RESOURCES;
	}
	__vos_irq_restore(irq_save);
	return ret;
}

