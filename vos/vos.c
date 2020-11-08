/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vos.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828������ע��
*********************************************************************************************************/

#include "vtype.h"
#include "vos.h"
#include "vlist.h"
#include "vbitmap.h"


struct StVosTask gArrVosTask[MAX_VOSTASK_NUM]; //�������飬ռ�ÿռ�

struct list_head gListTaskDelay;//���ֶ�ʱ�������ӵ����������

struct list_head gListTaskFree;//�������񣬿��Լ����������

struct StVosTask *pRunningTask; //�������е�����

struct StVosTask *pReadyTask; //׼��Ҫ�л�������

volatile u32  gVOSTicks = 0;

volatile u32 gMarkTicksNearest = TIMEOUT_INFINITY_U32; //��¼���������

volatile u32 VOSIntNesting = 0;

volatile u32 VOSRunning = 0;

volatile u32 VOSCtxSwtFlag = 0; //�Ƿ��л������ģ������־��ִ�����������л�����0

volatile u32 vos_dis_irq_counter = 0;//��¼����irq disable����


long long stack_idle[1024];

u32 gArrPrioBitMap[MAX_VOS_TASK_PRIO_NUM/8/4]; //ÿbit��λ�������ȼ���ֵ
u8  gArrPrio2Taskid[MAX_VOS_TASK_PRIO_NUM]; //ͨ�����ȼ����ҵ�һ����ͬ���ȼ�������ͷ����

volatile u32 flag_cpu_collect = 0; //�Ƿ��ڲɼ�cpuռ����


/********************************************************************************************************
* ������StVosTask *tasks_bitmap_iterate(void **iter);
* ����: ������������λͼ
* ����:
* [1] iter: ������������¼��һ��Ҫ���ҵ�λ�ã����Ϊ0����ӵ�һ����ʼ����
* ���أ��������ȼ����е�ÿһ������ָ�룬�������NULL�����ʾû�о�������
* ע�⣺��
*********************************************************************************************************/
StVosTask *tasks_bitmap_iterate(void **iter)
{
	u32 pos = (u32)*iter;
	u8 *p8 = (u8*)gArrPrioBitMap;
	while (pos < sizeof(gArrPrioBitMap) * 8) {
		if ((pos & 0x7) == 0 && p8[pos>>3] == 0) {//����һ���ֽ�
			pos += 8;
		}
		else {//����8bit
			do {
				if (p8[pos>>3] & 1 << (pos & 0x7)) {
					*iter = (void*)(pos + 1);
					goto END_ITERATE;
				}
				pos++;
			} while (pos & 0x7);
		}
	}
	return 0;

END_ITERATE:
	return &gArrVosTask[gArrPrio2Taskid[pos]];
}

/********************************************************************************************************
* ������ u32 __vos_irq_save();
* ����: ���жϣ�Ȼ�󷵻ع��ж�ǰ��״̬���������ֵ��__vos_irq_restore(u32 save)���ָ�ԭ��״̬��֧�ֶ��Ƕ��
* ����: ��
* ���أ��������ȼ����е�ÿһ������ָ�룬�������NULL�����ʾû�о�������
* ע�⣺����TASK_LEVEL����֧���̼߳��û�ģʽ������Ȩģʽ������Ҫsvc�л�����ȨȻ��ر��ж�
*********************************************************************************************************/
u32 __vos_irq_save()
{
	u32 vos_save;
#if (TASK_LEVEL)
	u32 pri_save = 0;
	u32 irq_save = 0;
	if (vos_dis_irq_counter == 0 && VOSIntNesting == 0) {
		pri_save = vos_privileged_save(); //���ﲻ�ܽ�ֹ�ж�ǰ����
	}
	irq_save = __local_irq_save();
	vos_save = (pri_save<<1) | (irq_save<<0);
#else
	vos_save = __local_irq_save();
#endif
	return vos_save;
}

/********************************************************************************************************
* ������ void __vos_irq_restore(u32 save);
* ����: ���жϣ�֧�ֶ��Ƕ��
* ����:
* [1] save: __vos_irq_save()����ֵ��Ϊ����
* ���أ���
* ע�⣺����TASK_LEVEL����֧���̼߳��û�ģʽ������Ȩģʽ������Ҫsvc�л�����ȨȻ��ر��ж�
*********************************************************************************************************/
void __vos_irq_restore(u32 save)
{
#if (TASK_LEVEL)
	__local_irq_restore(save&0x01);
	if (vos_dis_irq_counter == 0 && VOSIntNesting == 0) {
		vos_privileged_restore(save>>1);//���ﲻ�ܽ�ֹ�ж�ǰ����
	}
#else
	__local_irq_restore(save);
#endif
}

/********************************************************************************************************
* ������ u32 VOSGetTicks();
* ����: ��ȡϵͳ��ticks������ticks�ڶ�ʱ�ж���ÿ���ۼ�1���޷���32λ��ֵ�������ͷ��������Ӱ��
* ����: ��
* ���أ���ǰϵͳ�ļ���ֵ
* ע�⣺��
*********************************************************************************************************/
u32 VOSGetTicks()
{
	u32 ticks;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	ticks = gVOSTicks;
	__vos_irq_restore(irq_save);
	return ticks;
}

/********************************************************************************************************
* ������ u32 VOSGetTimeMs();
* ����: ��ȡϵͳ��ǰ���еĺ���������ticksת���ɺ���󷵻�
* ����: ��
* ���أ���ǰϵͳ���к�����
* ע�⣺��
*********************************************************************************************************/
u32 VOSGetTimeMs()
{
	u32 ticks = 0;
	ticks = VOSGetTicks();
	return ticks * TICKS_INTERVAL_MS;
}

/********************************************************************************************************
* ������ u32 VOSTaskInit();
* ����: ϵͳ�����ʼ�����������������ʼ��
* ����:	��
* ���أ��鿴����ֵ
* ע�⣺��
*********************************************************************************************************/
u32 VOSTaskInit()
{
	s32 ret = VERROR_NO_ERROR;
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();

	//��ʼ����������λͼ�������ȼ�����IDӳ���
	memset(gArrPrioBitMap, 0, sizeof(gArrPrioBitMap));
	memset(gArrPrio2Taskid, 0, sizeof(gArrPrio2Taskid));

	//��ʼ������������кͿ����������
	INIT_LIST_HEAD(&gListTaskFree);
	INIT_LIST_HEAD(&gListTaskDelay);

	//�������������ӵ��������������
	for (i=0; i<MAX_VOSTASK_NUM; i++) {
		list_add_tail(&gArrVosTask[i].list, &gListTaskFree);
		gArrVosTask[i].status = VOS_STA_FREE;
	}
	__local_irq_restore(irq_save);

	//����idle����
	TaskIdleBuild();

	return ret;
}

/********************************************************************************************************
* ������ StVosTask *VOSGetTaskFromId(s32 task_id);
* ����: ͨ������ID��������ṹָ��
* ����:
* [1] task_id: ����ID
* ���أ�������ָ��, ���򷵻�NULL
* ע�⣺��
*********************************************************************************************************/
StVosTask *VOSGetTaskFromId(s32 task_id)
{
	if (task_id < 0 || task_id >= MAX_VOSTASK_NUM) return 0;
	return &gArrVosTask[task_id];
}

/********************************************************************************************************
* ������s32 VOSGetCurTaskId();
* ����:  ��ȡ��ǰ�����ID
* ����:  ��
* ���أ����ص�ǰ��������ID, ���ϵͳû��ʼ���ȣ������ж������ģ��򷵻�-1
* ע�⣺��
*********************************************************************************************************/
s32 VOSGetCurTaskId()
{
	if (VOSRunning == 1 && VOSIntNesting == 0) return 0;
		return pRunningTask - &gArrVosTask[0];
	return -1;
}



/********************************************************************************************************
* ������StVosTask *VOSTaskReadyListCutTask(StVosTask *pCutTask);
* ����: ��������λͼ�У�ȡ��ĳ������
* ����:
* [1] pCutTask: ָ��Ҫȡ����ĳ������ָ��
* ���أ�������ָ��
* ע�⣺��
*********************************************************************************************************/
StVosTask *VOSTaskReadyListCutTask(StVosTask *pCutTask)
{
	StVosTask *ptasknext = 0;
	u32 irq_save = 0;

	irq_save = __local_irq_save();

	if (list_empty(&pCutTask->list)) {//Ϊ�գ�֤��ֻ��һ���ڵ㣬�����λͼĳ��λ
		bitmap_clr(pCutTask->prio, gArrPrioBitMap); //���ȼ���λͼ��λ
		list_del(&pCutTask->list);//����ͬ���ȼ�����������ɾ���Լ�
		//gArrPrio2Taskid[pCutTask->prio] = 0xFF; //��Ч����IDֵ, ע�⣺���ﲻʹ��Ҳ���ԣ���Ϊλͼ�Ѿ������Ƿ�������
	}
	else {//֤����ͬ���ȼ��ж��������ʱҪ�Ѻ���������id���õ�gArrPrio2Taskid���У�ͬʱ������ɾ��������ȼ�������
		ptasknext = list_entry(pCutTask->list.next, struct StVosTask, list);
		list_del(&pCutTask->list);//����ͬ���ȼ�����������ɾ���Լ�
		gArrPrio2Taskid[pCutTask->prio] = ptasknext->id;
	}

	__local_irq_restore(irq_save);
	return pCutTask;
}

/********************************************************************************************************
* ������void VOSTaskPrioMoveUp(StVosTask *pMutexOwnerTask, struct list_head *list_head);
* ����: ����Ŀǰ������ӵ���ߵ��������ȼ�������������ȼ���ת����
* ����:
* [1] pMutexOwnerTask: ������ӵ���ߣ�����
* [2] list_head: ĳ����������������������ͷ
* ���أ���
* ע�⣺��
*********************************************************************************************************/
static void VOSTaskPrioMoveUp(StVosTask *pMutexOwnerTask, struct list_head *list_head)
{
	struct list_head *list_owner = 0;
	struct list_head *list_dest = 0;
	StVosTask *ptask_dest = 0;
	if (list_head==0) return;
	list_dest = list_owner = &pMutexOwnerTask->list;
	//���ȼ�ð������
	while (list_dest->prev != list_head) {
		list_dest = list_dest->prev;
		ptask_dest = list_entry(list_dest, struct StVosTask, list);
		if (ptask_dest->prio <= pMutexOwnerTask->prio) {//�ҵ�Ҫ�����λ�ã������λ�ú����������������
			if (list_dest != list_owner->prev) {//���������������ȼ��Ѿ����ʺϵ�λ�þͲ����κ�����������
				list_move(list_owner, list_dest);
			}
			break;//�������������ѭ����������������ľ������񣬿��ܶ������ָ����������������Ҫ����
		}
	}
	if (list_dest->prev == list_head) {//���뵽��һ��λ��
		if (list_dest != list_owner) {//���������������ȼ��Ѿ����ʺϵ�λ�þͲ����κ�����������
			list_move(list_owner, list_dest);
		}
	}
}

/********************************************************************************************************
* ������void VOSTaskRaisePrioBeforeBlock(StVOSMutex *pMutex);
* ����: �Ѿ��������������ָ����ͬ������ƿ飨ֻ��������������������������׼������ǰ�ĵ�ǰ����һ��
*       �����ǰ�������ȼ�����ӵ���������򲻴���
* ����:
* [1] pMutex: ����������ָ��
* ���أ���
* ע�⣺�ú������������ж������
*********************************************************************************************************/
void VOSTaskRaisePrioBeforeBlock(StVOSMutex *pMutex)
{
	void *pObj = 0;
	struct list_head *list_head = 0;
	StVosTask *pMutexOwnerTask = pMutex->ptask_owner;
	u32 irq_save = 0;

	if (pMutexOwnerTask->prio <= pRunningTask->prio) return;//׼���������������ȼ�����ӵ���ߣ�ֱ�ӷ���

	irq_save = __local_irq_save();
	//���ܱ��������
	//������ӵ���߿����ھ����������棬Ҳ�п����������������棬��������������
	//֤����ӵ�����Ȼ�ȡ����������������������ź����ȣ�����ʱ������������
	if (pMutexOwnerTask->status == VOS_STA_READY) {//�ھ�����������ȼ�������У�
		VOSTaskReadyListCutTask(pMutexOwnerTask);//�Ӿ����������
		pMutexOwnerTask->prio = pRunningTask->prio; //ɾ����ֵ
		VOSReadyTaskPrioSet(pMutexOwnerTask); //����
	}
	else if (pMutexOwnerTask->status == VOS_STA_BLOCK) {//��������������ȼ�������У�
		pObj = pMutexOwnerTask->psyn;
		switch(pMutexOwnerTask->status & BLOCK_SUB_MASK) {
		case VOS_BLOCK_SEMP:
			list_head = &((StVOSSemaphore*)pObj)->list_task;
		case VOS_BLOCK_EVENT:
			list_head = 0;
		case VOS_BLOCK_MUTEX:
			list_head = &((StVOSMutex*)pObj)->list_task;
		default: //ֻ����ʱ��ֻ���޸���������ȼ�����
			pMutexOwnerTask->prio = pRunningTask->prio; //ֱ�Ӹ�ֵ
			break;
		}
		pMutexOwnerTask->prio = pRunningTask->prio; //�ȸ�ֵ

		VOSTaskPrioMoveUp(pMutexOwnerTask, list_head);//ð������
	}
END_RAISE_PRIO:
	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* ������void VOSTaskRestorePrioBeforeRelease();
* ����: ���ͷŻ�����ʱ��Ҳͬʱ��ԭ���ȼ�
* ����: ��
* ���أ���
* ע�⣺�ú������������ж������
*********************************************************************************************************/
void VOSTaskRestorePrioBeforeRelease()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (pRunningTask->prio_base != pRunningTask->prio) {
		pRunningTask->prio = pRunningTask->prio_base;
	}
	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* ������void VOSReadyTaskPrioSet(StVosTask *pInsertTask);
* ����: ���������õ���������λͼ�У����λͼ���Ѿ����ø�λ���Ͳ��뵽����ͬ���ȼ�����������ĩβ
* ����:
* [1] pInsertTask: Ҫ���ӵ�����λͼ������ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSReadyTaskPrioSet(StVosTask *pInsertTask)
{
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (bitmap_get(pInsertTask->prio, gArrPrioBitMap)) {//�Ѿ���������λ�����뵽β��
		ptask = &gArrVosTask[gArrPrio2Taskid[pInsertTask->prio]];
		//������ͬ���ȼ��������жϣ�ֱ�Ӳ��뵽���
		list_add_tail(&pInsertTask->list, &ptask->list);
	}
	else {//û���ô����ȼ�λ��ֱ������λ������gArrPrioBitMap�����Ӧ��λ��Ϊ�����task id
		bitmap_set(pInsertTask->prio, gArrPrioBitMap); //���ȼ���λͼ��λ
		gArrPrio2Taskid[pInsertTask->prio] = pInsertTask->id; //��������ID
		INIT_LIST_HEAD(&pInsertTask->list); //��ʼ���б��Լ�ָ���Լ�
	}
	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* ������void VOSTaskDelayListInsert(StVosTask *pInsertTask);
* ����: ��������뵽��ʱ��ʱ�����������У����������������ǰ��������ʱ�����ӽ���Զ����
* ����:
* [1] pInsertTask: Ҫ���ӵ���������������ָ��
* ���أ���
* ע�⣺��������¼���������������ӵ����������������Ҳ���ӵ����������ź�������������
*********************************************************************************************************/
void VOSTaskDelayListInsert(StVosTask *pInsertTask)
{
	StVosTask *ptask_delay = 0;
	struct list_head *list;
	struct list_head *phead = 0;
	u32 irq_save = 0;

	irq_save = __local_irq_save();

	phead = &gListTaskDelay;

	//������У�����ʱ������С������������
	list_for_each(list, phead) {
		ptask_delay = list_entry(list, struct StVosTask, list_delay);
		//pInsertTask->ticks_alert < ptask_delay->ticks_alert
		if (TICK_CMP(pInsertTask->ticks_alert, ptask_delay->ticks_alert, gVOSTicks) < 0) {//����Խ��������Խǰ
			list_add_tail(&pInsertTask->list_delay, list);
			break;
		}
	}
	if (list == phead) {//�Ҳ�������λ�ã���ֱ�Ӳ��뵽���������
		list_add_tail(&pInsertTask->list_delay, phead);
	}

	//������ʱ������ҲҪ�����������
	//gMarkTicksNearest > pInsertTask->ticks_alert
	if (TICK_CMP(gMarkTicksNearest,pInsertTask->ticks_alert,pInsertTask->ticks_start) > 0) {
		gMarkTicksNearest = pInsertTask->ticks_alert;
	}
	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* ������void VOSTaskDelayListWakeUp();
* ����: ��ʱ���ж��﻽����ʱ��ʱ�������������Ѿ����������
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTaskDelayListWakeUp()
{
	StVosTask *ptask_delay = 0;
	struct list_head *list;
	struct list_head *list_temp;
	struct list_head *phead = 0;
	u32 irq_save = 0;

	irq_save = __local_irq_save();

	gMarkTicksNearest = TIMEOUT_INFINITY_U32;

	phead = &gListTaskDelay;

	//����ʱ������С������������
	list_for_each_safe (list, list_temp, phead) {
		ptask_delay = list_entry(list, struct StVosTask, list_delay);

		//ptask_delay->ticks_alert <= gVOSTicks
		if (TICK_CMP(gVOSTicks,ptask_delay->ticks_alert,ptask_delay->ticks_start) >= 0) {//���Ѳ����ӵ�������
			//�Ͽ���ǰ��ʱ����
			list_del(&ptask_delay->list_delay);
			//������ź����������б��У�ҲҪ�Ͽ�
			list_del(&ptask_delay->list);
			//��ʼ������
			ptask_delay->status = VOS_STA_READY;
			ptask_delay->block_type = 0;
			ptask_delay->ticks_start = 0;
			ptask_delay->ticks_alert = 0;
			ptask_delay->wakeup_from = VOS_WAKEUP_FROM_DELAY;
			//���ӵ��������б�
			VOSReadyTaskPrioSet(ptask_delay);
		}
		else {
			gMarkTicksNearest = ptask_delay->ticks_alert;
			break;
		}
	}
	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* ������void VOSTaskBlockListInsert(StVosTask *pInsertTask, struct list_head *phead);
* ����: ��������뵽ĳ���������������ź������������ȣ��������У����������ȼ��Ӹߵ�������
* ����:
* [1] pInsertTask: Ҫ���ӵ���������������ָ��
* [2] phead: �������������ź�������������������ͷ
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTaskBlockListInsert(StVosTask *pInsertTask, struct list_head *phead)
{
	StVosTask *ptask = 0;
	struct list_head *list;
	u32 irq_save = 0;

	irq_save = __local_irq_save();
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list, phead) {
		ptask = list_entry(list, struct StVosTask, list);
		if (pInsertTask->prio < ptask->prio) {//��ֵԽС�����ȼ�Խ�ߣ������ͬ���ȼ�������뵽������ͬ���ȼ�����
			list_add_tail(&pInsertTask->list, list);
			break;
		}
	}
	if (list == phead) {//�Ҳ�������λ�ã���ֱ�Ӳ��뵽���������
		list_add_tail(&pInsertTask->list, phead);
	}
	__local_irq_restore(irq_save);

}

/********************************************************************************************************
* ������void VOSTaskBlockListRelease(StVosTask *pReleaseTask);
* ����: �ͷ���������
* ����:
* [1] pReleaseTask: Ҫ�ͷŵ�����ָ��
* ���أ���
* ע�⣺������ҲҪ����ʱ��ʱ�������ͷ�
*********************************************************************************************************/
void VOSTaskBlockListRelease(StVosTask *pReleaseTask)
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//�ͷ��������������
	if (!(pReleaseTask->block_type & VOS_BLOCK_EVENT)) {
		list_del(&pReleaseTask->list);//�ͷŵ�һ���Ϳ��ԣ���Ϊ���ȼ�����, eventûʹ��������ͷŲ�Ӱ�졣
	}

	//ע����ʱ�������ɾ�����Ǳ�ͷ��ҲҪ�����������
	if (gListTaskDelay.next == &pReleaseTask->list_delay) {
		pReleaseTask->ticks_alert = gVOSTicks;//������ʱ��ȥ����
	}
	//�ͷ���ʱ�����������
	list_del(&pReleaseTask->list_delay);

	//���ӵ��������б�
	VOSReadyTaskPrioSet(pReleaseTask);

	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* ������s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask);
* ����: �������е�����;���λͼ��������ȼ�������������ȼ��Ƚ�
* ����:
* [1] pRunTask: �������е�����ָ��
* ���أ�
* ����ֵ > 0��֤����������������ȼ����ߣ���ֵ��С��������Ҫ����
* ����ֵ < 0��֤����������������ȼ����ͣ���ֵ���󣩣���Ҫ����
* ����ֵ = 0: ֤����������������ȼ���ͬ�� ��Ҫ���ȣ�����ʱ��Ƭ
* ע�⣺��
*********************************************************************************************************/
s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask)
{
	s32 ret = 1;//Ĭ�Ϸ����������������õ���
	s32 highest;

	u32 irq_save = 0;
	irq_save = __local_irq_save();

	highest = TaskHighestPrioGet(gArrPrioBitMap, MAX_COUNTS(gArrPrioBitMap)); //��ֵԽ�٣����ȼ�Խ��
	if (highest != -1) {
		ret = highest - pRunTask->prio;
	}
	else {
		ret = 1; //����Ҫ����
	}
	__local_irq_restore(irq_save);
	return ret;
}

/********************************************************************************************************
* ������StVosTask *VOSTaskReadyCutPriorest();
* ����: ȡ������λͼ�������ȼ���ߵ�����
* ����: ��
* ���أ�����������������ȼ�����ָ��
* ע�⣺��
*********************************************************************************************************/
StVosTask *VOSTaskReadyCutPriorest()
{
	StVosTask *ptasknext = 0;
	StVosTask *pHightask = 0;
	s32 highest;
	u32 irq_save = 0;

	irq_save = __local_irq_save();
	highest = TaskHighestPrioGet(gArrPrioBitMap, MAX_COUNTS(gArrPrioBitMap));
	if (highest != -1) {
		pHightask = &gArrVosTask[gArrPrio2Taskid[highest]];
		if (list_empty(&pHightask->list)) {//Ϊ�գ�֤��ֻ��һ���ڵ㣬�����λͼĳ��λ
			bitmap_clr(highest, gArrPrioBitMap); //���ȼ���λͼ��λ
			list_del(&pHightask->list);//����ͬ���ȼ�����������ɾ���Լ�
		}
		else {//֤����ͬ���ȼ��ж��������ʱҪ�Ѻ���������id���õ�gArrPrio2Taskid���У�ͬʱ������ɾ��������ȼ�������
			ptasknext = list_entry(pHightask->list.next, struct StVosTask, list);
			list_del(&pHightask->list);//����ͬ���ȼ�����������ɾ���Լ�
			gArrPrio2Taskid[highest] = ptasknext->id;
		}
		//����������ȼ����������������Ҫ�����������״̬
		pHightask->ticks_timeslice = MAX_TICKS_TIMESLICE;
		pHightask->status = VOS_STA_RUNNING;
	}
	__local_irq_restore(irq_save);
	return pHightask;
}

/********************************************************************************************************
* ������s32 PrepareForCortexSwitch();
* ����: �������л�ǰ׼����������Ҫ��ȡ����������λͼ�����ȼ���ߵ�����ֵ��pReadyTask������
* ����: ��
* ���أ�����true�������Ѿ�׼���ã����������л������ģ������л�
* ע�⣺��
*********************************************************************************************************/
s32 PrepareForCortexSwitch()
{
	pReadyTask = VOSTaskReadyCutPriorest();
	return pReadyTask && pRunningTask;//����������Ϊ0
}

/********************************************************************************************************
* ������void VOSTaskEntry(void *param);
* ����: �������񶼴����������������������������������
* ����:
* [1] param: ��ʱû�õ�
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTaskEntry(void *param)
{
	u32 irq_save = 0;
	if (pRunningTask) {
		pRunningTask->task_fun(pRunningTask->param);
	}
	//��������յ���������������
	irq_save = __local_irq_save();
	//���ӵ������б���
	pRunningTask->status = VOS_STA_FREE;
	list_add_tail(&pRunningTask->list, &gListTaskFree);
	__local_irq_restore(irq_save);
	VOSTaskSchedule();
}

/********************************************************************************************************
* ������s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
*					void *pstack, u32 stack_size, s32 prio, s8 *task_nm);
* ����: ��������
* ����:
* [1] task_fun: ����Ҫִ�е����̺���
* [2] param: ���̺�������
* [3] pstack: ����ջָ���ַ��ջ�׵�ַ��
* [4] stack_size: ����ջ��С
* [5] prio: �������ȼ�
* [5] task_nm: ��������
* ���أ�����ID, �������ʧ�ܣ�����-1
* ע�⣺��
*********************************************************************************************************/
s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm)
{
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	if (list_empty(&gListTaskFree)) goto END_CREATE;

	ptask = list_entry(gListTaskFree.next, StVosTask, list); //��ȡ��һ����������
	list_del(gListTaskFree.next); //�������������ɾ����һ����������

	if (prio > MAX_VOS_TASK_PRIO_IDLE) {//���ȼ����ܳ���255�����Ե���255
		prio = MAX_VOS_TASK_PRIO_IDLE;
	}

	memset(ptask, 0, sizeof(*ptask));

	ptask->id = (ptask - gArrVosTask);

	ptask->prio = ptask->prio_base = prio;

	ptask->stack_size = stack_size;

	ptask->pstack_top = (u32*)((u8*)pstack + stack_size);

	if (pstack && stack_size) {
		memset(pstack, 0x64, stack_size); //��ʼ��ջ����Ϊ0x64, ջ���ʹ��
	}
	ptask->name = task_nm;

	ptask->param = param;

	ptask->task_fun = task_fun;

	ptask->status = VOS_STA_READY;

	ptask->psyn = 0;

	ptask->block_type = 0;//û�κ�����

	ptask->ticks_used_start = -1; //��ֹͳ��cpuʹ����

	ptask->event_mask = 0;
#if VOS_LIST_DEBUG
	ptask->list.pTask = ptask;
	ptask->list_delay.pTask = ptask;
#endif
	//��ʼ��ջ����,����18��u32��С
	ptask->pstack = ptask->pstack_top;
	*(--ptask->pstack) = 0x01000000; //xPSR
	*(--ptask->pstack) = (u32) VOSTaskEntry; //xPSR
	*(--ptask->pstack) = 0; //LR
	*(--ptask->pstack) = 0; //R12
	*(--ptask->pstack) = 0; //R3
	*(--ptask->pstack) = 0; //R2
	*(--ptask->pstack) = 0; //R1
	*(--ptask->pstack) = 0; //R0
	*(--ptask->pstack) = 0; //R11
	*(--ptask->pstack) = 0; //R10
	*(--ptask->pstack) = 0; //R9
	*(--ptask->pstack) = 0; //R8
	*(--ptask->pstack) = 0; //R7
	*(--ptask->pstack) = 0; //R6
	*(--ptask->pstack) = 0; //R5
	*(--ptask->pstack) = 0; //R4
#if (TASK_LEVEL)
	*(--ptask->pstack) = 0x03; //CONTROL //#2:��Ȩ+PSP, #3:�û���+PSP
#else
	*(--ptask->pstack) = 0x02; //CONTROL //#2:��Ȩ+PSP, #3:�û���+PSP
#endif
	*(--ptask->pstack) = 0xFFFFFFFDUL; //EXC_RETURN

	//���뵽��������
	VOSReadyTaskPrioSet(ptask);

END_CREATE:
	__vos_irq_restore(irq_save);

	if (VOSRunning && ptask && pRunningTask->prio > ptask->prio) {
		//��������ʱ���½����������ȼ����������е����ȼ��ߣ����л�
		VOSTaskSchedule();
	}
	//����ע�⣬�������������ȼ����������ˣ�ִ�и������ȼ�����л����������ִ�С�
	return ptask ? ptask->id : -1;
}

/********************************************************************************************************
* ������s32 VOSTaskDelete(s32 task_id);
* ����:  ɾ������
* ����:
* [1] task_id: ����id
* ���أ��ɹ���ʧ��
* ע�⣺��
*********************************************************************************************************/
s32 VOSTaskDelete(s32 task_id)
{
	kprintf("XXXXXXXXXXXXXXXX %s:%d\r\n", __FUNCTION__, task_id);
	return 0;
}

/********************************************************************************************************
* ������void task_idle(void *param);
* ����: ������������
* ����:
* [1] param: ��ʱû�õ�
* ���أ���
* ע�⣺�������񲻿�������������Ҫ����״̬��������״̬
*********************************************************************************************************/
void task_idle(void *param)
{
	/*
	 * ע�⣺ ���������ܼ��κ�ϵͳ��ʱ�����Լ�Ӳ��ʱ��
	 * ԭ�� ������������û����������У����¾�������ʣ��һ��ʱ�����޷����г�ȥ��
	 *       ���ʣ��һ�����������л���ȥ����ô��ǰcpu��ɶ��������Ҫ����������������
	 */
	static u32 mark_time=0;
	mark_time = VOSGetTimeMs();
	while (1) {
		//��ֹ������������
//		if ((s32)(VOSGetTimeMs()-mark_time) > 1000) {
//			mark_time = VOSGetTimeMs();
//			VOSTaskSchTabDebug();
//		}
	}
}

/********************************************************************************************************
* ������void VOSStarup();
* ����: VOSϵͳ������һ������������IDLE����
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSStarup()
{
	if (VOSRunning == 0) { //������һ������ʱ�����ø�VOSRunningΪ1
		RunFirstTask(); //���ص�һ��������ʱ����һ����IDLE����
		while (1) ;;//�������ܵ�����
	}
}

/********************************************************************************************************
* ������void VOSIntEnter();
* ����: �κ��жϽ���ʱ���ã���Ҫ�Ǽ�¼�жϲ������������ж��˳�ʱ����ϵͳ����
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSIntEnter()
{
	u32 irq_save = 0;
    if (VOSRunning)
    {
    	irq_save = __local_irq_save();
        if (VOSIntNesting < MAX_INFINITY_U32) {
        	VOSIntNesting++;
        }
        __local_irq_restore(irq_save);
    }
}

/********************************************************************************************************
* ������void VOSIntExit();
* ����: �κ��ж��˳�ʱ���ã���Ҫ�Ǽ�¼�жϲ������������ж��˳�ʱ����ϵͳ����
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSIntExit()
{
	u32 irq_save = 0;
    if (VOSRunning)
    {
    	irq_save = __local_irq_save();
        if (VOSIntNesting) VOSIntNesting--;
        if (VOSIntNesting == 0) {
        	VOSTaskScheduleISR();
        }
        __local_irq_restore(irq_save);
    }
}


/********************************************************************************************************
* ������void TaskIdleBuild();
* ����: ������������
* ����: ��
* ���أ���
* ע�⣺���ۺ�ʱ�������и�IDLE�����ھ�������
*********************************************************************************************************/
void TaskIdleBuild()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	VOSTaskCreate(task_idle, 0, stack_idle, sizeof(stack_idle), TASK_PRIO_MAX, "idle");
	pRunningTask = VOSTaskReadyCutPriorest();//��ȡ��һ��������������pRunningTaskָ�룬׼������
	__local_irq_restore(irq_save);
}


/********************************************************************************************************
* ������void VOSTaskSchedule();
* ����: ���������ĵ��ȣ�����Ҫ���������������ģ�������ж������������VOSTaskScheduleISR
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTaskSchedule()
{
	s32 ret = 0;
	u32 irq_save = 0;
	if (VOSRunning == 0) return; //��û��ʼ���л�ֱ�ӷ���

	if (VOSCtxSwtFlag == 1) return; //�Ѿ��������л���־���Ͳ����ٱȽ�

	irq_save = __local_irq_save();

	if (pRunningTask->status == VOS_STA_FREE || //�������٣��������
			pRunningTask->status == VOS_STA_BLOCK) {//�����������������
		VOSCtxSwtFlag = 1;
	}
	else {
		//�Ƚ��������е��������ȼ��;����������������ȼ�
		ret = VOSTaskReadyCmpPrioTo(pRunningTask);
		//�������е��������ȼ���ͬ�����  ������Ҫ�����л�
		if (ret <= 0) {
			VOSCtxSwtFlag = 1;
			//�����е��������ӵ���������
			pRunningTask->status = VOS_STA_READY;
			VOSReadyTaskPrioSet(pRunningTask);
		}
	}

	__local_irq_restore(irq_save);

	if (VOSCtxSwtFlag) {
		asm_ctx_switch();//����PendSV_Handler�ж�
	}
}

/********************************************************************************************************
* ������void VOSTaskScheduleISR();
* ����: �ж������ĵ��ȣ�����Ҫ�������ж������ģ���������������������VOSTaskSchedule
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTaskScheduleISR()
{
	s32 ret = 0;
	u32 irq_save = 0;
	if (VOSRunning == 0) return; //��û��ʼ���л�ֱ�ӷ���

	if (VOSCtxSwtFlag == 1) return; //�Ѿ��������л���־���Ͳ����ٱȽ�

	irq_save = __local_irq_save();

	if (pRunningTask->status == VOS_STA_FREE || //�������٣��������
			pRunningTask->status == VOS_STA_BLOCK) {//�����������������
		VOSCtxSwtFlag = 1;
	}
	else {
		//�Ƚ��������е��������ȼ��;����������������ȼ�
		ret = VOSTaskReadyCmpPrioTo(pRunningTask);
		if (ret < 0 || (ret == 0 && pRunningTask->ticks_timeslice == 0))//ret == 0 && ticks_timeslice==0:ʱ��Ƭ���꣬ͬʱ������������ͬ���ȼ���Ҳ���л�
		{
			VOSCtxSwtFlag = 1;
			//�����е��������ӵ���������
			pRunningTask->status = VOS_STA_READY;
			VOSReadyTaskPrioSet(pRunningTask);
		}
		if (pRunningTask->ticks_timeslice > 0) { //ʱ��Ƭ����Ϊ��������һ����ͬ���ȼ���������������У��Ϳ��������л�
			pRunningTask->ticks_timeslice--;
		}
	}

	__local_irq_restore(irq_save);

	if (VOSCtxSwtFlag) {
		asm_ctx_switch();//����PendSV_Handler�ж�
	}
}

/********************************************************************************************************
* ������void VOSSysTick();
* ����: Tick����, �⺯����Ҫ�ڶ�ʱ�ж�������
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSSysTick()
{
	u32 irq_save;
	irq_save = __local_irq_save();
	gVOSTicks++;
	__local_irq_restore(irq_save);
	if (VOSRunning && gVOSTicks >= gMarkTicksNearest) {//�����죬�����������У��Ѷ�Ӧ�������������ӵ�����������
		VOSTaskDelayListWakeUp();
	}
}

/********************************************************************************************************
* ������s32 VOSCurContexStatus();
* ����: ��ȡ��ǰִ��������״̬���ж������ģ����������ģ�ϵͳ������
* ����: ��
* ���أ��ж������ģ����������ģ�ϵͳ������
* ע�⣺��
*********************************************************************************************************/
s32 VOSCurContexStatus()
{
	if (VOSIntNesting != 0) {
		return CONTEXT_ISR;
	}
	else if (VOSRunning) {
		return CONTEXT_TASK;
	}
	else {
		return CONTEXT_SYS;
	}
}


/********************************************************************************************************
* ������void VOSTaskDelay(u32 ms);
* ����: ��ʱ���������������ĵ��ã�������ж������ģ���Ӳ����ʱ
* ����:
* [1] ms: ��ʱ��λ����
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VOSTaskDelay(u32 ms)
{
	//����жϱ��رգ�ϵͳ��������ȣ���ֱ��Ӳ��ʱ
	if (VOSIntNesting > 0) {
		VOSDelayUs(ms*1000);
	}
	//����������ϵͳ��������ʱ
	u32 irq_save = 0;

	if (ms) {//������ʱ����
		irq_save = __vos_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
		//gMarkTicksNearest > pRunningTask->ticks_alert
		if (TICK_CMP(gMarkTicksNearest,pRunningTask->ticks_alert,pRunningTask->ticks_start) > 0) {
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}

		pRunningTask->status = VOS_STA_BLOCK; //���ӵ���������
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		VOSTaskDelayListInsert(pRunningTask); //�ѵ�ǰ����ֱ�Ӳ嵽��ʱ��������

		__vos_irq_restore(irq_save);
	}
	//msΪ0���л�����, ����VOS_STA_BLOCK_DELAY��־����������
	VOSTaskSchedule();
}

/**********************************************************************
 * ********************************************************************
 * ������Ҫ�Ƕ���鿴�ں���Ϣ�ĺ���
 * ********************************************************************
 **********************************************************************/

int vvsprintf(char *buf, int len, char* format, ...);
#define sprintf vvsprintf

/********************************************************************************************************
* ������void VOSTaskSchTabDebug();
* ����: ��ӡ��������λͼ�������������
* ����: ��
* ���أ���
* ע�⣺�˺�����Ҫ�ǵ���ʱʹ��
*********************************************************************************************************/
void VOSTaskSchTabDebug()
{
	static u8 buf[1024];
	u8 temp[100];
	s32 prio_mark = -1;
	u32 mark = 0;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;

	u32 irq_save;
	irq_save = __vos_irq_save();

	buf[0] = 0;

	vvsprintf(temp, sizeof(temp), "[(%d) ", pRunningTask?pRunningTask->id:-1);
	strcat(buf, temp);
	vvsprintf(temp, sizeof(temp), "(%d) ", pReadyTask?pReadyTask->id:-1);
	strcat(buf, temp);

	//������У��������ȼ��Ӹߵ�����������
	strcat(buf, "(");

	void *iter = 0; //��ͷ����
	while (ptask_prio = tasks_bitmap_iterate(&iter)) {
		vvsprintf(temp, sizeof(temp), "%d", ptask_prio->id);
		strcat(buf, temp);
		strcat(buf, "->");

	}
	if (ptask_prio==0) {
		if (buf[strlen(buf)-1] == '>') {
			buf[strlen(buf)-2] = 0;
		}
	}
	strcat(buf, ")");
	phead = &gListTaskDelay;
	//������У��������ȼ��Ӹߵ�����������
	strcat(buf, "(");
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list_delay);
		vvsprintf(temp, sizeof(temp), "%d", ptask_prio->id);
		strcat(buf, temp);
		if (list_prio->next != phead) {
			strcat(buf, "->");
		}
	}
	strcat(buf, ")]\r\n");
	kprintf("%s", buf);
	__vos_irq_restore(irq_save);
}

/********************************************************************************************************
* ������void HookTaskSwitchOut();
* ����: ��ǰ����׼���л���ȥǰ���øú�����Ŀǰ�����������ͳ������ʹ��cpuռ���ʼ���
* ����: ��
* ���أ���
* ע�⣺�˺�����Ҫ������shell����task�б����鿴cpuռ����
*********************************************************************************************************/
void HookTaskSwitchOut()
{
	if (VOSRunning && pRunningTask->ticks_used_start != -1) {
		if (gVOSTicks - pRunningTask->ticks_used_start > 0) {
			pRunningTask->ticks_used_cnts += gVOSTicks - pRunningTask->ticks_used_start;
		}
	}
}

/********************************************************************************************************
* ������void HookTaskSwitchOut();
* ����: ������ȼ������Ѿ��л�Ϊ��ǰ����������øú�����Ŀǰ�����������ͳ������ʹ��cpuռ���ʼ���
* ����: ��
* ���أ���
* ע�⣺�˺�����Ҫ������shell����task�б����鿴cpuռ����
*********************************************************************************************************/
void HookTaskSwitchIn()
{
	if (VOSRunning && pRunningTask->ticks_used_start != -1) {
		pRunningTask->ticks_used_start = gVOSTicks;
	}
}

/********************************************************************************************************
* ������void CaluTasksCpuUsedRateStart();
* ����: shell����task�������������ʾ�����Ϣ
* ����: ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void CaluTasksCpuUsedRateStart()
{
	u32 irq_save = 0;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;

	if (VOSRunning==0 || flag_cpu_collect) return;

	irq_save = __local_irq_save();

	//���������е�����ҲҪ������
	pRunningTask->ticks_used_start = gVOSTicks;

	//���ȼ���ʹ��λͼ
	void *iter = 0; //��ͷ����
	while (ptask_prio = tasks_bitmap_iterate(&iter)) {
		ptask_prio->ticks_used_start = gVOSTicks;
	}

	phead = &gListTaskDelay; //����������ȫ����������ʱ������
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list_delay);
		ptask_prio->ticks_used_start = gVOSTicks;
	}

	flag_cpu_collect = 1; //���òɼ���־

	__local_irq_restore(irq_save);

}

/********************************************************************************************************
* ������s32 CaluTasksCpuUsedRateShow(struct StTaskInfo *arr, s32 cnts, s32 mode);
* ����: shell����task�������������ʾ�����Ϣ
* ����:
* [1] arr: �û��ṩ��������Ϣ����
* [2] cnts: ������Ϣ�ĸ���
* [3] mode: mode==0, ��������; mode==1, ��������ʱ���ۼӲɼ���
* ���أ�������Ч������ĸ������������cnts >= ����ֵ�����г��������񣬷����г�����������ʱcpuͳ�Ʋ�׼
* ע�⣺��
*********************************************************************************************************/
s32 CaluTasksCpuUsedRateShow(struct StTaskInfo *arr, s32 cnts, s32 mode)
{
	u32 irq_save = 0;
	s32 i = 0;
	s32 j = 0;
	s32 ret = 0;
	s32 ticks_totals = 0;
	StVosTask *ptask_prio = 0;
	s8 stack_status[16];
	struct list_head *list_prio;
	struct list_head *phead = 0;
	s32 stack_left = 0;
	//������ȫ��Ԫ�ص�id����Ϊ-1
	for (i=0; i<cnts; i++) {
		arr[i].id = -1;
	}

	if (flag_cpu_collect==0) {
		CaluTasksCpuUsedRateStart();
		flag_cpu_collect = 1;
	}

	irq_save = __local_irq_save();

	i = 0;
	//���������е�����ҲҪ����
	if (i < cnts) {
		arr[i].id = pRunningTask->id;
		arr[i].ticks = pRunningTask->ticks_used_cnts;
		arr[i].prio = pRunningTask->prio_base;
		arr[i].stack_top = pRunningTask->pstack_top;
		arr[i].stack_size = pRunningTask->stack_size;
		arr[i].stack_ptr = pRunningTask->pstack;
		arr[i].name = pRunningTask->name;
		ticks_totals += arr[i].ticks;
		if (mode==0) {//����
			pRunningTask->ticks_used_start = -1;
			pRunningTask->ticks_used_cnts = 0;
		}
	}

	void *iter = 0; //��ͷ����
	while (ptask_prio = tasks_bitmap_iterate(&iter)) {
		i++;
		if (i < cnts) {
			arr[i].id = ptask_prio->id;
			arr[i].ticks = ptask_prio->ticks_used_cnts;
			arr[i].prio = ptask_prio->prio_base;
			arr[i].stack_top = ptask_prio->pstack_top;
			arr[i].stack_size = ptask_prio->stack_size;
			arr[i].stack_ptr = ptask_prio->pstack;
			arr[i].name = ptask_prio->name;
			ticks_totals += arr[i].ticks;
			if (mode==0) {//����
				ptask_prio->ticks_used_start = -1;
				ptask_prio->ticks_used_cnts = 0;
			}
		}
		else {//����
			break;
		}
	}

	phead = &gListTaskDelay; //����������ȫ����������ʱ������
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list_delay);
		i++;
		if (i < cnts) {
			arr[i].id = ptask_prio->id;
			arr[i].ticks = ptask_prio->ticks_used_cnts;
			arr[i].prio = ptask_prio->prio_base;
			arr[i].stack_top = ptask_prio->pstack_top;
			arr[i].stack_size = ptask_prio->stack_size;
			arr[i].stack_ptr = ptask_prio->pstack;
			arr[i].name = ptask_prio->name;
			ticks_totals += arr[i].ticks;
			if (mode==0) {//����
				ptask_prio->ticks_used_start = -1;
				ptask_prio->ticks_used_cnts = 0;
			}
		}
		else {//����
			break;
		}
	}
	if (mode==0) {//���������ɼ�
		flag_cpu_collect = 0;
	}
	__local_irq_restore(irq_save);

	ret = i;

	//��ӡ����������Ϣ
	//����������򣬴��Ż�������idΨһ��ֱ�Ӵ�0��ʼ��
	kprintf("�����\t���ȼ�\tCPU(�ٷֱ�)\tջ����ַ\tջ��ǰ��ַ\tջ�ܳߴ�\tջʣ��ߴ�\tջ״̬\t������\r\n");
	for (i=0; i<MAX_VOSTASK_NUM; i++) {
		for (j=0; j<cnts; j++) {
			if (arr[j].id == i) {
				//��ӡ������Ϣ
				memset(stack_status, 0, sizeof(stack_status));
				strcpy(stack_status, "����");
				if (arr[j].stack_ptr <= arr[j].stack_top - arr[j].stack_size) {
					stack_left = 0;
				}
				else {//����
					stack_left = arr[j].stack_size - (arr[j].stack_top - arr[j].stack_ptr);
				}
				//���ջ�Ƿ��޸Ļ����, ���ջ���Ƿ�0x64���޸�
				if (*(u32*)(arr[j].stack_top - arr[j].stack_size) != 0x64646464) {
					strcpy(stack_status, "�ƻ�"); //�����Ǳ��Լ��ƻ����߱��Լ�����ı����ƻ�
				}
				if (ticks_totals==0) ticks_totals = 1; //������ĸ����Ϊ0
				kprintf("%04d\t%03d\t\t%03d\t%08x\t%08x\t%08x\t%08x\t%s\t%s\r\n",
						arr[j].id, arr[j].prio, (u32)(arr[j].ticks*100/ticks_totals), arr[j].stack_top,
						arr[j].stack_ptr, arr[j].stack_size, stack_left, stack_status, arr[j].name);
				break;
			}
		}
	}
	return ret;
}

/********************************************************************************************************
* ������s32 GetTaskIdByName(u8 *name);
* ����: ͨ������������ȡ����ID
* ����:
* [1] name: ������
* ���أ�����ID,����Ҳ������񣬷���-1
* ע�⣺��
*********************************************************************************************************/
s32 GetTaskIdByName(u8 *name)
{
	s32 id_ret = -1;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	void *iter = 0; //��ͷ����
	u32 irq_save;
	irq_save = __vos_irq_save();

	if (strcmp(name, pRunningTask->name) == 0) {
		id_ret = pRunningTask->id;
		goto END_GETID;
	}

	while (ptask_prio = tasks_bitmap_iterate(&iter)) {
		if (strcmp(name, ptask_prio->name) == 0) {
			id_ret = ptask_prio->id;
			goto END_GETID;
		}
	}

	phead = &gListTaskDelay;
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list_delay);
		if (strcmp(name, ptask_prio->name) == 0) {
			id_ret = ptask_prio->id;
			goto END_GETID;
		}
	}

END_GETID:
	__vos_irq_restore(irq_save);

	return id_ret;
}
