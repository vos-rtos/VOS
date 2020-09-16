/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vconf.h
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/


#ifndef __VCONF_H__
#define __VCONF_H__

#define VOS_LIST_DEBUG 0

#define TASK_LEVEL  0 //0:����������Ȩ��+PSP,1�����������̼߳�+PSP

#define VOS_SHELL_TEST 0 //�Ƿ�򿪲�������

#define MAX_VOS_SEMAPHONRE_NUM  10
#define MAX_VOS_MUTEX_NUM   10
#define MAX_VOS_MSG_QUE_NUM   10

//#define MAX_VOS_TASK_PRIO	255U // 0-255���ȼ���0��ߣ�255���
#define MAX_VOS_TASK_PRIO_NUM	256U // 0-255���ȼ�����266���� 0��ߣ�255���
#define MAX_VOS_TASK_PRIO_IDLE	(MAX_VOS_TASK_PRIO_NUM-1) // 0-255���ȼ���0��ߣ�255���


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

#define VOS_TASK_NOT_INHERITANCE   (0)  //Ĭ�������ȼ��̳����������ȼ���ת���⣬�������Ϊ1���򲻴���ת����


//#define MAX_VOS_TASK_ID	(u8)(0xFF-1) //0xFF��ʾ��ЧID

#define MAX_CPU_NUM 1
#define MAX_VOSTASK_NUM  10 //���ܳ���255, ���Ե���255

#define TICKS_INTERVAL_MS 1 //systick�ļ����1ms

#define MAKE_TICKS(ms) (((ms)+TICKS_INTERVAL_MS-1)/(TICKS_INTERVAL_MS))
#define MAKE_TIME_MS(ticks) (TICKS_INTERVAL_MS*(ticks))

#define MAX_VOS_TIEMR_NUM 10
#define VOS_TASK_TIMER_PRIO  10
#define VOS_TASK_VSHELL_PRIO  10
#define MAX_TICKS_TIMESLICE 10

#endif
