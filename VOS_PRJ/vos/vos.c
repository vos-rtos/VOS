//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "vtype.h"
#include "vos.h"
#include "list.h"

struct StVosTask gArrVosTask[MAX_VOSTASK_NUM]; //�������飬ռ�ÿռ�

struct list_head gListTaskReady;//���������������

struct list_head gListTaskFree;//�������񣬿��Լ����������

struct list_head gListTaskBlock;//�����������

struct StVosTask *pRunningTask; //�������е�����

struct StVosTask *pReadyTask; //׼��Ҫ�л�������

volatile s64  gVOSTicks = 0;

volatile s64 gMarkTicksNearest = MAX_SIGNED_VAL_64; //��¼���������

u32 SVC_EXC_RETURN; //SVC����󱣴棬Ȼ�󷵻�ʱҪ�õ� (cortex m4)

volatile u32 VOSIntNesting = 0;

volatile u32 VOSRunning = 0;




long long stack_idle[1024];


s64 VOSGetTicks()
{
	s64 ticks;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	ticks = gVOSTicks;
	__local_irq_restore(irq_save);
	return ticks;
}

s64 VOSGetTimeMs()
{
	s64 ticks = 0;
	ticks = VOSGetTicks();
	return ticks * TICKS_INTERVAL_MS;
}

u32 VOSTaskInit()
{
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//��ʼ������������кͿ����������
	INIT_LIST_HEAD(&gListTaskReady);
	INIT_LIST_HEAD(&gListTaskFree);
	INIT_LIST_HEAD(&gListTaskBlock);
	//�������������ӵ��������������
	for (i=0; i<MAX_VOSTASK_NUM; i++) {
		list_add_tail(&gArrVosTask[i].list, &gListTaskFree);
		gArrVosTask[i].status = VOS_STA_FREE;
		//gArrVosTask[i].prio = TASK_PRIO_INVALID;
	}
	__local_irq_restore(irq_save);
	return 0;
}

StVosTask *VOSGetTaskFromId(s32 task_id)
{
	if (task_id < 0 || task_id >= MAX_VOSTASK_NUM) return 0;
	return &gArrVosTask[task_id];
}

u32 VOSTaskDelay(u32 ms)
{
	//����жϱ��رգ�ϵͳ��������ȣ���ֱ��Ӳ��ʱ
	//todo
	//����������ϵͳ��������ʱ
	u32 irq_save = 0;

	if (ms) {//������ʱ����
		irq_save = __local_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		//VOSTaskSwitch(TASK_SWITCH_ACTIVE);

		__local_irq_restore(irq_save);
	}
	//msΪ0���л�����, ����VOS_STA_BLOCK_DELAY��־����������
	VOSTaskSchedule();
	return 0;
}
//�Ѿ��������������ָ����ͬ������ƿ飨ֻ������������������������׼������ǰ�ĵ�ǰ����һ��
//�����ǰ�������ȼ�����ӵ���������򲻴���
//�ú������������ж������
s32 VOSTaskRaisePrioBeforeBlock(StVosTask *pMutexOwnerTask)
{
	StVosTask *ptask_dest = 0;
	StVosTask *ptask_temp = 0;
	StVosTask *ptask_ready = 0;
	struct list_head *list_owner = 0;
	struct list_head *list_dest = 0;

	struct list_head *list_head = 0;

	u32 irq_save = 0;
	irq_save = __local_irq_save();

	if (pMutexOwnerTask->prio > pRunningTask->prio) {
		pMutexOwnerTask->prio = pRunningTask->prio;
	}
	//������ӵ���߿����ھ����������棬Ҳ�п����������������棬��������������
	//֤����ӵ�����Ȼ�ȡ����������������������ź����ȣ�����ʱ������������
	if (pMutexOwnerTask->status == VOS_STA_READY) {//�ھ�����������ȼ�������У�
		list_head = &gListTaskReady;
	}
	else if (pMutexOwnerTask->status == VOS_STA_BLOCK) {//��������������ȼ�������У�
		list_head = &gListTaskBlock;
	}
	else {
		goto END_RAISE_PRIO;
	}

	list_dest = list_owner = &pMutexOwnerTask->list;
	//���ȼ�ð������
	while (list_dest->prev != list_head) {
		list_dest = list_dest->prev;
		ptask_dest = list_entry(list_dest, struct StVosTask, list);
		if (ptask_dest->prio <= pMutexOwnerTask->prio) {//�ҵ�Ҫ�����λ�ã������λ�ú����������������
			if (list_dest != list_owner->prev) {//���������������ȼ��Ѿ����ʺϵ�λ�þͲ����κ����������
				list_move(list_owner, list_dest);
			}
			break;//�������������ѭ����������������ľ������񣬿��ܶ������ָ����������������Ҫ����
		}
	}
	if (list_dest->prev == list_head) {//���뵽��һ��λ��
		if (list_dest != list_owner) {//���������������ȼ��Ѿ����ʺϵ�λ�þͲ����κ����������
			list_move(list_owner, list_dest);
		}
	}

END_RAISE_PRIO:
	__local_irq_restore(irq_save);
	return 0;
}

s32 VOSTaskRestorePrioBeforeRelease()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (pRunningTask->prio_base != pRunningTask->prio) {
		pRunningTask->prio = pRunningTask->prio_base;
	}
	__local_irq_restore(irq_save);
	return 0;
}

//�ѵ�ǰ������뵽�������������ȸߵ�����
//�����ȼ����뵽����ͷ�������ͬ���ȼ�������뵽��ͬ���ȼ������
//���������ȼ�������Ҫ��Ϊ�˲���ʱ����ͷ��ʼ���ң�����ҵ����ͷ�ͬ���źš�
//��֤���������ȼ���ߵ����޻�ȡ��Դ������
void VOSTaskPrtList(s32 which_list);
u32 VOSTaskCheck();
u32 VOSTaskListPrioInsert(StVosTask *pTask, s32 which_list)
{
	s32 ret = -1;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	u32 irq_save = 0;

//	if (VOSTaskCheck()>=2) {
//		VOSTaskPrtList(VOS_LIST_READY);
//		VOSTaskPrtList(VOS_LIST_BLOCK);
//	}
	irq_save = __local_irq_save();
	switch (which_list) {
	case VOS_LIST_READY:
		phead = &gListTaskReady;
		break;
	case VOS_LIST_BLOCK:
		phead = &gListTaskBlock;
		break;
	}

	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		if (ptask_prio->prio > pTask->prio) {//��ֵԽС�����ȼ�Խ�ߣ������ͬ���ȼ�������뵽������ͬ���ȼ�����
			list_add_tail(&pTask->list, list_prio);
			ret = 0;
			break;
		}
	}
	if (list_prio == phead) {//�Ҳ�������λ�ã���ֱ�Ӳ��뵽��������
		list_add_tail(&pTask->list, phead);
	}
	__local_irq_restore(irq_save);

	return ret;
}

//�����������Ƿ��бȵ�ǰ���е����ȼ�����
//����������֤����������������ȼ����ߣ���ֵ��С��������Ҫ����
//���ظ�����֤����������������ȼ����ͣ���ֵ���󣩣���Ҫ����
//���� 0  : ֤����������������ȼ���ͬ�� ��Ҫ���ȣ�����ʱ��Ƭ
s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask)
{
	s32 ret = 1;//Ĭ�Ϸ��������������õ���
	StVosTask *ptask_prio = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (!list_empty(&gListTaskReady)) {
		ptask_prio = list_entry(gListTaskReady.next, struct StVosTask, list);
		ret = ptask_prio->prio - pRunTask->prio;
	}
	__local_irq_restore(irq_save);
	return ret;
}

//��ȡ��ǰ�������������ȼ���ߵ�����
StVosTask *VOSTaskReadyCutPriorest()
{
	StVosTask *ptask_prio = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//������У��������ȼ��Ӹߵ�����������
	if (!list_empty(&gListTaskReady)) {
		ptask_prio = list_entry(gListTaskReady.next, struct StVosTask, list);
		list_del(gListTaskReady.next); //�������������ɾ����һ����������
		ptask_prio->ticks_timeslice = MAX_TICKS_TIMESLICE;
	}
	__local_irq_restore(irq_save);
	return ptask_prio;
}
//�û�ջPSP״̬���л���Ҫ��svn 1
void VOSTaskEntry(void *param)
{
	if (pRunningTask) {
		pRunningTask->task_fun(param);
	}
	//��������յ���������������
	pRunningTask->status = VOS_STA_FREE;
	VOSTaskSchedule();
}

u32 VOSTaskCheck()
{
	s32 prio_mark = -1;
	u32 irq_save = 0;
	u32 mark = 0;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	irq_save = __local_irq_save();

	phead = &gListTaskReady;

	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		if (ptask_prio->id == 2) {
			mark++;
			break;
		}
	}
	phead = &gListTaskBlock;
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		if (ptask_prio->id == 2) {
			mark++;
			break;
		}
	}
	__local_irq_restore(irq_save);
	return mark;
}

void VOSTaskPrtList(s32 which_list)
{
	s32 prio_mark = -1;
	u32 irq_save = 0;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	irq_save = __local_irq_save();
	switch (which_list) {
	case VOS_LIST_READY:
		phead = &gListTaskReady;
		kprintf("Ready List Infomation: \r\n");
		break;
	case VOS_LIST_BLOCK:
		phead = &gListTaskBlock;
		kprintf("Block List Infomation: \r\n");
		break;
	}
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);

		if (prio_mark != ptask_prio->prio) {
			if (prio_mark != -1) kprintf("]");
			else kprintf("[");
		}
		kprintf("(id:%d,n:\"%s\",p:%d,s:%d,b:%d,ds:%d,de:%d)", ptask_prio->id, ptask_prio->name, ptask_prio->prio,
					ptask_prio->status, ptask_prio->block_type, (u32)ptask_prio->ticks_start, (u32)ptask_prio->ticks_alert);
		prio_mark = ptask_prio->prio;
	}
	if (!list_empty(phead)) kprintf("]\r\n");
	__local_irq_restore(irq_save);
}

void VOSTaskSchTable()
{
	s32 prio_mark = -1;
	u32 irq_save = 0;
	u32 mark = 0;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	irq_save = __local_irq_save();

	phead = &gListTaskReady;

	kprintf("[(%d) ", pRunningTask?pRunningTask->id:-1);

	kprintf("(%d) ", pReadyTask?pReadyTask->id:-1);

	//������У��������ȼ��Ӹߵ�����������
	kprintf("(");
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		kprintf("%d", ptask_prio->id);
		if (list_prio->next != phead) {
			kprintf("->");
		}
	}
	kprintf(")");

	phead = &gListTaskBlock;
	//������У��������ȼ��Ӹߵ�����������
	kprintf("(");
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		kprintf("%d", ptask_prio->id);
		if (list_prio->next != phead) {
			kprintf("->");
		}
	}
	kprintf(")]\r\n");

	__local_irq_restore(irq_save);
	return mark;
}


s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm)
{
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (list_empty(&gListTaskFree)) goto END_CREATE;
	ptask = list_entry(gListTaskFree.next, StVosTask, list); //��ȡ��һ����������
	list_del(gListTaskFree.next); //�������������ɾ����һ����������
	ptask->id = (ptask - gArrVosTask);

	ptask->prio = ptask->prio_base = prio;

	ptask->stack_size = stack_size;

	ptask->pstack_top = (u8*)pstack + stack_size;

	ptask->name = task_nm;

	ptask->param = param;

	ptask->task_fun = task_fun;

	ptask->status = VOS_STA_READY;

	ptask->psyn = 0;

	ptask->block_type = 0;//û�κ�����

	ptask->list.pTask = ptask;

	//��ʼ��ջ����
	ptask->pstack = ptask->pstack_top - (18 << 2);
	HW32_REG((ptask->pstack + (16 << 2))) = (unsigned long) VOSTaskEntry;
	HW32_REG((ptask->pstack + (17 << 2))) = 0x01000000;
	HW32_REG((ptask->pstack + (1 << 2))) = 0x03;
	HW32_REG((ptask->pstack)) = 0xFFFFFFFDUL;

	//���뵽��������
	VOSTaskListPrioInsert(ptask, VOS_LIST_READY);

END_CREATE:
	__local_irq_restore(irq_save);

	if (pRunningTask && ptask && pRunningTask->prio > ptask->prio) {
		//��������ʱ���½����������ȼ����������е����ȼ��ߣ����л�
		VOSTaskSchedule();
	}
	//����ע�⣬�������������ȼ����������ˣ�ִ�и������ȼ�����л����������ִ�С�
	return ptask ? ptask->id : -1;
}

//�������������������, ���ǰ��������з���������������ӵ���������
void VOSTaskBlockWaveUp()
{
	StVosTask *ptask_block = 0;
	struct list_head *list_block;
	struct list_head *list_temp;
	u32 irq_save = 0;
	s64 latest_ticks = MAX_SIGNED_VAL_64; //���������
	irq_save = __local_irq_save();
	//�����������У��ѷ�����������ӵ���������
	list_for_each_safe(list_block, list_temp, &gListTaskBlock) {
		ptask_block = list_entry(list_block, struct StVosTask, list);
		//����ʱ��ʱ����ʱ�䣬�����ʱ���Ը��ź��������������¼�ͬʱ������
		if (ptask_block->block_type & VOS_BLOCK_DELAY) { //�Զ�����ʱ��������
			if (gVOSTicks >= ptask_block->ticks_alert) {//��鳬ʱ
				//�Ͽ���ǰ��������
				list_del(&ptask_block->list);
				//�޸�״̬
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				ptask_block->wakeup_from = VOS_WAKEUP_FROM_DELAY;
				//��ӵ���������
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
			else if (ptask_block->ticks_alert < latest_ticks){//�ų��Ѿ���ʱ�����ӣ���û�г�ʱ����������ļ�¼����
				latest_ticks = ptask_block->ticks_alert;
			}
		}
		//����������ʱ����ͬʱ���ã�����������¼����ǵ����ġ�
		//�����ź��������¼�
		if (ptask_block->block_type & VOS_BLOCK_SEMP) { //�Զ����ź�����������
			StVOSSemaphore *pSem = (StVOSSemaphore *)ptask_block->psyn;
			if (pSem->left > 0 || pSem->distory == 1) {//������һ���ź��� �����ź�����ɾ��
				if (pSem->left > 0) pSem->left--;
				//�Ͽ���ǰ��������
				list_del(&ptask_block->list);
				//�޸�״̬
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				//ptask_block->psyn = 0;
				if (pSem->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_SEM;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_SEM_DEL;
				}
				//��ӵ���������
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
		}
		//�������������¼�
		else if (ptask_block->block_type & VOS_BLOCK_MUTEX) { //�Զ��廥������������
			StVOSMutex *pMutex = (StVOSMutex *)ptask_block->psyn;
			if (pMutex->counter > 0 || pMutex->distory == 1) {//������һ���ź��� ���߻�������ɾ��
				if (pMutex->counter > 0) {
					pMutex->counter--;
					pMutex->ptask_owner = ptask_block;
				}
				//�Ͽ���ǰ��������
				list_del(&ptask_block->list);
				//�޸�״̬
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				//ptask_block->psyn = 0;
				if (pMutex->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MUTEX;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MUTEX_DEL;
				}
				//��ӵ���������
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
		}
		//�����¼������¼�
		else if (ptask_block->block_type & VOS_BLOCK_EVENT) { //�Զ��廥������������
			if (ptask_block->event_mask == 0 || //���event_maskΪ0��������κ��¼�
					ptask_block->event_mask & ptask_block->event) { //�������õ��¼���mask���ʹ�������
				//�Ͽ���ǰ��������
				list_del(&ptask_block->list);
				//�޸�״̬
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				ptask_block->event = 0;
				ptask_block->event_mask = 0;
				ptask_block->wakeup_from = VOS_WAKEUP_FROM_EVENT;
				//��ӵ���������
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
		}
		//������Ϣ���л����¼�
		//�������������¼�
		else if (ptask_block->block_type & VOS_BLOCK_MSGQUE) { //�Զ��廥������������
			StVOSMsgQueue *pMsgQue = (StVOSMsgQueue *)ptask_block->psyn;
			if (pMsgQue->msg_cnts > 0 || pMsgQue->distory == 1) {//������һ���ź��� ���߻�������ɾ��
				if (pMsgQue->msg_cnts > 0) {
					//
				}
				//�Ͽ���ǰ��������
				list_del(&ptask_block->list);
				//�޸�״̬
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				//ptask_block->psyn = 0;
				if (pMsgQue->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MSGQUE;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MSGQUE_DEL;
				}
				//��ӵ���������
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
		}
	}
	gMarkTicksNearest = latest_ticks;


	/**/

	__local_irq_restore(irq_save);
}

void task_idle(void *param)
{
	/*
	 * ע�⣺ ���������ܼ��κ�ϵͳ��ʱ�����Լ�Ӳ��ʱ��
	 * ԭ�� ������������û����������У����¾�������ʣ��һ��ʱ�����޷����г�ȥ��
	 *       ���ʣ��һ�����������л���ȥ����ô��ǰcpu��ɶ��������Ҫ����������������
	 */
	while (1) {//��ֹ������������
		//VOSTaskSchTable();
	}
}


void VOSStarup()
{
	if (VOSRunning == 0) { //������һ������ʱ�����ø�VOSRunningΪ1
		__asm volatile ("svc 0\n");
	}
}

static void VOSCortexSwitch()
{
	if (pReadyTask) {
		return ;
	}

	pReadyTask = VOSTaskReadyCutPriorest();

	if (pReadyTask==0) {
		return;
	}

	if (pRunningTask->status == VOS_STA_FREE) {//���յ���������
		list_add_tail(&pRunningTask->list, &gListTaskFree);
	}
	else if (pRunningTask->status == VOS_STA_BLOCK) { //��ӵ���������
		VOSTaskListPrioInsert(pRunningTask, VOS_LIST_BLOCK);
	}
	else {//��ӵ��������У�������Ϊ�����������ȼ���������ͬ���ȼ�ʱ���ѵ�ǰ�����л���ȥ
		VOSTaskListPrioInsert(pRunningTask, VOS_LIST_READY);
	}

	//SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; //�����������л��ж�
	asm_ctx_switch();
}


//from==TASK_SWITCH_ACTIVE: �����л� (����Ӧ�ò�ֱ�ӷ���cpu��������ʱ�䣬����л�����)
//from==TASK_SWITCH_PASSIVE: �����л�(����ʱ�Ӷ�ʱ���л��������жϷ����л�����SVC),��ͬ������Ҫ�鿴ʱ��Ƭ��
void VOSTaskSwitch(u32 from)
{
	s32 ret = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (pRunningTask && !list_empty(&gListTaskReady)) {
		ret = VOSTaskReadyCmpPrioTo(pRunningTask); //�Ƚ������������ȼ���������������������ȼ�
		if (from == TASK_SWITCH_ACTIVE) {//�����л����񣬲���Ҫ�ж�ʱ��Ƭ�����ݣ�ֱ���л�
			if (ret <= 0 || //�������е��������ȼ���ͬ����ͣ���Ҫ�����л�
				pRunningTask->status == VOS_STA_FREE || //����״̬�����FREE״̬��Ҳ��Ҫ�л�
				pRunningTask->status == VOS_STA_BLOCK //������ӵ���������
				) {
				VOSCortexSwitch();
			}
		}
		else if (from == TASK_SWITCH_PASSIVE) {//ϵͳ��ʱ�л��������л�
			if (ret < 0 || //ret < 0: ���������и������ȼ���ǿ���л�
				pRunningTask->status == VOS_STA_FREE || //���ͷŵ�������ǿ���л�
				pRunningTask->status == VOS_STA_BLOCK || //������Ҫ��ӵ���������
				(ret == 0 && pRunningTask->ticks_timeslice == 0 )//ret == 0 && ticks_timeslice==0:ʱ��Ƭ���꣬ͬʱ������������ͬ���ȼ���Ҳ���л�
			)
			{
				VOSCortexSwitch();
			}
			if (pRunningTask->ticks_timeslice > 0) { //ʱ��Ƭ����Ϊ��������һ����ͬ���ȼ���������������У��Ϳ��������л�
				pRunningTask->ticks_timeslice--;
			}
		}
		else {//��֧�֣����������Ϣ

		}
	}
	__local_irq_restore(irq_save);
}

void  VOSIntEnter()
{
	u32 irq_save = 0;
    if (VOSRunning) {
    	irq_save = __local_irq_save();
        if (VOSIntNesting < 0xFFFFFFFFU) {
        	VOSIntNesting++;                      /* Increment ISR nesting level                        */
        }
        __local_irq_restore(irq_save);
    }
}

void  VOSIntExit ()
{
	u32 irq_save = 0;

    if (VOSRunning) {
    	irq_save = __local_irq_save();
        if (VOSIntNesting) VOSIntNesting--;
        if (VOSIntNesting == 0) {
        	VOSTaskSwitch(TASK_SWITCH_PASSIVE);
        	//asm_ctx_switch();
        }
        __local_irq_restore(irq_save);
    }
}

//����IDLE�������ۺ�ʱ�������и�IDLE�����ھ�������
void TaskIdleBuild()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	VOSTaskCreate(task_idle, 0, stack_idle, sizeof(stack_idle), TASK_PRIO_MAX, "idle");
	if (!list_empty(&gListTaskReady)) {
		pRunningTask = list_entry(gListTaskReady.next, StVosTask, list); //��ȡ��һ����������
		list_del(gListTaskReady.next); //�������������ɾ����һ����������
	}
	__local_irq_restore(irq_save);
}

//����������û�ģʽ��ʹ�ã��л�������
//����VOSTaskSwitch��������Ȩģʽ�·��ʣ������쳣��
void VOSTaskSchedule()
{
	__asm volatile ("svc 1\n");
}


void VOSSysTick()
{
	u32 irq_save;
	irq_save = __local_irq_save();
	gVOSTicks++;
	__local_irq_restore(irq_save);
	if (VOSRunning && gVOSTicks >= gMarkTicksNearest) {//�����죬�����������У��Ѷ�Ӧ������������ӵ�����������
		VOSTaskBlockWaveUp();
	}
}



