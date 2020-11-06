/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vexcept.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/
#include "vos.h"

#define printk   kprintf //�쳣��ӡ����������ӡ��ֱ�Ӳ����Ĵ���


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
* ������void VOSExceptHandler(u32 *sp, s32 is_psp);
* ����:  �쳣����ʱ���ѽ����쳣ǰ��ջ��ӡ���������lst�ļ��ƺ�������ջ
* ����:
* [1] sp: ������msp��psp
* [2] is_psp: ָʾpspջ����msp, 1:psp; 0:msp;
* ���أ���
* ע�⣺��
*********************************************************************************************************/
extern unsigned int _estack;
extern unsigned int __Main_Stack_Size;
extern struct StVosTask *pRunningTask;
void VOSExceptHandler(u32 *sp, s32 is_psp)
{
	u32 stack_size;
	//��ӡջ��Ϣ
	printk("Exception Infomation:\r\n");
	printk("++++++++++++++++++++++\r\n");
	//�ȴ�ӡ�Զ�����ļĴ�����Ϣ
	//R0, R1, R2, R3, R12, LR, PC, xPSR
	printk("R0:  0x%08x\tR1:  0x%08x\r\n", sp[0], sp[1]);
	printk("R2:  0x%08x\tR3:  0x%08x\r\n", sp[2], sp[3]);
	printk("R12: 0x%08x\tLR:  0x%08x\r\n", sp[4], sp[5]);
	printk("PC:  0x%08x\txPSR:0x%08x\r\n", sp[6], sp[7]);
	//��ӡ����ջ��Ϣ�����lst������������ջ
	if (is_psp) {//��ӡ����ǰ�����쳣ջ����
		if (pRunningTask) {
			stack_size = (u32)pRunningTask->pstack_top - (u32)sp;
			if (stack_size > pRunningTask->stack_size) { // ջ���
				printk("PSPջ���\r\n");
			}
			else {
				VOSDumpExceptStack (sp, stack_size/sizeof(u32));
			}
		}
	}
	else {//msp
		stack_size = (u32)&_estack - (u32)sp;
		if (stack_size > (u32)&__Main_Stack_Size) { // ջ���
			printk("MSPջ���\r\n");
		}
		else {
			VOSDumpExceptStack (sp, stack_size/sizeof(u32));
		}
	}

	//��ӡ����״̬�Ĵ�����Ϣ
}


