//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------

#include "vconf.h"
#include "vos.h"

extern struct StVosTask *pRunningTask;

extern volatile u32 VOSIntNesting;

StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name)
{
	if (VOSIntNesting > 0) return 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_CREATE,
			.u32param0 = max_sems,
			.u32param1 = init_sems,
			.u32param2 = name,
	};
	vos_sys_call(&sa);
	return (StVOSSemaphore *)sa.u32param0; //return;
}


s32 VOSSemTryWait(StVOSSemaphore *pSem)
{
	if (VOSIntNesting > 0) return 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_TRY_WAIT,
			.u32param0 = (u32)pSem,
	};
	vos_sys_call(&sa);
	return (s32)sa.u32param0; //return;
}



s32 VOSSemWait(StVOSSemaphore *pSem, u64 timeout_ms)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_WAIT,
			.u32param0 = (u32)pSem,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret==0) { //û�ź�����������������
		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ����ź�������
		case VOS_WAKEUP_FROM_DELAY:
			ret = 0;
			break;
		case VOS_WAKEUP_FROM_SEM:
			ret = 1;
			break;
		case VOS_WAKEUP_FROM_SEM_DEL:
			ret = -1;
			break;
		default:
			ret = 0;
			break;
		}
	}
	return ret;
}



s32 VOSSemRelease(StVOSSemaphore *pSem)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_RELEASE,
			.u32param0 = (u32)pSem,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}


s32 VOSSemDelete(StVOSSemaphore *pSem)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_SEM_DELETE,
			.u32param0 = (u32)pSem,
	};
	vos_sys_call(&sa);

	return (s32)sa.u32param0; //return;
}



//�����ǿ���������֮�ⴴ��
StVOSMutex *VOSMutexCreate(s8 *name)
{
	if (VOSIntNesting > 0) return 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_CREAT,
			.u32param0 = (u32)name,
	};
	vos_sys_call(&sa);

	return (StVOSMutex *)sa.u32param0; //return;
}


s32 VOSMutexWait(StVOSMutex *pMutex, s64 timeout_ms)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_WAIT,
			.u32param0 = (u32)pMutex,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret==0) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = -1;
			break;
		case VOS_WAKEUP_FROM_MUTEX:
			ret = 1;
			break;
		case VOS_WAKEUP_FROM_MUTEX_DEL:
			ret = -1;
			break;
		default:
			ret = 0;
			//printf info here
			break;
		}
	}
	return ret;
}




s32 VOSMutexRelease(StVOSMutex *pMutex)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_RELEASE,
			.u32param0 = (u32)pMutex,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}



s32 VOSMutexDelete(StVOSMutex *pMutex)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MUTEX_DELETE,
			.u32param0 = (u32)pMutex,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	return ret;
}


s32 VOSEventWait(u32 event_mask, u64 timeout_ms)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_WAIT,
			.u32param0 = (u32)event_mask,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	if (ret==0) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = -1;
			break;
		case VOS_WAKEUP_FROM_EVENT:
			ret = 1;
			break;
		default:
			ret = -1;
			//printf info here
			break;
		}
	}
	return ret;
}



s32 VOSEventSet(s32 task_id, u32 event)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_SET,
			.u32param0 = (u32)task_id,
			.u32param1 = (u32)event,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}

u32 VOSEventGet(s32 task_id)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_GET,
			.u32param0 = (u32)task_id,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	return ret;
}


s32 VOSEventClear(s32 task_id, u32 event)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_EVENT_CLEAR,
			.u32param0 = (u32)task_id,
			.u32param1 = (u32)event,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;
	return ret;
}

StVOSMsgQueue *VOSMsgQueCreate(s8 *pRingBuf, s32 length, s32 msg_size, s8 *name)
{
	if (VOSIntNesting > 0) return 0;
	StVOSMsgQueue * ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_CREAT,
			.u32param0 = (u32)pRingBuf,
			.u32param1 = (u32)length,
			.u32param2 = (u32)msg_size,
			.u32param3 = (u32)name,
	};
	vos_sys_call(&sa);

	ret = (StVOSMsgQueue *)sa.u32param0; //return;
	return ret;
}

//������ӵĸ������ɹ�����1����ʾ���һ����Ϣ�ɹ����������0����ʾ��������
s32 VOSMsgQuePut(StVOSMsgQueue *pMQ, void *pmsg, s32 len)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_PUT,
			.u32param0 = (u32)pMQ,
			.u32param1 = (u32)pmsg,
			.u32param2 = (u32)len,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret == 1) {
		//���Ѻ���������������ȣ���һ���ѵ��������ȼ����ڵ�ǰ�������л�,
		//��������VOSTaskSwitch(TASK_SWITCH_USER);���Ǳ�������Ȩģʽ��ʹ�á�
		VOSTaskSchedule();
	}
	return ret;
}

//������ӵĸ���
s32 VOSMsgQueGet(StVOSMsgQueue *pMQ, void *pmsg, s32 len, s64 timeout_ms)
{
	if (VOSIntNesting > 0) return 0;
	u8 *phead = 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_GET,
			.u32param0 = (u32)pMQ,
			.u32param1 = (u32)pmsg,
			.u32param2 = (u32)len,
			.u64param0 = (u64)timeout_ms,
	};
	vos_sys_call(&sa);

	ret = (s32)sa.u32param0; //return;

	if (ret==0) { //û��ȡ��������������������

		VOSTaskSchedule(); //������Ȳ�������������
		switch(pRunningTask->wakeup_from) { //�������Ǳ���ʱ�����ѻ��߻���������
		case VOS_WAKEUP_FROM_DELAY:
			ret = -1;
			break;
		case VOS_WAKEUP_FROM_MSGQUE: //����������Ϣ����ȡ��Ϣ
			phead = pMQ->pdata + pMQ->pos_head * pMQ->msg_size;
			len = (len <= pMQ->msg_size) ? len : pMQ->msg_size;
			memcpy(pmsg, phead, len);
			pMQ->pos_head++;
			pMQ->pos_head = pMQ->pos_head % pMQ->msg_maxs;
			pMQ->msg_cnts--;
			ret = 1;
			break;
		case VOS_WAKEUP_FROM_MSGQUE_DEL:
			ret = -1;
			break;
		default:
			ret = 0;
			//printf info here
			break;
		}
	}

	return ret;
}

s32 VOSMsgQueFree(StVOSMsgQueue *pMQ)
{
	if (VOSIntNesting > 0) return 0;
	s32 ret = 0;
	StVosSysCallParam sa = {
			.call_num = VOS_SYSCALL_MSGQUE_GET,
			.u32param0 = (u32)pMQ,
	};
	vos_sys_call(&sa);
	ret = (s32)sa.u32param0; //return;
	return ret;
}

/* ��ʱ�������ܷŵ����Ч��̫�� */
//u32 VOSTaskDelay(u32 ms)
//{
//	u32 ret = 0;
//	StVosSysCallParam sa = {
//			.call_num = VOS_SYSCALL_TASK_DELAY,
//			.u32param0 = (u32)ms,
//	};
//	vos_sys_call(&sa);
//	ret = (u32)sa.u32param0; //return;
//	if (ret == 0) VOSTaskSchedule();
//	return ret;
//}
//
//u32 SysCallVOSTaskDelay(StVosSysCallParam *psa)
//{
//	u32 ms = (u32)psa->u32param0;
//	if (ms) {//������ʱ����
//		pRunningTask->ticks_start = gVOSTicks;
//		pRunningTask->ticks_alert = gVOSTicks + MAKE_TICKS(ms);
//		if (pRunningTask->ticks_alert < gMarkTicksNearest) { //������ӽ��С�ڼ�¼������ֵ�������
//			gMarkTicksNearest = pRunningTask->ticks_alert;//����Ϊ���������
//		}
//		pRunningTask->status = VOS_STA_BLOCK; //��ӵ���������
//		pRunningTask->block_type |= VOS_BLOCK_DELAY;//ָ����������Ϊ����ʱ
//	}
//	return ms;
//}
u32 VOSTaskDelay(u32 ms)
{
	if (VOSIntNesting > 0) return 0;
	//����жϱ��رգ�ϵͳ��������ȣ���ֱ��Ӳ��ʱ
	//todo
	//����������ϵͳ��������ʱ
	if (ms) {//������ʱ����
		__asm volatile ("svc 3\n"); //VOS_SVC_NUM_DELAY
	}
	//msΪ0���л�����, ����VOS_STA_BLOCK_DELAY��־����������
	VOSTaskSchedule();
	return 0;
}


#if 0 //λͼ����
#define MAX_TASK_NUM 10;
u8 bitmap[10];
int bitmap_prinf(u8 *bitmap, s32 num)
{
	s32 line_num = 0;
	u32 n = 0;
	bitmap_for_each(n, num)
	{
		//printf("%d ", bitmap_get(n, bitmap) ? 1 : 0);
		printf("%d ", bitmap_get(n, bitmap));
		line_num++;
		if (line_num % 8 == 0) printf("\r\n");
	}
	printf("\r\n");
}

int bitmap_test() {
	u32 n = 0;
	memset(bitmap, 0, sizeof(bitmap));
	bitmap_set(2, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_clear(2, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(7, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(8, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(15, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(16, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));

#if 0
	bitmap_for_each(n, bitmap) {
		if (bitmap_check(n, bitmap)) {

		}
	}
#endif
}
#endif

