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
	//初始化就绪任务队列和空闲任务队列
	INIT_LIST_HEAD(&gListTaskReady);
	INIT_LIST_HEAD(&gListTaskFree);
	INIT_LIST_HEAD(&gListTaskBlock);
	//把所有任务链接到空闲任务队列中
	for (i=0; i<MAX_VOSTASK_NUM; i++) {
		list_add_tail(&gArrVosTask[i].list, &gListTaskFree);
		INIT_LIST_HEAD(&gArrVosTask[i].list_sib);//初始化兄弟节点，自己指向自己。
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
	//如果中断被关闭，系统不进入调度，则直接硬延时
	//todo
	//否则进入操作系统的闹钟延时
	u32 irq_save = 0;

	if (us) {//进入延时设置
		irq_save = __local_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(us);
		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //如果闹钟结点小于记录的最少值，则更新
			gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
		}
		pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时

		__local_irq_restore(irq_save);
	}
	//us为0，切换任务, 或者VOS_STA_BLOCK_DELAY标志，呼唤调度
	VOSTaskSchedule();

}


//就绪任务插入到优先级就绪队列中
u32 VOSTaskReadyInsert(StVosTask *pReadyTask)
{
	StVosTask *ptask_prio = 0;
	StVosTask *ptask_sib = 0;
	struct list_head *list_prio;
	struct list_head *list_sib;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//插入队列，必须优先级从高到低有序排列
	list_for_each(list_prio, &gListTaskReady) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list);
		if (pReadyTask->prio < ptask_prio->prio) {//数值越小，优先级越高
			list_add_tail(&pReadyTask->list, list_prio);
			goto END_INSERT;
		}
		else if (pReadyTask->prio == ptask_prio->prio){//数值相同，优先级相同，插入到兄弟链表中，基于时间片轮询
			list_add_tail(&pReadyTask->list_sib, &ptask_prio->list_sib);
			goto END_INSERT;
		}
		else {//数值越大，优先级越低，继续遍历

		}
	}
	//找不到合适位置，就直接查到优先级链表的最后
	list_add_tail(&pReadyTask->list, &gListTaskReady);

END_INSERT:
	__local_irq_restore(irq_save);
	return 0;
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
	StVosTask *ptask_sib = 0;
	struct list_head *list_sib;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//插入队列，必须优先级从高到低有序排列
	if (!list_empty(&gListTaskReady)) {
		ptask_prio = list_entry(gListTaskReady.next, struct StVosTask, list);
		list_del(gListTaskReady.next); //空闲任务队列里删除第一个空闲任务
		if (!list_empty(&ptask_prio->list_sib)){//如果没同优先级兄弟任务，直接断掉
			//已经断开优先级不同的队列，现在断开兄弟队列
			list_sib = ptask_prio->list_sib.next; //保存下个兄弟节点，准备插入到优先级队列中
			list_del(&ptask_prio->list_sib);
			INIT_LIST_HEAD(&ptask_prio->list_sib);//初始化兄弟节点，自己指向自己。
			//把后面兄弟队列添加到优先级队列中
			ptask_sib = list_entry(list_sib, struct StVosTask, list_sib);
			list_add(&ptask_sib->list, &gListTaskReady); //添加到优先级列表中
		}
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

StVosTask *VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, u8 *task_nm)
{
	StVosTask *pNewTask = 0;
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (list_empty(&gListTaskFree)) goto END_CREATE;
	ptask = list_entry(gListTaskFree.next, StVosTask, list); //获取第一个空闲任务
	list_del(gListTaskFree.next); //空闲任务队列里删除第一个空闲任务
	ptask->id = (ptask - gArrVosTask);

	ptask->prio = prio;

	ptask->stack_size = stack_size;

	ptask->pstack_top = (u8*)pstack + stack_size;

	ptask->name = task_nm;

	ptask->param = param;

	ptask->task_fun = task_fun;

	ptask->status = VOS_STA_READY;

	ptask->block_type = 0;//没任何阻塞

	//初始化栈内容
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
				VOSTaskReadyInsert(ptask_block);
			}
			else if (ptask_block->ticks_alert < latest_ticks){//排除已经超时的闹钟，把没有超时的闹钟最近的记录下来
				latest_ticks = ptask_block->ticks_alert;
			}
		}
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
				ptask_block->psyn = 0;
				if (pSem->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_SEM;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_SEM_DEL;
				}
				//添加到就绪队列
				VOSTaskReadyInsert(ptask_block);
			}
		}
		//处理互斥锁唤醒事件
		if (ptask_block->block_type & VOS_BLOCK_MUTEX) { //自定义互斥锁阻塞类型
			StVOSMutex *pMutex = (StVOSMutex *)ptask_block->psyn;
			if (pMutex->counter > 0 || pMutex->distory == 1) {//有至少一个信号量 或者互斥锁被删除
				if (pMutex->counter > 0) pMutex->counter--;
				//断开当前阻塞队列
				list_del(&ptask_block->list);
				//修改状态
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
				//添加到就绪队列
				VOSTaskReadyInsert(ptask_block);
			}
		}
		//处理事件唤醒事件
		if (ptask_block->block_type & VOS_BLOCK_EVENT) { //自定义互斥锁阻塞类型
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
				VOSTaskReadyInsert(ptask_block);
			}
		}
		//处理消息队列唤醒事件
		//处理互斥锁唤醒事件
		if (ptask_block->block_type & VOS_BLOCK_MSGQUE) { //自定义互斥锁阻塞类型
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
				ptask_block->psyn = 0;
				if (pMsgQue->distory == 0) {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MSGQUE;
				}
				else {
					ptask_block->wakeup_from = VOS_WAKEUP_FROM_MSGQUE_DEL;
				}
				//添加到就绪队列
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

	if (pRunningTask->status == VOS_STA_FREE) {//回收到空闲链表
		list_add_tail(&pRunningTask->list, &gListTaskFree);
	}
	else if (pRunningTask->status == VOS_STA_BLOCK) { //添加到阻塞队列
		list_add_tail(&pRunningTask->list, &gListTaskBlock);
	}
	else {//添加到就绪队列，这是因为遇到更高优先级，或者相同优先级时，把当前任务切换出去
		VOSTaskReadyInsert(pRunningTask);
	}

	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; //触发上下文切换中断
}


//from==TASK_USER_SWITCH: 应用层主动切换
//from==TASK_USER_SYSTICK: 时钟定时器切换，被动切换
void VOSTaskSwitch(u32 from)
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
	VOSTaskCreate(task_idle, 0, stack_idle, sizeof(stack_idle), 500, "idle");
	if (!list_empty(&gListTaskReady)) {
		pRunningTask = list_entry(gListTaskReady.next, StVosTask, list); //获取第一个空闲任务
		list_del(gListTaskReady.next); //空闲任务队列里删除第一个空闲任务
	}

	SVC_EXC_RETURN = HW32_REG((pRunningTask->pstack));
	_set_PSP((pRunningTask->pstack + 10 * 4));//栈指向R0
	NVIC_SetPriority(PendSV_IRQn, 0xFF);
	SysTick_Config(168000);
	_set_CONTROL(0x3);
	_ISB();
	__local_irq_restore(irq_save);
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
		TaskIdleBuild();
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
#if 1
	VOSTaskSwitch(TASK_SWITCH_SYSTICK);
#endif
}

