//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "vtype.h"
#include "vos.h"
#include "vlist.h"

#include "vbitmap.h"


struct StVosTask gArrVosTask[MAX_VOSTASK_NUM]; //任务数组，占用空间

struct list_head gListTaskDelay;//各种定时任务被连接到这个链表中

struct list_head gListTaskFree;//空闲任务，可以继续申请分配

struct StVosTask *pRunningTask; //正在运行的任务

struct StVosTask *pReadyTask; //准备要切换的任务

volatile u32  gVOSTicks = 0;

volatile u32 gMarkTicksNearest = TIMEOUT_INFINITY_U32; //记录最近闹钟响

volatile u32 VOSIntNesting = 0;

volatile u32 VOSRunning = 0;

volatile u32 VOSCtxSwtFlag = 0; //是否切换上下文，这个标志在执行完上下文切换后清0

volatile u32 vos_dis_irq_counter = 0;//记录调用irq disable层数


long long stack_idle[1024];

u32 VOSUserIRQSave();
void VOSUserIRQRestore(u32 save);

u32 gArrPrioBitMap[MAX_VOS_TASK_PRIO_NUM/8/4]; //每bit置位代表优先级数值
u8  gArrPrio2Taskid[MAX_VOS_TASK_PRIO_NUM]; //通过优先级查找第一个相同优先级的链表头任务


volatile u32 flag_cpu_collect = 0; //是否在采集cpu占用率

//遍历位图任务
StVosTask *tasks_bitmap_iterate(void **iter)
{
	u32 pos = (u32)*iter;
	u8 *p8 = (u8*)gArrPrioBitMap;
	while (pos < sizeof(gArrPrioBitMap) * 8) {
		if ((pos & 0x7) == 0 && p8[pos>>3] == 0) {//处理一个字节
			pos += 8;
		}
		else {//处理8bit
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


u32 __vos_irq_save()
{
	u32 vos_save;
#if (TASK_LEVEL)
	u32 pri_save = 0;
	u32 irq_save = 0;
	if (vos_dis_irq_counter == 0 && VOSIntNesting == 0) {
		pri_save = vos_privileged_save(); //这里不能禁止中断前调用
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
		vos_privileged_restore(save>>1);//这里不能禁止中断前调用
	}
#else
	__local_irq_restore(save);
#endif
}



u32 VOSGetTicks()
{
	u32 ticks;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	ticks = gVOSTicks;
	__vos_irq_restore(irq_save);
	return ticks;
}

u32 VOSGetTimeMs()
{
	u32 ticks = 0;
	ticks = VOSGetTicks();
	return ticks * TICKS_INTERVAL_MS;
}

u32 VOSTaskInit()
{
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();

	//memset(gArrPrio2Taskid, 0xFF, sizeof(gArrPrio2Taskid));//映射数组初始化为无效的任务ID
	memset(gArrPrio2Taskid, 0, sizeof(gArrPrio2Taskid));

	//初始化就绪任务队列和空闲任务队列
	INIT_LIST_HEAD(&gListTaskFree);
	INIT_LIST_HEAD(&gListTaskDelay);
	//把所有任务链接到空闲任务队列中
	for (i=0; i<MAX_VOSTASK_NUM; i++) {
		list_add_tail(&gArrVosTask[i].list, &gListTaskFree);
		gArrVosTask[i].status = VOS_STA_FREE;
	}
	__local_irq_restore(irq_save);

	TaskIdleBuild();//创建idle任务

	return 0;
}

StVosTask *VOSGetTaskFromId(s32 task_id)
{
	if (task_id < 0 || task_id >= MAX_VOSTASK_NUM) return 0;
	return &gArrVosTask[task_id];
}



//就绪队列表，取出某个任务
StVosTask *VOSTaskReadyListCutTask(StVosTask *pCutTask)
{
	StVosTask *ptasknext = 0;
	StVosTask *pHightask = 0;
	s32 highest;
	u32 irq_save = 0;

	irq_save = __local_irq_save();

	if (list_empty(&pCutTask->list)) {//为空，证明只有一个节点，则清空位图某个位
		bitmap_clr(pCutTask->prio, gArrPrioBitMap); //优先级表位图置位
		list_del(&pCutTask->list);//从相同优先级任务链表中删除自己
		//gArrPrio2Taskid[pCutTask->prio] = 0xFF; //无效任务ID值, 注意：这里不使用也可以，因为位图已经控制是否有数据
	}
	else {//证明相同优先级有多个任务，这时要把后面的任务的id设置到gArrPrio2Taskid表中，同时链表中删除最高优先级的任务
		ptasknext = list_entry(pCutTask->list.next, struct StVosTask, list);
		list_del(&pCutTask->list);//从相同优先级任务链表中删除自己
		gArrPrio2Taskid[pCutTask->prio] = ptasknext->id;
	}

	__local_irq_restore(irq_save);
	return pCutTask;
}

void VOSTaskPrioMoveUp(StVosTask *pMutexOwnerTask, struct list_head *list_head)
{
	struct list_head *list_owner = 0;
	struct list_head *list_dest = 0;
	StVosTask *ptask_dest = 0;
	if (list_head==0) return;
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
}

//把就绪队列里的所有指向相同互斥控制块（只处理互斥锁）的任务提升到跟准备阻塞前的当前任务一样
//如果当前任务优先级不如拥有者任务，则不处理
//该函数不可以在中断里调用
s32 VOSTaskRaisePrioBeforeBlock(StVOSMutex *pMutex)
{
	StVosTask *pMutexOwnerTask = pMutex->ptask_owner;
	StVosTask *ptask_dest = 0;
	StVosTask *ptask_temp = 0;
	StVosTask *ptask_ready = 0;
	struct list_head *list_owner = 0;
	struct list_head *list_dest = 0;
	void *pObj = 0;

	struct list_head *list_head = 0;

	if (pMutexOwnerTask->prio <= pRunningTask->prio) return 0;//准备阻塞的任务优先级不如拥有者，直接返回

	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//可能被多次提升
	//互斥锁拥有者可能在就绪队列里面，也有可能在阻塞队列里面，如果在阻塞队列里，
	//证明锁拥有者先获取到这个锁后在申请别的锁（信号量等）或延时进入阻塞队列
	if (pMutexOwnerTask->status == VOS_STA_READY) {//在就绪队列里（优先级排序队列）
		//list_head = &gListTaskReady;
		VOSTaskReadyListCutTask(pMutexOwnerTask);//从就绪表中清除
		pMutexOwnerTask->prio = pRunningTask->prio; //删除后赋值
		VOSReadyTaskPrioSet(pMutexOwnerTask); //插入
	}
	else if (pMutexOwnerTask->status == VOS_STA_BLOCK) {//在阻塞队列里（优先级排序队列）
		pObj = pMutexOwnerTask->psyn;
		switch(pMutexOwnerTask->status & BLOCK_SUB_MASK) {
		case VOS_BLOCK_SEMP:
			list_head = &((StVOSSemaphore*)pObj)->list_task;
		case VOS_BLOCK_EVENT:
			list_head = 0;
		case VOS_BLOCK_MUTEX:
			list_head = &((StVOSMutex*)pObj)->list_task;
		default: //只是延时，只是修改任务的优先级就行
			pMutexOwnerTask->prio = pRunningTask->prio; //直接赋值
			break;
		}
		pMutexOwnerTask->prio = pRunningTask->prio; //先赋值
		//冒泡上移
		VOSTaskPrioMoveUp(pMutexOwnerTask, list_head);
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
void VOSTaskPrtList(s32 which_list);
u32 VOSTaskCheck();
void VOSReadyTaskPrioSet(StVosTask *pInsertTask)
{
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	if (bitmap_get(pInsertTask->prio, gArrPrioBitMap)) {//已经有设置这位，插入到尾部
		ptask = &gArrVosTask[gArrPrio2Taskid[pInsertTask->prio]];
		//都是相同优先级，不用判断，直接插入到最后
		list_add_tail(&pInsertTask->list, &ptask->list);
	}
	else {//没设置此优先级位，直接设置位并更新gArrPrioBitMap表格对应的位置为任务的task id
		bitmap_set(pInsertTask->prio, gArrPrioBitMap); //优先级表位图置位
		gArrPrio2Taskid[pInsertTask->prio] = pInsertTask->id; //设置任务ID
		INIT_LIST_HEAD(&pInsertTask->list); //初始化列表自己指向自己
	}
	__local_irq_restore(irq_save);
}

u32 VOSTaskDelayListInsert(StVosTask *pInsertTask)
{
	s32 ret = -1;
	StVosTask *ptask_delay = 0;
	struct list_head *list;
	struct list_head *phead = 0;
	u32 irq_save = 0;

	irq_save = __local_irq_save();

	phead = &gListTaskDelay;

	//插入队列，闹钟时间必须从小到大有序排列
	list_for_each(list, phead) {
		ptask_delay = list_entry(list, struct StVosTask, list_delay);
		if (pInsertTask->ticks_alert < ptask_delay->ticks_alert) {//闹钟越近，排在越前
			list_add_tail(&pInsertTask->list_delay, list);
			ret = 0;
			break;
		}
	}
	if (list == phead) {//找不到合适位置，就直接插入到链表的最后
		list_add_tail(&pInsertTask->list_delay, phead);
	}

	//插入延时链表，也要更新最近闹钟
	if (pInsertTask->ticks_alert <  gMarkTicksNearest) {
		gMarkTicksNearest = pInsertTask->ticks_alert;
	}

	__local_irq_restore(irq_save);

	return ret;
}

u32 VOSTaskDelayListWakeUp()
{
	s32 ret = -1;
	StVosTask *ptask_delay = 0;
	struct list_head *list;
	struct list_head *list_temp;
	struct list_head *phead = 0;
	u32 irq_save = 0;

	irq_save = __local_irq_save();

	gMarkTicksNearest = TIMEOUT_INFINITY_U32;

	phead = &gListTaskDelay;

	//闹钟时间必须从小到大有序排列
	list_for_each_safe (list, list_temp, phead) {
		ptask_delay = list_entry(list, struct StVosTask, list_delay);
		if (ptask_delay->ticks_alert <= gVOSTicks) {//唤醒并添加到就绪表
			//断开当前延时队列
			list_del(&ptask_delay->list_delay);
			//如果在信号量等阻塞列表中，也要断开
			list_del(&ptask_delay->list);
			//初始化变量
			ptask_delay->status = VOS_STA_READY;
			ptask_delay->block_type = 0;
			ptask_delay->ticks_start = 0;
			ptask_delay->ticks_alert = 0;
			ptask_delay->wakeup_from = VOS_WAKEUP_FROM_DELAY;
			//添加到就绪队列表
			VOSReadyTaskPrioSet(ptask_delay);
		}
		else {
			gMarkTicksNearest = ptask_delay->ticks_alert;
			break;
		}
	}
	__local_irq_restore(irq_save);

	return ret;
}

u32 VOSTaskBlockListInsert(StVosTask *pInsertTask, struct list_head *phead)
{
	s32 ret = -1;
	StVosTask *ptask = 0;
	struct list_head *list;

	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//插入队列，必须优先级从高到低有序排列
	list_for_each(list, phead) {
		ptask = list_entry(list, struct StVosTask, list);
		if (pInsertTask->prio < ptask->prio) {//数值越小，优先级越高，如果相同优先级，则插入到所有相同优先级后面
			list_add_tail(&pInsertTask->list, list);
			ret = 0;
			break;
		}
	}
	if (list == phead) {//找不到合适位置，就直接插入到链表的最后
		list_add_tail(&pInsertTask->list, phead);
	}
	__local_irq_restore(irq_save);

	return ret;
}
//阻塞在信号量等任务，开始添加到就绪队列
u32 VOSTaskBlockListRelease(StVosTask *pReleaseTask)
{
	s32 ret = -1;
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	//释放阻塞对象的任务
	if (!(pReleaseTask->block_type & VOS_BLOCK_EVENT)) {
		list_del(&pReleaseTask->list);//释放第一个就可以，因为优先级排序, event没使用这个，释放不影响。
	}

	//注意延时链表如果删除的是表头，也要更新最近闹钟
	if (gListTaskDelay.next == &pReleaseTask->list_delay) {
		pReleaseTask->ticks_alert = gVOSTicks;//交给定时器去更新
	}
	//释放延时链表里的任务
	list_del(&pReleaseTask->list_delay);

	//添加到就绪队列表
	VOSReadyTaskPrioSet(pReleaseTask);

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
	s32 highest;
	StVosTask *ptask_prio = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();

	highest = TaskHighestPrioGet(gArrPrioBitMap, MAX_COUNTS(gArrPrioBitMap)); //数值越少，优先级越高
	if (highest != -1) {
		ret = highest - pRunTask->prio;
	}
	else {
		ret = 1; //不需要调度
	}
	__local_irq_restore(irq_save);
	return ret;
}

//获取当前就绪队列中优先级最高的任务
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
		if (list_empty(&pHightask->list)) {//为空，证明只有一个节点，则清空位图某个位
			bitmap_clr(highest, gArrPrioBitMap); //优先级表位图置位
			list_del(&pHightask->list);//从相同优先级任务链表中删除自己
			//gArrPrio2Taskid[highest] = 0xFF; //无效任务ID值
		}
		else {//证明相同优先级有多个任务，这时要把后面的任务的id设置到gArrPrio2Taskid表中，同时链表中删除最高优先级的任务
			ptasknext = list_entry(pHightask->list.next, struct StVosTask, list);
			list_del(&pHightask->list);//从相同优先级任务链表中删除自己
			gArrPrio2Taskid[highest] = ptasknext->id;
		}
		//砍下最高优先级就绪任务，这里必须要重新设置相关状态
		pHightask->ticks_timeslice = MAX_TICKS_TIMESLICE;
		pHightask->status = VOS_STA_RUNNING;
	}
	__local_irq_restore(irq_save);
	return pHightask;
}


s32 PrepareForCortexSwitch()
{
	pReadyTask = VOSTaskReadyCutPriorest();
	return pReadyTask && pRunningTask;//两个都不能为0
}
//所有任务总入口
void VOSTaskEntry(void *param)
{
	u32 irq_save = 0;
	if (pRunningTask) {
		pRunningTask->task_fun(pRunningTask->param);
	}
	//将任务回收到空闲任务链表中
	irq_save = __local_irq_save();
	//添加到空闲列表中
	pRunningTask->status = VOS_STA_FREE;
	list_add_tail(&pRunningTask->list, &gListTaskFree);
	__local_irq_restore(irq_save);
	VOSTaskSchedule();
}

//创建任务，不能在任务上下文创建(任务上下文使用VOSTaskCreate)
s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm)
{
	StVosTask *ptask = 0;
	u32 irq_save = 0;
	irq_save = __vos_irq_save();
	if (list_empty(&gListTaskFree)) goto END_CREATE;

	ptask = list_entry(gListTaskFree.next, StVosTask, list); //获取第一个空闲任务
	list_del(gListTaskFree.next); //空闲任务队列里删除第一个空闲任务

	if (prio > MAX_VOS_TASK_PRIO_IDLE) {//优先级不能超过255，可以等于255
		prio = MAX_VOS_TASK_PRIO_IDLE;
	}

	memset(ptask, 0, sizeof(*ptask));

	ptask->id = (ptask - gArrVosTask);

	ptask->prio = ptask->prio_base = prio;

	ptask->stack_size = stack_size;

	ptask->pstack_top = (u32*)((u8*)pstack + stack_size);

	if (pstack && stack_size) {
		memset(pstack, 0x64, stack_size); //初始化栈内容为0x64, 栈检测使用
	}
	ptask->name = task_nm;

	ptask->param = param;

	ptask->task_fun = task_fun;

	ptask->status = VOS_STA_READY;

	ptask->psyn = 0;

	ptask->block_type = 0;//没任何阻塞

	ptask->ticks_used_start = -1; //禁止统计cpu使用率

	ptask->event_mask = 0;

	ptask->list.pTask = ptask;

	ptask->list_delay.pTask = ptask;

	//初始化栈内容,至少18个u32大小
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
	*(--ptask->pstack) = 0x03; //CONTROL //#2:特权+PSP, #3:用户级+PSP
#else
	*(--ptask->pstack) = 0x02; //CONTROL //#2:特权+PSP, #3:用户级+PSP
#endif
	*(--ptask->pstack) = 0xFFFFFFFDUL; //EXC_RETURN

	//插入到就绪队列
	VOSReadyTaskPrioSet(ptask);

END_CREATE:
	__vos_irq_restore(irq_save);

	if (VOSRunning && ptask && pRunningTask->prio > ptask->prio) {
		//创建任务时，新建的任务优先级比正在运行的优先级高，则切换
		VOSTaskSchedule();
	}
	//这里注意，可能任务被新优先级的任务抢了，执行更高优先级后才切换到这里继续执行。
	return ptask ? ptask->id : -1;
}

void task_idle(void *param)
{
	/*
	 * 注意： 空闲任务不能加任何系统延时，可以加硬延时。
	 * 原因： 如果空闲任务被置换到阻塞队列，导致就绪任务剩下一个时，就无法被切出去。
	 *       如果剩下一个就绪任务被切换出去，那么当前cpu跑啥东西，需要空闲任务不能阻塞。
	 */
	static u32 mark_time=0;
	mark_time = VOSGetTimeMs();
	while (1) {//禁止空闲任务阻塞
//		if ((s32)(VOSGetTimeMs()-mark_time) > 1000) {
//			mark_time = VOSGetTimeMs();
//			VOSTaskSchTabDebug();
//		}
	}
}


void VOSStarup()
{
	if (VOSRunning == 0) { //启动第一个任务时会设置个VOSRunning为1
		RunFirstTask(); //加载第一个任务，这时任务不一定是IDLE任务
		while (1) ;;//不可能跑到这里
	}
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
        if (VOSIntNesting == 0) {
        	VOSTaskScheduleISR();
        }
        __local_irq_restore(irq_save);
    }
}

//创建IDLE任务，无论何时都至少有个IDLE任务在就绪队列
void TaskIdleBuild()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	VOSTaskCreate(task_idle, 0, stack_idle, sizeof(stack_idle), TASK_PRIO_MAX, "idle");
	pRunningTask = VOSTaskReadyCutPriorest();//获取第一个就绪任务，设置pRunningTask指针，准备启动
	__local_irq_restore(irq_save);
}

//任务上下文调度
void VOSTaskSchedule()
{
	s32 ret = 0;
	u32 irq_save = 0;
	if (VOSRunning == 0) return; //还没开始就切换直接返回

	if (VOSCtxSwtFlag == 1) return; //已经设置了切换标志，就不用再比较

	irq_save = __local_irq_save();

	if (pRunningTask->status == VOS_STA_FREE || //任务销毁，必须调度
			pRunningTask->status == VOS_STA_BLOCK) {//任务阻塞，必须调度
		VOSCtxSwtFlag = 1;
	}
	else {
		//比较正在运行的任务优先级和就绪队列里的最高优先级
		ret = VOSTaskReadyCmpPrioTo(pRunningTask);
		//正在运行的任务优先级相同或更低  ，则需要任务切换
		if (ret <= 0) {
			VOSCtxSwtFlag = 1;
			//把运行的任务添加到就绪表中
			pRunningTask->status = VOS_STA_READY;
			VOSReadyTaskPrioSet(pRunningTask);
		}
	}

	__local_irq_restore(irq_save);

	if (VOSCtxSwtFlag) {
		asm_ctx_switch();//触发PendSV_Handler中断
	}
}
//中断上下文调度
void VOSTaskScheduleISR()
{
	s32 ret = 0;
	u32 irq_save = 0;
	if (VOSRunning == 0) return; //还没开始就切换直接返回

	if (VOSCtxSwtFlag == 1) return; //已经设置了切换标志，就不用再比较

	irq_save = __local_irq_save();

	if (pRunningTask->status == VOS_STA_FREE || //任务销毁，必须调度
			pRunningTask->status == VOS_STA_BLOCK) {//任务阻塞，必须调度
		VOSCtxSwtFlag = 1;
	}
	else {
		//比较正在运行的任务优先级和就绪队列里的最高优先级
		ret = VOSTaskReadyCmpPrioTo(pRunningTask);
		if (ret < 0 || (ret == 0 && pRunningTask->ticks_timeslice == 0))//ret == 0 && ticks_timeslice==0:时间片用完，同时就绪队列有相同优先级，也得切换
		{
			VOSCtxSwtFlag = 1;
			//把运行的任务添加到就绪表中
			pRunningTask->status = VOS_STA_READY;
			VOSReadyTaskPrioSet(pRunningTask);
		}
		if (pRunningTask->ticks_timeslice > 0) { //时间片不能为负数，万一有相同优先级后来进入就绪队列，就可以立即切换
			pRunningTask->ticks_timeslice--;
		}
	}

	__local_irq_restore(irq_save);

	if (VOSCtxSwtFlag) {
		asm_ctx_switch();//触发PendSV_Handler中断
	}
}


void VOSSysTick()
{
	u32 irq_save;
	irq_save = __local_irq_save();
	gVOSTicks++;
	__local_irq_restore(irq_save);
	if (VOSRunning && gVOSTicks >= gMarkTicksNearest) {//闹钟响，查找阻塞队列，把对应的阻塞任务添加到就绪队列中
		VOSTaskDelayListWakeUp();
	}
}



u32 VOSTaskDelay(u32 ms)
{
	//如果中断被关闭，系统不进入调度，则直接硬延时
	if (VOSIntNesting > 0) {
		VOSDelayUs(ms*1000);
		return 0;
	}
	//否则进入操作系统的闹钟延时
	u32 irq_save = 0;

	if (ms) {//进入延时设置
		irq_save = __vos_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
		//临时这样设定，想办法如何处理溢出问题
		if (MAX_INFINITY_U32 - gVOSTicks < MAKE_TICKS(ms)) {
			pRunningTask->ticks_alert = MAX_INFINITY_U32;
		}
		else {
			pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
		}

		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //如果闹钟结点小于记录的最少值，则更新
			gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
		}
		pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时

		VOSTaskDelayListInsert(pRunningTask); //把当前任务直接插到延时任务链表

		__vos_irq_restore(irq_save);
	}
	//ms为0，切换任务, 或者VOS_STA_BLOCK_DELAY标志，呼唤调度
	VOSTaskSchedule();
	return 0;
}

/**********************************************************************
 * ********************************************************************
 * 下面主要是定义查看内核信息的函数
 * ********************************************************************
 **********************************************************************/

int vvsprintf(char *buf, int len, char* format, ...);
#define sprintf vvsprintf
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

	//插入队列，必须优先级从高到低有序排列
	strcat(buf, "(");

	void *iter = 0; //从头遍历
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
	//插入队列，必须优先级从高到低有序排列
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



//目前这个函数用于统计任务使用cpu占用率计算
//当前任务准备切换出去前，
void HookTaskSwitchOut()
{
	if (VOSRunning && pRunningTask->ticks_used_start != -1) {
		if (gVOSTicks - pRunningTask->ticks_used_start > 0) {
			pRunningTask->ticks_used_cnts += gVOSTicks - pRunningTask->ticks_used_start;
		}
	}
}
//目前这个函数用于统计任务使用cpu占用率计算
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

	//正在与运行的任务也要计算算
	pRunningTask->ticks_used_start = gVOSTicks;

	//优先级表使用位图
	void *iter = 0; //从头遍历
	while (ptask_prio = tasks_bitmap_iterate(&iter)) {
		ptask_prio->ticks_used_start = gVOSTicks;
	}

	phead = &gListTaskDelay; //阻塞的任务全部都会在延时链表中
	//插入队列，必须优先级从高到低有序排列
	list_for_each(list_prio, phead) {
		ptask_prio = list_entry(list_prio, struct StVosTask, list_delay);
		ptask_prio->ticks_used_start = gVOSTicks;
	}

	flag_cpu_collect = 1; //设置采集标志

	__local_irq_restore(irq_save);

}

//mode: 0; 立即结束。
//mode: 1; 不结束，时间累加采集。
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
	//把数组全部元素的id设置为-1
	for (i=0; i<cnts; i++) {
		arr[i].id = -1;
	}

	if (flag_cpu_collect==0) {
		CaluTasksCpuUsedRateStart();
		flag_cpu_collect = 1;
	}

	irq_save = __local_irq_save();

	i = 0;
	//正在与运行的任务也要计算
	if (i < cnts) {
		arr[i].id = pRunningTask->id;
		arr[i].ticks = pRunningTask->ticks_used_cnts;
		arr[i].prio = pRunningTask->prio_base;
		arr[i].stack_top = pRunningTask->pstack_top;
		arr[i].stack_size = pRunningTask->stack_size;
		arr[i].stack_ptr = pRunningTask->pstack;
		arr[i].name = pRunningTask->name;
		ticks_totals += arr[i].ticks;
		if (mode==0) {//结束
			pRunningTask->ticks_used_start = -1;
			pRunningTask->ticks_used_cnts = 0;
		}
	}

	void *iter = 0; //从头遍历
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
			if (mode==0) {//结束
				ptask_prio->ticks_used_start = -1;
				ptask_prio->ticks_used_cnts = 0;
			}
		}
		else {//跳出
			break;
		}
	}

	phead = &gListTaskDelay; //阻塞的任务全部都会在延时链表中
	//插入队列，必须优先级从高到低有序排列
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
			if (mode==0) {//结束
				ptask_prio->ticks_used_start = -1;
				ptask_prio->ticks_used_cnts = 0;
			}
		}
		else {//跳出
			break;
		}
	}
	if (mode==0) {//立即结束采集
		flag_cpu_collect = 0;
	}
	__local_irq_restore(irq_save);

	ret = i;

	//打印所有任务信息
	//按任务号排序，待优化，任务id唯一，直接从0开始找
	kprintf("任务号\t优先级\tCPU(百分比)\t栈顶地址\t栈当前地址\t栈总尺寸\t栈剩余尺寸\t栈状态\t任务名\r\n");
	for (i=0; i<MAX_VOSTASK_NUM; i++) {
		for (j=0; j<cnts; j++) {
			if (arr[j].id == i) {
				//打印任务信息
				memset(stack_status, 0, sizeof(stack_status));
				strcpy(stack_status, "正常");
				if (arr[j].stack_ptr <= arr[j].stack_top - arr[j].stack_size) {
					stack_left = 0;
				}
				else {//正常
					stack_left = arr[j].stack_size - (arr[j].stack_top - arr[j].stack_ptr);
				}
				//检查栈是否被修改或溢出, 检查栈底是否0x64被修改
				if (*(u32*)(arr[j].stack_top - arr[j].stack_size) != 0x64646464) {
					strcpy(stack_status, "破坏"); //可能是被自己破坏或者被自己下面的变量破坏
				}
				if (ticks_totals==0) ticks_totals = 1; //除法分母不能为0
				kprintf("%04d\t%03d\t\t%03d\t%08x\t%08x\t%08x\t%08x\t%s\t%s\r\n",
						arr[j].id, arr[j].prio, (u32)(arr[j].ticks*100/ticks_totals), arr[j].stack_top,
						arr[j].stack_ptr, arr[j].stack_size, stack_left, stack_status, arr[j].name);
				break;
			}
		}
	}
	return ret;
}

s32 GetTaskIdByName(u8 *name)
{
	s32 id_ret = -1;

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

	if (strcmp(name, pRunningTask->name) == 0) {
		id_ret = pRunningTask->id;
		goto END_GETID;
	}

	void *iter = 0; //从头遍历
	while (ptask_prio = tasks_bitmap_iterate(&iter)) {
		if (strcmp(name, ptask_prio->name) == 0) {
			id_ret = ptask_prio->id;
			goto END_GETID;
		}
	}

	phead = &gListTaskDelay;
	//插入队列，必须优先级从高到低有序排列
	strcat(buf, "(");
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


