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
	s32 max;  //最大信号量个数
	s32 left; //目前剩余的信号量个数
	s8 *name; //信号量名字
	s32 distory; //删除互斥锁标志，需要把就绪队列里的所有等待该锁的阻塞任务添加到就绪队列
	struct list_head list;
}StVOSSemaphore;

//互斥锁跟信号量实现基本一样
typedef struct StVOSMutex {
	s32 counter; //最大为1，就是互斥锁
	s8 *name; //互斥锁名字
	s32 distory; //删除互斥锁标志，需要把就绪队列里的所有等待该锁的阻塞任务添加到就绪队列
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
	u8 *pstack; //指向任务自己的栈指针
	u8 *pstack_top; //栈顶指针
	u32 stack_size; //栈最大size
	s32 prio; //任务优先级，值越低，优先级越高
	u32 id; //任务唯一ID
	u8 *name; //任务名
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
	struct list_head list_sib;//相同优先级任务兄弟链表，基于时间片来轮询调度
}StVosTask;

void _set_PSP(u32 psp);
void _ISB();

#define MAX_CPU_NUM 1
#define MAX_VOSTASK_NUM  10

#define TICKS_INTERVAL_US 1000 //systick的间隔，1000us

#define MAKE_TICKS(us) (((us)+TICKS_INTERVAL_US-1)/(TICKS_INTERVAL_US))

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


//关中断，并返回关中断前的值，多用于嵌套中断情况
extern u32 __local_irq_save();
//开中断，并把中断前的值恢复，多用于嵌套中断情况
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
