/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt
 *
 */


#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/err.h>
#if !NO_SYS
#include "sys_arch.h"
#endif
#include <lwip/stats.h>
#include <lwip/debug.h>
#include <lwip/sys.h>

#include <string.h>

#if !NO_SYS

/** Create a new mbox of specified size
 * @param mbox pointer to the mbox to create
 * @param size (miminum) number of messages in this mbox
 * @return ERR_OK if successful, another err_t otherwise */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	s32 ret = ERR_OK;
	u8 *pArrMsg = 0;

	if (mbox==0) {
		ret = ERR_ARG;
		goto SYS_MBOX_NEW;
	}

	sys_mbox_free(mbox);
	pArrMsg = (u8*)vmalloc(sizeof(struct StMsgItem)*size);
	if (pArrMsg == 0) {
		ret = ERR_MEM;
		goto SYS_MBOX_NEW;
	}
	mbox->vmalloc = pArrMsg;
	mbox->MsgQuePtr = VOSMsgQueCreate(pArrMsg, sizeof(struct StMsgItem)*size, sizeof(struct StMsgItem), "lwip_mq");
	if(mbox->MsgQuePtr == NULL) {
		if (mbox->vmalloc) {
			vfree(mbox->vmalloc);
			mbox->vmalloc = NULL;
		}
		ret = ERR_MEM;
		goto SYS_MBOX_NEW;
	}

SYS_MBOX_NEW:
	return ret;
}
/** Post a message to an mbox - may not fail
 * -> blocks if full, only used from tasks not from ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL) */
/*
 * 注意：这里设计在中断上下文处理数据，使用信号量，如果使用了mutex，必须在任务上下文处理数据。
 */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	struct StMsgItem item;
	if (mbox==0 || mbox->MsgQuePtr == 0) return;
	//if (CONTEXT_TASK != VOSCurContexStatus()) return;
	item.msg = msg;
	while (VERROR_NO_ERROR != VOSMsgQuePut(mbox->MsgQuePtr, &item, sizeof(item))) {
		VOSTaskDelay(1);
	}
}

/** Try to post a message to an mbox - may fail if full or ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL) */
/*
 * 注意：这里设计在中断上下文处理数据，使用信号量，如果使用了mutex，必须在任务上下文处理数据。
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	struct StMsgItem item;
	if (mbox==0) return ERR_MEM;
	if (mbox->MsgQuePtr == NULL) return ERR_MEM;
	//if (CONTEXT_TASK != VOSCurContexStatus()) return ERR_MEM;
	item.msg = msg;
	if (VERROR_NO_ERROR == VOSMsgQuePut(mbox->MsgQuePtr, &item, sizeof(item))) {
		return ERR_OK;
	}
	return ERR_MEM;
}

/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return time (in milliseconds) waited for a message, may be 0 if not waited
           or SYS_ARCH_TIMEOUT on timeout
 *         The returned time has to be accurate to prevent timer jitter! */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	struct StMsgItem item;
	u32 time_mark;
	if (mbox == 0) return SYS_ARCH_TIMEOUT;
	if (mbox->MsgQuePtr == NULL) return SYS_ARCH_TIMEOUT;
	time_mark = VOSGetTimeMs();
	if (VERROR_NO_ERROR != VOSMsgQueGet(mbox->MsgQuePtr, &item, sizeof(item), timeout)){
		return SYS_ARCH_TIMEOUT;
	}
	*msg = item.msg;
	return VOSGetTimeMs()-time_mark;
}

/** Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message
 * @return 0 (milliseconds) if a message has been received
 *         or SYS_MBOX_EMPTY if the mailbox is empty */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	u32_t ret = 0;
	if (mbox == 0) return SYS_MBOX_EMPTY;
	if (mbox->MsgQuePtr == NULL) return SYS_MBOX_EMPTY;
	if (mbox->MsgQuePtr->msg_cnts == 0) return SYS_MBOX_EMPTY;
	if (SYS_ARCH_TIMEOUT == (ret=sys_arch_mbox_fetch(mbox, msg, 0))){
		return SYS_MBOX_EMPTY;
	}
	return ret;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg)
{
  return sys_mbox_trypost(q, msg);
}


/** Delete an mbox
 * @param mbox mbox to delete */
void sys_mbox_free(sys_mbox_t *mbox)
{
	if (mbox) {
		if (mbox->MsgQuePtr) {
			VOSMsgQueFree(mbox->MsgQuePtr);
			mbox->MsgQuePtr = NULL;
		}
		if (mbox->vmalloc) {
			vfree(mbox->vmalloc);
			mbox->vmalloc = NULL;
		}
	}
}

/** Check if an mbox is valid/allocated: return 1 for valid, 0 for invalid */
int sys_mbox_valid(sys_mbox_t *mbox)
{
	int ret = 0;
	if (mbox == 0) return 0;
	if (mbox->MsgQuePtr == NULL) return 0;
	if (mbox->MsgQuePtr->msg_cnts == mbox->MsgQuePtr->msg_maxs) {
		kprintf("warning: sys_mbox_valid overflow!\r\n");
	}
	if (mbox->MsgQuePtr->msg_cnts < mbox->MsgQuePtr->msg_maxs) {
		ret = 1;
	}
	return ret;
}

/** Set an mbox invalid so that sys_mbox_valid returns 0 */
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	if (mbox) {
		sys_mbox_free(mbox);
	}
}

/** Create a new semaphore
 * @param sem pointer to the semaphore to create
 * @param count initial count of the semaphore
 * @return ERR_OK if successful, another err_t otherwise */
err_t sys_sem_new(sys_sem_t* sem, u8_t count)
{
	if (sem && *sem) {
		sys_sem_free(*sem);
	}
	*sem = VOSSemCreate(0xFFFF, count, "sem_lwip");
	if (*sem == 0) return ERR_MEM;
	return ERR_OK;
}
/** Signals a semaphore
 * @param sem the semaphore to signal */
void sys_sem_signal(sys_sem_t *sem)
{
	if (sem == 0 || *sem == 0) return ;
	VOSSemRelease(*sem);
}

/** Wait for a semaphore for the specified timeout
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return time (in milliseconds) waited for the semaphore
 *         or SYS_ARCH_TIMEOUT on timeout */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	u32 time_mark;
	struct StMsgItem item;
	if (sem == 0 || *sem == 0) return SYS_ARCH_TIMEOUT;
	time_mark = VOSGetTimeMs();
	if (timeout == 0) {//(0 = wait forever)
		timeout = VOS_WAIT_FOREVER_U32;
	}
	if (VERROR_NO_ERROR != VOSSemWait(*sem, timeout)){
		return SYS_ARCH_TIMEOUT;
	}
	return VOSGetTimeMs()-time_mark;
}

/** Delete a semaphore
 * @param sem semaphore to delete */
void sys_sem_free(sys_sem_t *sem)
{
	if (sem == 0 || *sem == 0) return ;
	VOSSemDelete(*sem);
	*sem = (void*)0;
}

/** Check if a sempahore is valid/allocated: return 1 for valid, 0 for invalid */
int sys_sem_valid(sys_sem_t *sem)
{
	if (sem == 0 || *sem == 0) return 0;
	return 1;
}

/** Set a semaphore invalid so that sys_sem_valid returns 0 */
void sys_sem_set_invalid(sys_sem_t *sem)
{
	sys_sem_free(sem);
}

/* sys_init() must be called before anthing else. */
void sys_init()
{
	//do something
}


/** The only thread function:
 * Creates a new thread
 * @param name human-readable name for the thread (used for debugging purposes)
 * @param thread thread-function
 * @param arg parameter passed to 'thread'
 * @param stacksize stack size in bytes for the new thread (may be ignored by ports)
 * @param prio priority of the new thread (may be ignored by ports) */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	s32 task_id = -1;
	u8 *pstack = 0;

	if (stacksize <= 0) {
		stacksize = 0x4000;
		pstack = (u8*)vmalloc(stacksize);
	}
	else {
		pstack = (u8*)vmalloc(stacksize);
	}

	if (prio <= 0) {
		prio = TASK_PRIO_NORMAL;
	}

	task_id = VOSTaskCreate(thread, arg, pstack, stacksize, prio, name);

	return task_id;
}

/** Returns the current time in milliseconds,
 * may be the same as sys_jiffies or at least based on it. */
u32_t sys_now()
{
	return VOSGetTimeMs();
}


#if LWIP_NETCONN_SEM_PER_THREAD
#error LWIP_NETCONN_SEM_PER_THREAD==1 not supported
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#endif /* !NO_SYS */
