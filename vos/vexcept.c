/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vexcept.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/
#include "vos.h"

#define printk   kprintf //异常打印必须阻塞打印，直接操作寄存器


void VOSDumpExceptStack (u32 *addr, s32 counts)
{
	s32 i = counts - 1;
	printk("stack data dump: \r\n");
	printk("++++++++++++++++++++++\r\n");
	for (i = 0; i < counts; i++) {
		printk("0x%08x ", addr[i]);
		if ((i+1)%4==0) printk("\r\n");
	}
	if (i%4) printk("\r\n");
	printk("++++++++++++++++++++++\r\n");
}

/********************************************************************************************************
* 函数：void VOSExceptHandler(u32 *sp, s32 is_psp);
* 描述:  异常产生时，把进入异常前的栈打印出来，结合lst文件推函数调用栈
* 参数:
* [1] sp: 可能是msp或psp
* [2] is_psp: 指示psp栈还是msp, 1:psp; 0:msp;
* 返回：无
* 注意：无
*********************************************************************************************************/
extern unsigned int _estack;
extern unsigned int __Main_Stack_Size;
extern struct StVosTask *pRunningTask;
void VOSExceptHandler(u32 *sp, s32 is_psp)
{
	u32 stack_size;
	//打印栈信息
	printk("Exception Infomation:\r\n");
	printk("++++++++++++++++++++++\r\n");
	//先打印自动保存的寄存器信息
	//R0, R1, R2, R3, R12, LR, PC, xPSR
	printk("R0:  0x%08x\tR1:  0x%08x\r\n", sp[0], sp[1]);
	printk("R2:  0x%08x\tR3:  0x%08x\r\n", sp[2], sp[3]);
	printk("R12: 0x%08x\tLR:  0x%08x\r\n", sp[4], sp[5]);
	printk("PC:  0x%08x\txPSR:0x%08x\r\n", sp[6], sp[7]);
	//打印调用栈信息，结合lst分析函数调用栈
	if (is_psp) {//打印出当前任务异常栈内容
		if (pRunningTask) {
			stack_size = (u32)pRunningTask->pstack_top - (u32)sp;
			if (stack_size > pRunningTask->stack_size) { // 栈溢出
				printk("PSP栈溢出\r\n");
			}
			else {
				VOSDumpExceptStack (sp, stack_size/sizeof(u32));
			}
		}
	}
	else {//msp
		stack_size = (u32)&_estack - (u32)sp;
		if (stack_size > (u32)&__Main_Stack_Size) { // 栈溢出
			printk("MSP栈溢出\r\n");
		}
		else {
			VOSDumpExceptStack (sp, stack_size/sizeof(u32));
		}
	}

	//打印各种状态寄存器信息
}


