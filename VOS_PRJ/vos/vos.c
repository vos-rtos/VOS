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

s64 VOSGetTimeUs()
{
	s64 ticks = 0;
	ticks = VOSGetTicks();
	return ticks * TICKS_INTERVAL_US;
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
		INIT_LIST_HEAD(&gArrVosTask[i].list_sib);//��ʼ���ֵܽڵ㣬�Լ�ָ���Լ���
		gArrVosTask[i].status = VOS_STA_FREE;
	}
	__local_irq_restore(irq_save);
	return 0;
}

StVosTask *VOSGetTaskFromId(s32 task_id)
{
	if (task_id < 0 || task_id >= MAX_VOSTASK_NUM) return 0;
	return &gArrVosTask[task_id];
}

u32 VOSTaskDelay(u32 us)
{
	//����жϱ��رգ�ϵͳ��������ȣ���ֱ��Ӳ��ʱ
	//todo
	//����������ϵͳ��������ʱ
	u32 irq_save = 0;

	if (us) {//������ʱ����
		irq_save = __local_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(us);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ

		__local_irq_restore(irq_save);
	}
	//usΪ0���л�����, ����VOS_STA_BLOCK_DELAY��־����������
	VOSTaskSchedule();

}


//����������뵽���ȼ�����������
u32 VOSTaskReadyInsert(StVosTask *pReadyTask)
{
	StVosTask *ptask_prio = 0;
	StVosTask *ptask_sib = 0;
	struct list_head *list_prio;
	struct list_head *list_sib;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, &gListTaskReady) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		if (pReadyTask->prio < ptask_prio->prio) {//��ֵԽС�����ȼ�Խ��
			list_add_tail(&pReadyTask->list, list_prio);
			goto END_INSERT;
		}
		else if (pReadyTask->prio == ptask_prio->prio){//��ֵ��ͬ�����ȼ���ͬ�����뵽�ֵ������У�����ʱ��Ƭ��ѯ
			list_add_tail(&pReadyTask->list_sib, &ptask_prio->list_sib);
			goto END_INSERT;
		}
		else {//��ֵԽ�����ȼ�Խ�ͣ���������

		}
	}
	//�Ҳ�������λ�ã���ֱ�Ӳ鵽���ȼ���������
	list_add_tail(&pReadyTask->list, &gListTaskReady);

END_INSERT:
	__local_irq_restore(irq_save);
	return 0;
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
	StVosTask *ptask_sib = 0;
	struct list_head *list_sib;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//������У��������ȼ��Ӹߵ�����������
	if (!list_empty(&gListTaskReady)) {
		ptask_prio = list_entry(gListTaskReady.next, struct StVosTask, list);
		list_del(gListTaskReady.next); //�������������ɾ����һ����������
		if (!list_empty(&ptask_prio->list_sib)){//���ûͬ���ȼ��ֵ�����ֱ�Ӷϵ�
			//�Ѿ��Ͽ����ȼ���ͬ�Ķ��У����ڶϿ��ֵܶ���
			list_sib = ptask_prio->list_sib.next; //�����¸��ֵܽڵ㣬׼�����뵽���ȼ�������
			list_del(&ptask_prio->list_sib);
			INIT_LIST_HEAD(&ptask_prio->list_sib);//��ʼ���ֵܽڵ㣬�Լ�ָ���Լ���
			//�Ѻ����ֵܶ�����ӵ����ȼ�������
			ptask_sib = list_entry(list_sib, struct StVosTask, list_sib);
			list_add(&ptask_sib->list, &gListTaskReady); //��ӵ����ȼ��б���
		}
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

StVosTask *VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, u8 *task_nm)
{
	StVosTask *pNewTask = 0;
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (list_empty(&gListTaskFree)) goto END_CREATE;
	ptask = list_entry(gListTaskFree.next, StVosTask, list); //��ȡ��һ����������
	list_del(gListTaskFree.next); //�������������ɾ����һ����������
	ptask->id = (ptask - gArrVosTask);

	ptask->prio = prio;

	ptask->stack_size = stack_size;

	ptask->pstack_top = (u8*)pstack + stack_size;

	ptask->name = task_nm;

	ptask->param = param;

	ptask->task_fun = task_fun;

	ptask->status = VOS_STA_READY;

	ptask->block_type = 0;//û�κ�����

	//��ʼ��ջ����
	ptask->pstack = ptask->pstack_top - (18 << 2);
	HW32_REG((ptask->pstack + (16 << 2))) = (unsigned long) VOSTaskEntry;
	HW32_REG((ptask->pstack + (17 << 2))) = 0x01000000;
	HW32_REG((ptask->pstack)) = 0xFFFFFFFDUL;
	HW32_REG((ptask->pstack + (1 << 2))) = 0x03;

	VOSTaskReadyInsert(ptask);

	pNewTask = ptask;

END_CREATE:
	__local_irq_restore(irq_save);
	return pNewTask;
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
				VOSTaskReadyInsert(ptask_block);
			}
			else if (ptask_block->ticks_alert < latest_ticks){//�ų��Ѿ���ʱ�����ӣ���û�г�ʱ����������ļ�¼����
				latest_ticks = ptask_block->ticks_alert;
			}
		}
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
				ptask_block->psyn = 0;
				if (pSem->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_SEM;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_SEM_DEL;
				}
				//��ӵ���������
				VOSTaskReadyInsert(ptask_block);
			}
		}
		//�������������¼�
		if (ptask_block->block_type & VOS_BLOCK_MUTEX) { //�Զ��廥������������
			StVOSMutex *pMutex = (StVOSMutex *)ptask_block->psyn;
			if (pMutex->counter > 0 || pMutex->distory == 1) {//������һ���ź��� ���߻�������ɾ��
				if (pMutex->counter > 0) pMutex->counter--;
				//�Ͽ���ǰ��������
				list_del(&ptask_block->list);
				//�޸�״̬
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				ptask_block->psyn = 0;
				if (pMutex->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MUTEX;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MUTEX_DEL;
				}
				//��ӵ���������
				VOSTaskReadyInsert(ptask_block);
			}
		}
		//�����¼������¼�
		if (ptask_block->block_type & VOS_BLOCK_EVENT) { //�Զ��廥������������
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
				VOSTaskReadyInsert(ptask_block);
			}
		}
		//������Ϣ���л����¼�
		//�������������¼�
		if (ptask_block->block_type & VOS_BLOCK_MSGQUE) { //�Զ��廥������������
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
				ptask_block->psyn = 0;
				if (pMsgQue->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MSGQUE;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MSGQUE_DEL;
				}
				//��ӵ���������
				VOSTaskReadyInsert(ptask_block);
			}
		}
	}
	gMarkTicksNearest = latest_ticks;
	__local_irq_restore(irq_save);
}

void task_idle(void *param)
{
	while (1) {
		VOSTaskDelay(1000);
	}
}

void VOSStarup()
{
	__asm volatile ("svc 0\n");
}


#include "cmsis/stm32f4xx.h"
#include "stm32f4-hal/stm32f4xx_hal.h"


void VOSCortexSwitch()
{
	pReadyTask = VOSTaskReadyCutPriorest();

	if (pRunningTask->status == VOS_STA_FREE) {//���յ���������
		list_add_tail(&pRunningTask->list, &gListTaskFree);
	}
	else if (pRunningTask->status == VOS_STA_BLOCK) { //��ӵ���������
		list_add_tail(&pRunningTask->list, &gListTaskBlock);
	}
	else {//��ӵ��������У�������Ϊ�����������ȼ���������ͬ���ȼ�ʱ���ѵ�ǰ�����л���ȥ
		VOSTaskReadyInsert(pRunningTask);
	}

	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; //�����������л��ж�
}


//from==TASK_USER_SWITCH: Ӧ�ò������л�
//from==TASK_USER_SYSTICK: ʱ�Ӷ�ʱ���л��������л�
void VOSTaskSwitch(u32 from)
{
	s32 ret = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (pRunningTask && !list_empty(&gListTaskReady)) {
		ret = VOSTaskReadyCmpPrioTo(pRunningTask); //�Ƚ������������ȼ���������������������ȼ�
		if (from == TASK_SWITCH_USER) {//�����л����񣬲���Ҫ�ж�ʱ��Ƭ�����ݣ�ֱ���л�
			if (ret <= 0 || //�������е��������ȼ���ͬ����ͣ���Ҫ�����л�
				pRunningTask->status == VOS_STA_FREE || //����״̬�����FREE״̬��Ҳ��Ҫ�л�
				pRunningTask->status == VOS_STA_BLOCK //������ӵ���������
				) {
				VOSCortexSwitch();
			}
		}
		else if (from == TASK_SWITCH_SYSTICK) {//ϵͳ��ʱ�л��������л�
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
//����IDLE�������ۺ�ʱ�������и�IDLE�����ھ�������
static void TaskIdleBuild()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	VOSTaskCreate(task_idle, 0, stack_idle, sizeof(stack_idle), 500, "idle");
	if (!list_empty(&gListTaskReady)) {
		pRunningTask = list_entry(gListTaskReady.next, StVosTask, list); //��ȡ��һ����������
		list_del(gListTaskReady.next); //�������������ɾ����һ����������
	}

	SVC_EXC_RETURN = HW32_REG((pRunningTask->pstack));
	_set_PSP((pRunningTask->pstack + 10 * 4));//ջָ��R0
	NVIC_SetPriority(PendSV_IRQn, 0xFF);
	SysTick_Config(168000);
	_set_CONTROL(0x3);
	_ISB();
	__local_irq_restore(irq_save);
}

//����������û�ģʽ��ʹ�ã��л�������
//����VOSTaskSwitch��������Ȩģʽ�·��ʣ������쳣��
void VOSTaskSchedule()
{
	__asm volatile ("svc 1\n");
}

extern void _set_CONTROL(u32 val);

void SVC_Handler_C(unsigned int *svc_args);

void SVC_Handler_C(unsigned int *svc_args)
{
	u8 svc_number;
	svc_number = ((char *)svc_args[6])[-2];
	switch(svc_number) {
	case 0://ϵͳ�ճ�ʼ����ɣ�������һ������
		TaskIdleBuild();
		break;
	case 1://�û��������������л����������ȼ��������û��������û�����
		VOSTaskSwitch(TASK_SWITCH_USER);
		break;
	default:
		break;
	}
}


void __attribute__ ((section(".after_vectors")))
SysTick_Handler()
{
	gVOSTicks++;

	if (gVOSTicks >= gMarkTicksNearest) {//�����죬�����������У��Ѷ�Ӧ������������ӵ�����������
		VOSTaskBlockWaveUp();
	}
#if 1
	VOSTaskSwitch(TASK_SWITCH_SYSTICK);
#endif
}

