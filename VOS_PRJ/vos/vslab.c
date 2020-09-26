/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vslab.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��	    ��: slab��Ҫ�������Ǵ���С�ڴ���䣬�ͷŲ��Ի��ӳ٣���Ƶ����������Ƚϴ󣬻����ڴ��˷��кô�
* ��    ʷ��
* --20200925�������ļ�, ʵ��slab�ڴ�ط����㷨���������������ڴ����obj_size_max��С��ֱ�Ӵ�buddy����
*********************************************************************************************************/
#include "vtype.h"
#include "vos.h"
#include "vlist.h"
#include "vslab.h"
#include "vheap.h"

#define VSLAB_MAGIC_NUM  	0x05152535

#define SLAB_PAGE_SIZE		(1024) //buddy�����㷨�е�1ҳռ�õĳߴ�
#define SLAB_STEP_SIZE  	(8)    //���Ե����Ĳ���������n*8
#define SLAB_CLASS_NUMS  	((SLAB_PAGE_SIZE+SLAB_STEP_SIZE-1)/SLAB_STEP_SIZE)  //�����������

#define SLAB_ALIGN_BYTES 	(8)		//8�ֽڱ߽����
#define SLAB_BITMAP_MAX  	((SLAB_PAGE_SIZE+SLAB_ALIGN_BYTES-1)/SLAB_ALIGN_BYTES)  //�����������

#define SLAB_NUM(arr) (sizeof(arr)/sizeof(arr[0]))


typedef struct StVSlabClass {
	struct list_head page_full;		//ҳ������block�Ѿ������ȥ��ҳ���ӵ����������
	struct list_head page_partial;	//ҳ�в���block�����ȥ��ҳ���ӵ������������
	struct list_head page_free;		//ҳ������block�����е�ҳ���ӵ������������
	u32    page_free_num;			//ͳ�Ƶ�ǰ���������ҳ��
	u32    page_full_num;			//ͳ�Ƶ�ǰ��ҳ�����ҳ��
	u32    page_partial_num;		//ͳ�Ƶ�ǰ����ҳ�����ҳ��
}StVSlabClass;

typedef struct StVSlabMgr {
	s32 align_size;
	s32 page_size; //buddy�㷨����1��ҳ�Ĵ�С
	s32 block_size_max; //����ó����Ķ������ߴ磬����ֱ��buddy�㷨����
	struct StVSlabClass slab_class[SLAB_CLASS_NUMS/2]; //�ܷ���������ͳ���2����֤���Է���2�����ϣ�����ֱ��buddy����
}StVSlabMgr;


//ÿҳ������Ŷ��������������ŵ�ͷ����Ϊ�������Լ��ٶ��������
typedef struct StVSlabPage {
	u32 magic;
	struct list_head list;
	u8 *block_base; //ָ��block������ʼ��ַ������buddy�����㷨�׵�ַ
	s32 block_size; //ÿ�������ռ�õ�bytes
	u32 block_cnts; //��ǰ�����ȥ�Ŀ���
	s32 block_max; 	//ÿҳ���Դ洢���������ĸ���
	u8 bitmap[(SLAB_BITMAP_MAX+8-1)/8]; //�洢�����λͼ����ʹ�ÿ�����������Ϊ��д�ƻ���������
									   //��������鳤�������ĳ��ȣ�����˵���ÿҳ1024�ֽڣ�
									   //�����ÿ������ߴ���8�ֽ�,  ��ô1024/8=128�������С��
									   //����λͼÿһλ����һ���������Ի�Ҫ����8
}StVSlabPage;

//����һ����0��ƫ��λ��
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
* ������struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes);
* ����:  vslab�ڴ�ط���������
* ����:
* [1] mem: slab�Ĺ�������ַ
* [2] len: slab�Ĺ�������ַ����λ���ֽ�
* [3] page_size: ҳ�ߴ磬Ҫ���buddy�����㷨��ҳ�Ĵ�С������
* [4] align_bytes: �ֽڶ��룬����8������8�ֽڶ���
* ���أ�slab�������ָ��
* ע�⣺��
*********************************************************************************************************/
struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes)
{
	struct StVSlabMgr *pSlabMgr = 0;
	s32 i = 0;
	s32 left = 0;
	if (len < sizeof(struct StVSlabMgr)) return 0;


	pSlabMgr = (struct StVSlabMgr*)mem;

	pSlabMgr->page_size = page_size;
	pSlabMgr->align_size = align_bytes;

	left = SLAB_PAGE_SIZE - sizeof(struct StVSlabPage);
	//���ﻹҪ�����ֽڶ�������
	left = left/align_bytes*align_bytes;
	//���뱣֤һ��ҳ��С���Դ��2�������С�������������壬���ټ����ڴ��˷ѣ�����buddy����
	//ͨ��page_size���Լ������������ߴ硣
	pSlabMgr->block_size_max = left / 2;

	for (i=0; i<SLAB_NUM(pSlabMgr->slab_class); i++) {
		INIT_LIST_HEAD(&pSlabMgr->slab_class[i].page_full);
		INIT_LIST_HEAD(&pSlabMgr->slab_class[i].page_partial);
		INIT_LIST_HEAD(&pSlabMgr->slab_class[i].page_free);
	}

	return pSlabMgr;
}

/********************************************************************************************************
* ������struct StVSlabPage *VSlabPageBuild(u8 *mem, s32 len, s32 block_size, s32 page_size, s32 align_bytes);
* ����: �������������vmalloc��ҳ�ﴴ������blocks�Ĺ�������
* ����:
* [1] mem: buddyϵͳvmalloc������ҳ����ַ
* [2] len: ҳ��С����λ���ֽ�
* [3] block_size: �����slab_class[n]��Ӧ����һ����С�����磺slab_class[6]�� ����8���������������56�ֽ�
* [4] align_bytes: �ֽڶ��룬����8������8�ֽڶ���
* ���أ�ҳ�������ָ��
* ע�⣺��
*********************************************************************************************************/
struct StVSlabPage *VSlabPageBuild(u8 *mem, s32 len, s32 block_size, s32 page_size, s32 align_bytes)
{
	s32 nBlocks = 0;
	s32 block_bytes = 0;
	struct StVSlabPage *pSlabPage = 0;

	if (mem == 0 || len < sizeof(struct StVSlabPage)) {
		return 0;
	}
	//ҳ����ߵ�ַ���ҳ��������Ϣ
	pSlabPage = (struct StVSlabPage*)(mem+len-sizeof(struct StVSlabPage));
	//block_bytesΪ����blocks���������ܴ�С
	block_bytes = (u8*)pSlabPage - mem;
	//���ﻹҪ�����ֽڶ�������
	block_bytes = block_bytes/align_bytes*align_bytes;
	//�������nBlocks����Ϊ��λͼ���鳣���ж���λ����Ҫ�������block_max���ж��Ƿ���ҳ
	nBlocks = block_bytes / block_size;

	memset(pSlabPage, 0, sizeof(struct StVSlabPage));

	pSlabPage->magic = VSLAB_MAGIC_NUM;
	pSlabPage->block_cnts = 0;
	pSlabPage->block_size = block_size;
	pSlabPage->block_max = nBlocks;
	pSlabPage->block_base = mem;

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
	s32 offset = FirstFreeBlockGet(page->bitmap, sizeof(page->bitmap), 0);
	if (offset < 0 || offset > page->block_max) {//��������ƫ��
		*status = SLAB_BITMAP_FULL;
		pnew = 0;
	}
	else {
		*status = SLAB_BITMAP_PARTIAL;
		//��0ƫ�ƿ�ʼ�ҵ�һ��Ϊ0�Ŀ��п�
		bitmap_set(offset, page->bitmap);
		pnew = &page->block_base[offset * page->block_size];
		//���ø�λ�� ��offset�鿴���Ƿ������0xFF����ʾû���κο��п�
		offset = FirstFreeBlockGet(page->bitmap, sizeof(page->bitmap), offset);
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
	//��֤size*2+ҳ������ С�ڵ���ҳ�ܴ�С������ֱ�Ӵ�buddy����
	if (index >= SLAB_NUM(pSlabMgr->slab_class)) {
		return 0;
	}
	//����size��Ӧ���ĸ�slab_class��
	index =  (size + SLAB_STEP_SIZE - 1) / SLAB_STEP_SIZE - 1;
	//�ҵ���Ӧ��slab_class���ҵ���class�Ĳ���������Ϊ��������������ڣ��϶�������һ��block�ɷ���
	head_partial = &pSlabMgr->slab_class[index].page_partial;
	if (list_empty(head_partial)) { //���partialΪ�գ�����Ҫ��free������ȡ��һ���ŵ�partial��
		head_free = &pSlabMgr->slab_class[index].page_free;
		if (list_empty(head_free)) {//���page_free����Ϊ�գ�����Ҫ��buddy������һ��ҳ����ʼ��
			slab_page = VSlabPageBuild(vmalloc(SLAB_PAGE_SIZE), SLAB_PAGE_SIZE,
					(index+1)*SLAB_STEP_SIZE, SLAB_PAGE_SIZE, SLAB_ALIGN_BYTES);
			if (slab_page == 0) {
				goto END_SLAB_BLOCK_ALLOC;
			}
			//���뵽partial����
			list_add(&slab_page->list, head_partial);
			pSlabMgr->slab_class[index].page_partial_num++;
		}
		else { //��ȡ��һ��page
			slab_page = list_entry(head_free->next, struct StVSlabPage, list);
			pSlabMgr->slab_class[index].page_free_num--;
		}
		//����λͼ���ҵ���һ�����п鲢��λ
		pnew = SlabBitMapSet(slab_page, &status);
		if (status == SLAB_BITMAP_FULL){ //��λ��������ҳ�Ƶ�page_full����
			//�ȴ�page_partial����ɾ��
			list_del(&slab_page->list);
			//����ӵ�page_full
			head_full = &pSlabMgr->slab_class[index].page_full;
			list_add(&slab_page->list, head_full);
			pSlabMgr->slab_class[index].page_full_num++;
		}
	}

END_SLAB_BLOCK_ALLOC:
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
	}
	bitmap_clr(offset, page->bitmap);

	//�����Ƿ�ȫ0�����ȫ0����Ҫ����SLAB_BITMAP_FEE״̬
	for (i=0; i<SLAB_NUM(page->bitmap); i++)
	{
		if (page->bitmap[i] != 0) {
			break;
		}
	}
	if (i==SLAB_NUM(page->bitmap)) { //���bitmapȫ0�����ؿ���ҳ��־
		*status = SLAB_BITMAP_FREE;
	}
}

/********************************************************************************************************
* ������void VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr);
* ����: �ͷ�ĳ��block��
* ����:
* [1] pSlabMgr: slab�������
* [2] ptr: �ͷſ��ָ��
* ���أ���
* ע�⣺��
*********************************************************************************************************/
void VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr)
{
	s32 index = 0;
	u8 *pPageBase = 0;
	s32 status = 0;
	s32 offset = 0;
	struct list_head *head_free = 0;
	struct StVSlabPage *slab_page = 0;
	//�鿴�ͷŵĵ�ַ���ڵ�ҳ��ַ��ͨ��buddy�������ϲ���
	pPageBase = (u8*)VHeapMgrGetPageBaseAddr(ptr, 0, 0);
	if (pPageBase == 0) {
		kprintf("error: %s, pPageBase==NULL!\r\n", __FUNCTION__);
		return;
	}
	//����ҳ��ַ���ҵ�ҳ������������ҳ�����ߵ�ַ��
	slab_page = (struct StVSlabPage *)(pPageBase + SLAB_PAGE_SIZE - sizeof(struct StVSlabPage));
	//У��magic��
	if (slab_page->magic != VSLAB_MAGIC_NUM) {
		kprintf("error: %s, slab_page->magic match failed!\r\n", __FUNCTION__);
		return;
	}
	//����ҳ���������block_size��ߴ磬����slab_class�����ŵ�ַ
	index =  (slab_page->block_size + SLAB_STEP_SIZE - 1) / SLAB_STEP_SIZE - 1;
	if (index >= SLAB_NUM(pSlabMgr->slab_class)) {
		return ;
	}
	//У���ͷŵĶ����ַ�Ƿ��Ƕ�����׵�ַ������Ƕ�������ĵ�ַ���ͷ�ʧ�ܣ����������ַ���ͷ�
	if(((u8*)ptr - pPageBase)%slab_page->block_size) {
		return ;
	}
	//�����ͷŵ�ָ���Ӧҳ��ַ��ƫ��λ��
	offset = ((u8*)ptr - pPageBase)/slab_page->block_size;
	//���ҳ��������λͼ��Ӧ��ƫ��λ���������ͷ���ĳ��block, ע�⻹����״̬����������״̬
	SlabBitMapClr(slab_page, &status, offset);
	//������������λ�󣬷���ȫ�����ǿ���block������partial���Ƶ�free����
	if (status == SLAB_BITMAP_FREE) {//�����ȫ��λͼ��Ҫ��ӵ�page_free������
		//��head_partial�Ͽ�����
		list_del(&slab_page->list);
		//���뵽free����
		head_free = &pSlabMgr->slab_class[index].page_free;
		list_add(&slab_page->list, head_free);
		pSlabMgr->slab_class[index].page_free_num++;
	}
}

void VSlabTest()
{
	u8 *pnew = 0;
	u8 *pmem = (u8*)vmalloc(1024*3);
	struct StVSlabMgr *pslab = VSlabBuild(pmem, 1024*3, SLAB_PAGE_SIZE, SLAB_ALIGN_BYTES);
	pnew = VSlabBlockAlloc(pslab, 50);
	VSlabBlockFree(pslab, pnew);
	VSlabBlockFree(pslab, pnew);
}



