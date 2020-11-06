/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vslab.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��	��: slab��Ҫ�������Ǵ���С�ڴ���䣬�ͷŲ��Ի��ӳ٣���Ƶ����������Ƚϴ󣬻����ڴ��˷��кô�
* ע	��: һ���Ѷ�Ӧһ��slab, �������ѵĵ�page_size��align_size��ͬ��Ҳ���Զ����
* ��    ʷ��
* --20200925�������ļ�, ʵ��slab�ڴ�ط����㷨���������������ڴ����ĳ���ߴ磬ֱ�Ӵ�buddy����

*********************************************************************************************************/
#include "vtype.h"
#include "vos.h"
#include "vlist.h"
#include "vslab.h"
#include "vheap.h"
#include "vmem.h"

#define VSLAB_LOCK() 	pSlabMgr->irq_save = __vos_irq_save()
#define VSLAB_UNLOCK()   __vos_irq_restore(pSlabMgr->irq_save)

#define VSLAB_MAGIC_NUM  	0x05152535

#ifndef SLAB_NUM
#define SLAB_NUM(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

//step_sizeҲҪ�������
#define ALIGN_SIZE(step, align) (((step)+ (align)-1)/(align) * (align))


typedef void* (*BUDDY_MALLOC_FUN)(unsigned int size);

typedef void (*BUDDY_FREE_FUN)(void *ptr);

typedef struct StVSlabClass {
	struct list_head page_full;		//ҳ������block�Ѿ������ȥ��ҳ���ӵ����������
	struct list_head page_partial;	//ҳ�в���block�����ȥ��ҳ���ӵ������������
	struct list_head page_free;		//ҳ������block�����е�ҳ���ӵ������������
	u32    page_free_num;			//ͳ�Ƶ�ǰ���������ҳ��
	u32    page_full_num;			//ͳ�Ƶ�ǰ��ҳ�����ҳ��
	u32    page_partial_num;		//ͳ�Ƶ�ǰ����ҳ�����ҳ��
}StVSlabClass;

//ͨ��һ�������ֶ�Ӧһ��slab�������������ͬ�ѣ�����ҳ�ߴ�����ͬ�ģ�ҳ���Թ�ͬʹ��
typedef struct StVSlabMgr {
	s8 *name;					//slab����������
	struct StVMemHeap *pheap;	//ָ��ĳ���̶��Ķѣ�Ŀǰֻ����һ����ָ��һ��������
	s32 irq_save;			//�ж�״̬�洢
	s32 page_header_size;	//ÿҳ�������Ĵ�С������bitmap�����С������λ�ֽڣ�����ʱ����ȷ��
	s32 slab_header_size;	//StVSlabMgr����Ĵ�С������class_base�����С������λ�ֽڣ�����ʱ����ȷ��
	s32 align_bytes;		//�����ֽڶ���
	s32 page_size;			//buddy�㷨����1��ҳ�Ĵ�С
	s32 step_size;			//���������Ĳ���������8��n*8
	s32 class_max;			//class_base ����ĳ���
	struct StVSlabClass class_base[0]; //�ܷ���������ͳ���2����֤���Է���2�����ϣ�����ֱ��buddy����
}StVSlabMgr;


//ÿҳ������Ŷ��������������ŵ�ͷ����Ϊ�������Լ��ٶ��������
typedef struct StVSlabPage {
	u32 magic;
	struct list_head list;
	s32 status;		//ָʾ���ĸ�����: SLAB_BITMAP_FREE, SLAB_BITMAP_PARTIAL, SLAB_BITMAP_FULL
	u8 *block_base; //ָ��block������ʼ��ַ������buddy�����㷨�׵�ַ
	s32 block_size; //ÿ�������ռ�õ�bytes
	s32 block_max; 	//ÿҳ���Դ洢���������ĸ���
	s32 bitmap_max; //�����С��Ϊ�˴����㣬�����������ֵ=(ҳ��С/step_size/8)
	u8	bitmap[0];  //blockλͼ�洢����
}StVSlabPage;

//�����ҵ���һ����0��ƫ��λ��
const u8 SlabBitMap[] =
{
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 0,
};

/********************************************************************************************************
* ������s32 FirstFreeBlockGet(u8 *bitmap, s32 num, s32 offset);
* ����: ��bitmap��offsetƫ��ֵ��Ϊ��ʼλ�ò��ҵ�һ�����п��ƫ��λ��
* ����:
* [1] bitmap: λͼ����
* [2] num: λͼ�����С����λ���ֽ�
* [3] offset: ��offset��Ϊ��ʼƫ��ֵ��ʼ����
* ���أ��ɹ�����ƫ��λ�ã����򷵻�-1
* ע�⣺��Ϊbitmap����ͨ�������ж���β�ͣ����Ǹպã��ģ���󶼻�����0���������ȫ������ȡ�����һ��ƫ��ֵӦ��
* ����β�����ֵ������ͨ���жϷ���ֵ�����������ƫ��ֵ���ж�
*********************************************************************************************************/
s32 FirstFreeBlockGet(u8 *bitmap, s32 num, s32 offset)
{
	s32 i = 0;
	s32 ret = -1;
	offset = offset / 8;
	for (i = offset; i < num; i++) {
		if (bitmap[i] != 0xFFU) {
			ret = i * 8 + SlabBitMap[bitmap[i]];
			break;
		}
	}
	return ret;
}

/********************************************************************************************************
* ������s32 VSlabPageHeaderGetSize(s32 page_size, s32 step_size, s32 align_bytes);
* ����: ����StVSlabPage��bitmap�����ܺ͵Ĵ�С
* ����:
* [1] page_size: slab�Ĺ�������ַ
* [2] align_bytes: slab�Ĺ�������ַ����λ���ֽ�
* [3] step_size: ҳ�ߴ磬Ҫ���buddy�����㷨��ҳ�Ĵ�С������
* ���أ�����StVSlabPage��bitmap�����ܺ͵Ĵ�С
* ע�⣺��
*********************************************************************************************************/
s32 VSlabPageHeaderGetSize(s32 page_size, s32 step_size, s32 align_bytes)
{
	//Ԥ��洢��ͬ�ߴ���������ֵ
	//���ֵ���㷽��������ÿҳ���洢�ռ䷨������2�����ݿ�+ҳ������<=ҳ��С
	//���ֵ = (ҳ��С / MAX(step_size, align_size))

	//step_size�������
	step_size = ALIGN_SIZE(step_size, align_bytes);
	//bitmap_max����ҳ������λͼ�����С�����ֵ��
	s32 bitmap_max = (page_size /step_size + 8 - 1) / 8;
	//������ҳͷ�ߴ磬����λͼ�����С
	return sizeof(struct StVSlabPage) + bitmap_max * 1;
}


/********************************************************************************************************
* ������s32 VSlabPageCalcBlockMaxSize(s32 page_size, s32 step_size, s32 align_bytes);
* ����: ����һҳ�����洢block�Ĵ�С
* ����:
* [1] page_size: slab�Ĺ�������ַ
* [2] align_bytes: slab�Ĺ�������ַ����λ���ֽ�
* [3] step_size: ҳ�ߴ磬Ҫ���buddy�����㷨��ҳ�Ĵ�С������
* ���أ�����һҳ�����洢block�Ĵ�С
* ע�⣺��
*********************************************************************************************************/
s32 VSlabPageCalcBlockMaxSize(s32 page_size, s32 step_size, s32 align_bytes)
{
	//step_size�������
	step_size = ALIGN_SIZE(step_size, align_bytes);
	s32 page_head_size = VSlabPageHeaderGetSize(page_size, step_size, align_bytes);
	s32 block_space = (page_size - page_head_size)/align_bytes * align_bytes;
	s32 block_max_size = block_space / 2; //��֤2��������
	return block_max_size / step_size * step_size; //��Ҫ��block_bytes����
}

/********************************************************************************************************
* ������s32 SlabMgrHeaderGetSize(s32 page_size, s32 align_bytes, s32 step_size);
* ����: ����SlabMgr��Ҫ�Ŀռ��С������class_base����Ĵ�С
* ����:
* [1] page_size: slab�Ĺ�������ַ
* [2] align_bytes: slab�Ĺ�������ַ����λ���ֽ�
* [3] step_size: ҳ�ߴ磬Ҫ���buddy�����㷨��ҳ�Ĵ�С������
* ���أ�SlabMgrͷ��class_base�����ܺ͵Ĵ�С
* ע�⣺��
*********************************************************************************************************/
s32 SlabMgrHeaderGetSize(s32 page_size, s32 align_bytes, s32 step_size)
{
	s32 block_max_bytes = 0; //ÿҳ�п����ߴ�ֵ
	//step_size�������
	step_size = ALIGN_SIZE(step_size, align_bytes);
	//����һҳ��������ݿ�ߴ�
	block_max_bytes = VSlabPageCalcBlockMaxSize(page_size, step_size, align_bytes);
	//�������ݿ����ߴ���Լ����class_base���͵�����
	s32 class_max = block_max_bytes / step_size;
	return sizeof(struct StVSlabMgr) + class_max * sizeof(struct StVSlabClass);
}

/********************************************************************************************************
* ������struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size, 
*                                                  s32 align_bytes, s32 step_size, s8 *name);
* ����:  vslab�ڴ�ط���������
* ����:
* [1] mem: slab�Ĺ�������ַ
* [2] len: slab�Ĺ�������ַ����λ���ֽ�
* [3] page_size: ҳ�ߴ磬Ҫ���buddy�����㷨��ҳ�Ĵ�С������
* [4] align_bytes: �ֽڶ��룬����8������8�ֽڶ���
* [5] step_size: ���Ե����Ĳ���
* [6] name: slab����
* [7] pheap: ��Ӧbuddyϵͳ�Ķ�ָ��
* ���أ�slab�������ָ��
* ע�⣺��
*********************************************************************************************************/
struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size,
								s32 align_bytes, s32 step_size, s8 *name, struct StVMemHeap *pheap)
{
	struct StVSlabMgr *pSlabMgr = 0;
	s32 block_max_bytes = 0; //ÿҳ�п����ߴ�ֵ
	s32 i = 0;
	s32 slab_header_size = SlabMgrHeaderGetSize(page_size, align_bytes, ALIGN_SIZE(step_size, align_bytes));
	s32 page_header_size = VSlabPageHeaderGetSize(page_size, step_size, align_bytes);
	if (len < slab_header_size) return 0;

	VSLAB_LOCK();

	pSlabMgr = (struct StVSlabMgr*)mem;

	//����һҳ��������ݿ�ߴ�
	block_max_bytes = VSlabPageCalcBlockMaxSize(page_size, step_size, align_bytes);
	//�������ݿ����ߴ���Լ����class_base���͵�����
	pSlabMgr->class_max = block_max_bytes / step_size;

	pSlabMgr->page_size = page_size;
	pSlabMgr->align_bytes = align_bytes;
	pSlabMgr->step_size = ALIGN_SIZE(step_size, align_bytes);
	pSlabMgr->page_header_size = page_header_size;
	pSlabMgr->slab_header_size = slab_header_size;
	pSlabMgr->pheap = pheap;
	pSlabMgr->name = name;

	//��ʼÿ���ҳ����
	for (i = 0; i<pSlabMgr->class_max; i++) {
		INIT_LIST_HEAD(&pSlabMgr->class_base[i].page_full);
		INIT_LIST_HEAD(&pSlabMgr->class_base[i].page_partial);
		INIT_LIST_HEAD(&pSlabMgr->class_base[i].page_free);
		pSlabMgr->class_base[i].page_full_num = 0;
		pSlabMgr->class_base[i].page_partial_num = 0;
		pSlabMgr->class_base[i].page_free_num = 0;
	}

	VSLAB_UNLOCK();
	return pSlabMgr;
}


/********************************************************************************************************
* ������struct StVSlabPage *VSlabPageBuild(u8 *mem, s32 len, s32 block_size, struct StVSlabMgr *pSlabMgr);
* ����: �������������vmalloc��ҳ�ﴴ������blocks�Ĺ�������
* ����:
* [1] mem: buddyϵͳvmalloc������ҳ����ַ
* [2] len: ҳ��С����λ���ֽ�
* [3] block_size: �����slab_class[n]��Ӧ����һ����С�����磺slab_class[6]�� ����8���������������56�ֽ�
* [4] align_bytes: �ֽڶ��룬����8������8�ֽڶ���
* [5] step_size: ������������λ���ֽ�
* [6] pSlabMgr: slabMgr����
* ���أ�ҳ�������ָ��
* ע�⣺��
*********************************************************************************************************/
struct StVSlabPage *VSlabPageBuild(u8 *mem, s32 len, s32 block_size, struct StVSlabMgr *pSlabMgr)
{
	s32 page_size = pSlabMgr->page_size;
	s32 align_bytes = pSlabMgr->align_bytes;
	s32 step_size = pSlabMgr->step_size;

	s32 nBlocks = 0;
	s32 block_space = 0;
	struct StVSlabPage *pSlabPage = 0;
	s32 page_header_size = VSlabPageHeaderGetSize(page_size, step_size, align_bytes);
	if (mem == 0 || len < page_header_size) {
		return 0;
	}
	VSLAB_LOCK();
	//ҳ����ߵ�ַ���ҳ��������Ϣ
	pSlabPage = (struct StVSlabPage*)(mem+len- page_header_size);
	//block_spaceΪ����blocks���������ܴ�С
	block_space = (u8*)pSlabPage - mem;
	//���ﻹҪ�����ֽڶ�������
	block_space = block_space /align_bytes*align_bytes;
	//�������nBlocks����Ϊ��λͼ���鳣���ж���λ����Ҫ�������block_max���ж��Ƿ���ҳ
	nBlocks = block_space / block_size;

	memset(pSlabPage, 0, page_header_size);

	pSlabPage->magic = VSLAB_MAGIC_NUM;
	//pSlabPage->block_cnts = 0;
	pSlabPage->block_size = block_size;
	pSlabPage->block_max = nBlocks;
	pSlabPage->block_base = mem;
	//pSlabPage->align_bytes = align_bytes;
	//ע�⣺����ʵ�ʲ�ʹ�����ֵ����8�ֽڶ���Ϳ��ԣ��������Լ��ٷ���λͼ�ٶ�
	//���ǣ�����Ŀռ����Ҫ�����ֵ���㣬�����Ϳ��Թ̶���ҳ����ռ䣬������ҡ�
	pSlabPage->bitmap_max = (nBlocks + 8 - 1) / 8;
	//bitmap_max����ҳ������λͼ�����С�����ֵ��
	//pSlabPage->bitmap_max = (page_size / step_size + 8 - 1) / 8;
	VSLAB_UNLOCK();

	return pSlabPage;
}


/********************************************************************************************************
* ������void *SlabBitMapSet(struct StVSlabPage *page, s32 *status);
* ����: ����������ҵ���һ������blocksλ�ú󷵻ظ�blocks��ַ�������ø�λ��Ȼ������Ƿ�λͼ�Ƿ�����
*       ���bitmap������Чλ��1���򷵻�״̬����ҳ�����򷵻ز���ҳ
* ����:
* [1] page: ҳ�������
* [2] status: ����״̬��ֻ��2��״̬������ҳ״̬��SLAB_BITMAP_PARTIAL������ҳ״̬��SLAB_BITMAP_FULL��
* ���أ�����ɹ������ط����block�ľ����ַ��
* ע�⣺��
*********************************************************************************************************/
void *SlabBitMapSet(struct StVSlabPage *page, s32 *status)
{
	void *pnew = 0;
	s32 offset = FirstFreeBlockGet(page->bitmap, page->bitmap_max, 0);
	if (offset < 0 || offset >= page->block_max) {//��������ƫ��
		*status = SLAB_BITMAP_FULL;
		pnew = 0;
	}
	else {
		*status = SLAB_BITMAP_PARTIAL;
		//��0ƫ�ƿ�ʼ�ҵ�һ��Ϊ0�Ŀ��п�
		bitmap_set(offset, page->bitmap);
		pnew = &page->block_base[offset * page->block_size];
		//���ø�λ�� ��offset�鿴���Ƿ������0xFF����ʾû���κο��п�
		offset = FirstFreeBlockGet(page->bitmap, page->bitmap_max, offset);
		//�������-1��֤��ȫ�����Ѿ������blocks, �������λͼ�����ֵ��
		//���պ��⣬���඼��0�������͵��ж�offset �Ƿ���ڵ���page->block_max��������
		if (offset < 0 || offset >= page->block_max) { //
			*status = SLAB_BITMAP_FULL;
		}
	}
	return pnew;
}

/********************************************************************************************************
* ������void *VSlabBlockAlloc(struct StVSlabMgr *pSlabMgr, s32 size);
* ����: ��slab����������ָ����С���ڴ�
* ����:
* [1] pSlabMgr: slab�������
* [2] size: ����ָ����С�ڴ�
* ���أ�����ɹ������ط����block�ľ����ַ��
* ע�⣺�������size�ߴ����slab��������ߴ磬����0����Ϊÿҳ�������ҳ�������+2�������ϵ�block��
* Ϊʲô2������Ϊ2�����ٿ������úÿռ䣬��Ȼbuddy̫�˷ѿռ�
* ���ֻ�ܵ���block����û�����壬ֱ�Ӵ�buddy���䡣
*********************************************************************************************************/
void *VSlabBlockAlloc(struct StVSlabMgr *pSlabMgr, s32 size)
{
	s32 index = 0;
	void *pnew = 0;
	s32 status = 0;
	struct list_head *head_partial = 0;
	struct list_head *head_free = 0;
	struct list_head *head_full = 0;
	struct StVSlabPage *slab_page = 0;
	s32 step_size = 0;

	if (pSlabMgr == 0 || size <= 0) return 0;

	step_size = pSlabMgr->step_size;

	//����size��Ӧ���ĸ�slab_class��
	index = (size + step_size - 1) / step_size - 1;

	//��֤size*2+ҳ������ С�ڵ���ҳ�ܴ�С������ֱ�Ӵ�buddy����
	if (index >= pSlabMgr->class_max) {
		return 0;
	}
	VSLAB_LOCK();
	//�ҵ���Ӧ��class���ҵ���class�Ĳ���������Ϊ��������������ڣ��϶�������һ��block�ɷ���
	head_partial = &pSlabMgr->class_base[index].page_partial;
	if (list_empty(head_partial)) { //���partialΪ�գ�����Ҫ��free������ȡ��һ���ŵ�partial��
		head_free = &pSlabMgr->class_base[index].page_free;
		if (list_empty(head_free)) {//���page_free����Ϊ�գ�����Ҫ��buddy������һ��ҳ����ʼ��

			slab_page = 0;
			if (pSlabMgr->pheap) { //�Ӷ�������һҳ
				VSLAB_UNLOCK();
				u8 *tmp_ptr = (u8*)VMemMalloc(pSlabMgr->pheap, pSlabMgr->page_size, 1);//1Ϊ��־slabʹ�ã��ͷ�ʱʹ��
				VSLAB_LOCK();
				slab_page = VSlabPageBuild(tmp_ptr, pSlabMgr->page_size, (index+1)*pSlabMgr->step_size, pSlabMgr);
			}
			if (slab_page == 0) {
				goto END_SLAB_BLOCK_ALLOC;
			}
		}
		else { //��ȡ��һ��page
			slab_page = list_entry(head_free->next, struct StVSlabPage, list);
			pSlabMgr->class_base[index].page_free_num--;
			//��free�����жϿ�
			list_del(&slab_page->list);
		}
		//���뵽partial����
		list_add(&slab_page->list, head_partial);
		pSlabMgr->class_base[index].page_partial_num++;
		slab_page->status = SLAB_BITMAP_PARTIAL;
	}
	else { //ֱ�Ӵ�partial����ȡ��һ��������
		slab_page = list_entry(head_partial->next, struct StVSlabPage, list);
	}
	//����λͼ���ҵ���һ�����п鲢��λ
	pnew = SlabBitMapSet(slab_page, &status);


	if (status == SLAB_BITMAP_FULL) { //��λ��������ҳ�Ƶ�page_full����
									  //�ȴ�page_partial����ɾ��
		list_del(&slab_page->list);
		pSlabMgr->class_base[index].page_partial_num--;

		//����ӵ�page_full
		head_full = &pSlabMgr->class_base[index].page_full;
		list_add(&slab_page->list, head_full);
		slab_page->status = SLAB_BITMAP_FULL;
		pSlabMgr->class_base[index].page_full_num++;
	}

END_SLAB_BLOCK_ALLOC:
	VSLAB_UNLOCK();

	return pnew;

}

/********************************************************************************************************
* ������void SlabBitMapClr(struct StVSlabPage *page, s32 *status, s32 offset);
* ����: ������������ͷ��ڴ溯�������λͼĳ��λ��Ȼ���ѯ�Ƿ�ȫ������У�����Ǿ��Ƿ��ؿ���״̬�����򷵻ز���״̬
* ����:
* [1] page: ҳ�������
* [2] status: ����״̬��ֻ��2��״̬������ҳ״̬��SLAB_BITMAP_PARTIAL���Ϳ���ҳ״̬��SLAB_BITMAP_FREE��
* [3] offset: Ҫ�����ƫ��λ��
* ���أ���
* ע�⣺�ظ��ͷ�, ����SLAB_BITMAP_PARTIAL״̬
*********************************************************************************************************/
void SlabBitMapClr(struct StVSlabPage *page, s32 *status, s32 offset)
{
	s32 i = 0;
	if (!bitmap_get(offset, page->bitmap)) { //�ظ��ͷ�
		*status = SLAB_BITMAP_PARTIAL; //�ظ��ͷ�, ���ز���ҳ״̬
		return;
	}
	if (offset >= page->block_max) {
		kprintf("error: %d offset >= page->block_max!\r\n", __FUNCTION__);
		return;
	}
	bitmap_clr(offset, page->bitmap);

	//�����Ƿ�ȫ0�����ȫ0����Ҫ����SLAB_BITMAP_FEE״̬
	for (i=0; i<page->bitmap_max; i++)
	{
		if (page->bitmap[i] != 0) {
			*status = SLAB_BITMAP_PARTIAL;
			break;
		}
	}
	if (i==page->bitmap_max) { //���bitmapȫ0�����ؿ���ҳ��־
		*status = SLAB_BITMAP_FREE;
	}
}

/********************************************************************************************************
* ������void VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr);
* ����: �ͷ�ĳ��block��
* ����:
* [1] pSlabMgr: slab�������
* [2] ptr: �ͷſ��ָ��
* ���أ�������Ϣ��ptrָ���Ƿ���slab�ķ���������Χ�ڣ�����ֵ��buddy�������ͷ�ʱ�ṩ��Ϣ
* ע�⣺��
*********************************************************************************************************/
s32 VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr)
{
	s32 ret = 0;
	s32 index = 0;
	u8 *pPageBase = 0;
	s32 status = 0;
	s32 offset = 0;
	struct list_head *head_free = 0;
	struct StVSlabPage *slab_page = 0;
	struct StVSlabPage *head_partial = 0;

	VSLAB_LOCK();
	//�鿴�ͷŵĵ�ַ���ڵ�ҳ��ַ��ͨ��buddy�������ϲ���
	if (pSlabMgr->pheap) {
		pPageBase = (u8*)VMemGetPageBaseAddr(pSlabMgr->pheap, ptr);
	}
	//pPageBase = (u8*)VHeapMgrGetPageBaseAddr(ptr, 0, 0);
	if (pPageBase == 0) {
		//kprintf("error: %s, pPageBase==NULL!\r\n", __FUNCTION__);
		goto END_SLAB_BLOCK_FREE;
	}
	//����ҳ��ַ���ҵ�ҳ������������ҳ�����ߵ�ַ��
	slab_page = (struct StVSlabPage *)(pPageBase + pSlabMgr->page_size - pSlabMgr->page_header_size);
	//У��magic��
	if (slab_page->magic != VSLAB_MAGIC_NUM) {
		//kprintf("error: %s, slab_page->magic match failed!\r\n", __FUNCTION__);
		goto END_SLAB_BLOCK_FREE;
	}

	//����ҳ���������block_size��ߴ磬����slab_class�����ŵ�ַ
	index =  (slab_page->block_size + pSlabMgr->step_size - 1) / pSlabMgr->step_size - 1;

	if (index >= pSlabMgr->class_max) {
		goto END_SLAB_BLOCK_FREE;
	}

	//ע�⣺slab_page->magic == VSLAB_MAGIC_NUM ֤�������slab���������ҳ����ʱret=1���ص�ַ����slab��������
	ret = 1;

	//У���ͷŵĶ����ַ�Ƿ��Ƕ�����׵�ַ������Ƕ�������ĵ�ַ���ͷ�ʧ�ܣ����������ַ���ͷ�
	if(((u8*)ptr - pPageBase)%slab_page->block_size) {
		goto END_SLAB_BLOCK_FREE;
	}
	//�����ͷŵ�ָ���Ӧҳ��ַ��ƫ��λ��
	offset = ((u8*)ptr - pPageBase)/slab_page->block_size;
	//���ҳ��������λͼ��Ӧ��ƫ��λ���������ͷ���ĳ��block, ע�⻹����״̬����������״̬
	SlabBitMapClr(slab_page, &status, offset);
	//������������λ�󣬷���ȫ�����ǿ���block������partial���Ƶ�free����
	if (status == SLAB_BITMAP_FREE) {//�����ȫ��λͼ��Ҫ��ӵ�page_free������
		//��head_partial�Ͽ�����
		list_del(&slab_page->list);
		pSlabMgr->class_base[index].page_partial_num--;

		//�����趨�Ŀ���ҳ����ֵ�����ͷ�ҳ��buddyϵͳ��
		if (pSlabMgr->class_base[index].page_free_num >= VSLAB_FREE_PAGES_THREHOLD) {
			//���һ��magic, ��ֹ����ͷ��ڴ��ַ�����ɵ���һ���ͷŵ���ҳ
			//����һ��ԭ��������magic�� ��ΪVMemFree�᳢���ȴ�slab�ͷ�
			slab_page->magic = 0;

			if (pSlabMgr->pheap) {
				VSLAB_UNLOCK();
				VMemFree (pSlabMgr->pheap , pPageBase, 1);//ָʾslab�����ͷź�����Ҫbuddy�ͷ��Լ����ڴ档
				VSLAB_LOCK();
			}
		}
		else {
			//���뵽free����
			head_free = &pSlabMgr->class_base[index].page_free;
			list_add(&slab_page->list, head_free);
			//����״̬
			slab_page->status = SLAB_BITMAP_FREE;
			pSlabMgr->class_base[index].page_free_num++;
		}

	}
	else {//����SLAB_BITMAP_PARTIAL��Ҫ�ж�slab_page�������Ǵ��������������ǴӲ���������
		if (slab_page->status == SLAB_BITMAP_FULL) {//����������������Ҫ�������Ƶ���������
		    //��head_full�Ͽ�����
			list_del(&slab_page->list);
			pSlabMgr->class_base[index].page_full_num--;

			//��ӵ���������
			head_partial = &pSlabMgr->class_base[index].page_partial;
			list_add(&slab_page->list, head_partial);
			//����״̬
			slab_page->status = SLAB_BITMAP_PARTIAL;
			pSlabMgr->class_base[index].page_partial_num++;
		}
	}

END_SLAB_BLOCK_FREE:

	VSLAB_UNLOCK();

	return ret;
}

/********************************************************************************************************
* ������s32 bitmap_iterate(void **iter, u8 val, void *bitmap, s32 nbits);
* ����: ������������λͼ
* ����:
* [1] iter: ������������¼��һ��Ҫ���ҵ�λ�ã����Ϊ0����ӵ�һ����ʼ����
* [2] val: 0��1�� �����0��ָʾ���ҵ�0�ͷ��أ������1��������1�ͷ��ء�
* [3] bitmap: λͼ�����ַ
* [4] nbits: λͼ�ĳ��ȣ�������λ��bits)
* ���أ�����ƫ��λ�ã����>0,�򷵻�����ƫ��ֵ�����-1������ʧ��
* ע�⣺��
*********************************************************************************************************/
s32 bitmap_iterate(void **iter, u8 val, void *bitmap, s32 nbits)
{
	u32 pos = (u32)*iter;
	u8 *p8 = (u8*)bitmap;
	u8 mask = 0;
	s32 i = 0;
#if 0
	for (i; i < (nbits + 7) / 8; i++) {
		kprintf("0x%02x ", ((u8*)bitmap)[i]);
	}
	kprintf("\r\n");
#endif

	mask = val ? 0 : 0xFFU;

	while (pos < nbits) {
		if ((pos & 0x7) == 0 && p8[pos >> 3] == mask) {//����һ���ֽ�
			pos += 8;
		}
		else {//����8bit
			do {
				if (val == !!(p8[pos >> 3] & 1 << (pos & 0x7))) {
					*iter = (void*)(pos + 1);
					goto END_ITERATE;
				}
				pos++;
				if (pos == nbits) { //���ĩβ���ܳ���nbits,֤���Ѿ���������
					*iter = (void*)(pos);
					break;
				}
			} while (pos & 0x7);
		}
	}
	//�Ҳ������򷵻�-1
	pos = -1;

END_ITERATE:

	return pos;
}

/********************************************************************************************************
* ������s32 VSlabBitMapGetFreeBlocks(u8 *bitmap, s32 nbits);
* ����: ��ȡÿҳ�п��п����
* ����:
* [1] bitmap: ҳ�����е�λͼ����
* [2] nbits: λ��(bits)��ע�ⲻ���������
* ���أ����п����
* ע�⣺��
*********************************************************************************************************/
s32 VSlabBitMapGetFreeBlocks(u8 *bitmap, s32 nbits)
{
	s32 free_cnts = 0;
	void *iter = 0; //��ͷ����,���ҿ��п����
	while (bitmap_iterate(&iter, 0, bitmap, nbits) > 0) {
		free_cnts++;
	}
	return free_cnts;
}

/********************************************************************************************************
* ������s32 VSlabInfoShow(struct StVSlabMgr *pSlabMgr);
* ����: ��ӡslab��������Ϣ,������shell����heap
* ����:
* [1] pSlabMgr: slab����ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
s32 VSlabInfohow(struct StVSlabMgr *pSlabMgr)
{
	s32 i;
	struct list_head *list;
	struct StVSlabClass *pClass = 0;
	struct StVSlabPage *pPage = 0;

	VSLAB_LOCK();
	kprintf(" slab ��������Ϣ��\r\n");
	kprintf(" slab ����: \"%s\", slab ͷ�ܴ�С: %d, ҳ�������ܴ�С: %d\r\n",
			pSlabMgr->name, pSlabMgr->slab_header_size, pSlabMgr->page_header_size);
	kprintf(" �����ֽ���: %d, ÿҳ��С: %d, �����ֽ�: %d, ���໮�ִ�С: %d\r\n",
			pSlabMgr->align_bytes, pSlabMgr->page_size,
			pSlabMgr->step_size, pSlabMgr->class_max);

	kprintf(" ҳ��������Ϣ��\r\n");
	for (i = 0; i<pSlabMgr->class_max; i++) {
		pClass = &pSlabMgr->class_base[i];
		if (!list_empty(&pClass->page_full)||
			!list_empty(&pClass->page_partial) ||
			!list_empty(&pClass->page_free)) {

			kprintf(" <������: %d>\r\n", i);
			if (!list_empty(&pClass->page_full)) {
				kprintf("   == ȫ��ҳ��: ���С<%d>, ҳ��<%04d>\r\n", (i+1)*pSlabMgr->step_size, pClass->page_full_num);
				list_for_each(list, &pClass->page_full) {
					pPage = list_entry(list, struct StVSlabPage, list);
					kprintf("   [ҳͷ��Ϣ: ħ��-0x%08x, ҳ��ַ-0x%08x, λͼ:λ��-<%04d>,���п���-<%04d>]\r\n",
						pPage->magic, pPage->block_base, pPage->block_max,
						VSlabBitMapGetFreeBlocks(pPage->bitmap, pPage->block_max));//pPage->block_max����λͼ��Ч��λ��
				}
			}
			if (!list_empty(&pClass->page_partial)) {
				kprintf("   == ����ҳ��: ���С<%d>, ҳ��<%04d>\r\n", (i + 1)*pSlabMgr->step_size, pClass->page_partial_num);
				list_for_each(list, &pClass->page_partial) {
					pPage = list_entry(list, struct StVSlabPage, list);
					kprintf("   [ҳͷ��Ϣ: ħ��-0x%08x, ҳ��ַ-0x%08x, λͼ:λ��-<%04d>,���п���-<%04d>]\r\n",
						pPage->magic, pPage->block_base, pPage->block_max,
						VSlabBitMapGetFreeBlocks(pPage->bitmap, pPage->block_max));//pPage->block_max����λͼ��Ч��λ��
				}
			}
			if (!list_empty(&pClass->page_free)) {
				kprintf("   == ȫ��ҳ��: ���С<%d>, ҳ��<%04d>\r\n", (i + 1)*pSlabMgr->step_size, pClass->page_free_num);
				list_for_each(list, &pClass->page_free) {
					pPage = list_entry(list, struct StVSlabPage, list);
					kprintf("   [ҳͷ��Ϣ: ħ��-0x%08x, ҳ��ַ-0x%08x, λͼ:λ��-<%04d>,���п���-<%04d>]\r\n",
						pPage->magic, pPage->block_base, pPage->block_max,
						VSlabBitMapGetFreeBlocks(pPage->bitmap, pPage->block_max));//pPage->block_max����λͼ��Ч��λ��
				}
			}
		}
	}
	VSLAB_UNLOCK();
	return 0;
}

/********************************************************************************************************
* ������s32 VSlabInfoDump(struct StVSlabMgr *pSlabMgr);
* ����: ��ӡslab��������Ϣ
* ����:
* [1] pSlabMgr: slab����ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
s32 VSlabInfoDump(struct StVSlabMgr *pSlabMgr)
{
	kprintf("###################################\r\n");
	VSlabInfohow(pSlabMgr);
	kprintf("------    end   ------\r\n");
	return 0;
}

#define VSLAB_ALIGN_SIZE	8
#define VSLAB_STEP_SIZE		8
#define VSLAB_PAGE_SIZE		1024

s32 VSlabBoudaryCheck(struct StVSlabMgr *pSlabMgr)
{
	int ret = 0;
	s32 i, j;
	s32 offset = 0;
	struct list_head *list;
	struct StVSlabClass *pClass = 0;
	struct StVSlabPage *pPage = 0;
	s32 counter_full = 0;
	s32 counter_free = 0;
	s32 counter_partial = 0;
	void *iter = 0; 
	VSLAB_LOCK();
	//�ж�pSlabMgr�ڲ�����Ԫ���Ƿ�����
	if (pSlabMgr->align_bytes != VSLAB_ALIGN_SIZE) BOUNDARY_ERROR();
	if (pSlabMgr->page_size != VSLAB_PAGE_SIZE) BOUNDARY_ERROR();
	if (pSlabMgr->step_size != VSLAB_STEP_SIZE) BOUNDARY_ERROR();
	if (pSlabMgr->slab_header_size != pSlabMgr->class_max * sizeof(struct StVSlabClass) + sizeof(struct StVSlabMgr)) BOUNDARY_ERROR();
	if (pSlabMgr->page_header_size != VSlabPageHeaderGetSize(VSLAB_PAGE_SIZE, VSLAB_STEP_SIZE, VSLAB_ALIGN_SIZE)) BOUNDARY_ERROR();
	
	//���ҳ������
	for (i = 0; i<pSlabMgr->class_max; i++) {
		pClass = &pSlabMgr->class_base[i];
		if (!list_empty(&pClass->page_full) ||
			!list_empty(&pClass->page_partial) ||
			!list_empty(&pClass->page_free)) {
			
			counter_full = 0;
			if (!list_empty(&pClass->page_full)) {
				list_for_each(list, &pClass->page_full) {
					pPage = list_entry(list, struct StVSlabPage, list);
					if (pPage->magic != VSLAB_MAGIC_NUM) BOUNDARY_ERROR();
					if (pPage->block_size != (i + 1)*pSlabMgr->step_size) BOUNDARY_ERROR();
					if (pPage->status != SLAB_BITMAP_FULL) BOUNDARY_ERROR();
					//���λͼ����Ŀռ�϶�Ϊ0
					for (j = (pPage->block_max+8-1)/8;  j < pPage->bitmap_max; j++) {
						if (pPage->bitmap[j] != 0) BOUNDARY_ERROR();
					}
					//���λͼ����ҳӦ�ö���λ
					iter = 0; //��ͷ����,���ҿ��п飬����-1������ȷ��
					while (bitmap_iterate(&iter, 0, pPage->bitmap, pPage->block_max) >= 0) {
						BOUNDARY_ERROR();
					}
					counter_full++;
				}
			}
			if (pClass->page_full_num != counter_full) BOUNDARY_ERROR();

			counter_partial = 0;
			if (!list_empty(&pClass->page_partial)) {
				list_for_each(list, &pClass->page_partial) {
					pPage = list_entry(list, struct StVSlabPage, list);
					if (pPage->magic != VSLAB_MAGIC_NUM) BOUNDARY_ERROR();
					if (pPage->block_size != (i + 1)*pSlabMgr->step_size) BOUNDARY_ERROR();
					if (pPage->status != SLAB_BITMAP_PARTIAL) BOUNDARY_ERROR();
					//���λͼ����Ŀռ�϶�Ϊ0
					for (j = (pPage->block_max + 8 - 1) / 8; j < pPage->bitmap_max; j++) {
						if (pPage->bitmap[j] != 0) BOUNDARY_ERROR();
					}
					//���λͼ����ҳӦ�ö���λ
					iter = 0; //��ͷ����,���ҿ��п飬Ӧ�ö���
					if (bitmap_iterate(&iter, 0, pPage->bitmap, pPage->block_max) < 0)  BOUNDARY_ERROR();
					iter = 0; //��ͷ����,���ҷ���飬Ӧ�ö���
					if (bitmap_iterate(&iter, 1, pPage->bitmap, pPage->block_max) < 0)  BOUNDARY_ERROR();
					counter_partial++;
				}
			}
			if (pClass->page_partial_num != counter_partial) BOUNDARY_ERROR();

			counter_free = 0;
			if (!list_empty(&pClass->page_free)) {
				list_for_each(list, &pClass->page_free) {
					pPage = list_entry(list, struct StVSlabPage, list);
					if (pPage->magic != VSLAB_MAGIC_NUM) BOUNDARY_ERROR();
					if (pPage->block_size != (i + 1)*pSlabMgr->step_size) BOUNDARY_ERROR();
					if (pPage->status != SLAB_BITMAP_FREE) BOUNDARY_ERROR();
					//���λͼ����Ŀռ�϶�Ϊ0
					for (j = (pPage->block_max + 8 - 1) / 8; j < pPage->bitmap_max; j++) {
						if (pPage->bitmap[j] != 0) BOUNDARY_ERROR();
					}
					//���λͼ����ҳӦ�ö���λ
					iter = 0; //��ͷ����,���ҷ���飬����-1������ȷ��
					while (bitmap_iterate(&iter, 1, pPage->bitmap, pPage->block_max) >= 0) {
						BOUNDARY_ERROR();
					}
					counter_free++;
				}
			}
			if (pClass->page_free_num > VSLAB_FREE_PAGES_THREHOLD) BOUNDARY_ERROR();
		}
	}
	VSLAB_UNLOCK();
	return 0;

ERROR_RET:
	VSLAB_UNLOCK();
	kprintf("*************\r\nERROR: %s, please check the code!!!\r\n*************\r\n", __FUNCTION__);
	while (1);
	return 0;
}
