/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vmem.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ:��
*********************************************************************************************************/

#ifndef __VMEM_H__
#define __VMEM_H__

#include "vtype.h"

#define VMEM_BELONG_TO_NONE (0xFF)  //ָ��������񣬻���ISR�����ģ���ϵͳδ����

#define AUTO_TEST_RANDOM  0 //AUTO_TEST_RANDOM == 1 ��pc��ʹ��vs2017�������

#define VMEM_TRACER_ENABLE	1 //����ڴ��Ƿ�Խ��

#define VMEM_TRACE_DESTORY 0 //����ڴ��Ƿ�д�ƻ�

#define ASSERT_TEST(t,s) \
	do {\
		if(!t) {\
			kprintf("error[%d]: %s test failed!!!\r\n", __LINE__, s); while(1);\
		}\
	}while(0)


struct StVMemHeap;
struct StVMemCtrlBlock;

typedef struct StVMemHeapInfo {
	s32 free_page_num; //ʣ�¿���ҳ�Ŀ���
	s32 used_page_num; //�Ѿ������ҳ�Ŀ���
	s32 cur_max_size; //��ǰ���ɷ���ĳߴ磬��λ�ֽ�
	s32 max_page_class; //���������ߴ磬��λ�ֽ�
	s32 align_bytes; //���ֽڶ���
	s32 page_counts; //��ʱûʲô��
	u32 page_size; //ÿҳ��С����λ�ֽڣ�ͨ��1024,2048�ȵ�
}StVMemHeapInfo;

struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes, s8 *name);
void *VMemMalloc(struct StVMemHeap *pheap, u32 size);
void VMemFree (struct StVMemHeap *pheap, void *p);
void *VMemRealloc(struct StVMemHeap *pheap, void *p, u32 size);

s32 VMemGetHeapInfo(struct StVMemHeap *pheap, struct StVMemHeapInfo *pheadinfo);

#endif
