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
	mrs	r0, psp
	tst	lr, #0x10
	it eq
	vstmdbeq r0!, {s16-s31}
	mov r2, lr
	mrs r3, control
	stmdb r0!, {r2-r11}
	ldr r1, =pRunningTask
	ldr r2, [r1]
	str r0, [r2]
	ldr r4, =pReadyTask
	ldr r4, [r4]
	str r4, [r1]
	ldr r0, [r4]
	ldmia r0!, {r2-r11}
	mov lr, r2
	msr control, r3
	isb
	tst lr, #0x10
	it eq
	vldmiaeq r0!, {s16-s31}
	msr psp, r0
	bx lr

.text
.align 2
.global RunFirstTask
.thumb
.thumb_func
RunFirstTask:
	//push	{lr}
    LDR     R0, =VOSRunning //设置系统运行标志
    MOVS    R1, #1
    STRB    R1, [R0]

	LDR 	R0, =pRunningTask
	LDR		R1, =SVC_EXC_RETURN
	LDR		R2, [R0]
	LDR		R2, [R2] //ptask->pstack
	LDR		R3, [R2] //*ptask->pstack
	STR		R3,	[R1] //写SVC中断的LR值
	MOV		R0, #10
    LSL		R0, R0, #2 // 10 * 4
	ADD 	R1, R2, R0
    MSR		PSP, R1    //设置PSP的值
	LDR     R0, =0xE000ED22 //设置PendSV_Handler为最低优先级
	LDR     R1, =0xFF
	STRB    R1, [R0]

	MRS     R0, CONTROL //设置线程模式)，用户级
	ORR     R0, R0, #3
	MSR     CONTROL, R0
	ISB
	//BL		VOSSysTickSet //设置tick间隔
	//ISB
	//pop 	{pc}

.text
.align 2
.global _set_CONTROL
.thumb
.thumb_func
_set_CONTROL:
	msr control, r0
	bx lr

.text
.align 2
.global _ISB
.thumb
.thumb_func
_ISB:
	ISB
	bx lr

.text
.align 2
.global _set_PSP
.thumb
.thumb_func
_set_PSP:
	msr psp, r0
	bx lr


//.text
//.align 2
//.global __local_irq_save
//.thumb
//.thumb_func
//__local_irq_save:
//    mrs     r0, basepri
//    mov     r1, #0xff
//    msr     basepri, r1
//    bx      lr

.text
.align 2
.global __local_irq_save
.thumb
.thumb_func
__local_irq_save:
    MRS     R0, PRIMASK
    CPSID   I
    BX      LR

//.text
//.align 2
//.global __local_irq_restore
//.thumb
//.thumb_func
//__local_irq_restore:
//    msr     basepri, r0
//    bx      lr

.text
.align 2
.global __local_irq_restore
.thumb
.thumb_func
__local_irq_restore:
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


