/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vmem.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200905�������ļ�, ʵ��buddy�ڴ涯̬�����㷨���ٶ������Ϻܿ죬���룬�ͷŶ����ñ�������
*********************************************************************************************************/

#include "vtype.h"
#include "vos.h"
#include "vmem.h"
#include "vlist.h"

//#define kprintf printf

#define ASSERT_TEST(t,s) do {if(t) {/*kprintf("info[%d]: %s test ok!!!\r\n", __LINE__, (s));*/}else{kprintf("error[%d]: %s test failed!!!\r\n", __LINE__, s); while(1);}}while(0)

#define BOUNDARY_ERROR() do {kprintf("BOUNDARY CHECK ERROR: code(%d)\r\n", __LINE__);goto ERROR_RET;} while(0)

#define VMEM_STATUS_UNKNOWN 0
#define VMEM_STATUS_USED 	1
#define VMEM_STATUS_FREE 	2

#define MAX_PAGE_CLASS_MAX 3

#define VMEM_MALLOC_PAD		0xAC //ʣ��ռ�����ַ�

#define VMEM_TRACER_ENABLE	1 //����ڴ��Ƿ�Խ��

struct StVMemCtrlBlock;

typedef struct StVMemHeap {
	u8 *mem_end; //�ڴ�����Ľ�����ַ�������ж��ͷ��Ƿ�Խ��
	s32 align_bytes; //���ֽڶ���
	s32 page_counts; //��ʱûʲô��
	u32 page_size; //ÿҳ��С����λ�ֽڣ�ͨ��1024,2048�ȵ�
	struct StVMemCtrlBlock *pMCB_base; //�ڴ���ƿ����ַ����page_baseҳ��ַ��ƽ�����飬ƫ����һ��
	u8 *page_base; //����ҳ����ַ����pMCB_base���ƿ���ƽ�����飬ƫ����һ��
	struct list_head page_class[MAX_PAGE_CLASS_MAX]; //page_class[0] ָ��2^0��page������page_class[1]ָ��2^1��page������
	struct list_head mcb_used; //������ʹ�õ��ڴ����ӵ���������У�������Ժ��Ų����⣬ͬʱ��ѹ�����Կ���ʹ�ã�
}StVMemHeap;

//ÿ��ҳ��Ӧ��һ���ڴ���ƿ飬����1��ҳ1k�ֽڣ����ƻ�������sizeof(StVMemCtrlBlock)/1k
//����StVMemCtrlBlock���������ҳ��ƽ�����飬ƫ��ֵһ�£����������Ϊ��malloc�����룬д����Ӱ������鼮
//ƽ������Ҳ��Ϊ�˼����ͷ��ڴ�ʱ���ϲ����ڵ��ڴ棬���ñ�������
typedef struct StVMemCtrlBlock {
	struct list_head list;
#if VMEM_TRACER_ENABLE
	u16 used_num; //��ǰʹ���˶����ֽڣ����������Ƿ�Խ��д�룬�÷�����ʣ�����ȥ���ָ�����ݣ�����Ƿ�Խ��д��
#endif
	u16 page_max; //�������˶���ҳ, 2^n
	u32 status; //VMEM_STATUS_FREE,VMEM_STATUS_USED,VMEM_STATUS_UNKNOWN
}StVMemCtrlBlock;

#define ALIGN_UP(mem, align) 	((u32)(mem) & ~((align)-1))
#define ALIGN_DOWN(mem, align) 	(((u32)(mem)+(align)-1) & ~((align)-1))

/********************************************************************************************************
* ������u32 CutPageClassSize(u32 size, s32 page_size, s32 *index, s32 *count);
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
static u32 CutPageClassSize(u32 size, s32 page_size, s32 *index, s32 *count)
{
	u32 mast_out = 0;
	u32 mast = MAX_PAGE_CLASS_MAX;
	s32 cnts = size / page_size;
	mast_out = (~0 >> mast) << mast;
	while (cnts && mast--) {
		mast_out >>= 1;
		if (cnts & mast_out) {
			cnts &= mast_out;
			if (index) *index = mast;
			if (count) *count = cnts >> mast;
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
	index = (mast > MAX_PAGE_CLASS_MAX) ? -1 : mast - 1;

	return index;
}

/********************************************************************************************************
* ������StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes);
* ����:  �Ѵ���
* ����:
* [1] mem: �û��ṩ�������ֶѵ��ڴ�
* [2] len: �û��ṩ�ڴ�Ĵ�С����λ���ֽ�
* [3] page_size: ҳ�ߴ磬1024,2048��
* [4] align_bytes: �ֽڶ��룬����8������8�ֽڶ���
* ���أ��ѹ������ָ��
* ע�⣺����ṩ���ڴ泬����page_class[n]���ռ䣬��page_class[n]���ֵ�����������Ӷ�����ڵ����ݿ�
*********************************************************************************************************/
struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes)
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
		len < page_size * 2) return 0;

	pheap = (StVMemHeap*)ALIGN_DOWN(mem, align_bytes);

	memset(pheap, 0, sizeof(StVMemHeap));


	pheap->mem_end = (u8*)ALIGN_UP(mem + len, align_bytes); ////��һ��mem_end�����ַ�����������ĵ�ַ

	size = pheap->mem_end - (u8*)pheap;

	head_size = sizeof(StVMemHeap) + (size / page_size) * sizeof(StVMemCtrlBlock);

	pheap->pMCB_base = (StVMemCtrlBlock*)(pheap + 1);

	pheap->page_base = (u8*)ALIGN_DOWN((u8*)pheap + head_size, align_bytes); //��һ��page_base�����ַ�����������ĵ�ַ
	pheap->page_counts = (pheap->mem_end - pheap->page_base) / page_size;

	pheap->page_size = page_size;
	pheap->align_bytes = align_bytes;

	pheap->page_base = (u8*)ALIGN_DOWN(&pheap->pMCB_base[pheap->page_counts], align_bytes); //�ڶ��β��������ĵ�ַ��������������pMCB_baseĩβ
	pheap->mem_end = (u8*)ALIGN_DOWN(&pheap->page_base[pheap->page_counts*pheap->page_size], align_bytes);//�ڶ��β��������ĵ�ַ����������page_baseβ��

	//��ʼ��pageͷ������Ԫ�ض���һ������˫������
	for (i = 0; i<MAX_PAGE_CLASS_MAX; i++)
	{
		INIT_LIST_HEAD(&pheap->page_class[i]); //�Լ�ָ���Լ�
	}
	//��ʼ���Ѿ������ȥ���ڴ����ӵ��������
	INIT_LIST_HEAD(&pheap->mcb_used);

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
		for (i = 0; i<cnts; i++) {
			pMCB = &pheap->pMCB_base[(offset_size + i * (1 << index)*page_size) / page_size];
			pMCB->page_max = 1 << index;
			pMCB->status = VMEM_STATUS_FREE;
			list_add_tail(&pMCB->list, &pheap->page_class[index]);
		}
		offset_size += temp;
	}

	return pheap;
}

/********************************************************************************************************
* ������void *VMemMalloc(StVMemHeap *pheap, u32 size);
* ����:  ָ����������size�ռ��ڴ�
* ����:
* [1] pheap: ָ���Ķ�
* [2] size: ����Ĵ�С����λ�ֽ�
* ���أ�������ڴ�ռ���ʼ��ַ
* ע�⣺buddy�㷨����������ڴ���Ӧ������Ϊ�գ����������룬ֱ������ɹ���ʧ�ܡ�
*********************************************************************************************************/
void *VMemMalloc(struct StVMemHeap *pheap, u32 size)
{
	void *pObj = 0;
	s32 i;
	s32 index;
	s32 index_top;//��������ڴ�ռ���Է���
	StVMemCtrlBlock *pMCB = 0;//���ѣ�����Ǹ�
	StVMemCtrlBlock *pMCBRight = 0; //���ѣ��ұ��Ǹ�
	if (size > pheap->page_size * (1 << (MAX_PAGE_CLASS_MAX - 1))) {//�����С ���� ����
		return 0;
	}
	index = GetPageClassIndex(size, pheap->page_size);
	if (index < 0) goto END_VMEMMALLOC;

	//��������ڴ�
	index_top = -1;
	for (i = index; i<MAX_PAGE_CLASS_MAX; i++)
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

			pMCBRight->page_max = pMCB->page_max = pMCB->page_max / 2;
#if VMEM_TRACER_ENABLE
			pMCBRight->used_num = pMCB->used_num = 0;
#endif
			pMCBRight->status = pMCB->status = VMEM_STATUS_FREE;

			index_top--;
			//���뵽page_class[n]�У� �������ұߵģ���߼�������
			list_add_tail(&pMCBRight->list, &pheap->page_class[index_top]);
		}
		//ȡ�����䵽���ڴ�
		pMCB->status = VMEM_STATUS_USED;
#if VMEM_TRACER_ENABLE
		pMCB->used_num = size;
#endif
		pMCB->page_max = 1 << index_top; //page_class[n]��Ӧ�����ҳ�� ���ͷ�ʱ��Ҫ�õ���
		INIT_LIST_HEAD(&pMCB->list); //�Լ�ָ���Լ���
		//���ӵ����������У��������ʹ��
		list_add_tail(&pMCB->list, &pheap->mcb_used);

		//���ҳ��ַ
		pObj = pheap->page_base + (pMCB - pheap->pMCB_base) * pheap->page_size;

#if VMEM_TRACER_ENABLE
		//���̶��ַ��� �������Խ��
		memset(pObj, VMEM_MALLOC_PAD, pMCB->page_max * pheap->page_size - pMCB->used_num);
#endif
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
void VMemFree(struct StVMemHeap *pheap, void *p)
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

	//�ͷ�ʱҲ�ô��ѷ���������ɾ��
	if (pMCB->status == VMEM_STATUS_USED) {
		list_del(&pMCB->list);
	}

	while (1) {
		if (page_max >= 1 << (MAX_PAGE_CLASS_MAX - 1)) { //������page_class[n]���ĳߴ磬����
			break;
		}
		if (offset % (page_max << 1) == 0) {//��ǰ���Ǻϲ�������
											//���Һϲ����ҿ��Ƿ���� (buddy)
			pMCBTemp = &pheap->pMCB_base[offset + page_max];
			if (pMCBTemp->status == VMEM_STATUS_FREE && //�ҿ��ǿ���
				pMCBTemp->page_max == pMCB->page_max) { //�ҿ�����ͬһ��������
														//�ͷ��ұ�
				list_del(&pMCBTemp->list);
				//����ұ�
#if VMEM_TRACER_ENABLE
				pMCBTemp->used_num = 0;
#endif
				pMCBTemp->status = 0;
				pMCBTemp->page_max = 0;
				pMCBTemp->list.prev = 0;
				pMCBTemp->list.next = 0;
				//�ϲ��ұ�
				pMCB->page_max <<= 1;
#if VMEM_TRACER_ENABLE
				pMCB->used_num = 0;
#endif
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
			pMCBTemp = &pheap->pMCB_base[offset - page_max];
			if (pMCBTemp->status == VMEM_STATUS_FREE && //����ǿ���
				pMCBTemp->page_max == pMCB->page_max) { //�ҿ�����ͬһ��������
														//�ͷ����
				list_del(&pMCBTemp->list);
				//����ұ�
#if VMEM_TRACER_ENABLE
				pMCB->used_num = 0;
#endif
				pMCB->status = 0;
				pMCB->page_max = 0;
				pMCB->list.prev = 0;
				pMCB->list.next = 0;
				//�ϲ��ұ�
				pMCBTemp->page_max <<= 1;
#if VMEM_TRACER_ENABLE
				pMCBTemp->used_num = 0;
#endif
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
	while (pMCB->page_max != 1 << index) index++;
	if (index < MAX_PAGE_CLASS_MAX) {
		//����ӵ���������
		pMCB->status = VMEM_STATUS_FREE;
#if VMEM_TRACER_ENABLE
		pMCB->used_num = 0;
#endif
		list_add_tail(&pMCB->list, &pheap->page_class[index]);//��ӵ���������ĩβ
	}
	return;
}
/********************************************************************************************************
* ������void *VMemRealloc(StVMemHeap *pheap, void *p, u32 size);
* ����:  ָ���������������ڴ�
* ����:
* [1] pheap: ָ���Ķ�
* [2] p: ԭָ�룬�������0����malloc(size)
* [3] size: �����ڴ��С���������0�����ͷ�ԭ��pָ����ڴ棬ͬʱ����0
* ���أ�����ָ�룬ָ�����·����С���ڴ档�������ʧ�ܣ��򷵻�NULL
* ע�⣺����������ԭ�������㹻�ռ䣬��ֱ�ӷ���ԭָ�룬���򷵻���ָ�롣
*********************************************************************************************************/
void *VMemRealloc(struct StVMemHeap *pheap, void *p, u32 size)
{
	void *ptr = (void*)0;
	struct StVMemCtrlBlock *pMCB = 0;
	if (p == 0) { //ָ��Ϊ�գ���ֱ��malloc
		ptr = VMemMalloc(pheap, size);
		goto END_VMEMREALLOC;
	}
	if (size == 0) { //ָ�벻Ϊ�գ�sizeΪ0�����ͷ�p, ͬʱ����0
		VMemFree(pheap, p);
		goto END_VMEMREALLOC;
	}
	//�ж��Ǳ��ѵ�malloc
	if ((u8*)p >= pheap->page_base &&
		(u8*)p < pheap->mem_end &&
		((u8*)p - pheap->page_base) % pheap->page_size == 0) {
		pMCB = &pheap->pMCB_base[((u8*)p - pheap->page_base) / pheap->page_size];
		//�ж�size�Ƿ񳬳��������ߴ�
		if (pMCB->status == VMEM_STATUS_USED && //�������Ѿ������ȥ״̬
			pMCB->page_max * pheap->page_size >= size) { //�����ʣ��ռ��㹻��
			ptr = p;
			goto END_VMEMREALLOC;
		}
	}
	//����ʣ��ռ���߲��ڱ������ڣ�ֱ��malloc��memcpy
	ptr = VMemMalloc(pheap, size);
	if (ptr) {
		memcpy(ptr, p, size);
	}

END_VMEMREALLOC:
	return ptr;
}

/********************************************************************************************************
* ������s32 VMemInfoDump(struct StVMemHeap *pheap);
* ����:  �����ڴ�
* ����:
* [1] pheap: �Ѷ���ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
s32 VMemInfoDump(struct StVMemHeap *pheap)
{
	s32 i;
	struct list_head *list;
	StVMemCtrlBlock *pMCB = 0;
	for (i = 0; i<MAX_PAGE_CLASS_MAX; i++) {
		if (list_empty(&pheap->page_class[i])) {
			kprintf("page_class[%02d]: head->NULL\r\n", i);
		}
		else {
			kprintf("page_class[%02d]: head", i);
			list_for_each(list, &pheap->page_class[i]) {
				pMCB = list_entry(list, struct StVMemCtrlBlock, list);
				kprintf("->(page:%d,max:%d,stat:%d)", pMCB - pheap->pMCB_base, pMCB->page_max, pMCB->status);
			}
			kprintf("\r\n");
		}
	}

	if (list_empty(&pheap->mcb_used)) {
		kprintf("mem_used: head->NULL\r\n");
	}
	else {
		kprintf("mem_used: head");
		list_for_each(list, &pheap->mcb_used) {
			pMCB = list_entry(list, struct StVMemCtrlBlock, list);
#if VMEM_TRACER_ENABLE
			kprintf("->(page:%d,max:%d,used:%d,stat:%d)", pMCB - pheap->pMCB_base, pMCB->page_max, pMCB->used_num, pMCB->status);
#else
			kprintf("->(page:%d,len:%d,stat:%d)", pMCB - pheap->pMCB_base, pMCB->page_max, pMCB->status);
#endif
		}
		kprintf("\r\n");
	}

	return 0;
}

/********************************************************************************************************
* ������void VMemTraceDestory(struct StVMemHeap *pheap);
* ����: ��鵱ǰ����malloc���ڴ��Ƿ��б�д�ƻ���ֻ���ʣ��ռ䡣
* ����:
* [1] pheap: �Ѷ���ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VMemTraceDestory(struct StVMemHeap *pheap)
{
	s32 i;
	struct list_head *list;
	StVMemCtrlBlock *pMCB = 0;
	u8 *pChar = 0;
	//����Ѿ�����ɵ��ڴ��Ƿ񱻴۸�

	list_for_each(list, &pheap->mcb_used) {
		pMCB = list_entry(list, struct StVMemCtrlBlock, list);
		pChar = pheap->page_base + (pMCB - pheap->pMCB_base) * pheap->page_size;
		for (i = 0; i <= pMCB->page_max * pheap->page_size - pMCB->used_num; i++) {
			if (pChar[i] != VMEM_MALLOC_PAD) {//error
				ASSERT_TEST(0,__FUNCTION__);
				break;
			}
		}
	}
	return ;
}

/********************************************************************************************************
* ������s32 VBoudaryCheck(struct StVMemHeap *pheap);
* ����: �ѹ�������ڲ�Ԫ�ؼ���Ƿ�Ϸ���
* ����:
* [1] pheap: �Ѷ���ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
s32 VBoudaryCheck(struct StVMemHeap *pheap)
{
	int ret = 0;
	s32 i;
	s32 offset = 0;
	s32 free_total = 0;
	s32 used_total = 0;
	struct list_head *list;
	struct list_head *list_temp;
	StVMemCtrlBlock *pMCB = 0;
	StVMemCtrlBlock *pMCBTemp = 0;

	//�ж�pheap�ڲ�����Ԫ���Ƿ�����
	if ((u8*)(pheap + 1) != (u8*)pheap->pMCB_base) BOUNDARY_ERROR();
	//��������ĩβ��ҳ�������Ŀ�ʼ
	if ((u8*)ALIGN_DOWN(&pheap->pMCB_base[pheap->page_counts], pheap->align_bytes) != pheap->page_base) BOUNDARY_ERROR();
	//ҳ��������ĩβ����mem_end
	if ((u8*)ALIGN_DOWN(&pheap->page_base[pheap->page_counts * pheap->page_size], pheap->align_bytes) != pheap->mem_end) BOUNDARY_ERROR();
	//���ҳ������1024��������
	if (pheap->page_size % 1024 != 0) BOUNDARY_ERROR();
	//���ҳ����������ͷ(page_class[n])���ݼ����������Ƿ�����
	free_total = 0;
	for (i = 0; i<MAX_PAGE_CLASS_MAX; i++) {
		if (!list_empty(&pheap->page_class[i])) {//�ǿ�
			list_for_each(list, &pheap->page_class[i]) {
				pMCB = list_entry(list, struct StVMemCtrlBlock, list);
				//��������ܺ�
				free_total += pMCB->page_max*pheap->page_size;
				//У��MCBԪ���Ƿ�Ϸ�
				if (pMCB->status != VMEM_STATUS_FREE) BOUNDARY_ERROR();
				if (pMCB->page_max != 1<<i) BOUNDARY_ERROR();
				//���MCB�ı߽磨ƫ�������������
				if ((pMCB-pheap->pMCB_base)%(1<<i) != 0) BOUNDARY_ERROR();
			}
			//����κ������߽粻������, ��Ϊ����û����ַ��������Ҫע�⴦��һ��
			if (i != MAX_PAGE_CLASS_MAX - 1) {//ע�⣺���һ������������������������ַ����Ϊû�취����������ֻ��������һ��
				list_for_each(list, &pheap->page_class[i]) {
					pMCB = list_entry(list, struct StVMemCtrlBlock, list);
					offset = pMCB - pheap->pMCB_base;
					list_temp = list->next;
					while (&pheap->page_class[i] != list_temp) {
						pMCBTemp = list_entry(list_temp, struct StVMemCtrlBlock, list);
						if (offset % (1 << i) == 0) { //������ߵ�ַ�������������ұߵ�ַ�������ұ������϶���Ҫ�ϲ�����һ�㣩
							if (&pMCB[0 + (1 << i)] == pMCBTemp) BOUNDARY_ERROR();
						}
						else { //���������������ַ
							if (&pMCB[0 - (1 << i)] == pMCBTemp) BOUNDARY_ERROR();
						}
						list_temp = list_temp->next;
					}
				}
			}
		}
	}
	//����ѷ���������ܺ�
	used_total = 0;
	list_for_each(list, &pheap->mcb_used) {
		pMCB = list_entry(list, struct StVMemCtrlBlock, list);
		if (pMCB->status != VMEM_STATUS_USED) BOUNDARY_ERROR();
		used_total += pMCB->page_max*pheap->page_size;
	}
	//���п��������ܺ�+�ѷ��������ܺ͵��������ڴ��ܺ�
	if (used_total + free_total != pheap->page_counts * pheap->page_size) BOUNDARY_ERROR();

	return 1;

ERROR_RET:
	VMemInfoDump(pheap);
	while (1);
	return 0;
}

u8 arr_heap[1024 * 10 + 10];
extern s32 VBoudaryCheck(struct StVMemHeap *pheap);
extern s32 VMemInfoDump(struct StVMemHeap *pheap);
/********************************************************************************************************
* ������void vmem_test();
* ����:  ����
* ����:  ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void vmem_test()
{

	kprintf("u32=%d\r\n", sizeof(u32));

	StVMemHeap *pheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 1024, 8);
	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	u8 *pnew = (u8*)VMemMalloc(pheap, 1024 * 1 + 1);
	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	u8 *pnew1 = (u8*)VMemMalloc(pheap, 1024 * 1 + 1);

	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	VMemFree(pheap, pnew);

	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);

	VMemFree(pheap, pnew1);

	VMemInfoDump(pheap);
	VBoudaryCheck(pheap);
}




