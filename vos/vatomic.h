/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vatomic.h
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200912：创建文件
*********************************************************************************************************/
#ifndef __VATOMIC_H__
#define __VATOMIC_H__

#include "vtype.h"

typedef struct StAtomicLock {
	volatile s32 lock;
}StAtomicLock;


void VAtomicLockInit(StAtomicLock *pAtom);
s32 VAtomicLockTry(StAtomicLock *pAtom);
void VAtomicLock(StAtomicLock *pAtom);
void VAtomicUnlock(StAtomicLock *pAtom);

u32 VAtomicLockIrqSave(StAtomicLock *pAtom);
void VAtomicLockIrqRestore(StAtomicLock *pAtom, u32 irq_save);

#endif
