.syntax         unified
.thumb
.file   "cpu.S"

.global svc_exc_return

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
	ldr	r1, =svc_exc_return
	str	lr, [r1]
	bl	SVC_Handler_C
	ldr	r1, =svc_exc_return
	ldr	lr, [r1]
	bx	lr

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


.text
.align 2
.global __local_irq_save
.thumb
.thumb_func
__local_irq_save:
    mrs     r0, basepri
    mov     r1, #0xff
    msr     basepri, r1
    bx      lr

.text
.align 2
.global __local_irq_restore
.thumb
.thumb_func
__local_irq_restore:
    msr     basepri, r0
    bx      lr


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
