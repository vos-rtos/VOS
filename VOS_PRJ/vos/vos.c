//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "vtype.h"
#include "vos.h"
#include "vlist.h"

struct StVosTask gArrVosTask[MAX_VOSTASK_NUM]; //�������飬ռ�ÿռ�

struct list_head gListTaskReady;//���������������

struct list_head gListTaskFree;//�������񣬿��Լ����������

struct list_head gListTaskBlock;//�����������

struct StVosTask *pRunningTask; //�������е�����

struct StVosTask *pReadyTask; //׼��Ҫ�л�������

volatile s64  gVOSTicks = 0;

volatile s64 gMarkTicksNearest = MAX_SIGNED_VAL_64; //��¼���������

volatile u32 VOSIntNesting = 0;

volatile u32 VOSRunning = 0;

u32 VOSCtxSwitching = 0; //

volatile u32 vos_dis_irq_counter = 0;//��¼����irq disable����


long long stack_idle[1024];

u32 VOSUserIRQSave();
void VOSUserIRQRestore(u32 save);


volatile u32 flag_cpu_collect = 0; //�Ƿ��ڲɼ�cpuռ����

//Ŀǰ�����������ͳ������ʹ��cpuռ���ʼ���
//��ǰ����׼���л���ȥǰ��
void HookTaskSwitchOut()
{
	if (VOSRunning && pRunningTask->ticks_used_start != -1) {
		if (gVOSTicks - pRunningTask->ticks_used_start > 0) {
			pRunningTask->ticks_used_cnts += gVOSTicks - pRunningTask->ticks_used_start;
		}
	}
}
//Ŀǰ�����������ͳ������ʹ��cpuռ���ʼ���
void HookTaskSwitchIn()
{
	if (VOSRunning && pRunningTask->ticks_used_start != -1) {
		pRunningTask->ticks_used_start = gVOSTicks;
	}
}

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

	phead = &gListTaskReady;
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		ptask_prio->ticks_used_start = gVOSTicks;
	}
	phead = &gListTaskBlock;
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		ptask_prio->ticks_used_start = gVOSTicks;
	}

	flag_cpu_collect = 1; //���òɼ���־

	__local_irq_restore(irq_save);
}

//mode: 0; ����������
//mode: 1; ��������ʱ���ۼӲɼ���
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

	phead = &gListTaskReady;
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
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
	phead = &gListTaskBlock;
	//������У��������ȼ��Ӹߵ�����������
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
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



s64 VOSGetTicks()
{
	s64 ticks;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	ticks = gVOSTicks;
	__vos_irq_restore(irq_save);
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

	TaskIdleBuild();//����idle����

	return 0;
}

StVosTask *VOSGetTaskFromId(s32 task_id)
{
	if (task_id < 0 || task_id >= MAX_VOSTASK_NUM) return 0;
	return &gArrVosTask[task_id];
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
		pRunningTask->task_fun(pRunningTask->param);
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
	irq_save = __vos_irq_save();
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
	__vos_irq_restore(irq_save);
}

//�������񣬲��������������Ĵ���(����������ʹ��VOSTaskCreate)
s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm)
{
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	if (list_empty(&gListTaskFree)) goto END_CREATE;
	ptask = list_entry(gListTaskFree.next, StVosTask, list); //��ȡ��һ����������
	list_del(gListTaskFree.next); //�������������ɾ����һ����������
	ptask->id = (ptask - gArrVosTask);

	ptask->prio = ptask->prio_base = prio;

	ptask->stack_size = stack_size;

	ptask->pstack_top = (u32*)((u8*)pstack + stack_size);

	if (pstack && stack_size) {
		memset(pstack, 0x64, stack_size); //��ʼ��ջ����Ϊ064, ջ���ʹ��
	}
	ptask->name = task_nm;

	ptask->param = param;

	ptask->task_fun = task_fun;

	ptask->status = VOS_STA_READY;

	ptask->psyn = 0;

	ptask->block_type = 0;//û�κ�����

	ptask->ticks_used_start = -1; //��ֹͳ��cpuʹ����

	ptask->list.pTask = ptask;

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
	VOSTaskListPrioInsert(ptask, VOS_LIST_READY);

END_CREATE:
	__vos_irq_restore(irq_save);

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
		switch (ptask_block->block_type & BLOCK_SUB_MASK) {
			case VOS_BLOCK_SEMP: //�Զ����ź�����������
			{
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
				break;
			}
			//�������������¼�
			case VOS_BLOCK_MUTEX: //�Զ��廥������������
			{
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
				break;
			}
			//�����¼������¼�
			case VOS_BLOCK_EVENT:
			{ //�Զ��廥������������
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
				break;
			}
			//������Ϣ���л����¼�
			//�������������¼�
			case VOS_BLOCK_MSGQUE:
			{//�Զ��廥������������
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
				break;
			}
			case 0: //ֻ����ʱVOS_BLOCK_DELAY�����
				break;
			default:
				kprintf("ERROR: CAN NOT BE HERE!\r\n");
				while(1) ;
				break;
		}
	}
	gMarkTicksNearest = latest_ticks;

	__local_irq_restore(irq_save);
}

void task_idle(void *param)
{
	/*
	 * ע�⣺ ���������ܼ��κ�ϵͳ��ʱ�����Լ�Ӳ��ʱ��
	 * ԭ�� ������������û����������У����¾�������ʣ��һ��ʱ�����޷����г�ȥ��
	 *       ���ʣ��һ�����������л���ȥ����ô��ǰcpu��ɶ��������Ҫ����������������
	 */
	static s64 mark_time=0;
	mark_time = VOSGetTimeMs();
	while (1) {//��ֹ������������
//		if ((s32)(VOSGetTimeMs()-mark_time) > 1000) {
//			mark_time = VOSGetTimeMs();
//			SysCallVOSTaskSchTabDebug();
//		}
	}
}


void VOSStarup()
{
	if (VOSRunning == 0) { //������һ������ʱ�����ø�VOSRunningΪ1
		RunFirstTask(); //���ص�һ��������ʱ����һ����IDLE����
		while (1) ;;//�������ܵ�����
	}
}

static void VOSCortexSwitch()
{
	u32 irq_save = 0;
	if (pReadyTask) {
		return ;
	}

	pReadyTask = VOSTaskReadyCutPriorest();

	if (pReadyTask==0) {
		return;
	}
	irq_save = __local_irq_save();
	switch(pRunningTask->status) {
		case VOS_STA_FREE: //���յ���������
		{
			//�������id��Ŵ�С�������
			//__local_irq_restore(irq_save);
			list_add_tail(&pRunningTask->list, &gListTaskFree);
			//irq_save = __local_irq_save();
			break;
		}
		case VOS_STA_BLOCK: //��ӵ���������
		{
			__local_irq_restore(irq_save);
			VOSTaskListPrioInsert(pRunningTask, VOS_LIST_BLOCK);
			irq_save = __local_irq_save();
			break;
		}
		default://��ӵ��������У�������Ϊ�����������ȼ���������ͬ���ȼ�ʱ���ѵ�ǰ�����л���ȥ
		{	__local_irq_restore(irq_save);
			VOSTaskListPrioInsert(pRunningTask, VOS_LIST_READY);
			irq_save = __local_irq_save();
			break;
		}
	}
	__local_irq_restore(irq_save);
	//SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; //�����������л��ж�
	asm_ctx_switch();
}


//from==TASK_SWITCH_ACTIVE: �����л� (����Ӧ�ò�ֱ�ӷ���cpu��������ʱ�䣬����л�����)
//from==TASK_SWITCH_PASSIVE: �����л�(����ʱ�Ӷ�ʱ���л��������жϷ����л�����SVC),��ͬ������Ҫ�鿴ʱ��Ƭ��
void VOSTaskSwitch(u32 from)
{
	s32 ret = 0;
	u32 irq_save = 0;
	if (VOSRunning==0) return; //��û��ʼ���л�ֱ�ӷ���
	irq_save = __local_irq_save();
	if (pRunningTask && !list_empty(&gListTaskReady)) {
		ret = VOSTaskReadyCmpPrioTo(pRunningTask); //�Ƚ������������ȼ���������������������ȼ�
		switch(from)
		{
			case TASK_SWITCH_ACTIVE://�����л����񣬲���Ҫ�ж�ʱ��Ƭ�����ݣ�ֱ���л�
			{
				if (ret <= 0 || //�������е��������ȼ���ͬ����ͣ���Ҫ�����л�
					pRunningTask->status == VOS_STA_FREE || //����״̬�����FREE״̬��Ҳ��Ҫ�л�
					pRunningTask->status == VOS_STA_BLOCK //������ӵ���������
					) {
					__local_irq_restore(irq_save);
					VOSCortexSwitch();
					irq_save = __local_irq_save();
				}
				break;
			}
			case TASK_SWITCH_PASSIVE://ϵͳ��ʱ�л��������л�
			{
				if (ret < 0 || //ret < 0: ���������и������ȼ���ǿ���л�
					pRunningTask->status == VOS_STA_FREE || //���ͷŵ�������ǿ���л�
					pRunningTask->status == VOS_STA_BLOCK || //������Ҫ��ӵ���������
					(ret == 0 && pRunningTask->ticks_timeslice == 0 )//ret == 0 && ticks_timeslice==0:ʱ��Ƭ���꣬ͬʱ������������ͬ���ȼ���Ҳ���л�
				)
				{
					__local_irq_restore(irq_save);
					VOSCortexSwitch();
					irq_save = __local_irq_save();
				}
				if (pRunningTask->ticks_timeslice > 0) { //ʱ��Ƭ����Ϊ��������һ����ͬ���ȼ���������������У��Ϳ��������л�
					pRunningTask->ticks_timeslice--;
				}
				break;
			}
			default: //��֧�֣����������Ϣ
				kprintf("ERROR: VOSTaskSwitch\r\n");
				while(1) ;
				break;
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
        	VOSIntNesting++;
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
        //__local_irq_restore(irq_save);
        if (VOSIntNesting == 0) {
        	VOSTaskSwitch(TASK_SWITCH_PASSIVE);
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
	//u32 irq_save;
	//irq_save = __vos_irq_save();
	VOSTaskSwitch(TASK_SWITCH_ACTIVE);
	//__vos_irq_restore(irq_save);
}

void VOSTaskSchTabDebug()
{
	s32 prio_mark = -1;
	u32 mark = 0;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	u32 irq_save;
	irq_save = __vos_irq_save();
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
	__vos_irq_restore(irq_save);
}

//���Ŀǰ�������ж������Ļ������������ġ�
s32 VOSCortexCheck()
{
	s32 status;
	if (VOSRunning) {
		status = VOSIntNesting ? VOS_RUNNING_BY_INTERRUPTE:VOS_RUNNING_BY_TASK;
	}
	else {
		status = VOS_RUNNING_BY_STARTUP;
	}
	return status;
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

u32 VOSTaskDelay(u32 ms)
{
	//����жϱ��رգ�ϵͳ��������ȣ���ֱ��Ӳ��ʱ
	if (VOSIntNesting > 0) {
		VOSDelayUs(ms*1000);
		return 0;
	}
	//����������ϵͳ��������ʱ
	u32 irq_save = 0;

	if (ms) {//������ʱ����
		irq_save = __vos_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
		}
		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
		__vos_irq_restore(irq_save);
	}
	//msΪ0���л�����, ����VOS_STA_BLOCK_DELAY��־����������
	VOSTaskSchedule();
	return 0;
}





