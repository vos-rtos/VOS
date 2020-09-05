/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vmem.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200905�������ļ�, ʵ��buddy�ڴ涯̬�����㷨���ٶ������Ϻܿ죬���룬�ͷŶ����ò�������
*********************************************************************************************************/

#include "vtype.h"
#include "vos.h"
#include "vmem.h"
#include "vlist.h"

#define VMEM_STATUS_UNKNOWN 0
#define VMEM_STATUS_USED 	1
#define VMEM_STATUS_FREE 	2

#define MAX_PAGE_CLASS_MAX 3


extern struct StVMemCtrlBlock;

typedef struct StVMemHeap{
	u8 *mem_end; //�ڴ�����Ľ�����ַ�������ж��ͷ��Ƿ�Խ��
	s32 page_counts; //��ʱûʲô��
	s32 page_size; //ÿҳ��С����λ�ֽڣ�ͨ��1024,2048�ȵ�
	struct StVMemCtrlBlock *pMCB_base; //�ڴ���ƿ����ַ����page_baseҳ��ַ��ƽ�����飬ƫ����һ��
	u8 *page_base; //����ҳ����ַ����pMCB_base���ƿ���ƽ�����飬ƫ����һ��
	struct list_head page_class[MAX_PAGE_CLASS_MAX]; //page_class[0] ָ��2^0��page������page_class[1]ָ��2^1��page������
}StVMemHeap;

//ÿ��ҳ��Ӧ��һ���ڴ���ƿ飬����1��ҳ1k�ֽڣ����ƻ�������sizeof(StVMemCtrlBlock)/1k
//����StVMemCtrlBlock���������ҳ��ƽ�����飬ƫ��ֵһ�£����������Ϊ��malloc�����룬д����Ӱ������鼮
//ƽ������Ҳ��Ϊ�˼����ͷ��ڴ�ʱ���ϲ����ڵ��ڴ棬���ñ�������
typedef struct StVMemCtrlBlock{
	struct list_head list;
	u16 used_num; //��ǰʹ���˶����ֽ�
	u16 page_max; //�������˶���ҳ, 2^n
	u32 status; //VMEM_STATUS_FREE,VMEM_STATUS_USED,VMEM_STATUS_UNKNOWN
}StVMemCtrlBlock;

#define ALIGN_UP(mem, align) 	((u32)(mem) & ~((align)-1))
#define ALIGN_DOWN(mem, align) 	(((u32)(mem)+(align)-1) & ~((align)-1))

/********************************************************************************************************
* ������u32 CutPageClassSize(s32 size, s32 page_size, s32 *index, s32 *count);
* ����:  �ڴ��ʼ��ʱ����Ҫ�����������ĳߴ磬Ȼ�����������һ��
* ����:
* [1] size: �û���Ҫ���ִ洢�ռ��С
* [2] page_size: ÿҳ�ռ��С��ͨ��Ϊ1024��2048���ȵ�
* [3] index: ��������������index����page_class[index]��������ʵ����2^n�е�nֵ
* [4] count: �����ж��ٸ���������2^n����ֵ����������size����2^n��ֹһ���������ﷵ�ر�����
*                         ����ֵҲ�Ǳ��������ֵ
* ���أ����洢���ֽ��������磺�������Ϊ1025�� ����1024��
* ע�⣺�������size�ĳߴ粻��������ŵ�����page_class[MAX_PAGE_CLASS_MAX-1]���ʣ�ಿ�ִ�ŵ�
*      page_class[x < MAX_PAGE_CLASS_MAX]�ֱ�����ٵ�page_size��1������
*********************************************************************************************************/
static u32 CutPageClassSize(s32 size, s32 page_size, s32 *index, s32 *count)
{
	u32 mast_out = 0;
	u32 mast = MAX_PAGE_CLASS_MAX;
	s32 cnts = size / page_size;
	mast_out = (~0 >> mast) << mast;
	while (cnts && mast--) {
		mast_out >>= 1;
		if (cnts & mast_out)  {
			cnts &= mast_out;
			if (index) *index = mast;
			if (count) *count = cnts>>mast;
			break;
		}
	}
	return cnts * page_size;
}

/********************************************************************************************************
* ������s32 GetPageClassIndex(s32 size, s32 page_size);
* ����:  ��������size����Ҫ����2^n�������ţ���������100, ����0��ÿҳ1024�ֽڣ�2^0����1������Ҫ����Ŀռ�
* ����:
* [1] size: �û���Ҫ�����ڴ�Ĵ�С
* [2] page_size: ÿҳ�ռ��С��ͨ��Ϊ1024��2048���ȵ�
* ���أ�����page_class[n]��������
* ע�⣺��
*********************************************************************************************************/
s32 GetPageClassIndex(s32 size, s32 page_size)
{
	s32 index = 0;
	u32 mast_out = 0;
	u32 mast = 0;
	s32 cnts = (size + page_size - 1) / page_size;
	mast_out = (~0 >> mast) << mast;

	while (cnts & mast_out) {
		mast_out <<= 1;
		mast++;
		if (mast > MAX_PAGE_CLASS_MAX) {
			index = -1;
			break;
		}
	}
	index = (mast > MAX_PAGE_CLASS_MAX) ? -1 : mast-1;

	return index;
}

/********************************************************************************************************
* ������void VMemDebug (StVMemHeap *pheap);
* ����:  �����ڴ�
* ����:
* [1] pheap: �Ѷ���ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VMemDebug (StVMemHeap *pheap)
{
	s32 i;
	struct list_head *list;
	StVMemCtrlBlock *pMCB = 0;
	for (i=0; i<MAX_PAGE_CLASS_MAX; i++) {
		if (list_empty(&pheap->page_class[i])) {
			kprintf("page_class[%02d]: head->NULL\r\n", i);
		}
		else {
			kprintf("page_class[%02d]: head", i);
			list_for_each(list, &pheap->page_class[i]) {
				pMCB = list_entry(list, struct StVMemCtrlBlock, list);
				kprintf("->(page:%d,len:%d,stat:%d)", pMCB - pheap->pMCB_base, pMCB->page_max, pMCB->status);
			}
			kprintf("\r\n");
		}
	}
}

/********************************************************************************************************
* ������StVMemHeap *VMemCreate(u8 *mem, s32 len, s32 page_size, s32 align_bytes);
* ����:  �Ѵ���
* ����:
* [1] mem: �û��ṩ�������ֶѵ��ڴ�
* [2] len: �û��ṩ�ڴ�Ĵ�С����λ���ֽ�
* [3] page_size: ҳ�ߴ磬1024,2048��
* [4] align_bytes: �ֽڶ��룬����8������8�ֽڶ���
* ���أ��ѹ������ָ��
* ע�⣺����ṩ���ڴ泬����page_class[n]���ռ䣬��page_class[n]���ֵ�����������Ӷ�����ڵ����ݿ�
*********************************************************************************************************/
StVMemHeap *VMemCreate(u8 *mem, s32 len, s32 page_size, s32 align_bytes)
{
	s32 i;
	s32 index;
	s32 cnts;
	s32 temp;
	s32 reserve;
	s32 offset_size;

	s32 size = 0;
	s32 head_size;
	StVMemCtrlBlock *pMCB = 0;
	StVMemHeap *pheap = 0;

	if (mem == 0 || len < 0 || len < sizeof(StVMemHeap) ||
			len < page_size * 2 ) return 0;

	pheap = (StVMemHeap*)ALIGN_DOWN(mem, align_bytes);

	memset(pheap, 0, sizeof(StVMemHeap));


	pheap->mem_end = (u8*)ALIGN_UP(mem+len, align_bytes);

	size = pheap->mem_end - (u8*)pheap;

	head_size = sizeof(StVMemHeap) + (size/page_size) * sizeof(StVMemCtrlBlock);

	pheap->pMCB_base = (StVMemCtrlBlock*)(pheap+1);

	pheap->page_base = (u8*)ALIGN_DOWN((u8*)pheap+head_size, align_bytes);
	pheap->page_counts = (pheap->mem_end - pheap->page_base) / page_size;

	pheap->page_size = page_size;

	//��ʼ��pageͷ������Ԫ�ض���һ������˫������
	for (i=0; i<MAX_PAGE_CLASS_MAX; i++)
	{
		INIT_LIST_HEAD(&pheap->page_class[i]); //�Լ�ָ���Լ�
	}

	//��ʼ������
	memset(pheap->pMCB_base, 0, pheap->page_base - (u8*)pheap->pMCB_base);
	temp = 0;
	offset_size = 0;
	reserve = pheap->page_counts * page_size;
	while (1) {
		index = -1;
		temp = CutPageClassSize(reserve - offset_size, page_size, &index, &cnts);
		if (temp == 0) {
			break;
		}
		//��cnts > 1, ֤��page[MAX_PAGE_CLASS_MAX-1]����һ�����������飬��������MAX_PAGE_CLASS_MAX
		//���򣬽�����ڴ������ķŵ�page[MAX_PAGE_CLASS_MAX-1]������
		for (i=0; i<cnts; i++) {
			pMCB = &pheap->pMCB_base[(offset_size+i*(1<<index)*page_size)/page_size];
			pMCB->page_max = 1 << index;
			pMCB->status = VMEM_STATUS_FREE;
			list_add_tail(&pMCB->list, &pheap->page_class[index]);
		}
		offset_size += temp;
	}

	return pheap;
}

/********************************************************************************************************
* ������void *VMemMalloc(StVMemHeap *pheap, s32 size);
* ����:  ָ����������size�ռ��ڴ�
* ����:
* [1] pheap: ָ���Ķ�
* [2] size: ����Ĵ�С����λ�ֽ�
* ���أ�������ڴ�ռ���ʼ��ַ
* ע�⣺buddy�㷨����������ڴ���Ӧ������Ϊ�գ����������룬ֱ������ɹ���ʧ�ܡ�
*********************************************************************************************************/
void *VMemMalloc(StVMemHeap *pheap, s32 size)
{
	void *pObj = 0;
	s32 i;
	s32 index;
	s32 offset;
	s32 index_top;//��������ڴ�ռ���Է���
	StVMemCtrlBlock *pMCB = 0;//���ѣ�����Ǹ�
	StVMemCtrlBlock *pMCBRight = 0; //���ѣ��ұ��Ǹ�
	if (size > pheap->page_size * (1<<(MAX_PAGE_CLASS_MAX-1))) {//�����С ���� ����
		return 0;
	}
	index = GetPageClassIndex(size, pheap->page_size);
	if (index < 0) goto END_VMEMMALLOC;

	//��������ڴ�
	index_top = -1;
	for (i=index; i<MAX_PAGE_CLASS_MAX; i++)
	{
		if (!list_empty(&pheap->page_class[i])) {
			//pheap->page_class[index]
			index_top = i;
			break;
		}
	}
	if (index_top >= 0) { //������Ҫ������
		//��ȡ���ϲ�������ݿ�
		pMCB = (StVMemCtrlBlock *)list_entry(pheap->page_class[index_top].next, struct StVMemCtrlBlock, list);
		list_del(&pMCB->list);//ȡ����һ���ڴ��

		while (index_top != index) {
			//һ��Ϊ��
			pMCBRight = pMCB + (1 << index_top) / 2;

			pMCBRight->page_max = pMCB->page_max = pMCB->page_max/2;

			pMCBRight->used_num = pMCB->used_num = 0;

			pMCBRight->status = pMCB->status = VMEM_STATUS_FREE;

			index_top--;
			//���뵽page_class[n]�У� �������ұߵģ���߼�������
			list_add_tail(&pMCBRight->list, &pheap->page_class[index_top]);
		}
		//ȡ�����䵽���ڴ�
		pMCB->status = VMEM_STATUS_USED;
		pMCB->used_num = size;
		pMCB->page_max = 1<<index_top; //page_class[n]��Ӧ�����ҳ�� ���ͷ�ʱ��Ҫ�õ���
		INIT_LIST_HEAD(&pMCB->list); //�Լ�ָ���Լ���

		//���ҳ��ַ
		pObj = pheap->page_base + (pMCB - pheap->pMCB_base) * pheap->page_size;
	}

END_VMEMMALLOC:

	return pObj;
}

/********************************************************************************************************
* ������VMemFree (StVMemHeap *pheap, void *p);
* ����:  �ͷ�ָ�����������ڴ�
* ����:
* [1] pheap: ָ���Ķ�
* [2] p: �ͷŵĵ�ַ�������ַ����������ʱ����ʼ��ַ
* ���أ���
* ע�⣺buddy�㷨������ڴ��ͷź��ھ�Ҳ���У���ϲ�����һ����page_class[n]�������У�ֱ�������
*********************************************************************************************************/
void VMemFree (StVMemHeap *pheap, void *p)
{
	s32 size = 0;
	s32 offset = 0;
	s32 page_max = 0;
	s32 index = 0;
	struct StVMemCtrlBlock *pMCB = 0;
	struct StVMemCtrlBlock *pMCBTemp = 0;
	if ((u8*)p < pheap->page_base || (u8*)p >= pheap->mem_end) return; //��ַ��Χ���
	//�ж�p��ַ������ҳ����룬���߲��ͷ�
	size = (u8*)p - pheap->page_base;
	if (size % pheap->page_size) return; //��ַ���������page_size����

	offset = size / pheap->page_size;
	pMCB = &pheap->pMCB_base[offset];
	page_max = pMCB->page_max;

	while (1) {
		if (page_max >= 1<<(MAX_PAGE_CLASS_MAX-1)) { //������page_class[n]���ĳߴ磬����
			break;
		}
		if (offset % (page_max << 1) == 0) {//��ǰ���Ǻϲ�������
			//���Һϲ����ҿ��Ƿ���� (buddy)
			pMCBTemp = &pheap->pMCB_base[offset+page_max];
			if (pMCBTemp->status == VMEM_STATUS_FREE && //�ҿ��ǿ���
					pMCBTemp->page_max == pMCB->page_max )  { //�ҿ�����ͬһ��������
				//�ͷ��ұ�
				list_del(&pMCBTemp->list);
				//����ұ�
				pMCBTemp->used_num = 0;
				pMCBTemp->status = 0;
				pMCBTemp->page_max = 0;
				pMCBTemp->list.prev = 0;
				pMCBTemp->list.next = 0;
				//�ϲ��ұ�
				pMCB->page_max <<= 1;
				pMCB->used_num = 0;
				pMCB->status = VMEM_STATUS_FREE;
				//���ϲ�ϲ�
				//pMCB = pMCB;
				page_max = pMCB->page_max;
				offset = pMCB - pheap->pMCB_base;
			}
			else {
				break;
			}
		}
		else {//��ǰ���Ǻϲ�����ҿ�
			//���Һϲ�������Ƿ���� (buddy)
			pMCBTemp = &pheap->pMCB_base[offset-page_max];
			if (pMCBTemp->status == VMEM_STATUS_FREE && //����ǿ���
					pMCBTemp->page_max == pMCB->page_max )  { //�ҿ�����ͬһ��������
				//�ͷ����
				list_del(&pMCBTemp->list);
				//����ұ�
				pMCB->used_num = 0;
				pMCB->status = 0;
				pMCB->page_max = 0;
				pMCB->list.prev = 0;
				pMCB->list.next = 0;
				//�ϲ��ұ�
				pMCBTemp->page_max <<= 1;
				pMCBTemp->used_num = 0;
				pMCBTemp->status = VMEM_STATUS_FREE;
				//���ϲ�ϲ�
				pMCB = pMCBTemp;
				page_max = pMCB->page_max;
				offset = pMCB - pheap->pMCB_base;
			}
			else {
				break;
			}
		}
	}
	//��MCB���뵽ָ��page_class[n]����β��
	index = 0;
	while (pMCB->page_max != 1<<index) index++;
	if (index < MAX_PAGE_CLASS_MAX) {
		pMCB->status = VMEM_STATUS_FREE;
		pMCB->used_num = 0;
		list_add_tail(&pMCB->list, &pheap->page_class[index]);//��ӵ���������ĩβ
	}
	return;
}

u8 arr_heap[1024*10+10];
/********************************************************************************************************
* ������void vmem_test();
* ����:  ����
* ����:  ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void vmem_test()
{
	StVMemHeap *pheap = VMemCreate(&arr_heap[0], sizeof(arr_heap), 1024, 8);
	VMemDebug (pheap);

	u8 *pnew = (u8*)VMemMalloc(pheap, 1024*1+1);
	VMemDebug (pheap);

	u8 *pnew1 = (u8*)VMemMalloc(pheap, 1024*1+1);

	VMemDebug (pheap);

	VMemFree (pheap, pnew);

	VMemDebug (pheap);

	VMemFree (pheap, pnew1);

	VMemDebug (pheap);
}


