//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------


#ifndef __VOS_H__
#define __VOS_H__

#include "vconf.h"
#include "vtype.h"
#include "vlist.h"
#include "verror.h"

//A: ��ǰȫ�ֵ�tick
//B: �Ƚ϶������ֹtick, ע����ֹtick��ֵ��һ�����ڿ�ʼtick������ֱ�ӱȽϣ�ʹ�þ���ȽϷ�
//C: �Ƚ϶���Ŀ�ʼtick
//���ؽ���� A��BС������-1��A��B�󷵻�1�� A����B����0
//ע�⣺����A,B,S�������޷��űȽϣ�֧�����
#define TICK_CMP(A,B,S) ((A)-(S) < (B)-(S) ? -1 : ((A) == (B) ? 0 : 1))

#define EVENT_USART1_RECV	(1<<0)

typedef void (*task_fun_t)(void *param);

//����vshell��������
#define VSHELL_FUN(f) \
	f(s8 **parr, s32 cnts);\
	const char *f##_name __attribute__((section(".vshell_name"),noload,used))=#f; \
	const char *f##_note __attribute__((section(".vshell_note"),noload,used))=0; \
	extern void f(s8 **parr, s32 cnts);\
	void *f##_fun __attribute__((section(".vshell_fun"),noload,used))=(void*)f;\
	void (f) \

//����vshell��������, ins�������
#define VSHELL_FUN_NOTE(f,note) \
	f(s8 **parr, s32 cnts);\
	const char *f##_name __attribute__((section(".vshell_name"),noload,used))=#f; \
	const char *f##_note __attribute__((section(".vshell_note"),noload,used))=#note; \
	extern void f(s8 **parr, s32 cnts);\
	void *f##_fun __attribute__((section(".vshell_fun"),noload,used))=(void*)f;\
	void (f) \

#define VSHELL_PROMPT	"vshell/>"

#if VOS_TASK_NOT_INHERITANCE
#define VOSTaskRestorePrioBeforeRelease
#define VOSTaskRaisePrioBeforeBlock
#endif

extern struct StVosTask;

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
#define VOS_STA_RUNNING			(u32)(3) //��������

#define VOS_BLOCK_DELAY			(u32)(1<<0) //����ʱ��������
#define VOS_BLOCK_SEMP			(u32)(1<<1) //�ź�������
#define VOS_BLOCK_EVENT			(u32)(1<<2) //�¼�����
#define VOS_BLOCK_MUTEX			(u32)(1<<3) //��������


#define BLOCK_SUB_MASK (VOS_BLOCK_SEMP|VOS_BLOCK_MUTEX|VOS_BLOCK_EVENT/*|VOS_BLOCK_MSGQUE*/)

#define TASK_PRIO_INVALID		(s32)(-1) //�����ȼ������IDLE
#define TASK_PRIO_REAL			(s32)(100)
#define TASK_PRIO_HIGH			(s32)(130)
#define TASK_PRIO_NORMAL		(s32)(150)
#define TASK_PRIO_LOW			(s32)(200)
#define TASK_PRIO_MAX			(s32)(255) //�����ȼ������IDLE

#define VOS_SVC_NUM_STARTUP			(u32)(0) //svc 0 ���ص�һ������
#define VOS_SVC_NUM_SCHEDULE		(u32)(1) //svc 1 ��������
#define VOS_SVC_NUM_DELAY			(u32)(3) //svc 3 ϵͳ��ʱ�����Ч��
#define VOS_SVC_NUM_SYSCALL			(u32)(5) //svc 5 ����ϵͳ��������
#define VOS_SVC_PRIVILEGED_MODE		(u32)(6) //svc 6 �л�����Ȩģʽ

typedef struct StVosSysCallParam {
	u32 call_num; //ϵͳ���ú�
	u32 u32param0; //����32λ0
	u32 u32param1; //����32λ1
	u32 u32param2; //����32λ2
	u32 u32param3; //����32λ3
	u32 u32param4; //����32λ4
	u32 u32param5; //����32λ5
	u64 u64param0; //����64λ0
	u64 u64param1; //����64λ1
}StVosSysCallParam;

typedef struct StVOSSemaphore {
	s32 max;  //����ź�������
	s32 left; //Ŀǰʣ����ź�������
	s8 *name; //�ź�������
	u8 bitmap[(MAX_VOSTASK_NUM+sizeof(u8)-1)/sizeof(u8)];//ÿλ�ص�ƫ�������Ǳ�ռ�������id.
	s32 distory; //ɾ����������־����Ҫ�Ѿ�������������еȴ�����������������ӵ���������
	struct list_head list;
	struct list_head list_task;//������������
}StVOSSemaphore;

//���������ź���ʵ�ֻ���һ��
typedef struct StVOSMutex {
	s32 counter; //���Ϊ1�����ǻ�����
	s8 *name; //����������
	s32 distory; //ɾ����������־����Ҫ�Ѿ�������������еȴ�����������������ӵ���������
	struct StVosTask *ptask_owner; //ָ��ĳ������ӵ���������ֻ��һ������ӵ�������
	struct list_head list;
	struct list_head list_task;//������������
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
	StVOSSemaphore  *psem;
}StVOSMsgQueue;

typedef struct StVosTask {
	u32 *pstack; //ָ�������Լ���ջָ��, ����ŵ��ṹ���һ��λ�ã������Ҫʹ�������Ա

	u32 *pstack_top; //ջ��ָ��
	u32 stack_size; //ջ���size
	s32 prio; //�������ȼ���ֵԽ�ͣ����ȼ�Խ��
	s32 prio_base; //ԭʼ���ȼ���
	u32 id; //����ΨһID
	s8 *name; //������
	volatile u32 status; //����״̬
	volatile u32 block_type; //���������������ָ����������
	void (*task_fun)(void *param);
	void *param;
	u32 ticks_start;//�������õ㣬����ʹ��TICK_CMP���бȽ�
	u32 ticks_alert;//������ڵ�
	u32 ticks_timeslice; //ʱ��Ƭ
	void *psyn; //ָ��ͬ����Ϣ���ƿ�
	u32 wakeup_from; //����ͬ���źŵ���Դ��
	u32 event_mask;  //�¼����ͣ�32λ�¼�
	u32 event_stop;  //ֹͣ����ָ�����¼�
	u32 ticks_used_start; //ÿ�ν���cpu,��¼��ʼʱ�䣬�ȴ��л���ȥʱ���ѵ�ǰʱ���ȥ���ʱ��Ȼ���ۼӵ�ticks_used_cnts
	u32 ticks_used_cnts; //ͳ��cpuʹ����
	struct list_head list;//������������ȼ���������,���ȼ��ߵ��ŵ�ͷ�����ȼ��͵���β
	struct list_head list_delay;//��ʱ�����б�ע���������������ʱҲ�����ź�����ʱ����
}StVosTask;


typedef struct StTaskInfo {
	s32 id;
	s32 prio;
	u32 ticks;
	s32 stack_top;
	s32 stack_size;
	s32 stack_ptr;//��ǰָ���λ��
	s8 *name;
}StTaskInfo;


enum {
	VOS_TIMER_TYPE_ONE_SHOT,
	VOS_TIMER_TYPE_PERIODIC,
};

enum {
	VOS_TIMER_STA_FREE,
	VOS_TIMER_STA_RUNNING,
	VOS_TIMER_STA_STOPPED,
};

typedef  void (*VOS_TIMER_CB)(void *ptimer, void *parg);

typedef struct StVOSTimer{
	u32	type; //�������ͣ�one shot �� periodic
	u32 ticks_alert; //����ʱ���
	u32 ticks_start; //��ʼʱ���
	u32 ticks_delay; //�趨����ʱ
	VOS_TIMER_CB callback; //��ʱ���������ûص�����
	void *arg; //�ص������Ĳ���
	s8 *name; //��ʱ������
	u32 status; //״̬
	struct list_head list;
} StVOSTimer;


u32 VOSTaskBlockListInsert(StVosTask *pInsertTask, struct list_head *phead);
u32 VOSTaskBlockListRelease(StVosTask *pReleaseTask);

void VOSTimerInit();
StVOSTimer *VOSTimerCreate(s32 type, u32 delay_ms, VOS_TIMER_CB callback, void *arg, s8 *name);
s32 VOSTimerDelete(StVOSTimer *pTimer);
s32 VOSTimerStart(StVOSTimer *pTimer);
s32 VOSTimerStop(StVOSTimer *pTimer);
s32 VOSTimerGetStatus(StVOSTimer *pTimer);
s32 VOSTimerGetLeftTime(StVOSTimer *pTimer);


//���жϣ������ع��ж�ǰ��ֵ��������Ƕ���ж����
extern u32 __local_irq_save();
//���жϣ������ж�ǰ��ֵ�ָ���������Ƕ���ж����
extern void __local_irq_restore(u32 old);

extern void local_irq_disable(void);

extern void local_irq_enable(void);

void VOSSemInit();
StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name);
s32 VOSSemWait(StVOSSemaphore *pSem, u32 timeout_ms);
s32 VOSSemTryWait(StVOSSemaphore *pSem);
s32 VOSSemRelease(StVOSSemaphore *pSem);
s32 VOSSemDelete(StVOSSemaphore *pSem);

void VOSMutexInit();
StVOSMutex *VOSMutexCreate(s8 *name);
s32 VOSMutexWait(StVOSMutex *pMutex, u32 timeout_ms);
s32 VOSMutexRelease(StVOSMutex *pMutex);
s32 VOSMutexDelete(StVOSMutex *pMutex);

s32 VOSEventWait(u32 event_mask, u32 timeout_ms);
s32 VOSEventSet(s32 task_id, u32 event);
u32 VOSEventGet(s32 task_id);
s32 VOSEventDisable(s32 task_id, u32 event);
s32 VOSEventEnable(s32 task_id, u32 event);

void VOSMsgQueInit();
StVOSMsgQueue *VOSMsgQueCreate(s8 *pRingBuf, s32 length, s32 msg_size, s8 *name);
s32 VOSMsgQuePut(StVOSMsgQueue *pMQ, void *pmsg, s32 len);
s32 VOSMsgQueGet(StVOSMsgQueue *pMQ, void *pmsg, s32 len, u32 timeout_ms);
s32 VOSMsgQueFree(StVOSMsgQueue *pMQ);

u32 VOSTaskDelay(u32 ms);

u32 VOSGetTicks();
u32 VOSGetTimeMs();

//����������ʹ��VOSTaskCreate
s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm);

u32 VOSTaskInit();
StVosTask *VOSGetTaskFromId(s32 task_id);
s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask);
StVosTask *VOSTaskReadyCutPriorest();
void VOSTaskEntry(void *param);

void VOSStarup();
void VOSTaskSchedule();
void VOSSysTick();


s32 VOSTaskRaisePrioBeforeBlock(StVOSMutex *pMutex);
s32 VOSTaskRestorePrioBeforeRelease();


//���Ŀǰ�������ж������Ļ������������ġ�
s32 VOSCortexCheck();

extern void asm_ctx_switch(); //����PendSV_Handler�ж�

u32 __vos_irq_save();
void __vos_irq_restore(u32 save);

#endif
