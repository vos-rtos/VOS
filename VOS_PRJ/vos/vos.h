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


typedef void (*task_fun_t)(void *param);

/*
  * 位图操作宏定义
 */
#define bitmap_for_each(pos, bitmap_byte) \
	for (pos = 0; pos < bitmap_byte*8; pos++)

#define bitmap_get(n, bitmap)		(!!(((u8*)(bitmap))[(n)>>3] & 1<<((n)&0x07)))
#define bitmap_clear(n, bitmap)		(((u8*)(bitmap))[(n)>>3] &= ~(1<<((n)&0x07)))
#define bitmap_set(n, bitmap)		(((u8*)(bitmap))[(n)>>3] |= 1<<((n)&0x07))



#define MCU_FREQUENCY_HZ (u32)(168000000)

#define MAX_SIGNED_VAL_64 (0x7FFFFFFFFFFFFFFF)

#define VOS_TASK_NOT_INHERITANCE   (0)  //默认是优先级继承来处理优先级反转问题，如果定义为1，则不处理反转问题

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
	TASK_SWITCH_ACTIVE = 0,
	TASK_SWITCH_PASSIVE,
};


void _set_PSP(u32 psp);
void _ISB();

#define MAX_CPU_NUM 1
#define MAX_VOSTASK_NUM  10

#define TICKS_INTERVAL_MS 1 //systick的间隔，1ms

#define MAKE_TICKS(ms) (((ms)+TICKS_INTERVAL_MS-1)/(TICKS_INTERVAL_MS))
#define MAKE_TIME_MS(ticks) (TICKS_INTERVAL_MS*(ticks))

#define HW32_REG(ADDRESS) (*((volatile unsigned long *)(ADDRESS)))

#define MAX_TICKS_TIMESLICE 10


#define VOS_WAKEUP_FROM_SEM			(u32)(0)
#define VOS_WAKEUP_FROM_SEM_DEL		(u32)(1) //删除信号量，必须通知的各等待信号量的任务添加到就绪队列
#define VOS_WAKEUP_FROM_DELAY		(u32)(2)
#define VOS_WAKEUP_FROM_MUTEX		(u32)(3)
#define VOS_WAKEUP_FROM_MUTEX_DEL	(u32)(4) //删除互斥锁，必须通知的各等待互斥锁的任务添加到就绪队列
#define VOS_WAKEUP_FROM_EVENT		(u32)(5)
#define VOS_WAKEUP_FROM_EVENT_DEL	(u32)(6) //删除事件，必须通知的各等待事件的任务添加到就绪队列
#define VOS_WAKEUP_FROM_MSGQUE		(u32)(7)
#define VOS_WAKEUP_FROM_MSGQUE_DEL	(u32)(8)


#define	VOS_STA_FREE 			(u32)(0) //空闲队列回收
#define VOS_STA_READY			(u32)(1) //就绪队列
#define VOS_STA_BLOCK			(u32)(2) //就绪队列


#define VOS_BLOCK_DELAY			(u32)(1<<0) //自延时引起阻塞
#define VOS_BLOCK_SEMP			(u32)(1<<1) //信号量阻塞
#define VOS_BLOCK_EVENT			(u32)(1<<2) //事件阻塞
#define VOS_BLOCK_MUTEX			(u32)(1<<3) //互斥阻塞
#define VOS_BLOCK_MSGQUE		(u32)(1<<4) //消息队列阻塞

#define TASK_PRIO_INVALID		(s32)(-1) //次优先级分配给IDLE
#define TASK_PRIO_REAL			(s32)(100)
#define TASK_PRIO_HIGH			(s32)(130)
#define TASK_PRIO_NORMAL		(s32)(150)
#define TASK_PRIO_LOW			(s32)(200)
#define TASK_PRIO_MAX			(s32)(255) //次优先级分配给IDLE

#define VOS_SVC_NUM_STARTUP			(u32)(0) //svc 0 加载第一个任务
#define VOS_SVC_NUM_SCHEDULE		(u32)(1) //svc 1 主动调度
#define VOS_SVC_NUM_DELAY			(u32)(3) //svc 3 系统延时，提高效率
#define VOS_SVC_NUM_SYSCALL			(u32)(5) //svc 5 进入系统调用例程

#define VOS_SYSCALL_MUTEX_CREAT   	(u32)(10)
#define VOS_SYSCALL_MUTEX_WAIT		(u32)(11)
#define VOS_SYSCALL_MUTEX_RELEASE	(u32)(12)
#define VOS_SYSCALL_MUTEX_DELETE	(u32)(13)

#define VOS_SYSCALL_SEM_CREATE		(u32)(14)
#define VOS_SYSCALL_SEM_WAIT		(u32)(15)
#define VOS_SYSCALL_SEM_TRY_WAIT	(u32)(16)
#define VOS_SYSCALL_SEM_RELEASE		(u32)(17)
#define VOS_SYSCALL_SEM_DELETE		(u32)(18)

#define VOS_SYSCALL_EVENT_WAIT		(u32)(19)
#define VOS_SYSCALL_EVENT_SET		(u32)(20)
#define VOS_SYSCALL_EVENT_GET		(u32)(21)
#define VOS_SYSCALL_EVENT_CLEAR		(u32)(22)

#define VOS_SYSCALL_MSGQUE_CREAT	(u32)(23)
#define VOS_SYSCALL_MSGQUE_PUT		(u32)(24)
#define VOS_SYSCALL_MSGQUE_GET		(u32)(25)
#define VOS_SYSCALL_MSGQUE_FREE		(u32)(26)

#define VOS_SYSCALL_TASK_DELAY   	(u32)(27)

#define VOS_SYSCALL_TASK_CREATE		(u32)(28)

#define VOS_SYSCALL_SCH_TAB_DEBUG	(u32)(29)

typedef struct StVosSysCallParam {
	u32 call_num; //系统调用号
	u32 u32param0; //参数32位0
	u32 u32param1; //参数32位1
	u32 u32param2; //参数32位2
	u32 u32param3; //参数32位3
	u32 u32param4; //参数32位4
	u32 u32param5; //参数32位5
	u64 u64param0; //参数64位0
	u64 u64param1; //参数64位1
}StVosSysCallParam;

typedef struct StVOSSemaphore {
	s32 max;  //最大信号量个数
	s32 left; //目前剩余的信号量个数
	s8 *name; //信号量名字
	u8 bitmap[(MAX_VOSTASK_NUM+sizeof(u8)-1)/sizeof(u8)];//每位特的偏移数就是被占用任务的id.
	s32 distory; //删除互斥锁标志，需要把就绪队列里的所有等待该锁的阻塞任务添加到就绪队列
	struct list_head list;
}StVOSSemaphore;

//互斥锁跟信号量实现基本一样
typedef struct StVOSMutex {
	s32 counter; //最大为1，就是互斥锁
	s8 *name; //互斥锁名字
	s32 distory; //删除互斥锁标志，需要把就绪队列里的所有等待该锁的阻塞任务添加到就绪队列
	struct StVosTask *ptask_owner; //指向某个任务拥有这个锁，只有一个任务拥有这个锁
	struct list_head list;
}StVOSMutex;

//消息队列都固定size的item，环形缓冲实现消息队列
typedef struct StVOSMsgQueue {
	s8 *name;     //消息队列名字
	u8 *pdata;    //内存指针，用户提供
	s32 length;   //开始分配总的size
	s32 pos_head; //指向消息队列头,索引表示
	s32 pos_tail; //指向消息队列尾,索引表示
	s32 msg_cnts; //队列里有效的消息个数
	s32 msg_maxs; //队列里消息总个数
	s32 msg_size; //每个消息的大小（字节）
	s32 distory; //删除互斥锁标志，需要把就绪队列里的所有等待该锁的阻塞任务添加到就绪队列
	struct list_head list;
}StVOSMsgQueue;

typedef struct StVosTask {
	u8 *pstack; //指向任务自己的栈指针, 必须放到结构体第一个位置，汇编里要使用这个成员

	u8 *pstack_top; //栈顶指针
	u32 stack_size; //栈最大size
	s32 prio; //任务优先级，值越低，优先级越高
	s32 prio_base; //原始优先级。
	u32 id; //任务唯一ID
	s8 *name; //任务名
	volatile u32 status; //任务状态
	volatile u32 block_type; //如果阻塞，则这里指名阻塞类型
	void (*task_fun)(void *param);
	void *param;
	s64 ticks_start;//闹钟设置点
	s64 ticks_alert;//闹钟响节点
	u32 ticks_timeslice; //时间片
	void *psyn; //指向同步信息控制块
	u32 wakeup_from; //唤醒同步信号的来源，
	u32 event_mask;  //事件类型，32位事件
	u32 event;  //事件类型，32位事件

	struct list_head list;//空闲链表和优先级任务链表,优先级高的排第头，优先级低的排尾
}StVosTask;


enum {
	VOS_TIMER_TYPE_ONE_SHOT,
	VOS_TIMER_TYPE_PERIODIC,
};

enum {
	VOS_TIMER_STA_FREE,
	VOS_TIMER_STA_RUNNING,
	VOS_TIMER_STA_STOPPED,
};

#define MAX_VOS_TIEMR_NUM 10
#define VOS_TASK_TIMER_PRIO  10

typedef  void (*VOS_TIMER_CB)(void *ptimer, void *parg);

typedef struct StVOSTimer{
	u32	type; //两种类型，one shot 和 periodic
	u32 ticks_alert; //结束时间点
	u32 ticks_start; //开始时间点
	u32 ticks_delay; //设定的延时
	VOS_TIMER_CB callback; //定时器到，调用回调函数
	void *arg; //回调函数的参数
	s8 *name; //定时器名字
	u32 status; //状态
	struct list_head list;
} StVOSTimer;


void VOSTimerInit();
StVOSTimer *VOSTimerCreate(s32 type, u32 delay_ms, VOS_TIMER_CB callback, void *arg, s8 *name);
s32 VOSTimerDelete(StVOSTimer *pTimer);
s32 VOSTimerStart(StVOSTimer *pTimer);
s32 VOSTimerStop(StVOSTimer *pTimer);
s32 VOSTimerGetStatus(StVOSTimer *pTimer);
s32 VOSTimerGetLeftTime(StVOSTimer *pTimer);


//关中断，并返回关中断前的值，多用于嵌套中断情况
extern u32 __local_irq_save();
//开中断，并把中断前的值恢复，多用于嵌套中断情况
extern void __local_irq_restore(u32 old);

extern void local_irq_disable(void);

extern void local_irq_enable(void);

void VOSSemInit();
StVOSSemaphore *VOSSemCreate(s32 max_sems, s32 init_sems, s8 *name);
s32 VOSSemWait(StVOSSemaphore *pSem, u64 timeout_ms);
s32 VOSSemTryWait(StVOSSemaphore *pSem);
s32 VOSSemRelease(StVOSSemaphore *pSem);
s32 VOSSemDelete(StVOSSemaphore *pSem);

StVOSSemaphore *VOSSemCreateSysCall(StVosSysCallParam *psa);
s32 SysCallVOSSemTryWait(StVosSysCallParam *psa);
s32 SysCallVOSSemWait(StVosSysCallParam *psa);
s32 SysCallVOSSemRelease(StVosSysCallParam *psa);

void VOSMutexInit();
StVOSMutex *VOSMutexCreate(s8 *name);
s32 VOSMutexWait(StVOSMutex *pMutex, s64 timeout_ms);
s32 VOSMutexRelease(StVOSMutex *pMutex);
s32 VOSMutexDelete(StVOSMutex *pMutex);
s32 SysCallVOSSemDelete(StVosSysCallParam *psa);

StVOSMutex *SysCallVOSMutexCreate(StVosSysCallParam *psa);
s32 SysCallVOSMutexWait(StVosSysCallParam *psa);
s32 SysCallVOSMutexRelease(StVosSysCallParam *psa);
s32 SysCallVOSMutexDelete(StVosSysCallParam *psa);

s32 VOSEventWait(u32 event_mask, u64 timeout_ms);
s32 VOSEventSet(s32 task_id, u32 event);
u32 VOSEventGet(s32 task_id);
s32 VOSEventClear(s32 task_id, u32 event);

s32 SysCallVOSEventWait(StVosSysCallParam *psa);
s32 SysCallVOSEventSet(StVosSysCallParam *psa);
u32 SysCallVOSEventGet(StVosSysCallParam *psa);
s32 SysCallVOSEventClear(StVosSysCallParam *psa);

void VOSMsgQueInit();
StVOSMsgQueue *VOSMsgQueCreate(s8 *pRingBuf, s32 length, s32 msg_size, s8 *name);
s32 VOSMsgQuePut(StVOSMsgQueue *pMQ, void *pmsg, s32 len);
s32 VOSMsgQueGet(StVOSMsgQueue *pMQ, void *pmsg, s32 len, s64 timeout_ms);
s32 VOSMsgQueFree(StVOSMsgQueue *pMQ);

StVOSMsgQueue *SysCallVOSMsgQueCreate(StVosSysCallParam *psa);
s32 SysCallVOSMsgQuePut(StVosSysCallParam *psa);
s32 SysCallVOSMsgQueGet(StVosSysCallParam *psa);
s32 SysCallVOSMsgQueFree(StVosSysCallParam *psa);

u32 VOSTaskDelay(u32 ms);
u32 SysCallVOSTaskDelay(StVosSysCallParam *psa);

s64 VOSGetTicks();
s64 VOSGetTimeMs();

//任务上下文使用VOSTaskCreate
s32 VOSTaskCreate(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm);
s32 SysCallVOSTaskCreate(StVosSysCallParam *psa);

//创建任务，不能在任务上下文创建(任务上下文使用VOSTaskCreate)
s32 VOSTaskInBuild(void (*task_fun)(void *param), void *param,
		void *pstack, u32 stack_size, s32 prio, s8 *task_nm);
u32 VOSTaskInit();
StVosTask *VOSGetTaskFromId(s32 task_id);
u32 VOSTaskListPrioInsert(StVosTask *pTask, s32 which_list);
s32 VOSTaskReadyCmpPrioTo(StVosTask *pRunTask);
StVosTask *VOSTaskReadyCutPriorest();
void VOSTaskEntry(void *param);
void VOSTaskBlockWaveUp();
void VOSStarup();
void VOSTaskSchedule();
void VOSSysTick();
s32 VOSTaskRaisePrioBeforeBlock(StVosTask *pMutexOwnerTask);
s32 VOSTaskRestorePrioBeforeRelease();


extern void asm_ctx_switch(); //触发PendSV_Handler中断

#endif
