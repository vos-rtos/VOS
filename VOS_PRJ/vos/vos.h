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

//A: 当前全局的tick
//B: 比较对象的终止tick, 注意终止tick数值不一定少于开始tick，不能直接比较，使用距离比较法
//C: 比较对象的开始tick
//返回结果： A比B小，返回-1，A比B大返回1， A等于B返回0
//注意：这里A,B,S必须是无符号比较，支持溢出
#define TICK_CMP(A,B,S) ((A)-(S) < (B)-(S) ? -1 : ((A) == (B) ? 0 : 1))

#define EVENT_USART1_RECV	(1<<0)

typedef void (*task_fun_t)(void *param);

//定义vshell函数声明
#define VSHELL_FUN(f) \
	f(s8 **parr, s32 cnts);\
	const char *f##_name __attribute__((section(".vshell_name"),noload,used))=#f; \
	const char *f##_note __attribute__((section(".vshell_note"),noload,used))=0; \
	extern void f(s8 **parr, s32 cnts);\
	void *f##_fun __attribute__((section(".vshell_fun"),noload,used))=(void*)f;\
	void (f) \

//定义vshell函数声明, ins添加声明
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
#define VOS_STA_RUNNING			(u32)(3) //就绪队列

#define VOS_BLOCK_DELAY			(u32)(1<<0) //自延时引起阻塞
#define VOS_BLOCK_SEMP			(u32)(1<<1) //信号量阻塞
#define VOS_BLOCK_EVENT			(u32)(1<<2) //事件阻塞
#define VOS_BLOCK_MUTEX			(u32)(1<<3) //互斥阻塞


#define BLOCK_SUB_MASK (VOS_BLOCK_SEMP|VOS_BLOCK_MUTEX|VOS_BLOCK_EVENT/*|VOS_BLOCK_MSGQUE*/)

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
#define VOS_SVC_PRIVILEGED_MODE		(u32)(6) //svc 6 切换到特权模式

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
	struct list_head list_task;//阻塞任务链表
}StVOSSemaphore;

//互斥锁跟信号量实现基本一样
typedef struct StVOSMutex {
	s32 counter; //最大为1，就是互斥锁
	s8 *name; //互斥锁名字
	s32 distory; //删除互斥锁标志，需要把就绪队列里的所有等待该锁的阻塞任务添加到就绪队列
	struct StVosTask *ptask_owner; //指向某个任务拥有这个锁，只有一个任务拥有这个锁
	struct list_head list;
	struct list_head list_task;//阻塞任务链表
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
	StVOSSemaphore  *psem;
}StVOSMsgQueue;

typedef struct StVosTask {
	u32 *pstack; //指向任务自己的栈指针, 必须放到结构体第一个位置，汇编里要使用这个成员

	u32 *pstack_top; //栈顶指针
	u32 stack_size; //栈最大size
	s32 prio; //任务优先级，值越低，优先级越高
	s32 prio_base; //原始优先级。
	u32 id; //任务唯一ID
	s8 *name; //任务名
	volatile u32 status; //任务状态
	volatile u32 block_type; //如果阻塞，则这里指名阻塞类型
	void (*task_fun)(void *param);
	void *param;
	u32 ticks_start;//闹钟设置点，必须使用TICK_CMP进行比较
	u32 ticks_alert;//闹钟响节点
	u32 ticks_timeslice; //时间片
	void *psyn; //指向同步信息控制块
	u32 wakeup_from; //唤醒同步信号的来源，
	u32 event_mask;  //事件类型，32位事件
	u32 event_stop;  //停止接收指定的事件
	u32 ticks_used_start; //每次进入cpu,记录开始时间，等待切换出去时，把当前时间减去这个时间然后累加到ticks_used_cnts
	u32 ticks_used_cnts; //统计cpu使用率
	struct list_head list;//空闲链表和优先级任务链表,优先级高的排第头，优先级低的排尾
	struct list_head list_delay;//定时任务列表，注意这里包括单独延时也包括信号量超时任务。
}StVosTask;


typedef struct StTaskInfo {
	s32 id;
	s32 prio;
	u32 ticks;
	s32 stack_top;
	s32 stack_size;
	s32 stack_ptr;//当前指向的位置
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


u32 VOSTaskBlockListInsert(StVosTask *pInsertTask, struct list_head *phead);
u32 VOSTaskBlockListRelease(StVosTask *pReleaseTask);

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

//任务上下文使用VOSTaskCreate
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


//检查目前运行在中断上下文还是任务上下文。
s32 VOSCortexCheck();

extern void asm_ctx_switch(); //触发PendSV_Handler中断

u32 __vos_irq_save();
void __vos_irq_restore(u32 save);

#endif
