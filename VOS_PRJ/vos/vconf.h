/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vconf.h
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/


#ifndef __VCONF_H__
#define __VCONF_H__

#define VOS_LIST_DEBUG 0

#define TASK_LEVEL  0 //0:任务工作在特权级+PSP,1：任务工作在线程级+PSP

#define VOS_SHELL_TEST 0 //是否打开测试命令

#define MAX_VOS_SEMAPHONRE_NUM  10
#define MAX_VOS_MUTEX_NUM   10
#define MAX_VOS_MSG_QUE_NUM   10

//#define MAX_VOS_TASK_PRIO	255U // 0-255优先级，0最高，255最低
#define MAX_VOS_TASK_PRIO_NUM	256U // 0-255优先级，共266个， 0最高，255最低
#define MAX_VOS_TASK_PRIO_IDLE	(MAX_VOS_TASK_PRIO_NUM-1) // 0-255优先级，0最高，255最低


#define MCU_FREQUENCY_HZ (u32)(168000000)

//#define MAX_SIGNED_VAL_64 (0x7FFFFFFFFFFFFFFF)
//#define MAX_UNSIGNED_VAL_64 (0xFFFFFFFFFFFFFFFF)
//
//#define TIMEOUT_INFINITY_U64 (0xFFFFFFFFFFFFFFFF)
//#define TIMEOUT_INFINITY_S64 (0x7FFFFFFFFFFFFFFF)



#define MAX_INFINITY_U32 (0xFFFFFFFFU)
#define MAX_INFINITY_S32 (0x7FFFFFFF)
#define TIMEOUT_INFINITY_U32 (0xFFFFFFFFU)
#define TIMEOUT_INFINITY_S32 (0x7FFFFFFF)
#define VOS_WAIT_FOREVER_U32 (0xFFFFFFFFU)
#define VOS_WAIT_FOREVER_S32 (0x7FFFFFFF)

//#define VOS_WAIT_FOREVER_U64 (0xFFFFFFFFFFFFFFFF)
//#define VOS_WAIT_FOREVER_S64 (0x7FFFFFFFFFFFFFFF)

#define VOS_TASK_NOT_INHERITANCE   (0)  //默认是优先级继承来处理优先级反转问题，如果定义为1，则不处理反转问题


//#define MAX_VOS_TASK_ID	(u8)(0xFF-1) //0xFF表示无效ID

#define MAX_CPU_NUM 1
#define MAX_VOSTASK_NUM  10 //不能超过255, 可以等于255

#define TICKS_INTERVAL_MS 1 //systick的间隔，1ms

#define MAKE_TICKS(ms) (((ms)+TICKS_INTERVAL_MS-1)/(TICKS_INTERVAL_MS))
#define MAKE_TIME_MS(ticks) (TICKS_INTERVAL_MS*(ticks))

#define MAX_VOS_TIEMR_NUM 10
#define VOS_TASK_TIMER_PRIO  10
#define VOS_TASK_VSHELL_PRIO  10
#define MAX_TICKS_TIMESLICE 10

#endif
