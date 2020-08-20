////----------------------------------------------------
//// Copyright (c) 2020, VOS Open source. All rights reserved.
//// Author: 156439848@qq.com; vincent_cws2008@gmail.com
//// History:
////	     2020-08-16: initial by vincent.
////------------------------------------------------------
//
//
//#include "vconf.h"
//#include "vos.h"
//#include "vlist.h"
//
//extern struct StVosTask *pRunningTask;
//extern volatile s64  gVOSTicks;
//extern volatile s64 gMarkTicksNearest;
//extern struct StVosTask *pReadyTask;
//extern struct list_head gListTaskBlock;
//
//extern struct list_head gListTaskReady;
//
//static struct list_head gListSemaphore;//空闲信号量链表
//
//static struct list_head gListMutex;//空闲互斥锁链表
//
//static struct list_head gListMsgQue;//空闲消息队列链表
//
//StVOSSemaphore gVOSSemaphore[MAX_VOS_SEMAPHONRE_NUM];
//
//StVOSMutex gVOSMutex[MAX_VOS_MUTEX_NUM];
//
//StVOSMsgQueue gVOSMsgQue[MAX_VOS_MSG_QUE_NUM];
//
//
//
//
//void VOSSemInit()
//{
//	s32 i = 0;
//	u32 irq_save = 0;
//	irq_save = __local_irq_save();
//	//初始化信号量队列
//	INIT_LIST_HEAD(&gListSemaphore);
//	//把所有任务链接到空闲信号量队列中
//	for (i=0; i<MAX_VOS_SEMAPHONRE_NUM; i++) {
//		list_add_tail(&gVOSSemaphore[i].list, &gListSemaphore);
//	}
//	__local_irq_restore(irq_save);
//}
//
//
//
//void VOSMutexInit()
//{
//	s32 i = 0;
//	u32 irq_save = 0;
//	irq_save = __local_irq_save();
//	//初始化信号量队列
//	INIT_LIST_HEAD(&gListMutex);
//	//把所有任务链接到空闲信号量队列中
//	for (i=0; i<MAX_VOS_MUTEX_NUM; i++) {
//		list_add_tail(&gVOSMutex[i].list, &gListMutex);
//		gVOSMutex[i].counter = -1; //初始化为-1，表示无效
//		gVOSMutex[i].ptask_owner = 0;
//	}
//	__local_irq_restore(irq_save);
//}
//
//
//void VOSMsgQueInit()
//{
//	s32 i = 0;
//	u32 irq_save = 0;
//
//	irq_save = __local_irq_save();
//	//初始化信号量队列
//	INIT_LIST_HEAD(&gListMsgQue);
//	//把所有任务链接到空闲信号量队列中
//	for (i=0; i<MAX_VOS_MSG_QUE_NUM; i++) {
//		list_add_tail(&gVOSMsgQue[i].list, &gListMsgQue);
//	}
//	__local_irq_restore(irq_save);
//}
//
//
//
//StVOSSemaphore *SysCallVOSSemCreate(StVosSysCallParam *psa)
//{
//	s32 max_sems = (s32)psa->u32param0;
//	s32 init_sems = (s32)psa->u32param1;
//	s8 *name = (s32)psa->u32param2;
//
//	StVOSSemaphore *pSemaphore = 0;
//	if (!list_empty(&gListSemaphore)) {
//		pSemaphore = list_entry(gListSemaphore.next, StVOSSemaphore, list); //获取第一个空闲信号量
//		list_del(gListSemaphore.next);
//		pSemaphore->name = name;
//		pSemaphore->max = max_sems;
//		if (init_sems > max_sems) {
//			init_sems = max_sems;
//		}
//		pSemaphore->left = init_sems; //初始化信号量为满
//		memset(pSemaphore->bitmap, 0, sizeof(pSemaphore->bitmap)); //清空位图，位图指向占用该信号量的任务id.
//	}
//	return  pSemaphore;
//}
//
//s32 SysCallVOSSemTryWait(StVosSysCallParam *psa)
//{
//	StVOSSemaphore *pSem = (StVOSSemaphore *)psa->u32param0;
//	s32 ret = -1;
//	if (pSem->max == 0) return -1; //信号量可能不存在或者被释放
//	if (pSem->left > 0) {
//		pSem->left--;
//		ret = 1;
//	}
//	return ret;
//}
//
//s32 SysCallVOSSemWait(StVosSysCallParam *psa)
//{
//	StVOSSemaphore *pSem = (StVOSSemaphore*)psa->u32param0;
//	u64 timeout_ms = (u64)psa->u64param0;
//
//	s32 ret = 0;
//
//	if (pSem->max == 0) return -1; //信号量可能不存在或者被释放
//
//	if (pSem->left > 0) {
//		pSem->left--;
//		ret = 1;
//	}
//	else {//把当前任务切换到阻塞队列
//		pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
//		//信号量阻塞类型
//		pRunningTask->block_type |= VOS_BLOCK_SEMP;//信号量类型
//		pRunningTask->psyn = pSem;
//		//同时是超时时间类型
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //如果闹钟结点小于记录的最少值，则更新
//			gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
//		}
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时
//	}
//	return ret;
//}
//
//s32 SysCallVOSSemRelease(StVosSysCallParam *psa)
//{
//	StVOSSemaphore *pSem = (StVOSSemaphore*)psa->u32param0;
//	s32 ret = 0;
//	u32 irq_save = 0;
//	if (pSem->max == 0) return -1; //信号量可能不存在或者被释放
//
//	if (pSem->left < pSem->max) {
//		pSem->left++; //释放信号量
//		VOSTaskBlockWaveUp(); //唤醒在阻塞队列里阻塞的等待该信号量的任务
//		pRunningTask->psyn = 0; //清除指向资源的指针。
//		//bitmap_clear(pRunningTask->id, pSem->bitmap); //清除任务编号，如果同个任务多次获取相同信号量该如何处理？
//		ret = 1;
//	}
//	return ret;
//}
//
//
//s32 SysCallVOSSemDelete(StVosSysCallParam *psa)
//{
//	StVOSSemaphore *pSem = (StVOSSemaphore*)psa->u32param0;
//	s32 ret = -1;
//	u32 irq_save = 0;
//
//	if (!list_empty(&gListSemaphore)) {
//
//		if (pSem->max == pSem->left) {//所有任务都归还信号量，这时可以释放。
//			list_del(pSem);
//			list_add_tail(&pSem->list, &gListSemaphore);
//			pSem->max = 0;
//			pSem->name = 0;
//			pSem->left = 0;
//			//pSem->distory = 0;
//			ret = 0;
//		}
//	}
//	return ret;
//}
//
////创建是可以在任务之外创建
//StVOSMutex *SysCallVOSMutexCreate(StVosSysCallParam *psa)
//{
//	s8 *name = (s8*)psa->u32param0;
//
//	StVOSMutex *pMutex = 0;
//	if (!list_empty(&gListMutex)) {
//		pMutex = list_entry(gListMutex.next, StVOSMutex, list); //获取第一个空闲互斥锁
//		list_del(gListMutex.next);
//		pMutex->name = name;
//		pMutex->counter = 1;
//		pMutex->ptask_owner =  0;
//	}
//	return  pMutex;
//}
//
//
//s32 SysCallVOSMutexWait(StVosSysCallParam *psa)
//{
//	StVOSMutex *pMutex = (StVOSMutex *)psa->u32param0;
//	s64 timeout_ms = (s64)psa->u64param0;
//
//	s32 ret = 0;
//
//	if (pRunningTask == 0) return -1; //需要在用户任务里执行
//	if (pMutex->counter == -1) return -1; //信号量可能不存在或者被释放
//
//	if (pMutex->counter > 1) pMutex->counter = 1;
//
//	if (pMutex->counter > 0) {
//		pMutex->counter--;
//		pMutex->ptask_owner = pRunningTask;
//		//pRunningTask->psyn = pMutex; //也要设置指向资源的指针，优先级反转需要用到
//		ret = 1;
//	}
//	else {//把当前任务切换到阻塞队列
//		pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
//
//		//信号量阻塞类型
//		pRunningTask->block_type |= VOS_BLOCK_MUTEX;//互斥锁类型
//		pRunningTask->psyn = pMutex;
//		//同时是超时时间类型
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //如果闹钟结点小于记录的最少值，则更新
//			gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
//		}
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时
//
//		VOSTaskRaisePrioBeforeBlock(pMutex->ptask_owner); //阻塞前处理优先级反转问题，提升就绪队列里的任务优先级
//	}
//	return ret;
//}
//
//s32 SysCallVOSMutexRelease(StVosSysCallParam *psa)
//{
//	StVOSMutex *pMutex = (StVOSMutex *)psa->u32param0;
//	s32 ret = 0;
//	if (pMutex->counter == -1) return -1; //互斥锁可能不存在或者被释放
//	if (pMutex->counter > 1) pMutex->counter = 1;
//
//	if (pMutex->counter < 1 && pMutex->ptask_owner == pRunningTask) {//互斥量必须要由拥有者才能释放
//		pMutex->counter++; //释放互斥锁
//		VOSTaskBlockWaveUp(); //唤醒在阻塞队列里阻塞的等待该互斥锁的任务
//		VOSTaskRestorePrioBeforeRelease();//恢复自身的优先级
//		pRunningTask->psyn = 0; //清除指向资源的指针。
//		pMutex->ptask_owner = 0; //清除拥有者
//		ret = 1;
//	}
//	return ret;
//}
//
//
//s32 SysCallVOSMutexDelete(StVosSysCallParam *psa)
//{
//	StVOSMutex *pMutex = (StVOSMutex *)psa->u32param0;
//	s32 ret = -1;
//
//	if (!list_empty(&gListSemaphore)) {
//		if (pMutex->ptask_owner == 0 && pMutex->counter == 1) {//互斥锁没被引用，直接释放
//			//删除自己
//			list_del(pMutex);
//			list_add_tail(&pMutex->list, &gListMutex);
//			pMutex->name = 0;
//			pMutex->counter = -1;
//			ret = 0;
//		}
//	}
//	return ret;
//}
//
//
//s32 SysCallVOSEventWait(StVosSysCallParam *psa)
//{
//	u32 event_mask = (u32)psa->u32param0;
//	u64 timeout_ms = (u64)psa->u64param0;
//	s32 ret = 0;
//
//	//把当前任务切换到阻塞队列
//	pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
//
//	//信号量阻塞类型
//	pRunningTask->block_type |= VOS_WAKEUP_FROM_EVENT;//事件类型
//	pRunningTask->psyn = 0;
//	pRunningTask->event_mask = event_mask;
//	//同时是超时时间类型
//	pRunningTask->ticks_start = gVOSTicks;
//	pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//	if (pRunningTask->ticks_alert < gMarkTicksNearest) { //如果闹钟结点小于记录的最少值，则更新
//		gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
//	}
//	pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时
//
//	return ret;
//}
//
//
//s32 SysCallVOSEventSet(StVosSysCallParam *psa)
//{
//	s32 task_id = (s32)psa->u32param0;
//	u32 event = (u32)psa->u32param1;
//	s32 ret = -1;
//
//	StVosTask *pTask = VOSGetTaskFromId(task_id);
//	if (pTask) {
//		pTask->event = event;
//		VOSTaskBlockWaveUp(); //唤醒在阻塞队列里阻塞的等待该事件的任务
//		ret = 1;
//	}
//	return ret;
//}
//
//
//u32 SysCallVOSEventGet(StVosSysCallParam *psa)
//{
//	s32 task_id = (s32)psa->u32param0;
//	u32 event_mask = 0;
//
//	StVosTask *pTask = VOSGetTaskFromId(task_id);
//	if (pTask) {
//		event_mask = pTask->event_mask;
//	}
//	return event_mask;
//}
//
//
//s32 SysCallVOSEventClear(StVosSysCallParam *psa)
//{
//	s32 task_id = (s32)psa->u32param0;
//	u32 event = (u32)psa->u32param1;
//	u32 mask = 0;
//
//	StVosTask *pTask = VOSGetTaskFromId(task_id);
//	if (pTask) {
//		pTask->event_mask &= (~event);
//		mask = pTask->event_mask;
//	}
//	return mask;
//}
//
//
//StVOSMsgQueue *SysCallVOSMsgQueCreate(StVosSysCallParam *psa)
//{
//	s8 *pRingBuf = (s8*)psa->u32param0;
//	s32 length = (s32)psa->u32param1;
//	s32 msg_size = (s32)psa->u32param2;
//	s8 *name = (s8*)psa->u32param3;
//
//	StVOSMsgQueue *pMsgQue = 0;
//
//	if (!list_empty(&gListMsgQue)) {
//		pMsgQue = list_entry(gListMsgQue.next, StVOSMsgQueue, list);
//		list_del(gListMsgQue.next);
//		pMsgQue->name = name;
//		pMsgQue->pdata = pRingBuf;
//		pMsgQue->length = length;
//		pMsgQue->pos_head = 0;
//		pMsgQue->pos_tail = 0;
//		pMsgQue->msg_cnts = 0;
//		pMsgQue->msg_maxs = length/msg_size;
//		pMsgQue->msg_size = msg_size;
//	}
//	return  pMsgQue;
//}
//
//s32 SysCallVOSMsgQuePut(StVosSysCallParam *psa)
//{
//	StVOSMsgQueue *pMQ = (StVOSMsgQueue *)psa->u32param0;
//	void *pmsg = psa->u32param1;
//	s32 len = (s32)psa->u32param2;
//
//	s32 ret = 0;
//	u8 *ptail = 0;
//
//	if (pMQ->pos_tail != pMQ->pos_head || //头不等于尾，可以添加新消息
//		pMQ->msg_cnts == 0) { //队列为空，可以添加新消息
//		ptail = pMQ->pdata + pMQ->pos_tail * pMQ->msg_size;
//		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
//		memcpy(ptail, pmsg, len);
//		pMQ->pos_tail++;
//		pMQ->pos_tail = pMQ->pos_tail % pMQ->msg_maxs;
//		pMQ->msg_cnts++;
//		VOSTaskBlockWaveUp(); //唤醒在阻塞队列里阻塞的等待该信号量的任务
//		//VOSTaskRestorePrioBeforeRelease(pRunningTask);//恢复自身的优先级
//		pRunningTask->psyn = 0; //清除指向资源的指针。
//		ret = 1;
//	}
//	return ret;
//}
//
//s32 SysCallVOSMsgQueGet(StVosSysCallParam *psa)
//{
//	StVOSMsgQueue *pMQ = (StVOSMsgQueue *)psa->u32param0;
//	void *pmsg = (void*)psa->u32param1;
//	s32 len = (s32)psa->u32param2;
//	s64 timeout_ms = (s64)psa->u64param0;
//	s32 ret = 0;
//	u8 *phead = 0;
//
//	if (pMQ->msg_cnts > 0) {//有消息
//		phead = pMQ->pdata + pMQ->pos_head * pMQ->msg_size;
//		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
//		memcpy(pmsg, phead, len);
//		pMQ->pos_head++;
//		pMQ->pos_head = pMQ->pos_head % pMQ->msg_maxs;
//		pMQ->msg_cnts--;
//
//		//pRunningTask->psyn = pMQ; //也要设置指向资源的指针，优先级反转需要用到
//		ret = 1;
//	}
//	else {//没消息，进入就绪队列
//		pRunningTask->status = VOS_STA_BLOCK; //添加到阻塞队列
//
//		//信号量阻塞类型
//		pRunningTask->block_type |= VOS_BLOCK_MSGQUE;//消息队列类型
//		pRunningTask->psyn = pMQ;
//		//同时是超时时间类型
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //如果闹钟结点小于记录的最少值，则更新
//			gMarkTicksNearest = pRunningTask->ticks_alert;//更新为最近的闹钟
//		}
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//指明阻塞类型为自延时
//
//		//VOSTaskRaisePrioBeforeBlock(pRunningTask); //阻塞前处理优先级反转问题，提升就绪队列里的任务优先级
//	}
//	return ret;
//}
//
//
//s32 SysCallVOSMsgQueFree(StVosSysCallParam *psa)
//{
//	StVOSMsgQueue *pMQ = (StVOSMsgQueue *)psa->u32param0;
//	s32 ret = -1;
//
//	if (!list_empty(&gListMsgQue)) {
//
//		if (pMQ->msg_cnts == 0) {//消息为空，释放
////			//清除消息队列，是否需要把就绪队列里的所有等待该信号量的阻塞任务添加到就绪队列
////			pMQ->distory = 1;
////			VOSTaskBlockWaveUp(); //唤醒在阻塞队列里阻塞的等待该信号量的任务
//
//			list_del(pMQ);
//			list_add_tail(&pMQ->list, &gListMsgQue);
//			pMQ->distory = 0;
//			pMQ->name = 0;
//			pMQ->pdata = 0;
//			pMQ->length = 0;
//			pMQ->pos_head = 0;
//			pMQ->pos_tail = 0;
//			pMQ->msg_cnts = 0;
//			pMQ->msg_maxs = 0;
//			pMQ->msg_size = 0;
//			ret = 0;
//		}
//	}
//	return ret;
//}
//
//
//void SysCallVOSTaskSchTabDebug(StVosSysCallParam *psa)
//{
//	s32 prio_mark = -1;
//	u32 mark = 0;
//	StVosTask *ptask_prio = 0;
//	struct list_head *list_prio;
//	struct list_head *phead = 0;
//	phead = &gListTaskReady;
//	kprintf("[(%d) ", pRunningTask?pRunningTask->id:-1);
//	kprintf("(%d) ", pReadyTask?pReadyTask->id:-1);
//	//插入队列，必须优先级从高到低有序排列
//	kprintf("(");
//	list_for_each(list_prio, phead) {
//		ptask_prio = list_entry(list_prio, struct StVosTask, list);
//		kprintf("%d", ptask_prio->id);
//		if (list_prio->next != phead) {
//			kprintf("->");
//		}
//	}
//	kprintf(")");
//	phead = &gListTaskBlock;
//	//插入队列，必须优先级从高到低有序排列
//	kprintf("(");
//	list_for_each(list_prio, phead) {
//		ptask_prio = list_entry(list_prio, struct StVosTask, list);
//		kprintf("%d", ptask_prio->id);
//		if (list_prio->next != phead) {
//			kprintf("->");
//		}
//	}
//	kprintf(")]\r\n");
//}
//
//s32 SysCallVOSGetC(StVosSysCallParam *psa);
//void VOSSysCall(StVosSysCallParam *psa)
//{
//	u32 ret = 0;
//	if (psa==0) return;
//	switch (psa->call_num)
//	{
//	case VOS_SYSCALL_MUTEX_CREAT:
//		ret = (u32)SysCallVOSMutexCreate(psa);
//		break;
//	case VOS_SYSCALL_MUTEX_WAIT:
//		ret = (u32)SysCallVOSMutexWait(psa);
//		break;
//	case VOS_SYSCALL_MUTEX_RELEASE:
//		ret = (u32)SysCallVOSMutexRelease(psa);
//		break;
//	case VOS_SYSCALL_MUTEX_DELETE:
//		ret = (u32)SysCallVOSMutexDelete(psa);
//		break;
//	case VOS_SYSCALL_SEM_CREATE:
//		ret = (u32)SysCallVOSSemCreate(psa);
//		break;
//	case VOS_SYSCALL_SEM_TRY_WAIT:
//		ret = (u32)SysCallVOSSemTryWait(psa);
//		break;
//	case VOS_SYSCALL_SEM_WAIT:
//		ret = (u32)SysCallVOSSemWait(psa);
//		break;
//	case VOS_SYSCALL_SEM_RELEASE:
//		ret =(u32)SysCallVOSSemRelease(psa);
//		break;
//	case VOS_SYSCALL_SEM_DELETE:
//		ret = (u32)SysCallVOSSemDelete(psa);
//		break;
//	case VOS_SYSCALL_EVENT_WAIT:
//		ret = (u32)SysCallVOSEventWait(psa);
//		break;
//	case VOS_SYSCALL_EVENT_SET:
//		ret = (u32)SysCallVOSEventSet(psa);
//		break;
//	case VOS_SYSCALL_EVENT_GET:
//		ret = (u32)SysCallVOSEventGet(psa);
//		break;
//	case VOS_SYSCALL_EVENT_CLEAR:
//		ret = (u32)SysCallVOSEventClear(psa);
//		break;
//	case VOS_SYSCALL_MSGQUE_CREAT:
//		ret = (u32)SysCallVOSMsgQueCreate(psa);
//		break;
//	case VOS_SYSCALL_MSGQUE_PUT:
//		ret = (u32)SysCallVOSMsgQuePut(psa);
//		break;
//	case VOS_SYSCALL_MSGQUE_GET:
//		ret = (u32)SysCallVOSMsgQueGet(psa);
//		break;
//	case VOS_SYSCALL_MSGQUE_FREE:
//		ret = (u32)SysCallVOSMsgQueFree(psa);
//		break;
////	case VOS_SYSCALL_TASK_DELAY: //效率低，禁用
////		ret = (u32)SysCallVOSTaskDelay(psa);
////		break;
//	case VOS_SYSCALL_TASK_CREATE:
//		ret = (u32)SysCallVOSTaskCreate(psa);
//		break;
//	case VOS_SYSCALL_SCH_TAB_DEBUG:
//		SysCallVOSTaskSchTabDebug(psa);
//		ret = 0;
//		break;
//	case VOS_SYSCALL_GET_CHAR:
//		ret = (u32)SysCallVOSGetC(psa);
//	default:
//		break;
//	}
//	psa->u32param0 = ret; //返回值
//}
