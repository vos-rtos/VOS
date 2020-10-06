/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vmem.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��	    ��: ʵ�ֵ����ѵ��ڴ������ͷŵȲ���
* ��    ʷ��
* --20200905�������ļ�, ʵ��buddy�ڴ涯̬�����㷨���ٶ������Ϻܿ죬���룬�ͷŶ����ñ�������
*********************************************************************************************************/

#include "vtype.h"
#include "vos.h"
#include "vlist.h"
#include "vmem.h"
#include "vheap.h"

#define VMEM_LOCK() 	pheap->irq_save = __vos_irq_save()
#define VMEM_UNLOCK()   __vos_irq_restore(pheap->irq_save)

/********************************************************************************************************
* ������u8 GetCurTaskId();
* ����: ��ȡ��ǰ���е�����id, �����ISR�����Ļ���ϵͳ����û������ֱ�ӷ���VMEM_BELONG_TO_NONE
* ����: ��
* ���أ����洢���ֽ��������磺�������Ϊ1025�� ����1024��
* ע�⣺WIN32����ֱ������
*********************************************************************************************************/
extern volatile u32 VOSIntNesting;
extern volatile u32 VOSRunning;
extern struct StVosTask *pRunningTask;
u8 GetCurTaskId()
{
	u8 ret = VMEM_BELONG_TO_NONE;
	if (VOSIntNesting == 0 && VOSRunning && pRunningTask) {
		ret = (u8)pRunningTask->id;
	}
	return ret;
}

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
	s32 cnts = (size + page_size - 1) / page_size;
	while (1 << index < cnts) {
		index++;
	}
	if (cnts == 0 || index >= MAX_PAGE_CLASS_MAX) index = -1;

	return index;
}

/********************************************************************************************************
* ������struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes,
*														s32 heap_attr, s8 *name, s32 enable_slab);
* ����:  �Ѵ���
* ����:
* [1] mem: �û��ṩ�������ֶѵ��ڴ�
* [2] len: �û��ṩ�ڴ�Ĵ�С����λ���ֽ�
* [3] page_size: ҳ�ߴ磬1024,2048��
* [4] align_bytes: �ֽڶ��룬����8������8�ֽڶ���
* [5] heap_attr: ��ר�öѺ�ͨ�öѣ�����malloc����ͨ�ö�����
* [6] name: ������
* [7] enable_slab: �Ƿ��slab������
* ���أ��ѹ������ָ��
* ע�⣺����ṩ���ڴ泬����page_class[n]���ռ䣬��page_class[n]���ֵ�����������Ӷ�����ڵ����ݿ�
*********************************************************************************************************/
struct StVMemHeap *VMemBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes,
												s32 heap_attr, s8 *name, s32 enable_slab)
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

	s32 slag_header_size = 0;

#if VOS_SLAB_ENABLE
	//����SLAB�������������ռ�
	if (enable_slab) {
		slag_header_size = SlabMgrHeaderGetSize(page_size, align_bytes, VOS_SLAB_STEP_SIZE);
	}
#endif

	if (mem == 0 || len < 0 || len < sizeof(StVMemHeap)+slag_header_size ||
		len < page_size * 2) return 0;

	pheap = (StVMemHeap*)ALIGN_DOWN(mem, align_bytes);

	memset(pheap, 0, sizeof(StVMemHeap));


	pheap->mem_end = (u8*)ALIGN_UP(mem + len, align_bytes); ////��һ��mem_end�����ַ�����������ĵ�ַ

	size = pheap->mem_end - (u8*)pheap;

	head_size = sizeof(StVMemHeap) + slag_header_size + (size / page_size) * sizeof(StVMemCtrlBlock);
#if VOS_SLAB_ENABLE
	if (pheap && enable_slab) {
		pheap->slab_ptr = (struct StVSlabMgr *)(pheap + 1);
		pheap->slab_ptr = VSlabBuild(pheap->slab_ptr, slag_header_size, page_size,
						align_bytes, VOS_SLAB_STEP_SIZE, "vslab", pheap);
	}
	else
#endif
	{
		pheap->slab_ptr = 0;
	}

	if (pheap->slab_ptr == 0) {
		pheap->pMCB_base = (StVMemCtrlBlock*)(pheap + 1);
	}
	else {
		pheap->pMCB_base = (StVMemCtrlBlock*)((u8*)pheap->slab_ptr + slag_header_size);
	}

	pheap->page_base = (u8*)ALIGN_DOWN((u8*)pheap + head_size, align_bytes); //��һ��page_base�����ַ�����������ĵ�ַ
	pheap->page_counts = (pheap->mem_end - pheap->page_base) / page_size;

	pheap->page_size = page_size;
	pheap->align_bytes = align_bytes;
	pheap->name = name;
	pheap->heap_attr = heap_attr;

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

		VMEM_LOCK();
		//��cnts > 1, ֤��page[MAX_PAGE_CLASS_MAX-1]����һ�����������飬��������MAX_PAGE_CLASS_MAX
		//���򣬽�����ڴ������ķŵ�page[MAX_PAGE_CLASS_MAX-1]������
		for (i = 0; i<cnts; i++) {
			pMCB = &pheap->pMCB_base[(offset_size + i * (1 << index)*page_size) / page_size];
			pMCB->page_max = 1 << index;
			pMCB->status = VMEM_STATUS_FREE;
			pMCB->task_id = VMEM_BELONG_TO_NONE;

			list_add_tail(&pMCB->list, &pheap->page_class[index]);

		}
		VMEM_UNLOCK();

		offset_size += temp;
	}
	//��ӵ��ѹ�������
	if (pheap) {
		VHeapMgrAdd(pheap);
	}
#if 0 //���slab��������̬���䣬Ҳ���ԣ�����Ŀǰ��slab��Ϊbuddy�Ĺ������е�һ���ֿ��ܸ��ã���ֹд���������ƻ�slabͷ����
#if VOS_SLAB_ENABLE
	/******** ��ʼ��SLAB������ ***********/
	if (pheap && enable_slab) {
		s32 vslab_size = SlabMgrHeaderGetSize(page_size, align_bytes, VOS_SLAB_STEP_SIZE);
		u8 *vslab_ptr = (u8*)VMemMalloc(pheap, SlabMgrHeaderGetSize(page_size, align_bytes, VOS_SLAB_STEP_SIZE));
		pheap->slab_ptr = VSlabBuild(vslab_ptr, vslab_size, page_size,
				align_bytes, VOS_SLAB_STEP_SIZE, "vslab", pheap);
	}
	else
#endif
	{
		pheap->slab_ptr = 0;
	}
#endif
	/******** *********** ***********/
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


#if VOS_SLAB_ENABLE
	/******** �Ȳ���SLAB������ ***********/
	if (pheap && pheap->slab_ptr) {
		pObj = VSlabBlockAlloc(pheap->slab_ptr, size);
		if (pObj) return pObj;
	}
	/******** *********** ***********/
#endif

	if (size > pheap->page_size * (1 << (MAX_PAGE_CLASS_MAX - 1))) {//�����С ���� ����
		return 0;
	}
	index = GetPageClassIndex(size, pheap->page_size);
	if (index < 0) goto END_VMEMMALLOC;

	//��������ڴ�
	index_top = -1;

	VMEM_LOCK();
	for (i = index; i<MAX_PAGE_CLASS_MAX; i++)
	{

		if (!list_empty(&pheap->page_class[i])) {
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
		//�趨����ID
		pMCB->task_id = GetCurTaskId();

#if VMEM_TRACER_ENABLE
		pMCB->used_num = size;
#endif
		pMCB->page_max = 1 << index_top; //page_class[n]��Ӧ�����ҳ�� ���ͷ�ʱ��Ҫ�õ���

		//���ӵ����������У��������ʹ��
		list_add_tail(&pMCB->list, &pheap->mcb_used);

		//���ҳ��ַ
		pObj = pheap->page_base + (pMCB - pheap->pMCB_base) * pheap->page_size;

#if VMEM_TRACER_ENABLE
#if	VMEM_TRACE_DESTORY
		//���̶��ַ��� �������
		memset((u8*)pObj+pMCB->used_num, VMEM_MALLOC_PAD, pMCB->page_max * pheap->page_size - pMCB->used_num);
#endif
#endif
	}
	VMEM_UNLOCK();


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

#if VOS_SLAB_ENABLE
	/******** ���ͷ�SLAB������ ***********/
	if (pheap && pheap->slab_ptr && VSlabBlockFree(pheap->slab_ptr, p)) {
		//ָ����slab��Χ�ڣ�ֱ�ӷ���
		return ;
	}
	/******** *********** ***********/
#endif
	//ָ����slab��Χ�⣬����slab�ͷ�
	if ((u8*)p < pheap->page_base || (u8*)p >= pheap->mem_end) return; //��ַ��Χ���
																	   //�ж�p��ַ������ҳ����룬���߲��ͷ�
	size = (u8*)p - pheap->page_base;
	if (size % pheap->page_size) return; //��ַ���������page_size����

	offset = size / pheap->page_size;
	pMCB = &pheap->pMCB_base[offset];
	page_max = pMCB->page_max;

	if (pMCB->status != VMEM_STATUS_USED) {
		return;
	}

	VMEM_LOCK();
	//�ͷ�ʱҲ�ô��ѷ���������ɾ��
	list_del(&pMCB->list);

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
		//����ʱ����id����Ϊ��
		pMCB->task_id = VMEM_BELONG_TO_NONE;

#if VMEM_TRACER_ENABLE
		pMCB->used_num = 0;
#endif

		list_add_tail(&pMCB->list, &pheap->page_class[index]);//��ӵ���������ĩβ
	}
	VMEM_UNLOCK();
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

#if VOS_SLAB_ENABLE
	/******** ���ͷ�SLAB������ ***********/
	if (pheap && pheap->slab_ptr && VSlabBlockFree(pheap->slab_ptr, p)) {
		//ָ����slab��Χ�ڣ�ֱ�ӷ���
		ptr = VMemMalloc(pheap, size);
		if (ptr) {
			memcpy(ptr, p, size);
		}
		goto END_VMEMREALLOC;
	}
	/******** *********** ***********/
#endif

	//�ж��Ǳ��ѵ�malloc
	VMEM_LOCK();
	if ((u8*)p >= pheap->page_base &&
		(u8*)p < pheap->mem_end &&
		((u8*)p - pheap->page_base) % pheap->page_size == 0) {
		pMCB = &pheap->pMCB_base[((u8*)p - pheap->page_base) / pheap->page_size];
		//�ж�size�Ƿ񳬳��������ߴ�
		if (pMCB->status == VMEM_STATUS_USED && //�������Ѿ������ȥ״̬
			pMCB->page_max * pheap->page_size >= size) { //�����ʣ��ռ��㹻��
			ptr = p;
			VMEM_UNLOCK();
			goto END_VMEMREALLOC;
		}
	}
	VMEM_UNLOCK();
	//����ʣ��ռ���߲��ڱ������ڣ�ֱ��malloc��memcpy
	ptr = VMemMalloc(pheap, size);
	if (ptr) {
		memcpy(ptr, p, size);
	}

END_VMEMREALLOC:
	return ptr;
}

/********************************************************************************************************
* ������void *VMemGetPageBaseAddr(struct StVMemHeap *pheap, void *any_addr);
* ����: ��ָ�������ҵ��κε�ַ��Ӧ��ҳ�������ַ���ڶѵ�ַ��Χ�ڣ�����0
* ����:
* [1] pheap: ָ���Ķ�
* [2] any_addr: �κε�ַ��Ϣ
* ���أ���Ӧ����ĳ��ҳ�Ļ���ַ
* ע�⣺�ú�������slab����������Ҷѻ���ַ��
*********************************************************************************************************/
void *VMemGetPageBaseAddr(struct StVMemHeap *pheap, void *any_addr)
{
	//��ַ���ڶ���ķ�Χ��
	if ((u8*)any_addr < pheap->page_base || (u8*)any_addr >= pheap->mem_end) {
		return 0;
	}
	return ((u8*)any_addr - pheap->page_base)/pheap->page_size*pheap->page_size + pheap->page_base;
}

/********************************************************************************************************
* ������s32 VMemGetHeapInfo(struct StVMemHeap *pheap, struct StVMemHeapInfo *pheadinfo);
* ����: ��ѯ�ѵ���Ϣ,�����ܹ�����ҳ���ܹ��ѷ���ҳ����ǰ������ռ䣬�ѷ���������ռ��
* ����:
* [1] pheap: ָ���Ķ�
* [2] pheadinfo: �������ͳ����Ϣ
* ���أ�true or false
* ע�⣺��
*********************************************************************************************************/
s32 VMemGetHeapInfo(struct StVMemHeap *pheap, struct StVMemHeapInfo *pheadinfo)
{
	s32 i;
	s32 free_total = 0;
	s32 used_total = 0;
	s32 cur_max_size = 0;
	struct list_head *list;
	StVMemCtrlBlock *pMCB = 0;

	if (pheap == 0 || pheadinfo == 0) return 0;

	free_total = 0;

	VMEM_LOCK();
	for (i = 0; i<MAX_PAGE_CLASS_MAX; i++) {
		if (!list_empty(&pheap->page_class[i])) {//�ǿ�
			list_for_each(list, &pheap->page_class[i]) {
				pMCB = list_entry(list, struct StVMemCtrlBlock, list);
				//��������ܺ�
				free_total += pMCB->page_max*pheap->page_size;
			}
			cur_max_size = (1<<i) * pheap->page_size;
		}
	}
	//�ѷ���������ܺ�
	used_total = 0;
	list_for_each(list, &pheap->mcb_used) {
		pMCB = list_entry(list, struct StVMemCtrlBlock, list);
		used_total += pMCB->page_max*pheap->page_size;
	}

	VMEM_UNLOCK();

	pheadinfo->align_bytes = pheap->align_bytes;
	pheadinfo->page_counts = pheap->page_counts;
	pheadinfo->page_size = pheap->page_size;
	pheadinfo->max_page_class = MAX_PAGE_CLASS_MAX - 1;
	pheadinfo->cur_max_size = cur_max_size;
	pheadinfo->used_page_bytes = used_total;
	pheadinfo->free_page_bytes = free_total;

	return 1;
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
	VMEM_LOCK();
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
	VMEM_UNLOCK();

#if VOS_SLAB_ENABLE
	s32 VSlabInfoDump(struct StVSlabMgr *pSlabMgr);
	if (pheap && pheap->slab_ptr) {
		VSlabInfoDump(pheap->slab_ptr);
	}
#endif

	return 0;
}

/********************************************************************************************************
* ������s32 VMemTraceDestory(struct StVMemHeap *pheap);
* ����: ��鵱ǰ����malloc���ڴ��Ƿ��б�д�ƻ���ֻ���ʣ��ռ䡣
* ����:
* [1] pheap: �Ѷ���ָ��
* ���أ�-1���ƻ���  0��û���ƻ�
* ע�⣺��
*********************************************************************************************************/
s32 VMemTraceDestory(struct StVMemHeap *pheap)
{

	s32 i;
	s32 ret = 0;
	s32 offset = 0;
	struct list_head *list;
	StVMemCtrlBlock *pMCB = 0;
	u8 *pBase = 0;

	VMEM_LOCK();
	//����Ѿ�����ɵ��ڴ��Ƿ񱻴۸�
	list_for_each(list, &pheap->mcb_used) {
		pMCB = list_entry(list, struct StVMemCtrlBlock, list);
		offset = pMCB - pheap->pMCB_base;
		pBase = pheap->page_base + offset * pheap->page_size;
		for (i = pMCB->used_num; i < pMCB->page_max * pheap->page_size; i++) {
			if (pBase[i] != VMEM_MALLOC_PAD) {//ҳ������ⲻ��Խ��
				//ASSERT_TEST(0, __FUNCTION__);
				ret = -1;
				break;
			}
		}
	}
	VMEM_UNLOCK();

	return ret;
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


#if VOS_SLAB_ENABLE
	if ((u8*)(pheap + 1) != (u8*)pheap->slab_ptr) BOUNDARY_ERROR();
	s32 slag_header_size = SlabMgrHeaderGetSize(pheap->page_size, pheap->align_bytes, VOS_SLAB_STEP_SIZE);
	if ((u8*)((u8*)pheap->slab_ptr+slag_header_size) != (u8*)pheap->pMCB_base) BOUNDARY_ERROR();
#else
	//�ж�pheap�ڲ�����Ԫ���Ƿ�����
	if ((u8*)(pheap + 1) != (u8*)pheap->pMCB_base) BOUNDARY_ERROR();
#endif
	//��������ĩβ��ҳ�������Ŀ�ʼ
	if ((u8*)ALIGN_DOWN(&pheap->pMCB_base[pheap->page_counts], pheap->align_bytes) != pheap->page_base) BOUNDARY_ERROR();
	//ҳ��������ĩβ����mem_end
	if ((u8*)ALIGN_DOWN(&pheap->page_base[pheap->page_counts * pheap->page_size], pheap->align_bytes) != pheap->mem_end) BOUNDARY_ERROR();

	VMEM_LOCK();
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
						if (offset % (1 <<(i+1)) == 0) { //i+1,����һ����룬������ߵ�ַ�������������ұߵ�ַ�������ұ������϶���Ҫ�ϲ�����һ�㣩
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

	VMEM_UNLOCK();

#if VOS_SLAB_ENABLE
	s32 VSlabBoudaryCheck(struct StVSlabMgr *pSlabMgr);
	if (pheap && pheap->slab_ptr) {
		VSlabBoudaryCheck(pheap->slab_ptr);
	}
#endif

	return 1;

ERROR_RET:
	VMEM_UNLOCK();

	VMemInfoDump(pheap);
	kprintf("*************\r\nERROR: %s, please check the code!!!\r\n*************\r\n", __FUNCTION__);
	while (1);
	return 0;
}




