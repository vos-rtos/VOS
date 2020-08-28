/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vos.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/


#ifndef __VERROR_H__
#define __VERROR_H__

#define VERROR_OK				( 0)


#define VERROR_NO_ERROR			( 0) //无错误
#define VERROR_PARAMETER		(-1) //参数错误
#define VERROR_TIMEOUT			(-2) //超时
#define VERROR_NO_RESOURCES		(-3) //资源不够
#define VERROR_WAIT_RESOURCES	(-4) //需要继续等待
#define VERROR_DELETE_RESOURCES	(-5) //需要继续等待
#define VERROR_INT_CORTEX		(-6) //中断上下文不能执行错误
#define VERROR_MUTEX_RELEASE	(-7) //互斥锁只有拥有者才能释放
#define VERROR_NO_SCHEDULE		(-8) //系统还没开始调度
#define VERROR_EMPTY_RESOURCES	(-9) //资源为空
#define VERROR_FULL_RESOURCES	(-10) //资源为满
#define VERROR_TIMER_RUNNING	(-11) //定时器运行中
#define VERROR_UNKNOWN			(-100) //不明错误


#endif
