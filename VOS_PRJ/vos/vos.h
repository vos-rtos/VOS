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

#define MAX_SIGNED_VAL_64 (0x7FFFFFFFFFFFFFFF)


enum {
	TASK_SWITCH_USER = 0,
	TASK_SWITCH_SYSTICK,
};

typedef struct StVOSSemaphore {
	s32 max;  //����ź�������
	s32 left; //Ŀǰʣ����ź�������
	s8 *name; //�ź�������
	s32 distory; //ɾ����������־����Ҫ�Ѿ�������������еȴ�����������������ӵ���������
	struct list_head list;
}StVOSSemaphore;

//���������ź���ʵ�ֻ���һ��
typedef struct StVOSMutex {
	s32 counter; //���Ϊ1�����ǻ�����
	s8 *name; //����������
	s32 distory; //ɾ����������־����Ҫ�Ѿ�������������еȴ�����������������ӵ���������
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
	u8 *pstack; //ָ�������Լ���ջָ��
	u8 *pstack_top; //ջ��ָ��
	u32 stack_size; //ջ���size
	s32 prio; //�������ȼ���ֵԽ�ͣ����ȼ�Խ��
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
	struct list_head list_sib;//��ͬ���ȼ������ֵ���������ʱ��Ƭ����ѯ����
}StVosTask;

void _set_PSP(u32 psp);
void _ISB();

#define MAX_CPU_NUM 1
#define MAX_VOSTASK_NUM  10

#define TICKS_INTERVAL_US 1000 //systick�ļ����1000us

#define MAKE_TICKS(us) (((us)+TICKS_INTERVAL_US-1)/(TICKS_INTERVAL_US))

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


//���жϣ������ع��ж�ǰ��ֵ��������Ƕ���ж����
extern u32 __local_irq_save();
//���жϣ������ж�ǰ��ֵ�ָ���������Ƕ���ж����
extern void __local_irq_restore(u32 old);

extern void local_irq_disable(void);

extern void local_irq_enable(void);

void VOSTaskSwitch(u32 from);

void VOSSemInit();

StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name);

s32 VOSSemWait(StVOSSemaphore *pSem, u64 timeout_us);

s32 VOSSemRelease(StVOSSemaphore *pSem);

s32 VOSSemDelete(StVOSSemaphore *pSem);


StVosTask *VOSGetTaskFromId(s32 task_id);

#endif
