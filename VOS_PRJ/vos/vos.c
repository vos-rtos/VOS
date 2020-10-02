/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vos.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

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

u32 gArrPrioBitMap[MAX_VOS_TASK_PRIO_NUM/8/4]; //每bit置位代表优先级数值
u8  gArrPrio2Taskid[MAX_VOS_TASK_PRIO_NUM]; //通过优先级查找第一个相同优先级的链表头任务

volatile u32 flag_cpu_collect = 0; //是否在采集cpu占用率


/********************************************************************************************************
* 函数：StVosTask *tasks_bitmap_iterate(void **iter);
* 描述: 遍历就绪任务位图
* 参数:
* [1] iter: 迭代变量，记录下一个要查找的位置，如果为0，则从第一个开始查找
* 返回：返回优先级表中的每一个任务指针，如果返回NULL，则表示没有就绪任务
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数： u32 __vos_irq_save();
* 描述: 关中断，然后返回关中断前的状态，这个返回值被__vos_irq_restore(u32 save)来恢复原来状态，支持多层嵌套
* 参数: 无
* 返回：返回优先级表中的每一个任务指针，如果返回NULL，则表示没有就绪任务
* 注意：开关TASK_LEVEL，是支持线程级用户模式（非特权模式），需要svc切换到特权然后关闭中断
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数： void __vos_irq_restore(u32 save);
* 描述: 开中断，支持多层嵌套
* 参数:
* [1] save: __vos_irq_save()返回值作为参数
* 返回：无
* 注意：开关TASK_LEVEL，是支持线程级用户模式（非特权模式），需要svc切换到特权然后关闭中断
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数： u32 VOSGetTicks();
* 描述: 获取系统的ticks计数，ticks在定时中断里每次累加1，无符号32位数值，溢出重头计数不受影响
* 参数: 无
* 返回：当前系统的计数值
* 注意：无
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
* 函数： u32 VOSGetTimeMs();
* 描述: 获取系统当前运行的毫秒数，将ticks转化成毫秒后返回
* 参数: 无
* 返回：当前系统运行毫秒数
* 注意：无
*********************************************************************************************************/
u32 VOSGetTimeMs()
{
	u32 ticks = 0;
	ticks = VOSGetTicks();
	return ticks * TICKS_INTERVAL_MS;
}

/********************************************************************************************************
* 函数： u32 VOSTaskInit();
* 描述: 系统任务初始化，包括空闲任务初始化
* 参数:	无
* 返回：查看返回值
* 注意：无
*********************************************************************************************************/
u32 VOSTaskInit()
{
	s32 ret = VERROR_NO_ERROR;
	s32 i = 0;
	u32 irq_save = 0;
	irq_save = __local_irq_save();

	//初始化就绪任务位图表和优先级任务ID映射表
	memset(gArrPrioBitMap, 0, sizeof(gArrPrioBitMap));
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

	//创建idle任务
	TaskIdleBuild();

	return ret;
}

/********************************************************************************************************
* 函数： StVosTask *VOSGetTaskFromId(s32 task_id);
* 描述: 通过任务ID返回任务结构指针
* 参数:
* [1] task_id: 任务ID
* 返回：该任务指针, 否则返回NULL
* 注意：无
*********************************************************************************************************/
StVosTask *VOSGetTaskFromId(s32 task_id)
{
	if (task_id < 0 || task_id >= MAX_VOSTASK_NUM) return 0;
	return &gArrVosTask[task_id];
}

/********************************************************************************************************
* 函数：s32 VOSGetCurTaskId();
* 描述:  获取当前任务的ID
* 参数:  无
* 返回：返回当前运行任务ID, 如果系统没开始调度，或者中断上下文，则返回-1
* 注意：无
*********************************************************************************************************/
s32 VOSGetCurTaskId()
{
	if (VOSRunning == 1 && VOSIntNesting == 0) return 0;
		return pRunningTask - &gArrVosTask[0];
	return -1;
}



/********************************************************************************************************
* 函数：StVosTask *VOSTaskReadyListCutTask(StVosTask *pCutTask);
* 描述: 就绪任务位图中，取出某个任务
* 参数:
* [1] pCutTask: 指定要取出的某个任务指针
* 返回：该任务指针
* 注意：无
*********************************************************************************************************/
StVosTask *VOSTaskReadyListCutTask(StVosTask *pCutTask)
{
	StVosTask *ptasknext = 0;
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

/********************************************************************************************************
* 函数：void VOSTaskPrioMoveUp(StVosTask *pMutexOwnerTask, struct list_head *list_head);
* 描述: 提升目前互斥锁拥有者的任务优先级，解决任务优先级翻转问题
* 参数:
* [1] pMutexOwnerTask: 互斥锁拥有者（任务）
* [2] list_head: 某个阻塞量的阻塞任务链表头
* 返回：无
* 注意：无
*********************************************************************************************************/
static void VOSTaskPrioMoveUp(StVosTask *pMutexOwnerTask, struct list_head *list_head)
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

/********************************************************************************************************
* 函数：void VOSTaskRaisePrioBeforeBlock(StVOSMutex *pMutex);
* 描述: 把就绪队列里的所有指向相同互斥控制块（只处理互斥锁）的任务提升到跟准备阻塞前的当前任务一样
*       如果当前任务优先级不如拥有者任务，则不处理
* 参数:
* [1] pMutex: 互斥锁对象指针
* 返回：无
* 注意：该函数不可以在中断里调用
*********************************************************************************************************/
void VOSTaskRaisePrioBeforeBlock(StVOSMutex *pMutex)
{
	void *pObj = 0;
	struct list_head *list_head = 0;
	StVosTask *pMutexOwnerTask = pMutex->ptask_owner;
	u32 irq_save = 0;

	if (pMutexOwnerTask->prio <= pRunningTask->prio) return;//准备阻塞的任务优先级不如拥有者，直接返回

	irq_save = __local_irq_save();
	//可能被多次提升
	//互斥锁拥有者可能在就绪队列里面，也有可能在阻塞队列里面，如果在阻塞队列里，
	//证明锁拥有者先获取到这个锁后在申请别的锁（信号量等）或延时进入阻塞队列
	if (pMutexOwnerTask->status == VOS_STA_READY) {//在就绪队列里（优先级排序队列）
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

		VOSTaskPrioMoveUp(pMutexOwnerTask, list_head);//冒泡上移
	}
END_RAISE_PRIO:
	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* 函数：void VOSTaskRestorePrioBeforeRelease();
* 描述: 当释放互斥锁时，也同时还原优先级
* 参数: 无
* 返回：无
* 注意：该函数不可以在中断里调用
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
* 函数：void VOSReadyTaskPrioSet(StVosTask *pInsertTask);
* 描述: 把任务设置到就绪任务位图中，如果位图中已经设置该位，就插入到该相同优先级任务链表的末尾
* 参数:
* [1] pInsertTask: 要添加到就绪位图的任务指针
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VOSTaskDelayListInsert(StVosTask *pInsertTask);
* 描述: 把任务插入到延时定时器任务链表中，这个链表里的任务是按闹钟响的时间距离从近到远排列
* 参数:
* [1] pInsertTask: 要添加到阻塞链表的任务指针
* 返回：无
* 注意：该链表记录所有阻塞任务，链接到该链表的任务可能也链接到阻塞对象（信号量，互斥锁）
*********************************************************************************************************/
void VOSTaskDelayListInsert(StVosTask *pInsertTask)
{
	StVosTask *ptask_delay = 0;
	struct list_head *list;
	struct list_head *phead = 0;
	u32 irq_save = 0;

	irq_save = __local_irq_save();

	phead = &gListTaskDelay;

	//插入队列，闹钟时间必须从小到大有序排列
	list_for_each(list, phead) {
		ptask_delay = list_entry(list, struct StVosTask, list_delay);
		//pInsertTask->ticks_alert < ptask_delay->ticks_alert
		if (TICK_CMP(pInsertTask->ticks_alert, ptask_delay->ticks_alert, gVOSTicks) < 0) {//闹钟越近，排在越前
			list_add_tail(&pInsertTask->list_delay, list);
			break;
		}
	}
	if (list == phead) {//找不到合适位置，就直接插入到链表的最后
		list_add_tail(&pInsertTask->list_delay, phead);
	}

	//插入延时链表，也要更新最近闹钟
	//gMarkTicksNearest > pInsertTask->ticks_alert
	if (TICK_CMP(gMarkTicksNearest,pInsertTask->ticks_alert,pInsertTask->ticks_start) > 0) {
		gMarkTicksNearest = pInsertTask->ticks_alert;
	}
	__local_irq_restore(irq_save);
}

/********************************************************************************************************
* 函数：void VOSTaskDelayListWakeUp();
* 描述: 定时器中断里唤醒延时定时器任务链表中已经到达的任务
* 参数: 无
* 返回：无
* 注意：无
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

	//闹钟时间必须从小到大有序排列
	list_for_each_safe (list, list_temp, phead) {
		ptask_delay = list_entry(list, struct StVosTask, list_delay);

		//ptask_delay->ticks_alert <= gVOSTicks
		if (TICK_CMP(gVOSTicks,ptask_delay->ticks_alert,ptask_delay->ticks_start) >= 0) {//唤醒并添加到就绪表
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
}

/********************************************************************************************************
* 函数：void VOSTaskBlockListInsert(StVosTask *pInsertTask, struct list_head *phead);
* 描述: 把任务插入到某个阻塞对象（例如信号量，互斥锁等）的链表中，按任务优先级从高到低排序
* 参数:
* [1] pInsertTask: 要添加到阻塞链表的任务指针
* [2] phead: 阻塞量（例如信号量，互斥量）的链表头
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSTaskBlockListInsert(StVosTask *pInsertTask, struct list_head *phead)
{
	StVosTask *ptask = 0;
	struct list_head *list;
	u32 irq_save = 0;

	irq_save = __local_irq_save();
	//插入队列，必须优先级从高到低有序排列
	list_for_each(list, phead) {
		ptask = list_entry(list, struct StVosTask, list);
		if (pInsertTask->prio < ptask->prio) {//数值越小，优先级越高，如果相同优先级，则插入到所有相同优先级后面
			list_add_tail(&pInsertTask->list, list);
			break;
		}
	}
	if (list == phead) {//找不到合适位置，就直接插入到链表的最后
		list_add_tail(&pInsertTask->list, phead);
	}
	__local_irq_restore(irq_save);

}

/********************************************************************************************************
* 函数：void VOSTaskBlockListRelease(StVosTask *pReleaseTask);
* 描述: 释放阻塞任务
* 参数:
* [1] pReleaseTask: 要释放的任务指针
* 返回：无
* 注意：别忘记也要在延时定时链表中释放
*********************************************************************************************************/
void VOSTaskBlockListRelease(StVosTask *pReleaseTask)
{
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
}

/********************************************************************************************************
* 函数：s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask);
* 描述: 正在运行的任务和就绪位图里最高优先级的任务进行优先级比较
* 参数:
* [1] pRunTask: 正在运行的任务指针
* 返回：
* 返回值 > 0：证明正运行任务的优先级更高（数值更小），不需要调度
* 返回值 < 0：证明正运行任务的优先级更低（数值更大），需要调度
* 返回值 = 0: 证明正运行任务的优先级相同， 需要调度，基于时间片
* 注意：无
*********************************************************************************************************/
s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask)
{
	s32 ret = 1;//默认返回整数，代表不用调度
	s32 highest;

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

/********************************************************************************************************
* 函数：StVosTask *VOSTaskReadyCutPriorest();
* 描述: 取出就绪位图表里优先级最高的任务
* 参数: 无
* 返回：就绪任务中最高优先级任务指针
* 注意：无
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
		if (list_empty(&pHightask->list)) {//为空，证明只有一个节点，则清空位图某个位
			bitmap_clr(highest, gArrPrioBitMap); //优先级表位图置位
			list_del(&pHightask->list);//从相同优先级任务链表中删除自己
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

/********************************************************************************************************
* 函数：s32 PrepareForCortexSwitch();
* 描述: 上下文切换前准备工作，主要是取出就绪任务位图中优先级最高的任务赋值到pReadyTask变量中
* 参数: 无
* 返回：返回true，代表已经准备好，可以正常切换上下文，否则不切换
* 注意：无
*********************************************************************************************************/
s32 PrepareForCortexSwitch()
{
	pReadyTask = VOSTaskReadyCutPriorest();
	return pReadyTask && pRunningTask;//两个都不能为0
}

/********************************************************************************************************
* 函数：void VOSTaskEntry(void *param);
* 描述: 所有任务都从这里出发，这里包括任务结束的清理工作
* 参数:
* [1] param: 暂时没用到
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
*					void *pstack, u32 stack_size, s32 prio, s8 *task_nm);
* 描述: 创建任务
* 参数:
* [1] task_fun: 任务要执行的例程函数
* [2] param: 例程函数参数
* [3] pstack: 任务栈指针地址（栈底地址）
* [4] stack_size: 任务栈大小
* [5] prio: 任务优先级
* [5] task_nm: 任务名字
* 返回：任务ID, 如果创建失败，返回-1
* 注意：无
*********************************************************************************************************/
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
#if VOS_LIST_DEBUG
	ptask->list.pTask = ptask;
	ptask->list_delay.pTask = ptask;
#endif
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

/********************************************************************************************************
* 函数：s32 VOSTaskDelete(s32 task_id);
* 描述:  删除任务
* 参数:
* [1] task_id: 任务id
* 返回：成功或失败
* 注意：无
*********************************************************************************************************/
s32 VOSTaskDelete(s32 task_id)
{
	kprintf("XXXXXXXXXXXXXXXX %s:%d\r\n", __FUNCTION__, task_id);
	return 0;
}

/********************************************************************************************************
* 函数：void task_idle(void *param);
* 描述: 创建空闲任务
* 参数:
* [1] param: 暂时没用到
* 返回：无
* 注意：空闲任务不可以阻塞，必须要就绪状态或者运行状态
*********************************************************************************************************/
void task_idle(void *param)
{
	/*
	 * 注意： 空闲任务不能加任何系统延时，可以加硬延时。
	 * 原因： 如果空闲任务被置换到阻塞队列，导致就绪任务剩下一个时，就无法被切出去。
	 *       如果剩下一个就绪任务被切换出去，那么当前cpu跑啥东西，需要空闲任务不能阻塞。
	 */
	static u32 mark_time=0;
	mark_time = VOSGetTimeMs();
	while (1) {
		//禁止空闲任务阻塞
//		if ((s32)(VOSGetTimeMs()-mark_time) > 1000) {
//			mark_time = VOSGetTimeMs();
//			VOSTaskSchTabDebug();
//		}
	}
}

/********************************************************************************************************
* 函数：void VOSStarup();
* 描述: VOS系统启动第一个任务，这里是IDLE任务
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSStarup()
{
	if (VOSRunning == 0) { //启动第一个任务时会设置个VOSRunning为1
		RunFirstTask(); //加载第一个任务，这时任务不一定是IDLE任务
		while (1) ;;//不可能跑到这里
	}
}

/********************************************************************************************************
* 函数：void VOSIntEnter();
* 描述: 任何中断进入时设置，主要是记录中断层数，在最终中断退出时启动系统调度
* 参数: 无
* 返回：无
* 注意：无
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
* 函数：void VOSIntExit();
* 描述: 任何中断退出时设置，主要是记录中断层数，在最终中断退出时启动系统调度
* 参数: 无
* 返回：无
* 注意：无
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
* 函数：void TaskIdleBuild();
* 描述: 创建空闲任务
* 参数: 无
* 返回：无
* 注意：无论何时都至少有个IDLE任务在就绪队列
*********************************************************************************************************/
void TaskIdleBuild()
{
	u32 irq_save = 0;
	irq_save = __local_irq_save();
	VOSTaskCreate(task_idle, 0, stack_idle, sizeof(stack_idle), TASK_PRIO_MAX, "idle");
	pRunningTask = VOSTaskReadyCutPriorest();//获取第一个就绪任务，设置pRunningTask指针，准备启动
	__local_irq_restore(irq_save);
}


/********************************************************************************************************
* 函数：void VOSTaskSchedule();
* 描述: 任务上下文调度，这需要工作在任务上下文，如果是中断上下文则调用VOSTaskScheduleISR
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VOSTaskScheduleISR();
* 描述: 中断上下文调度，这需要工作在中断上下文，如果是任务上下文则调用VOSTaskSchedule
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：void VOSSysTick();
* 描述: Tick计数, 这函数需要在定时中断里添加
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：s32 VOSCurContexStatus();
* 描述: 获取当前执行上下文状态，中断上下文，任务上下文，系统上下文
* 参数: 无
* 返回：中断上下文，任务上下文，系统上下文
* 注意：无
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
* 函数：void VOSTaskDelay(u32 ms);
* 描述: 延时函数，任务上下文调用，如果是中断上下文，则硬件延时
* 参数:
* [1] ms: 延时单位毫秒
* 返回：无
* 注意：无
*********************************************************************************************************/
void VOSTaskDelay(u32 ms)
{
	//如果中断被关闭，系统不进入调度，则直接硬延时
	if (VOSIntNesting > 0) {
		VOSDelayUs(ms*1000);
	}
	//否则进入操作系统的闹钟延时
	u32 irq_save = 0;

	if (ms) {//进入延时设置
		irq_save = __vos_irq_save();
		pRunningTask->ticks_start = gVOSTicks;
		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
		//gMarkTicksNearest > pRunningTask->ticks_alert
		if (TICK_CMP(gMarkTicksNearest,pRunningTask->ticks_alert,pRunningTask->ticks_start) > 0) {
			gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
		}

		pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
		pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时

		VOSTaskDelayListInsert(pRunningTask); //把当前任务直接插到延时任务链表

		__vos_irq_restore(irq_save);
	}
	//ms为0，切换任务, 或者VOS_STA_BLOCK_DELAY标志，呼唤调度
	VOSTaskSchedule();
}

/**********************************************************************
 * ********************************************************************
 * 下面主要是定义查看内核信息的函数
 * ********************************************************************
 **********************************************************************/

int vvsprintf(char *buf, int len, char* format, ...);
#define sprintf vvsprintf

/********************************************************************************************************
* 函数：void VOSTaskSchTabDebug();
* 描述: 打印就绪任务位图表和阻塞任务表
* 参数: 无
* 返回：无
* 注意：此函数主要是调试时使用
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

/********************************************************************************************************
* 函数：void HookTaskSwitchOut();
* 描述: 当前任务准备切换出去前调用该函数，目前这个函数用于统计任务使用cpu占用率计算
* 参数: 无
* 返回：无
* 注意：此函数主要被用于shell命令task列表，查看cpu占用率
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
* 函数：void HookTaskSwitchOut();
* 描述: 最高优先级任务已经切换为当前运行任务调用该函数，目前这个函数用于统计任务使用cpu占用率计算
* 参数: 无
* 返回：无
* 注意：此函数主要被用于shell命令task列表，查看cpu占用率
*********************************************************************************************************/
void HookTaskSwitchIn()
{
	if (VOSRunning && pRunningTask->ticks_used_start != -1) {
		pRunningTask->ticks_used_start = gVOSTicks;
	}
}

/********************************************************************************************************
* 函数：void CaluTasksCpuUsedRateStart();
* 描述: shell命令task调用这个函数显示相关信息
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
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

/********************************************************************************************************
* 函数：s32 CaluTasksCpuUsedRateShow(struct StTaskInfo *arr, s32 cnts, s32 mode);
* 描述: shell命令task调用这个函数显示相关信息
* 参数:
* [1] arr: 用户提供的任务信息数组
* [2] cnts: 任务信息的个数
* [3] mode: mode==0, 立即结束; mode==1, 不结束，时间累加采集。
* 返回：返回有效的任务的个数，如果参数cnts >= 返回值，则列出所有任务，否则列出部分任务，这时cpu统计不准
* 注意：无
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

/********************************************************************************************************
* 函数：s32 GetTaskIdByName(u8 *name);
* 描述: 通过任务名来获取任务ID
* 参数:
* [1] name: 任务名
* 返回：任务ID,如果找不到任务，返回-1
* 注意：无
*********************************************************************************************************/
s32 GetTaskIdByName(u8 *name)
{
	s32 id_ret = -1;
	StVosTask *ptask_prio = 0;
	struct list_head *list_prio;
	struct list_head *phead = 0;
	void *iter = 0; //从头遍历
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
	//插入队列，必须优先级从高到低有序排列
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

