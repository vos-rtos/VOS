/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
/*  Porting by Michael Vysotsky <michaelvy@hotmail.com> August 2011   */

#define SYS_ARCH_GLOBALS

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/lwip_sys.h"
#include "lwip/mem.h"
#include "arch/sys_arch.h"

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	u8 *pArrMsg = (u8*)vmalloc(sizeof(struct StMsgItem)*size);
	mbox->vmalloc = pArrMsg;
	mbox->MsgQuePtr = VOSMsgQueCreate(pArrMsg, sizeof(struct StMsgItem)*size, sizeof(struct StMsgItem), "lwip_mq");
	if(mbox->MsgQuePtr != NULL)return ERR_OK;
	else return ERR_MEM;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	u8_t ucErr;
	VOSMsgQueFree(mbox->MsgQuePtr);
	vfree(mbox->vmalloc);
	mbox = NULL;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{	    
	if (msg == 0) return;
	struct StMsgItem item;
	item.msg = msg;
	while (VERROR_NO_ERROR != VOSMsgQuePut(mbox->MsgQuePtr, &item, sizeof(item))) {
		VOSTaskDelay(10*1000);
	}
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{ 
	struct StMsgItem item;
	if (msg == 0) return ERR_MEM;
	item.msg = msg;
	if (VERROR_NO_ERROR == VOSMsgQuePut(mbox->MsgQuePtr, &item, sizeof(item))) {
		return ERR_OK;
	}
	return ERR_MEM;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{ 
	struct StMsgItem item;
	u32 time_mark = VOSGetTimeMs();
	if (VERROR_NO_ERROR != VOSMsgQueGet(mbox->MsgQuePtr, &item, sizeof(item), timeout)){
		return SYS_ARCH_TIMEOUT;
	}
	*msg = item.msg;
	return VOSGetTimeMs()-time_mark;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	return sys_arch_mbox_fetch(mbox,msg,1);//尝试获取一个消息
}

int sys_mbox_valid(sys_mbox_t *mbox)
{  
	int ret = 0;
	ret = mbox->MsgQuePtr->msg_cnts < mbox->MsgQuePtr->msg_maxs;
	if (mbox->MsgQuePtr->msg_cnts == mbox->MsgQuePtr->msg_maxs) {
		kprintf("warning: sys_mbox_valid overflow!\r\n");
	}
	return ret;
} 

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	//mbox=NULL;
} 

err_t sys_sem_new(sys_sem_t* sem, u8_t count)
{
	*sem = VOSSemCreate(count, count, "sem_lwip");
	if (*sem == NULL) return ERR_MEM;
	return ERR_OK;
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	struct StMsgItem item;
	u32 time_mark = VOSGetTimeMs();
	if (VERROR_NO_ERROR != VOSSemWait(*sem, timeout)){
		return SYS_ARCH_TIMEOUT;
	}
	return VOSGetTimeMs()-time_mark;
}

void sys_sem_signal(sys_sem_t *sem)
{
	VOSSemRelease(*sem);
}

void sys_sem_free(sys_sem_t *sem)
{
	VOSSemDelete(*sem);
	*sem = (void*)0;
}

int sys_sem_valid(sys_sem_t *sem)
{
	return *sem != 0;
} 

void sys_sem_set_invalid(sys_sem_t *sem)
{
	*sem = NULL;
} 

void sys_init()
{ 
} 

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	s32 task_id = -1;
	u8 *pstack = 0;
	if(strcmp(name,TCPIP_THREAD_NAME)==0)//创建TCP IP内核任务
	{
		pstack = (u8*)vmalloc(stacksize);
		task_id = VOSTaskCreate(thread, arg, pstack, stacksize, prio, name);
	} 
	return task_id;
} 

u32_t sys_now()
{
	return VOSGetTimeMs();
}



