//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "vtype.h"
#include "vos.h"
#include "list.h"

struct StVosTask gArrVosTask[MAX_VOSTASK_NUM]; //任务数组，占用空间

struct list_head gListTaskReady;//就绪队列里的任务

struct list_head gListTaskFree;//空闲任务，可以继续申请分配

struct list_head gListTaskBlock;//阻塞任务队列

struct StVosTask *pRunningTask; //正在运行的任务

struct StVosTask *pReadyTask; //准备要切换的任务

volatile s64  gVOSTicks = 0;

volatile s64 gMarkTicksNearest = MAX_SIGNED_VAL_64; //记录最近闹钟响

u32 SVC_EXC_RETURN; //SVC进入后保存，然后返回时要用到 (cortex m4)


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
	//初始化就绪任务队列和空闲任务队列
	INIT_LIST_HEAD(&gListTaskReady);
	INIT_LIST_HEAD(&gListTaskFree);
	INIT_LIST_HEAD(&gListTaskBlock);
	//把所有任务链接到空闲任务队列中
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
	//如果中断被关闭，系统不进入调度，则直接硬延时
	//todo
	//否则进入操作系统的闹钟延时
	u32 irq_save = 0;

	if (ms) {//进入延时设置
		irq_save = __local_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //如果闹钟结点小于记录的最少值，则更新
			gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
		}
		pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时

		__local_irq_restore(irq_save);
	}
	//ms为0，切换任务, 或者VOS_STA_BLOCK_DELAY标志，呼唤调度
	VOSTaskSchedule();
	return 0;
}
//把就绪队列里的所有指向相同互斥控制块（只处理互斥锁）的任务提升到跟准备阻塞前的当前任务一样
//如果当前任务优先级不如拥有者任务，则不处理
//该函数不可以在中断里调用
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
	//互斥锁拥有者可能在就绪队列里面，也有可能在阻塞队列里面，如果在阻塞队列里，
	//证明锁拥有者先获取到这个锁后在申请别的锁（信号量等）或延时进入阻塞队列
	if (pMutexOwnerTask->status == VOS_STA_READY) {//在就绪队列里（优先级排序队列）
		list_head = &gListTaskReady;
	}
	else if (pMutexOwnerTask->status == VOS_STA_BLOCK) {//在阻塞队列里（优先级排序队列）
		list_head = &gListTaskBlock;
	}
	else {
		goto END_RAISE_PRIO;
	}

	list_dest = list_owner = &pMutexOwnerTask->list;
	//优先级冒泡提升
	while (list_dest->prev != list_head) {
		list_dest = list_dest->prev;
		ptask_dest = list_entry(list_dest, struct StVosTask, list);
		if (ptask_dest->prio <= pMutexOwnerTask->prio) {//找到要插入的位置，在这个位置后面插入提升的任务
			if (list_dest != list_owner->prev) {//如果提升任务的优先级已经是适合的位置就不做任何链表操作。
				list_move(list_owner, list_dest);
			}
			break;//跳出提升任务的循环，继续遍历后面的就绪任务，可能多个任务指向阻塞控制量，都要提升
		}
	}
	if (list_dest->prev == list_head) {//插入到第一个位置
		if (list_dest != list_owner) {//如果提升任务的优先级已经是适合的位置就不做任何链表操作。
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

//把当前任务插入到任务队列里，按优先高低排列
//高优先级插入到队列头，如果相同优先级，则插入到相同优先级的最后，
//这样的优先级队列主要是为了查找时，从头开始查找，如果找到就释放同步信号。
//保证队列里优先级最高的有限获取资源的能力
u32 VOSTaskListPrioInsert(StVosTask *pTask, s32 which_list)
{
	s32 ret = -1;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	u32 irq_save = 0;

	switch (which_list) {
	case VOS_LIST_READY:
		phead = &gListTaskReady;
		break;
	case VOS_LIST_BLOCK:
		phead = &gListTaskBlock;
		break;
	default:
		return -1;
	}
	irq_save = __local_irq_save();
	//插入队列，必须优先级从高到低有序排列
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		if (ptask_prio->prio > pTask->prio) {//数值越小，优先级越高，如果相同优先级，则插入到所有相同优先级后面
			list_add_tail(&pTask->list, list_prio);
			ret = 0;
			break;
		}
	}
	if (list_prio == phead) {//找不到合适位置，就直接插入到链表的最后
		list_add_tail(&pTask->list, phead);
	}
	__local_irq_restore(irq_save);

	return ret;
}

//检查就绪链表是否有比当前运行的优先级更高
//返回正数：证明正运行任务的优先级更高（数值更小），不需要调度
//返回负数：证明正运行任务的优先级更低（数值更大），需要调度
//返回 0  : 证明正运行任务的优先级相同， 需要调度，基于时间片
s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask)
{
	s32 ret = 1;//默认返回整数，代表不用调度
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

//获取当前就绪队列中优先级最高的任务
StVosTask *VOSTaskReadyCutPriorest()
{
	StVosTask *ptask_prio = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//插入队列，必须优先级从高到低有序排列
	if (!list_empty(&gListTaskReady)) {
		ptask_prio = list_entry(gListTaskReady.next, struct StVosTask, list);
		list_del(gListTaskReady.next); //空闲任务队列里删除第一个空闲任务
		ptask_prio->ticks_timeslice = MAX_TICKS_TIMESLICE;
	}
	__local_irq_restore(irq_save);
	return ptask_prio;
}
//用户栈PSP状态，切换需要用svn 1
void VOSTaskEntry(void *param)
{
	if (pRunningTask) {
		pRunningTask->task_fun(param);
	}
	//将任务回收到空闲任务链表中
	pRunningTask->status = VOS_STA_FREE;
	VOSTaskSchedule();
}

void VOSTaskPrtList(s32 which_list)
{
	s32 prio_mark = -1;
	u32 irq_save = 0;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	switch (which_list) {
	case VOS_LIST_READY:
		phead = &gListTaskReady;
		kprintf("Ready List Infomation: \r\n");
		break;
	case VOS_LIST_BLOCK:
		phead = &gListTaskBlock;
		kprintf("Block List Infomation: \r\n");
		break;
	default:
		return ;
	}
	irq_save = __local_irq_save();
	//插入队列，必须优先级从高到低有序排列
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);

		if (prio_mark == ptask_prio->prio) {
			kprintf("(id:%d,p:%d,n:\"%s\")", ptask_prio->id, ptask_prio->prio, ptask_prio->name);
		}
		else {
			if (prio_mark != -1) kprintf("]");
			kprintf("[(id:%d,p:%d,n:\"%s\")", ptask_prio->id, ptask_prio->prio, ptask_prio->name);
		}
		prio_mark = ptask_prio->prio;
	}
	kprintf("]\r\n");
	__local_irq_restore(irq_save);
}

s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm)
{
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (list_empty(&gListTaskFree)) goto END_CREATE;
	ptask = list_entry(gListTaskFree.next, StVosTask, list); //获取第一个空闲任务
	list_del(gListTaskFree.next); //空闲任务队列里删除第一个空闲任务
	ptask->id = (ptask - gArrVosTask);

	ptask->prio = ptask->prio_base = prio;

	ptask->stack_size = stack_size;

	ptask->pstack_top = (u8*)pstack + stack_size;

	ptask->name = task_nm;

	ptask->param = param;

	ptask->task_fun = task_fun;

	ptask->status = VOS_STA_READY;

	ptask->psyn = 0;

	ptask->block_type = 0;//没任何阻塞

	//初始化栈内容
	ptask->pstack = ptask->pstack_top - (18 << 2);
	HW32_REG((ptask->pstack + (16 << 2))) = (unsigned long) VOSTaskEntry;
	HW32_REG((ptask->pstack + (17 << 2))) = 0x01000000;
	HW32_REG((ptask->pstack)) = 0xFFFFFFFDUL;
	HW32_REG((ptask->pstack + (1 << 2))) = 0x03;
	//插入到就绪队列
	VOSTaskListPrioInsert(ptask, VOS_LIST_READY);

END_CREATE:
	__local_irq_restore(irq_save);

	if (pRunningTask && ptask && pRunningTask->prio > ptask->prio) {
		//创建任务时，新建的任务优先级比正在运行的优先级高，则切换
		VOSTaskSchedule();
	}
	//这里注意，可能任务被新优先级的任务抢了，执行更高优先级后才切换到这里继续执行。
	return ptask ? ptask->id : -1;
}

//唤醒阻塞队列里的任务, 就是把阻塞队列符合条件的任务添加到就绪队列
void VOSTaskBlockWaveUp()
{
	StVosTask *ptask_block = 0;
	struct list_head *list_block;
	struct list_head *list_temp;
	u32 irq_save = 0;
	s64 latest_ticks = MAX_SIGNED_VAL_64; //最近的闹钟
	irq_save = __local_irq_save();
	//遍历阻塞队列，把符合条件的添加到就绪队列
	list_for_each_safe(list_block, list_temp, &gListTaskBlock) {
		ptask_block = list_entry(list_block, struct StVosTask, list);
		//处理定时或超时唤醒时间，这个超时可以跟信号量，互斥锁，事件同时发生。
		if (ptask_block->block_type & VOS_BLOCK_DELAY) { //自定义延时阻塞类型
			if (gVOSTicks >= ptask_block->ticks_alert) {//检查超时
				//断开当前阻塞队列
				list_del(&ptask_block->list);
				//修改状态
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				ptask_block->wakeup_from = VOS_WAKEUP_FROM_DELAY;
				//添加到就绪队列
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
			else if (ptask_block->ticks_alert < latest_ticks){//排除已经超时的闹钟，把没有超时的闹钟最近的记录下来
				latest_ticks = ptask_block->ticks_alert;
			}
		}
		//除了上面延时可以同时设置，下面的阻塞事件都是单独的。
		//处理信号量唤醒事件
		if (ptask_block->block_type & VOS_BLOCK_SEMP) { //自定义信号量阻塞类型
			StVOSSemaphore *pSem = (StVOSSemaphore *)ptask_block->psyn;
			if (pSem->left > 0 || pSem->distory == 1) {//有至少一个信号量 或者信号量被删除
				if (pSem->left > 0) pSem->left--;
				//断开当前阻塞队列
				list_del(&ptask_block->list);
				//修改状态
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
				//添加到就绪队列
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
		}
		//处理互斥锁唤醒事件
		else if (ptask_block->block_type & VOS_BLOCK_MUTEX) { //自定义互斥锁阻塞类型
			StVOSMutex *pMutex = (StVOSMutex *)ptask_block->psyn;
			if (pMutex->counter > 0 || pMutex->distory == 1) {//有至少一个信号量 或者互斥锁被删除
				if (pMutex->counter > 0) {
					pMutex->counter--;
					pMutex->ptask_owner = ptask_block;
				}
				//断开当前阻塞队列
				list_del(&ptask_block->list);
				//修改状态
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
				//添加到就绪队列
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
		}
		//处理事件唤醒事件
		else if (ptask_block->block_type & VOS_BLOCK_EVENT) { //自定义互斥锁阻塞类型
			if (ptask_block->event_mask == 0 || //如果event_mask为0，则接受任何事件
					ptask_block->event_mask & ptask_block->event) { //或者设置的事件被mask到就触发操作
				//断开当前阻塞队列
				list_del(&ptask_block->list);
				//修改状态
				ptask_block->status = VOS_STA_READY;
				ptask_block->block_type = 0;
				ptask_block->ticks_start = 0;
				ptask_block->ticks_alert = 0;
				ptask_block->event = 0;
				ptask_block->event_mask = 0;
				ptask_block->wakeup_from = VOS_WAKEUP_FROM_EVENT;
				//添加到就绪队列
				VOSTaskListPrioInsert(ptask_block, VOS_LIST_READY);
			}
		}
		//处理消息队列唤醒事件
		//处理互斥锁唤醒事件
		else if (ptask_block->block_type & VOS_BLOCK_MSGQUE) { //自定义互斥锁阻塞类型
			StVOSMsgQueue *pMsgQue = (StVOSMsgQueue *)ptask_block->psyn;
			if (pMsgQue->msg_cnts > 0 || pMsgQue->distory == 1) {//有至少一个信号量 或者互斥锁被删除
				if (pMsgQue->msg_cnts > 0) {
					//
				}
				//断开当前阻塞队列
				list_del(&ptask_block->list);
				//修改状态
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
				//添加到就绪队列
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
	 * 注意： 空闲任务不能加任何系统延时，可以加硬延时。
	 * 原因： 如果空闲任务被置换到阻塞队列，导致就绪任务剩下一个时，就无法被切出去。
	 *       如果剩下一个就绪任务被切换出去，那么当前cpu跑啥东西，需要空闲任务不能阻塞。
	 */
	while (1) {//禁止空闲任务阻塞
		;
	}
}


void VOSStarup()
{
	__asm volatile ("svc 0\n");
}


#include "cmsis/stm32f4xx.h"
#include "stm32f4-hal/stm32f4xx_hal.h"


static void VOSCortexSwitch()
{
	pReadyTask = VOSTaskReadyCutPriorest();

	if (pRunningTask->status == VOS_STA_FREE) {//回收到空闲链表
		list_add_tail(&pRunningTask->list, &gListTaskFree);
	}
	else if (pRunningTask->status == VOS_STA_BLOCK) { //添加到阻塞队列
		VOSTaskListPrioInsert(pRunningTask, VOS_LIST_BLOCK);
	}
	else {//添加到就绪队列，这是因为遇到更高优先级，或者相同优先级时，把当前任务切换出去
		VOSTaskListPrioInsert(pRunningTask, VOS_LIST_READY);
	}

	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; //触发上下文切换中断
}


//from==TASK_USER_SWITCH: 应用层主动切换
//from==TASK_USER_SYSTICK: 时钟定时器切换，被动切换
static void VOSTaskSwitch(u32 from)
{
	s32 ret = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (pRunningTask && !list_empty(&gListTaskReady)) {
		ret = VOSTaskReadyCmpPrioTo(pRunningTask); //比较运行任务优先级跟就绪队列里的任务优先级
		if (from == TASK_SWITCH_USER) {//主动切换任务，不需要判断时间片等内容，直接切换
			if (ret <= 0 || //正在运行的任务优先级相同或更低，需要任务切换
				pRunningTask->status == VOS_STA_FREE || //任务状态如果是FREE状态，也需要切换
				pRunningTask->status == VOS_STA_BLOCK //任务添加到阻塞队列
				) {
				VOSCortexSwitch();
			}
		}
		else if (from == TASK_SWITCH_SYSTICK) {//系统定时切换，被动切换
			if (ret < 0 || //ret < 0: 就绪队列有更高优先级，强制切换
				pRunningTask->status == VOS_STA_FREE || //被释放掉的任务强制切换
				pRunningTask->status == VOS_STA_BLOCK || //任务需要添加到阻塞队列
				(ret == 0 && pRunningTask->ticks_timeslice == 0 )//ret == 0 && ticks_timeslice==0:时间片用完，同时就绪队列有相同优先级，也得切换
			)
			{
				VOSCortexSwitch();
			}
			if (pRunningTask->ticks_timeslice > 0) { //时间片不能为负数，万一有相同优先级后来进入就绪队列，就可以立即切换
				pRunningTask->ticks_timeslice--;
			}
		}
		else {//不支持，输出错误信息

		}
	}
	__local_irq_restore(irq_save);
}
//创建IDLE任务，无论何时都至少有个IDLE任务在就绪队列
static void TaskIdleBuild()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	VOSTaskCreate(task_idle, 0, stack_idle, sizeof(stack_idle), TASK_PRIO_MAX, "idle");
	if (!list_empty(&gListTaskReady)) {
		pRunningTask = list_entry(gListTaskReady.next, StVosTask, list); //获取第一个空闲任务
		list_del(gListTaskReady.next); //空闲任务队列里删除第一个空闲任务
	}
	__local_irq_restore(irq_save);
}

void RunFirstTask ()
{
	SVC_EXC_RETURN = HW32_REG((pRunningTask->pstack));
	_set_PSP((pRunningTask->pstack + 10 * 4));//栈指向R0
	NVIC_SetPriority(PendSV_IRQn, 0xFF);
	SysTick_Config(168000);
	_set_CONTROL(0x3);
	_ISB();

}

//这个必须在用户模式下使用，切换上下文
//但是VOSTaskSwitch必须在特权模式下访问，否则异常。
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
	case 0://系统刚初始化完成，启动第一个任务
		TaskIdleBuild();//创建idle任务
		RunFirstTask(); //加载第一个任务，这时任务不一定是IDLE任务
		break;
	case 1://用户任务主动调用切换到更高优先级任务，如果没有则继续用户任务
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
	if (gVOSTicks >= gMarkTicksNearest) {//闹钟响，查找阻塞队列，把对应的阻塞任务添加到就绪队列中
		VOSTaskBlockWaveUp();
	}
	VOSTaskSwitch(TASK_SWITCH_SYSTICK);
}

