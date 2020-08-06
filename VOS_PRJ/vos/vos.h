//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------


#ifndef __VOS_H__
#define __VOS_H__

#include "vtype.h"
#include "list.h"

/*
  * λͼ�����궨��
 */
#define bitmap_for_each(pos, bitmap_byte) \
	for (pos = 0; pos < bitmap_byte*8; pos++)

#define bitmap_get(n, bitmap)		(!!(((u8*)(bitmap))[(n)>>3] & 1<<((n)&0x07)))
#define bitmap_clear(n, bitmap)		(((u8*)(bitmap))[(n)>>3] &= ~(1<<((n)&0x07)))
#define bitmap_set(n, bitmap)		(((u8*)(bitmap))[(n)>>3] |= 1<<((n)&0x07))



#define MCU_FREQUENCY_HZ (u32)(168000000)

#define MAX_SIGNED_VAL_64 (0x7FFFFFFFFFFFFFFF)

#define VOS_TASK_NOT_INHERITANCE   (0)  //Ĭ�������ȼ��̳����������ȼ���ת���⣬�������Ϊ1���򲻴���ת����

#if VOS_TASK_NOT_INHERITANCE
#define VOSTaskRestorePrioBeforeRelease
#define VOSTaskRaisePrioBeforeBlock
#endif

extern struct StVosTask;

enum {
	VOS_LIST_READY = 0,
	VOS_LIST_BLOCK,
};

enum {
	TASK_SWITCH_USER = 0,
	TASK_SWITCH_SYSTICK,
};


void _set_PSP(u32 psp);
void _ISB();

#define MAX_CPU_NUM 1
#define MAX_VOSTASK_NUM  10

#define TICKS_INTERVAL_MS 1 //systick�ļ����1ms

#define MAKE_TICKS(ms) (((ms)+TICKS_INTERVAL_MS-1)/(TICKS_INTERVAL_MS))

#define HW32_REG(ADDRESS) (*((volatile unsigned long *)(ADDRESS)))

#define MAX_TICKS_TIMESLICE 10


#define VOS_WAKEUP_FROM_SEM			(u32)(0)
#define VOS_WAKEUP_FROM_SEM_DEL		(u32)(1) //ɾ���ź���������֪ͨ�ĸ��ȴ��ź�����������ӵ���������
#define VOS_WAKEUP_FROM_DELAY		(u32)(2)
#define VOS_WAKEUP_FROM_MUTEX		(u32)(3)
#define VOS_WAKEUP_FROM_MUTEX_DEL	(u32)(4) //ɾ��������������֪ͨ�ĸ��ȴ���������������ӵ���������
#define VOS_WAKEUP_FROM_EVENT		(u32)(5)
#define VOS_WAKEUP_FROM_EVENT_DEL	(u32)(6) //ɾ���¼�������֪ͨ�ĸ��ȴ��¼���������ӵ���������
#define VOS_WAKEUP_FROM_MSGQUE		(u32)(7)
#define VOS_WAKEUP_FROM_MSGQUE_DEL	(u32)(8)


#define	VOS_STA_FREE 			(u32)(0) //���ж��л���
#define VOS_STA_READY			(u32)(1) //��������
#define VOS_STA_BLOCK			(u32)(2) //��������


#define VOS_BLOCK_DELAY			(u32)(1<<0) //����ʱ��������
#define VOS_BLOCK_SEMP			(u32)(1<<1) //�ź�������
#define VOS_BLOCK_EVENT			(u32)(1<<2) //�¼�����
#define VOS_BLOCK_MUTEX			(u32)(1<<3) //��������
#define VOS_BLOCK_MSGQUE		(u32)(1<<4) //��Ϣ��������

#define TASK_PRIO_INVALID		(s32)(-1) //�����ȼ������IDLE
#define TASK_PRIO_REAL			(s32)(100)
#define TASK_PRIO_HIGH			(s32)(130)
#define TASK_PRIO_NORMAL		(s32)(150)
#define TASK_PRIO_LOW			(s32)(200)
#define TASK_PRIO_MAX			(s32)(255) //�����ȼ������IDLE




typedef struct StVOSSemaphore {
	s32 max;  //����ź�������
	s32 left; //Ŀǰʣ����ź�������
	s8 *name; //�ź�������
	u8 bitmap[(MAX_VOSTASK_NUM+sizeof(u8)-1)/sizeof(u8)];//ÿλ�ص�ƫ�������Ǳ�ռ�������id.
	s32 distory; //ɾ����������־����Ҫ�Ѿ�������������еȴ�����������������ӵ���������
	struct list_head list;
}StVOSSemaphore;

//���������ź���ʵ�ֻ���һ��
typedef struct StVOSMutex {
	s32 counter; //���Ϊ1�����ǻ�����
	s8 *name; //����������
	s32 distory; //ɾ����������־����Ҫ�Ѿ�������������еȴ�����������������ӵ���������
	struct StVosTask *ptask_owner; //ָ��ĳ������ӵ���������ֻ��һ������ӵ�������
	struct list_head list;
}StVOSMutex;

//��Ϣ���ж��̶�size��item�����λ���ʵ����Ϣ����
typedef struct StVOSMsgQueue {
	s8 *name;     //��Ϣ��������
	u8 *pdata;    //�ڴ�ָ�룬�û��ṩ
	s32 length;   //��ʼ�����ܵ�size
	s32 pos_head; //ָ����Ϣ����ͷ,������ʾ
	s32 pos_tail; //ָ����Ϣ����β,������ʾ
	s32 msg_cnts; //��������Ч����Ϣ����
	s32 msg_maxs; //��������Ϣ�ܸ���
	s32 msg_size; //ÿ����Ϣ�Ĵ�С���ֽڣ�
	s32 distory; //ɾ����������־����Ҫ�Ѿ�������������еȴ�����������������ӵ���������
	struct list_head list;
}StVOSMsgQueue;

typedef struct StVosTask {
	u8 *pstack; //ָ�������Լ���ջָ��, ����ŵ��ṹ���һ��λ�ã������Ҫʹ�������Ա
	u8 *pstack_top; //ջ��ָ��
	u32 stack_size; //ջ���size
	s32 prio; //�������ȼ���ֵԽ�ͣ����ȼ�Խ��
	s32 prio_base; //ԭʼ���ȼ���
	u32 id; //����ΨһID
	u8 *name; //������
	volatile u32 status; //����״̬
	volatile u32 block_type; //���������������ָ����������
	void (*task_fun)(void *param);
	void *param;
	s64 ticks_start;//�������õ�
	s64 ticks_alert;//������ڵ�
	u32 ticks_timeslice; //ʱ��Ƭ
	void *psyn; //ָ��ͬ����Ϣ���ƿ�
	u32 wakeup_from; //����ͬ���źŵ���Դ��
	u32 event_mask;  //�¼����ͣ�32λ�¼�
	u32 event;  //�¼����ͣ�32λ�¼�

	struct list_head list;//������������ȼ���������,���ȼ��ߵ��ŵ�ͷ�����ȼ��͵���β
}StVosTask;



//���жϣ������ع��ж�ǰ��ֵ��������Ƕ���ж����
extern u32 __local_irq_save();
//���жϣ������ж�ǰ��ֵ�ָ���������Ƕ���ж����
extern void __local_irq_restore(u32 old);

extern void local_irq_disable(void);

extern void local_irq_enable(void);

void VOSSemInit();

StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name);

s32 VOSSemWait(StVOSSemaphore *pSem, u64 timeout_ms);
s32 VOSSemTryWait(StVOSSemaphore *pSem);
s32 VOSSemRelease(StVOSSemaphore *pSem);

s32 VOSSemDelete(StVOSSemaphore *pSem);

void VOSMutexInit();

StVOSMutex *VOSMutexCreate(s8 *name);

s32 VOSMutexWait(StVOSMutex *pMutex, s64 timeout_ms);

s32 VOSMutexRelease(StVOSMutex *pMutex);
s32 VOSMutexDelete(StVOSMutex *pMutex);
s32 VOSEventWait(u32 event_mask, u64 timeout_ms);
s32 VOSEventSet(s32 task_id, u32 event);
u32 VOSEventGet(s32 task_id);
s32 VOSEventClear(s32 task_id, u32 event);
void VOSMsgQueInit();
StVOSMsgQueue *VOSMsgQueCreate(s8 *pRingBuf, s32 length, s32 msg_size, s8 *name);
s32 VOSMsgQuePut(StVOSMsgQueue *pMQ, void *pmsg, s32 len);
s32 VOSMsgQueGet(StVOSMsgQueue *pMQ, void *pmsg, s32 len, s64 timeout_ms);
s32 VOSMsgQueFree(StVOSMsgQueue *pMQ);

s64 VOSGetTicks();
s64 VOSGetTimeMs();
u32 VOSTaskInit();
StVosTask *VOSGetTaskFromId(s32 task_id);
u32 VOSTaskDelay(u32 ms);
u32 VOSTaskListPrioInsert(StVosTask *pTask, s32 which_list);
s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask);
StVosTask *VOSTaskReadyCutPriorest();
void VOSTaskEntry(void *param);
s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm);
void VOSTaskBlockWaveUp();
void VOSStarup();
void VOSTaskSchedule();

s32 VOSTaskRaisePrioBeforeBlock(StVosTask *pMutexOwnerTask);
s32 VOSTaskRestorePrioBeforeRelease();

#endif
