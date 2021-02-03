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

#define VOS_RUNTIME_BOUDARY_CHECK (1) //����ʱ�߽���

#define VOS_SLAB_ENABLE		(1) //��slab����������
#define VOS_SLAB_STEP_SIZE 	(8) //slab����������

#define VMEM_BELONG_TO_NONE (0xFF)  //ָ��������񣬻���ISR�����ģ���ϵͳδ����


#define AUTO_TEST_RANDOM  0 //AUTO_TEST_RANDOM == 1 ��pc��ʹ��vs2017�������
#define AUTO_TEST_BY_MAN  0

#define VMEM_TRACER_ENABLE	1 //����ڴ��Ƿ�Խ��

#define VMEM_TRACE_DESTORY 0 //����ڴ��Ƿ�д�ƻ�

#define VHEAP_ATTR_SYS		0 //ͨ�öѣ�ϵͳ�ѣ�, ϵͳ�Ѿ���������malloc��free����Ķ�
#define VHEAP_ATTR_PRIV		1 //ר�öѣ�˽�жѣ�����������ֻ�ȡר�ö�ָ��ʹ��

#define ASSERT_TEST(t,s) \
	do {\
		if(!t) {\
			kprintf("error[%d]: %s test failed!!!\r\n", __LINE__, s); while(1);\
		}\
	}while(0)


struct StVMemHeap;
struct StVMemCtrlBlock;

typedef struct StVMemHeapInfo {
	s32 free_page_bytes; //ʣ�¿���ҳ�����ֽ���
	s32 used_page_bytes; //�Ѿ������ҳ�����ֽ���
	s32 cur_max_size; //��ǰ���ɷ���ĳߴ磬��λ�ֽ�
	s32 max_page_class; //���������ߴ磬��λ�ֽ�
	s32 align_bytes; //���ֽڶ���
	s32 page_counts; //��ʱûʲô��
	u32 page_size; //ÿҳ��С����λ�ֽڣ�ͨ��1024,2048�ȵ�
}StVMemHeapInfo;


//#define kprintf printf

#define BOUNDARY_ERROR() \
	do { \
		kprintf("BOUNDARY CHECK ERROR: code(%d)\r\n", __LINE__);\
		goto ERROR_RET;\
	} while(0)

#define VMEM_STATUS_UNKNOWN 0
#define VMEM_STATUS_USED 	1
#define VMEM_STATUS_FREE 	2

#if AUTO_TEST_RANDOM
	#define MAX_PAGE_CLASS_MAX 7
#elif AUTO_TEST_BY_MAN
	#define MAX_PAGE_CLASS_MAX 3
#else
	#define MAX_PAGE_CLASS_MAX 13//11
#endif

#define VMEM_MALLOC_PAD		0xAC //ʣ��ռ�����ַ�

struct StVMemCtrlBlock;
struct StVSlabMgr;

enum {
	BLOCK_OWN_NONE = 0,
	BLOCK_OWN_SLAB,
	BLOCK_OWN_BUDDY,
};

typedef struct StVMemHeap {
	s8 *name; //������
	u8 *mem_end; //�ڴ�����Ľ�����ַ�������ж��ͷ��Ƿ�Խ��
	struct StVSlabMgr *slab_ptr; //ָ��slab������
	s32 irq_save;//�ж�״̬�洢
	s32 heap_attr; //�����������֣�һ����ר�öѣ���һ����ͨ�öѣ�ϵͳ�ѣ�
	s32 align_bytes; //���ֽڶ���
	s32 page_counts; //��ʱûʲô��
	u32 page_size; //ÿҳ��С����λ�ֽڣ�ͨ��1024,2048�ȵ�
	struct StVMemCtrlBlock *pMCB_base; //�ڴ���ƿ����ַ����page_baseҳ��ַ��ƽ�����飬ƫ����һ��
	u8 *page_base; //����ҳ����ַ����pMCB_base���ƿ���ƽ�����飬ƫ����һ��
	struct list_head page_class[MAX_PAGE_CLASS_MAX]; //page_class[0] ָ��2^0��page������page_class[1]ָ��2^1��page������
	struct list_head mcb_used; //������ʹ�õ��ڴ����ӵ���������У�������Ժ��Ų����⣬ͬʱ��ѹ�����Կ���ʹ�ã�
	struct list_head list_heap; //������
}StVMemHeap;

//ÿ��ҳ��Ӧ��һ���ڴ���ƿ飬����1��ҳ1k�ֽڣ����ƻ�������sizeof(StVMemCtrlBlock)/1k
//����StVMemCtrlBlock���������ҳ��ƽ�����飬ƫ��ֵһ�£����������Ϊ��malloc�����룬д����Ӱ������鼮
//ƽ������Ҳ��Ϊ�˼����ͷ��ڴ�ʱ���ϲ����ڵ��ڴ棬���ñ�������
typedef struct StVMemCtrlBlock {
	struct list_head list;
#if VMEM_TRACER_ENABLE
	u32 used_num; //��ǰʹ���˶����ֽڣ����������Ƿ�Խ��д�룬�÷�����ʣ�����ȥ���ָ�����ݣ�����Ƿ�Խ��д��
#endif
	u32 flag_who; //	BLOCK_OWN_NONE, BLOCK_OWN_SLAB, BLOCK_OWN_BUDDY
	u16 page_max; //�������˶���ҳ, 2^n
	s8 status; //VMEM_STATUS_FREE,VMEM_STATUS_USED,VMEM_STATUS_UNKNOWN
	//�ѷ����ڴ����ڵ�����id,ע�⣺������п飬��������Ϊ0xFF(��������), ���ǿ������������ڴ棨���⴦��
	//���ж������Ļ���ϵͳ����û����ǰ�����趨VMEM_BELONG_TO_NONE, ���벻������������ʹ��
	u8 task_id;

}StVMemCtrlBlock;

#define ALIGN_UP(mem, align) 	((u32)(mem) & ~((align)-1))
#define ALIGN_DOWN(mem, align) 	(((u32)(mem)+(align)-1) & ~((align)-1))


struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes,
												s32 heap_attr, s8 *name, s32 enable_slab);
void *VMemMalloc(struct StVMemHeap *pheap, u32 size, s32 is_slab);
s32 VMemFree (struct StVMemHeap *pheap, void *p, s32 is_slab);
void *VMemExpAlloc(struct StVMemHeap *pheap, void *p, u32 size);
void *VMemGetPageBaseAddr(struct StVMemHeap *pheap, void *any_addr);
s32 VMemGetHeapInfo(struct StVMemHeap *pheap, struct StVMemHeapInfo *pheadinfo);

#endif
