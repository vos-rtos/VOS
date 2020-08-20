//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------


#ifndef __VCONF_H__
#define __VCONF_H__

#define TASK_LEVEL  0 //0:����������Ȩ��+PSP,1�����������̼߳�+PSP

#define MAX_VOS_SEMAPHONRE_NUM  10
#define MAX_VOS_MUTEX_NUM   10
#define MAX_VOS_MSG_QUE_NUM   10


#define MCU_FREQUENCY_HZ (u32)(168000000)

#define MAX_SIGNED_VAL_64 (0x7FFFFFFFFFFFFFFF)
#define MAX_UNSIGNED_VAL_64 (0xFFFFFFFFFFFFFFFF)

#define TIMEOUT_INFINITY_U64 (0xFFFFFFFFFFFFFFFF)
#define TIMEOUT_INFINITY_S64 (0x7FFFFFFFFFFFFFFF)

#define TIMEOUT_INFINITY_U32 (0xFFFFFFFF)
#define TIMEOUT_INFINITY_S32 (0x7FFFFFFF)

#define VOS_TASK_NOT_INHERITANCE   (0)  //Ĭ�������ȼ��̳����������ȼ���ת���⣬�������Ϊ1���򲻴���ת����


#define MAX_CPU_NUM 1
#define MAX_VOSTASK_NUM  10

#define TICKS_INTERVAL_MS 1 //systick�ļ����1ms

#define MAKE_TICKS(ms) (((ms)+TICKS_INTERVAL_MS-1)/(TICKS_INTERVAL_MS))
#define MAKE_TIME_MS(ticks) (TICKS_INTERVAL_MS*(ticks))

#define MAX_VOS_TIEMR_NUM 10
#define VOS_TASK_TIMER_PRIO  10
#define VOS_TASK_VSHELL_PRIO  10
#define MAX_TICKS_TIMESLICE 10

#endif
