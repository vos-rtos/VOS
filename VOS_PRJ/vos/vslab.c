/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vslab.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 内	    容: slab主要的意义是处理小内存分配，释放策略会延迟，对频繁分配意义比较大，还有内存浪费有好处
* 历    史：
* --20200925：创建文件, 实现slab内存池分配算法，这里如果申请的内存大于obj_size_max大小，直接从buddy分配
*********************************************************************************************************/
#include "vtype.h"
#include "vos.h"
#include "vlist.h"
#include "vslab.h"
#include "vheap.h"

#define VSLAB_MAGIC_NUM  	0x05152535

#define SLAB_PAGE_SIZE		(1024) //buddy分配算法中的1页占用的尺寸
#define SLAB_STEP_SIZE  	(8)    //线性递增的步长，就是n*8
#define SLAB_CLASS_NUMS  	((SLAB_PAGE_SIZE+SLAB_STEP_SIZE-1)/SLAB_STEP_SIZE)  //对象类的总数

#define SLAB_ALIGN_BYTES 	(8)		//8字节边界对齐
#define SLAB_BITMAP_MAX  	((SLAB_PAGE_SIZE+SLAB_ALIGN_BYTES-1)/SLAB_ALIGN_BYTES)  //对象类的总数

#define SLAB_NUM(arr) (sizeof(arr)/sizeof(arr[0]))


typedef struct StVSlabClass {
	struct list_head page_full;		//页中所有block已经分配出去的页链接到这个满链表
	struct list_head page_partial;	//页中部分block分配出去的页链接到这个部分链表
	struct list_head page_free;		//页中所有block都空闲的页链接到这个空闲链表
	u32    page_free_num;			//统计当前空闲链表的页数
	u32    page_full_num;			//统计当前满页链表的页数
	u32    page_partial_num;		//统计当前部分页链表的页数
}StVSlabClass;

typedef struct StVSlabMgr {
	s32 align_size;
	s32 page_size; //buddy算法分配1个页的大小
	s32 block_size_max; //计算得出最大的对象分配尺寸，否则直接buddy算法分配
	struct StVSlabClass slab_class[SLAB_CLASS_NUMS/2]; //总分配对象类型除以2，保证可以分配2个以上，否则直接buddy分配
}StVSlabMgr;


//每页中最后存放对象块管理区，不放到头部因为这样可以减少对齐的依赖
typedef struct StVSlabPage {
	u32 magic;
	struct list_head list;
	u8 *block_base; //指向block数组起始地址，就是buddy分配算法首地址
	s32 block_size; //每个对象块占用的bytes
	u32 block_cnts; //当前分配出去的块数
	s32 block_max; 	//每页可以存储对象块的最大的个数
	u8 bitmap[(SLAB_BITMAP_MAX+8-1)/8]; //存储对象块位图，不使用空闲链表是因为怕写破坏链表问题
									   //这里的数组长度是最大的长度，就是说如果每页1024字节，
									   //如果按每个对象尺寸是8字节,  那么1024/8=128的数组大小，
									   //这里位图每一位代表一个对象，所以还要除以8
}StVSlabPage;

//表格第一个是0的偏移位置
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
* 函数：s32 FirstFreeBlockGet(u8 *bitmap, s32 num, s32 offset);
* 描述: 从bitmap的offset偏移值作为起始位置查找第一个空闲块的偏移位置
* 参数:
* [1] bitmap: 位图数组
* [2] num: 位图数组大小，单位：字节
* [3] offset: 从offset作为起始偏移值开始查找
* 返回：成功返回偏移位置，否则返回-1
* 注意：因为bitmap数组通常都是有多余尾巴（不是刚好）的，最后都基本是0，所以如果全满，获取到最后一个偏移值应该
* 就是尾巴里的值，可以通过判断返回值超过表达的最大偏移值来判断
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
* 函数：struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size, s32 align_bytes);
* 描述:  vslab内存池分配器创建
* 参数:
* [1] mem: slab的管理区地址
* [2] len: slab的管理区地址，单位：字节
* [3] page_size: 页尺寸，要配合buddy分配算法中页的大小来分配
* [4] align_bytes: 字节对齐，例如8，就是8字节对齐
* 返回：slab管理对象指针
* 注意：无
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
	//这里还要考虑字节对齐问题
	left = left/align_bytes*align_bytes;
	//必须保证一个页大小可以存放2个对象大小，这样才有意义，至少减少内存浪费，否则buddy分配
	//通过page_size可以计算最大对象分配尺寸。
	pSlabMgr->block_size_max = left / 2;

	for (i=0; i<SLAB_NUM(pSlabMgr->slab_class); i++) {
		INIT_LIST_HEAD(&pSlabMgr->slab_class[i].page_full);
		INIT_LIST_HEAD(&pSlabMgr->slab_class[i].page_partial);
		INIT_LIST_HEAD(&pSlabMgr->slab_class[i].page_free);
	}

	return pSlabMgr;
}

/********************************************************************************************************
* 函数：struct StVSlabPage *VSlabPageBuild(u8 *mem, s32 len, s32 block_size, s32 page_size, s32 align_bytes);
* 描述: 这个函数是最新vmalloc的页里创建管理blocks的管理区域
* 参数:
* [1] mem: buddy系统vmalloc出来的页基地址
* [2] len: 页大小，单位：字节
* [3] block_size: 这个是slab_class[n]对应的那一类块大小，例如：slab_class[6]， 基于8倍数递增，则就是56字节
* [4] align_bytes: 字节对齐，例如8，就是8字节对齐
* 返回：页管理对象指针
* 注意：无
*********************************************************************************************************/
struct StVSlabPage *VSlabPageBuild(u8 *mem, s32 len, s32 block_size, s32 page_size, s32 align_bytes)
{
	s32 nBlocks = 0;
	s32 block_bytes = 0;
	struct StVSlabPage *pSlabPage = 0;

	if (mem == 0 || len < sizeof(struct StVSlabPage)) {
		return 0;
	}
	//页面最高地址存放页管理区信息
	pSlabPage = (struct StVSlabPage*)(mem+len-sizeof(struct StVSlabPage));
	//block_bytes为所有blocks数据区的总大小
	block_bytes = (u8*)pSlabPage - mem;
	//这里还要考虑字节对齐问题
	block_bytes = block_bytes/align_bytes*align_bytes;
	//计算这个nBlocks，是为了位图数组常是有多余位，需要加入这个block_max来判断是否满页
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
* 函数：void *SlabBitMapSet(struct StVSlabPage *page, s32 *status);
* 描述: 这个函数先找到第一个空闲blocks位置后返回该blocks地址，再设置该位后，然后查找是否位图是否满，
*       如果bitmap所有有效位置1，则返回状态是满页，否则返回部分页
* 参数:
* [1] page: 页管理对象
* [2] status: 返回状态，只有2种状态，部分页状态（SLAB_BITMAP_PARTIAL）和满页状态（SLAB_BITMAP_FULL）
* 返回：如果成功，返回分配的block的具体地址，
* 注意：无
*********************************************************************************************************/
void *SlabBitMapSet(struct StVSlabPage *page, s32 *status)
{
	void *pnew = 0;
	s32 offset = FirstFreeBlockGet(page->bitmap, sizeof(page->bitmap), 0);
	if (offset < 0 || offset > page->block_max) {//超过最大的偏移
		*status = SLAB_BITMAP_FULL;
		pnew = 0;
	}
	else {
		*status = SLAB_BITMAP_PARTIAL;
		//从0偏移开始找第一个为0的空闲块
		bitmap_set(offset, page->bitmap);
		pnew = &page->block_base[offset * page->block_size];
		//设置该位， 从offset查看，是否最后都是0xFF，表示没有任何空闲块
		offset = FirstFreeBlockGet(page->bitmap, sizeof(page->bitmap), offset);
		//如果返回-1，证明全部是已经分配的blocks, 还有情况位图是最大值，
		//除刚好外，多余都是0，这样就得判断offset 是否大于等于page->block_max来决定。
		if (offset < 0 || offset >= page->block_max) { //
			*status = SLAB_BITMAP_FULL;
		}
	}
	return pnew;
}

/********************************************************************************************************
* 函数：void *VSlabBlockAlloc(struct StVSlabMgr *pSlabMgr, s32 size);
* 描述: 从slab分配器分配指定大小的内存
* 参数:
* [1] pSlabMgr: slab管理对象
* [2] size: 申请指定大小内存
* 返回：如果成功，返回分配的block的具体地址，
* 注意：这里如果size尺寸大于slab分配的最大尺寸，返回0，因为每页必须包含页管理对象+2个或以上的block，
* 为什么2个，因为2个至少可以利用好空间，不然buddy太浪费空间
* 如果只能单个block，就没有意义，直接从buddy分配。
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
	//保证size*2+页管理区 小于等于页总大小，否则直接从buddy分配
	if (index >= SLAB_NUM(pSlabMgr->slab_class)) {
		return 0;
	}
	//计算size对应到哪个slab_class中
	index =  (size + SLAB_STEP_SIZE - 1) / SLAB_STEP_SIZE - 1;
	//找到对应的slab_class后，找到该class的部分链表，因为部分链表如果存在，肯定有至少一个block可分配
	head_partial = &pSlabMgr->slab_class[index].page_partial;
	if (list_empty(head_partial)) { //如果partial为空，那需要从free链表中取出一个放到partial中
		head_free = &pSlabMgr->slab_class[index].page_free;
		if (list_empty(head_free)) {//如果page_free链表为空，就需要从buddy里申请一个页并初始化
			slab_page = VSlabPageBuild(vmalloc(SLAB_PAGE_SIZE), SLAB_PAGE_SIZE,
					(index+1)*SLAB_STEP_SIZE, SLAB_PAGE_SIZE, SLAB_ALIGN_BYTES);
			if (slab_page == 0) {
				goto END_SLAB_BLOCK_ALLOC;
			}
			//插入到partial链表
			list_add(&slab_page->list, head_partial);
			pSlabMgr->slab_class[index].page_partial_num++;
		}
		else { //获取第一个page
			slab_page = list_entry(head_free->next, struct StVSlabPage, list);
			pSlabMgr->slab_class[index].page_free_num--;
		}
		//查找位图，找到第一个空闲块并置位
		pnew = SlabBitMapSet(slab_page, &status);
		if (status == SLAB_BITMAP_FULL){ //置位后满，把页移到page_full链表
			//先从page_partial链表删除
			list_del(&slab_page->list);
			//再添加到page_full
			head_full = &pSlabMgr->slab_class[index].page_full;
			list_add(&slab_page->list, head_full);
			pSlabMgr->slab_class[index].page_full_num++;
		}
	}

END_SLAB_BLOCK_ALLOC:
	return pnew;

}

/********************************************************************************************************
* 函数：void SlabBitMapClr(struct StVSlabPage *page, s32 *status, s32 offset);
* 描述: 这个函数用于释放内存函数，清除位图某个位，然后查询是否全部块空闲，如果是就是返回空闲状态，否则返回部分状态
* 参数:
* [1] page: 页管理对象
* [2] status: 返回状态，只有2种状态，部分页状态（SLAB_BITMAP_PARTIAL）和空闲页状态（SLAB_BITMAP_FREE）
* [3] offset: 要清除的偏移位置
* 返回：无
* 注意：重复释放, 返回SLAB_BITMAP_PARTIAL状态
*********************************************************************************************************/
void SlabBitMapClr(struct StVSlabPage *page, s32 *status, s32 offset)
{
	s32 i = 0;
	if (!bitmap_get(offset, page->bitmap)) { //重复释放
		*status = SLAB_BITMAP_PARTIAL; //重复释放, 返回部分页状态
		return;
	}
	if (offset >= page->block_max) {
		kprintf("error: %d offset >= page->block_max!\r\n", __FUNCTION__);
	}
	bitmap_clr(offset, page->bitmap);

	//查找是否全0，如果全0，就要返回SLAB_BITMAP_FEE状态
	for (i=0; i<SLAB_NUM(page->bitmap); i++)
	{
		if (page->bitmap[i] != 0) {
			break;
		}
	}
	if (i==SLAB_NUM(page->bitmap)) { //如果bitmap全0，返回空闲页标志
		*status = SLAB_BITMAP_FREE;
	}
}

/********************************************************************************************************
* 函数：void VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr);
* 描述: 释放某个block块
* 参数:
* [1] pSlabMgr: slab管理对象
* [2] ptr: 释放块的指针
* 返回：无
* 注意：无
*********************************************************************************************************/
void VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr)
{
	s32 index = 0;
	u8 *pPageBase = 0;
	s32 status = 0;
	s32 offset = 0;
	struct list_head *head_free = 0;
	struct StVSlabPage *slab_page = 0;
	//查看释放的地址所在的页基址，通过buddy分配器上查找
	pPageBase = (u8*)VHeapMgrGetPageBaseAddr(ptr, 0, 0);
	if (pPageBase == 0) {
		kprintf("error: %s, pPageBase==NULL!\r\n", __FUNCTION__);
		return;
	}
	//根据页基址，找到页管理区，就在页面的最高地址上
	slab_page = (struct StVSlabPage *)(pPageBase + SLAB_PAGE_SIZE - sizeof(struct StVSlabPage));
	//校验magic数
	if (slab_page->magic != VSLAB_MAGIC_NUM) {
		kprintf("error: %s, slab_page->magic match failed!\r\n", __FUNCTION__);
		return;
	}
	//根据页管理区里的block_size块尺寸，计算slab_class索引号地址
	index =  (slab_page->block_size + SLAB_STEP_SIZE - 1) / SLAB_STEP_SIZE - 1;
	if (index >= SLAB_NUM(pSlabMgr->slab_class)) {
		return ;
	}
	//校验释放的对象地址是否是对象的首地址，如果是对象身体的地址，释放失败，这样避免地址乱释放
	if(((u8*)ptr - pPageBase)%slab_page->block_size) {
		return ;
	}
	//计算释放的指针对应页基址的偏移位置
	offset = ((u8*)ptr - pPageBase)/slab_page->block_size;
	//清除页管理区中位图响应的偏移位，这样就释放了某个block, 注意还返回状态，是清除后的状态
	SlabBitMapClr(slab_page, &status, offset);
	//如果刚清除了那位后，发现全部都是空闲block，将从partial链移到free链中
	if (status == SLAB_BITMAP_FREE) {//如果是全空位图，要添加到page_free链表中
		//从head_partial断开链表
		list_del(&slab_page->list);
		//插入到free链表
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



