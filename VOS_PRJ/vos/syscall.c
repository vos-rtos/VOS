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
//static struct list_head gListSemaphore;//�����ź�������
//
//static struct list_head gListMutex;//���л���������
//
//static struct list_head gListMsgQue;//������Ϣ��������
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
//	//��ʼ���ź�������
//	INIT_LIST_HEAD(&gListSemaphore);
//	//�������������ӵ������ź���������
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
//	//��ʼ���ź�������
//	INIT_LIST_HEAD(&gListMutex);
//	//�������������ӵ������ź���������
//	for (i=0; i<MAX_VOS_MUTEX_NUM; i++) {
//		list_add_tail(&gVOSMutex[i].list, &gListMutex);
//		gVOSMutex[i].counter = -1; //��ʼ��Ϊ-1����ʾ��Ч
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
//	//��ʼ���ź�������
//	INIT_LIST_HEAD(&gListMsgQue);
//	//�������������ӵ������ź���������
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
//		pSemaphore = list_entry(gListSemaphore.next, StVOSSemaphore, list); //��ȡ��һ�������ź���
//		list_del(gListSemaphore.next);
//		pSemaphore->name = name;
//		pSemaphore->max = max_sems;
//		if (init_sems > max_sems) {
//			init_sems = max_sems;
//		}
//		pSemaphore->left = init_sems; //��ʼ���ź���Ϊ��
//		memset(pSemaphore->bitmap, 0, sizeof(pSemaphore->bitmap)); //���λͼ��λͼָ��ռ�ø��ź���������id.
//	}
//	return  pSemaphore;
//}
//
//s32 SysCallVOSSemTryWait(StVosSysCallParam *psa)
//{
//	StVOSSemaphore *pSem = (StVOSSemaphore *)psa->u32param0;
//	s32 ret = -1;
//	if (pSem->max == 0) return -1; //�ź������ܲ����ڻ��߱��ͷ�
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
//	if (pSem->max == 0) return -1; //�ź������ܲ����ڻ��߱��ͷ�
//
//	if (pSem->left > 0) {
//		pSem->left--;
//		ret = 1;
//	}
//	else {//�ѵ�ǰ�����л�����������
//		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
//		//�ź�����������
//		pRunningTask->block_type |= VOS_BLOCK_SEMP;//�ź�������
//		pRunningTask->psyn = pSem;
//		//ͬʱ�ǳ�ʱʱ������
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
//			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
//		}
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
//	}
//	return ret;
//}
//
//s32 SysCallVOSSemRelease(StVosSysCallParam *psa)
//{
//	StVOSSemaphore *pSem = (StVOSSemaphore*)psa->u32param0;
//	s32 ret = 0;
//	u32 irq_save = 0;
//	if (pSem->max == 0) return -1; //�ź������ܲ����ڻ��߱��ͷ�
//
//	if (pSem->left < pSem->max) {
//		pSem->left++; //�ͷ��ź���
//		VOSTaskBlockWaveUp(); //���������������������ĵȴ����ź���������
//		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
//		//bitmap_clear(pRunningTask->id, pSem->bitmap); //��������ţ����ͬ�������λ�ȡ��ͬ�ź�������δ���
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
//		if (pSem->max == pSem->left) {//�������񶼹黹�ź�������ʱ�����ͷš�
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
////�����ǿ���������֮�ⴴ��
//StVOSMutex *SysCallVOSMutexCreate(StVosSysCallParam *psa)
//{
//	s8 *name = (s8*)psa->u32param0;
//
//	StVOSMutex *pMutex = 0;
//	if (!list_empty(&gListMutex)) {
//		pMutex = list_entry(gListMutex.next, StVOSMutex, list); //��ȡ��һ�����л�����
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
//	if (pRunningTask == 0) return -1; //��Ҫ���û�������ִ��
//	if (pMutex->counter == -1) return -1; //�ź������ܲ����ڻ��߱��ͷ�
//
//	if (pMutex->counter > 1) pMutex->counter = 1;
//
//	if (pMutex->counter > 0) {
//		pMutex->counter--;
//		pMutex->ptask_owner = pRunningTask;
//		//pRunningTask->psyn = pMutex; //ҲҪ����ָ����Դ��ָ�룬���ȼ���ת��Ҫ�õ�
//		ret = 1;
//	}
//	else {//�ѵ�ǰ�����л�����������
//		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
//
//		//�ź�����������
//		pRunningTask->block_type |= VOS_BLOCK_MUTEX;//����������
//		pRunningTask->psyn = pMutex;
//		//ͬʱ�ǳ�ʱʱ������
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
//			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
//		}
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
//
//		VOSTaskRaisePrioBeforeBlock(pMutex->ptask_owner); //����ǰ�������ȼ���ת���⣬����������������������ȼ�
//	}
//	return ret;
//}
//
//s32 SysCallVOSMutexRelease(StVosSysCallParam *psa)
//{
//	StVOSMutex *pMutex = (StVOSMutex *)psa->u32param0;
//	s32 ret = 0;
//	if (pMutex->counter == -1) return -1; //���������ܲ����ڻ��߱��ͷ�
//	if (pMutex->counter > 1) pMutex->counter = 1;
//
//	if (pMutex->counter < 1 && pMutex->ptask_owner == pRunningTask) {//����������Ҫ��ӵ���߲����ͷ�
//		pMutex->counter++; //�ͷŻ�����
//		VOSTaskBlockWaveUp(); //���������������������ĵȴ��û�����������
//		VOSTaskRestorePrioBeforeRelease();//�ָ���������ȼ�
//		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
//		pMutex->ptask_owner = 0; //���ӵ����
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
//		if (pMutex->ptask_owner == 0 && pMutex->counter == 1) {//������û�����ã�ֱ���ͷ�
//			//ɾ���Լ�
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
//	//�ѵ�ǰ�����л�����������
//	pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
//
//	//�ź�����������
//	pRunningTask->block_type |= VOS_WAKEUP_FROM_EVENT;//�¼�����
//	pRunningTask->psyn = 0;
//	pRunningTask->event_mask = event_mask;
//	//ͬʱ�ǳ�ʱʱ������
//	pRunningTask->ticks_start = gVOSTicks;
//	pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//	if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
//		gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
//	}
//	pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
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
//		VOSTaskBlockWaveUp(); //���������������������ĵȴ����¼�������
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
//	if (pMQ->pos_tail != pMQ->pos_head || //ͷ������β�������������Ϣ
//		pMQ->msg_cnts == 0) { //����Ϊ�գ������������Ϣ
//		ptail = pMQ->pdata + pMQ->pos_tail * pMQ->msg_size;
//		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
//		memcpy(ptail, pmsg, len);
//		pMQ->pos_tail++;
//		pMQ->pos_tail = pMQ->pos_tail % pMQ->msg_maxs;
//		pMQ->msg_cnts++;
//		VOSTaskBlockWaveUp(); //���������������������ĵȴ����ź���������
//		//VOSTaskRestorePrioBeforeRelease(pRunningTask);//�ָ���������ȼ�
//		pRunningTask->psyn = 0; //���ָ����Դ��ָ�롣
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
//	if (pMQ->msg_cnts > 0) {//����Ϣ
//		phead = pMQ->pdata + pMQ->pos_head * pMQ->msg_size;
//		len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
//		memcpy(pmsg, phead, len);
//		pMQ->pos_head++;
//		pMQ->pos_head = pMQ->pos_head % pMQ->msg_maxs;
//		pMQ->msg_cnts--;
//
//		//pRunningTask->psyn = pMQ; //ҲҪ����ָ����Դ��ָ�룬���ȼ���ת��Ҫ�õ�
//		ret = 1;
//	}
//	else {//û��Ϣ�������������
//		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
//
//		//�ź�����������
//		pRunningTask->block_type |= VOS_BLOCK_MSGQUE;//��Ϣ��������
//		pRunningTask->psyn = pMQ;
//		//ͬʱ�ǳ�ʱʱ������
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(timeout_ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
//			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
//		}
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
//
//		//VOSTaskRaisePrioBeforeBlock(pRunningTask); //����ǰ�������ȼ���ת���⣬����������������������ȼ�
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
//		if (pMQ->msg_cnts == 0) {//��ϢΪ�գ��ͷ�
////			//�����Ϣ���У��Ƿ���Ҫ�Ѿ�������������еȴ����ź���������������ӵ���������
////			pMQ->distory = 1;
////			VOSTaskBlockWaveUp(); //���������������������ĵȴ����ź���������
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
//	//������У��������ȼ��Ӹߵ�����������
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
//	//������У��������ȼ��Ӹߵ�����������
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
////	case VOS_SYSCALL_TASK_DELAY: //Ч�ʵͣ�����
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
//	psa->u32param0 = ret; //����ֵ
//}
