/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vatomic.h
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200912�������ļ�
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
