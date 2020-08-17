//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-01: initial by vincent.
//------------------------------------------------------


.syntax         unified
.thumb
.file   "cpu.S"

.global SVC_EXC_RETURN

.text
.align 2
.global SVC_Handler
.thumb
.thumb_func
SVC_Handler:
	tst	lr, #4
	ite	eq
	mrseq	r0, msp
	mrsne	r0, psp
	ldr	r1, =SVC_EXC_RETURN
	str	lr, [r1]
	bl	SVC_Handler_C
	ldr	r1, =SVC_EXC_RETURN
	ldr	lr, [r1]
	bx	lr

.text
.align 2
.global asm_ctx_switch
.thumb
.thumb_func
asm_ctx_switch:
	LDR     R0, =0xE000ED04 //触发PendSV_Handler中断
	LDR     R1, =0x10000000
	STR     R1, [R0]
	BX      LR

.text
.align 2
.global PendSV_Handler
.thumb
.thumb_func
PendSV_Handler:
    MRS     r0, PRIMASK //关中断
    CPSID   I
    push  {r0}
	//判断pRunningTask和pReadyTask指针是否为0，如果为0就不切换
    ldr r1, =pRunningTask
    ldr r2, =pReadyTask
	ldr r1, [r1]
	ldr r2, [r2]
	cmp r1, #0
	beq  INT_ENABLE
	cmp r2, #0
	beq  INT_ENABLE

	mrs	r0, psp //保存当前上下文
	tst	lr, #0x10 //测试浮点是否需要压栈
	it eq
	vstmdbeq r0!, {s16-s31}
	mov r2, lr  //r2 为中断进入前的lr
	mrs r3, control //r3为进入前的control
	stmdb r0!, {r2-r11} //r2,r3(lr,control), r4-r11 保存到任务栈
	ldr r1, =pRunningTask //保存当前运行任务的地址
	ldr r2, [r1]
	str r0, [r2] //psp保存到pRunningTask->ptask
	ldr r3, =pReadyTask
	ldr r4, [r3]
	str r4, [r1] //pReadyTask指向的任务赋值给pRunningTask
	ldr r0, [r4] //[r4]为pReadyTask的psp指针
	mov r2, #0
	str r2, [r3] //清空pReadyTask的值
	ldmia r0!, {r2-r11} //把pReadyTask->ptask栈恢复到当前的寄存器
	mov lr, r2
	msr control, r3
	isb
	tst lr, #0x10
	it eq
	vldmiaeq r0!, {s16-s31}
	msr psp, r0

INT_ENABLE:
	pop {r0}
	MSR PRIMASK, R0

	bx lr //返回后才切换到psp

//.text
//.align 2
//.global RunFirstTask
//.thumb
//.thumb_func
//RunFirstTask:
	//push	{lr}
//    LDR     R0, =VOSRunning //设置系统运行标志
//  MOVS    R1, #1
//   STRB    R1, [R0]

//	LDR 	R0, =pRunningTask
//	LDR		R1, =SVC_EXC_RETURN
//	LDR		R2, [R0]
//	LDR		R2, [R2] //ptask->pstack
//	LDR		R3, [R2] //*ptask->pstack
//	STR		R3,	[R1] //写SVC中断的LR值
//	MOV		R0, #10
//    LSL		R0, R0, #2 // 10 * 4
//	ADD 	R1, R2, R0
//    MSR		PSP, R1    //设置PSP的值
//	LDR     R0, =0xE000ED22 //设置PendSV_Handler为最低优先级
//	LDR     R1, =0xFF
//	STRB    R1, [R0]

//	MRS     R0, CONTROL //设置线程模式)，用户级
//	ORR     R0, R0, #2 //#2:特权+PSP, #3:用户级+PSP, 刚启动，这里需要特权模式
//	MSR     CONTROL, R0
//	ISB
	//BL		VOSSysTickSet //设置tick间隔
	//ISB
	//pop 	{pc}


.text
.align 2
.global RunFirstTask
.thumb
.thumb_func
RunFirstTask:
	//push	{lr}
	CPSID    I

    //设置PendSV_Handler为最低优先级
   	LDR     R0, =0xE000ED22
	LDR     R1, =0xFF
	STRB    R1, [R0]

	LDR 	R0, =pRunningTask
	LDR		R1, =SVC_EXC_RETURN
	LDR		R2, [R0]
	LDR		R2, [R2] //ptask->pstack
	LDR		R3, [R2] //*ptask->pstack
	//STR		R3,	[R1] //写SVC中断的LR值
	MOV		R0, #10
    LSL		R0, R0, #2 // 10 * 4
	ADD 	R1, R2, R0
    MSR		PSP, R1    //设置PSP的值, PSP指向 r0, r0->r1->r2->r3->r12->lr->pc->xPSR

	MRS     R0, CONTROL //设置线程模式)，用户级
	ORR     R0, R0, #2 //#2:特权+PSP, #3:用户级+PSP, 刚启动，这里需要特权模式
	MSR     CONTROL, R0
	ISB

	LDMIA   SP!, {R0-R3}
	LDMIA   SP!, {R12, LR}
	LDMIA   SP!, {R3, R4} //R4:xPSR, R3: PC

	CPSIE    I
	//设置回用户模式
	ORR     R2, R2, #3 //#2:特权+PSP, #3:用户级+PSP, 刚启动，这里需要特权模式
	MSR     CONTROL, R2
	ISB

	//设置系统运行标志
    LDR     R2, =VOSRunning
    MOVS    R1, #1
    STRB    R1, [R2]

	//跳转到 任务
	BX		R3 //R3: PC

.text
.align 2
.global _set_PSP
.thumb
.thumb_func
_set_PSP:
	msr psp, r0
	bx lr

.text
.align 2
.global __local_irq_save
.thumb
.thumb_func
__local_irq_save:
    MRS     R0, PRIMASK
    CPSID   I
    ldr 	R1, =vos_dis_irq_counter //计数嵌套层数
    ldr		R2, [R1]
    add		R2, #1 //加1
    str		R2, [R1]
    BX      LR

.text
.align 2
.global __local_irq_restore
.thumb
.thumb_func
__local_irq_restore:
	ldr 	R1, =vos_dis_irq_counter //计数嵌套层数
    ldr		R2, [R1]
    sub		R2, #1 //减1
    str		R2, [R1]
    MSR     PRIMASK, R0
    BX      LR


.text
.align 2
.global local_irq_disable
.thumb
.thumb_func
local_irq_disable:
    cpsid	i
    bx      lr

.text
.align 2
.global local_irq_enable
.thumb
.thumb_func
local_irq_enable:
    cpsie	i
    bx      lr


.text
.align 2
.global VOSDelayUs
.thumb
.thumb_func
VOSDelayUs:
	mov r1, #172  //168MHz: 1us执行168条指令 + 4条指令
	asr r1, r1, #1 //除2，因为内循环消耗2个指令
loop_1us:
	subs r1, #1
	bne loop_1us
	subs r0, #1
	bne VOSDelayUs
	bx lr

.text
.align 2
.global vos_sys_call
.thumb
.thumb_func
vos_sys_call:
    svc		5  //VOS_SVC_NUM_SYSCALL
    bx      lr

.text
.align 2
.global vos_privileged_save
.thumb
.thumb_func
vos_privileged_save:
	svc 	6 //VOS_SVC_PRIVILEGED_MODE
    BX      LR

.text
.align 2
.global vos_privileged_restore
.thumb
.thumb_func
vos_privileged_restore:
	MRS		R1, CONTROL //特权模式下获取control
	ORR		R2, R1, R0
	MSR     CONTROL, R2
    BX      LR

.text
.align 2
.global __switch_privileged_mode
.thumb
.thumb_func
__switch_privileged_mode:
	MRS     R0, CONTROL //只能读出control[0]位，control[1]位拥有是0，因为svn中断模式
	BIC		R1, R0, #1	//control[0]=0 //特权模式
	MSR     CONTROL, R1
	ISB
	BX		LR



