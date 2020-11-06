/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vslab.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 内	容: slab主要的意义是处理小内存分配，释放策略会延迟，对频繁分配意义比较大，还有内存浪费有好处
* 注	意: 一个堆对应一个slab, 如果多个堆的的page_size和align_size相同，也可以多个堆
* 历    史：
* --20200925：创建文件, 实现slab内存池分配算法，这里如果申请的内存大于某个尺寸，直接从buddy分配

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

//step_size也要对齐分配
#define ALIGN_SIZE(step, align) (((step)+ (align)-1)/(align) * (align))


typedef void* (*BUDDY_MALLOC_FUN)(unsigned int size);

typedef void (*BUDDY_FREE_FUN)(void *ptr);

typedef struct StVSlabClass {
	struct list_head page_full;		//页中所有block已经分配出去的页链接到这个满链表
	struct list_head page_partial;	//页中部分block分配出去的页链接到这个部分链表
	struct list_head page_free;		//页中所有block都空闲的页链接到这个空闲链表
	u32    page_free_num;			//统计当前空闲链表的页数
	u32    page_full_num;			//统计当前满页链表的页数
	u32    page_partial_num;		//统计当前部分页链表的页数
}StVSlabClass;

//通常一个堆名字对应一个slab分配器，如果不同堆，但是页尺寸是相同的，页可以共同使用
typedef struct StVSlabMgr {
	s8 *name;					//slab管理器名字
	struct StVMemHeap *pheap;	//指向某个固定的堆，目前只允许一个堆指定一个分配器
	s32 irq_save;			//中断状态存储
	s32 page_header_size;	//每页管理区的大小（包括bitmap数组大小），单位字节，创建时可以确定
	s32 slab_header_size;	//StVSlabMgr自身的大小（包括class_base数组大小），单位字节，创建时可以确定
	s32 align_bytes;		//多少字节对齐
	s32 page_size;			//buddy算法分配1个页的大小
	s32 step_size;			//线性增长的步长，例如8，n*8
	s32 class_max;			//class_base 数组的长度
	struct StVSlabClass class_base[0]; //总分配对象类型除以2，保证可以分配2个以上，否则直接buddy分配
}StVSlabMgr;


//每页中最后存放对象块管理区，不放到头部因为这样可以减少对齐的依赖
typedef struct StVSlabPage {
	u32 magic;
	struct list_head list;
	s32 status;		//指示在哪个链表: SLAB_BITMAP_FREE, SLAB_BITMAP_PARTIAL, SLAB_BITMAP_FULL
	u8 *block_base; //指向block数组起始地址，就是buddy分配算法首地址
	s32 block_size; //每个对象块占用的bytes
	s32 block_max; 	//每页可以存储对象块的最大的个数
	s32 bitmap_max; //数组大小，为了处理方便，这里总是最大值=(页大小/step_size/8)
	u8	bitmap[0];  //block位图存储数组
}StVSlabPage;

//查表格，找到第一个是0的偏移位置
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
* 函数：s32 VSlabPageHeaderGetSize(s32 page_size, s32 step_size, s32 align_bytes);
* 描述: 计算StVSlabPage和bitmap数组总和的大小
* 参数:
* [1] page_size: slab的管理区地址
* [2] align_bytes: slab的管理区地址，单位：字节
* [3] step_size: 页尺寸，要配合buddy分配算法中页的大小来分配
* 返回：计算StVSlabPage和bitmap数组总和的大小
* 注意：无
*********************************************************************************************************/
s32 VSlabPageHeaderGetSize(s32 page_size, s32 step_size, s32 align_bytes)
{
	//预算存储不同尺寸种类的最大值
	//最大值计算方法：根据每页最大存储空间法则：至少2个数据块+页管理区<=页大小
	//最大值 = (页大小 / MAX(step_size, align_size))

	//step_size对齐操作
	step_size = ALIGN_SIZE(step_size, align_bytes);
	//bitmap_max计算页管理区位图数组大小（最大值）
	s32 bitmap_max = (page_size /step_size + 8 - 1) / 8;
	//真正的页头尺寸，包括位图数组大小
	return sizeof(struct StVSlabPage) + bitmap_max * 1;
}


/********************************************************************************************************
* 函数：s32 VSlabPageCalcBlockMaxSize(s32 page_size, s32 step_size, s32 align_bytes);
* 描述: 计算一页中最大存储block的大小
* 参数:
* [1] page_size: slab的管理区地址
* [2] align_bytes: slab的管理区地址，单位：字节
* [3] step_size: 页尺寸，要配合buddy分配算法中页的大小来分配
* 返回：计算一页中最大存储block的大小
* 注意：无
*********************************************************************************************************/
s32 VSlabPageCalcBlockMaxSize(s32 page_size, s32 step_size, s32 align_bytes)
{
	//step_size对齐操作
	step_size = ALIGN_SIZE(step_size, align_bytes);
	s32 page_head_size = VSlabPageHeaderGetSize(page_size, step_size, align_bytes);
	s32 block_space = (page_size - page_head_size)/align_bytes * align_bytes;
	s32 block_max_size = block_space / 2; //保证2块数据区
	return block_max_size / step_size * step_size; //还要是block_bytes倍数
}

/********************************************************************************************************
* 函数：s32 SlabMgrHeaderGetSize(s32 page_size, s32 align_bytes, s32 step_size);
* 描述: 计算SlabMgr需要的空间大小，包括class_base数组的大小
* 参数:
* [1] page_size: slab的管理区地址
* [2] align_bytes: slab的管理区地址，单位：字节
* [3] step_size: 页尺寸，要配合buddy分配算法中页的大小来分配
* 返回：SlabMgr头和class_base数组总和的大小
* 注意：无
*********************************************************************************************************/
s32 SlabMgrHeaderGetSize(s32 page_size, s32 align_bytes, s32 step_size)
{
	s32 block_max_bytes = 0; //每页中块最大尺寸值
	//step_size对齐操作
	step_size = ALIGN_SIZE(step_size, align_bytes);
	//计算一页中最大数据块尺寸
	block_max_bytes = VSlabPageCalcBlockMaxSize(page_size, step_size, align_bytes);
	//根据数据块最大尺寸可以计算出class_base类型的种数
	s32 class_max = block_max_bytes / step_size;
	return sizeof(struct StVSlabMgr) + class_max * sizeof(struct StVSlabClass);
}

/********************************************************************************************************
* 函数：struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size, 
*                                                  s32 align_bytes, s32 step_size, s8 *name);
* 描述:  vslab内存池分配器创建
* 参数:
* [1] mem: slab的管理区地址
* [2] len: slab的管理区地址，单位：字节
* [3] page_size: 页尺寸，要配合buddy分配算法中页的大小来分配
* [4] align_bytes: 字节对齐，例如8，就是8字节对齐
* [5] step_size: 线性递增的步长
* [6] name: slab名字
* [7] pheap: 对应buddy系统的堆指针
* 返回：slab管理对象指针
* 注意：无
*********************************************************************************************************/
struct StVSlabMgr *VSlabBuild(u8 *mem, s32 len, s32 page_size,
								s32 align_bytes, s32 step_size, s8 *name, struct StVMemHeap *pheap)
{
	struct StVSlabMgr *pSlabMgr = 0;
	s32 block_max_bytes = 0; //每页中块最大尺寸值
	s32 i = 0;
	s32 slab_header_size = SlabMgrHeaderGetSize(page_size, align_bytes, ALIGN_SIZE(step_size, align_bytes));
	s32 page_header_size = VSlabPageHeaderGetSize(page_size, step_size, align_bytes);
	if (len < slab_header_size) return 0;

	VSLAB_LOCK();

	pSlabMgr = (struct StVSlabMgr*)mem;

	//计算一页中最大数据块尺寸
	block_max_bytes = VSlabPageCalcBlockMaxSize(page_size, step_size, align_bytes);
	//根据数据块最大尺寸可以计算出class_base类型的种数
	pSlabMgr->class_max = block_max_bytes / step_size;

	pSlabMgr->page_size = page_size;
	pSlabMgr->align_bytes = align_bytes;
	pSlabMgr->step_size = ALIGN_SIZE(step_size, align_bytes);
	pSlabMgr->page_header_size = page_header_size;
	pSlabMgr->slab_header_size = slab_header_size;
	pSlabMgr->pheap = pheap;
	pSlabMgr->name = name;

	//初始每类的页链表
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
* 函数：struct StVSlabPage *VSlabPageBuild(u8 *mem, s32 len, s32 block_size, struct StVSlabMgr *pSlabMgr);
* 描述: 这个函数是最新vmalloc的页里创建管理blocks的管理区域
* 参数:
* [1] mem: buddy系统vmalloc出来的页基地址
* [2] len: 页大小，单位：字节
* [3] block_size: 这个是slab_class[n]对应的那一类块大小，例如：slab_class[6]， 基于8倍数递增，则就是56字节
* [4] align_bytes: 字节对齐，例如8，就是8字节对齐
* [5] step_size: 递增步长，单位：字节
* [6] pSlabMgr: slabMgr对象
* 返回：页管理对象指针
* 注意：无
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
	//页面最高地址存放页管理区信息
	pSlabPage = (struct StVSlabPage*)(mem+len- page_header_size);
	//block_space为所有blocks数据区的总大小
	block_space = (u8*)pSlabPage - mem;
	//这里还要考虑字节对齐问题
	block_space = block_space /align_bytes*align_bytes;
	//计算这个nBlocks，是为了位图数组常是有多余位，需要加入这个block_max来判断是否满页
	nBlocks = block_space / block_size;

	memset(pSlabPage, 0, page_header_size);

	pSlabPage->magic = VSLAB_MAGIC_NUM;
	//pSlabPage->block_cnts = 0;
	pSlabPage->block_size = block_size;
	pSlabPage->block_max = nBlocks;
	pSlabPage->block_base = mem;
	//pSlabPage->align_bytes = align_bytes;
	//注意：这里实际不使用最大值，按8字节对齐就可以，这样可以加速访问位图速度
	//但是，计算的空间必须要按最大值计算，这样就可以固定的页管理空间，方便查找。
	pSlabPage->bitmap_max = (nBlocks + 8 - 1) / 8;
	//bitmap_max计算页管理区位图数组大小（最大值）
	//pSlabPage->bitmap_max = (page_size / step_size + 8 - 1) / 8;
	VSLAB_UNLOCK();

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
	s32 offset = FirstFreeBlockGet(page->bitmap, page->bitmap_max, 0);
	if (offset < 0 || offset >= page->block_max) {//超过最大的偏移
		*status = SLAB_BITMAP_FULL;
		pnew = 0;
	}
	else {
		*status = SLAB_BITMAP_PARTIAL;
		//从0偏移开始找第一个为0的空闲块
		bitmap_set(offset, page->bitmap);
		pnew = &page->block_base[offset * page->block_size];
		//设置该位， 从offset查看，是否最后都是0xFF，表示没有任何空闲块
		offset = FirstFreeBlockGet(page->bitmap, page->bitmap_max, offset);
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
	s32 step_size = 0;

	if (pSlabMgr == 0 || size <= 0) return 0;

	step_size = pSlabMgr->step_size;

	//计算size对应到哪个slab_class中
	index = (size + step_size - 1) / step_size - 1;

	//保证size*2+页管理区 小于等于页总大小，否则直接从buddy分配
	if (index >= pSlabMgr->class_max) {
		return 0;
	}
	VSLAB_LOCK();
	//找到对应的class后，找到该class的部分链表，因为部分链表如果存在，肯定有至少一个block可分配
	head_partial = &pSlabMgr->class_base[index].page_partial;
	if (list_empty(head_partial)) { //如果partial为空，那需要从free链表中取出一个放到partial中
		head_free = &pSlabMgr->class_base[index].page_free;
		if (list_empty(head_free)) {//如果page_free链表为空，就需要从buddy里申请一个页并初始化

			slab_page = 0;
			if (pSlabMgr->pheap) { //从堆里申请一页
				VSLAB_UNLOCK();
				u8 *tmp_ptr = (u8*)VMemMalloc(pSlabMgr->pheap, pSlabMgr->page_size, 1);//1为标志slab使用，释放时使用
				VSLAB_LOCK();
				slab_page = VSlabPageBuild(tmp_ptr, pSlabMgr->page_size, (index+1)*pSlabMgr->step_size, pSlabMgr);
			}
			if (slab_page == 0) {
				goto END_SLAB_BLOCK_ALLOC;
			}
		}
		else { //获取第一个page
			slab_page = list_entry(head_free->next, struct StVSlabPage, list);
			pSlabMgr->class_base[index].page_free_num--;
			//从free链表中断开
			list_del(&slab_page->list);
		}
		//插入到partial链表
		list_add(&slab_page->list, head_partial);
		pSlabMgr->class_base[index].page_partial_num++;
		slab_page->status = SLAB_BITMAP_PARTIAL;
	}
	else { //直接从partial链表取出一个来分配
		slab_page = list_entry(head_partial->next, struct StVSlabPage, list);
	}
	//查找位图，找到第一个空闲块并置位
	pnew = SlabBitMapSet(slab_page, &status);


	if (status == SLAB_BITMAP_FULL) { //置位后满，把页移到page_full链表
									  //先从page_partial链表删除
		list_del(&slab_page->list);
		pSlabMgr->class_base[index].page_partial_num--;

		//再添加到page_full
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
		return;
	}
	bitmap_clr(offset, page->bitmap);

	//查找是否全0，如果全0，就要返回SLAB_BITMAP_FEE状态
	for (i=0; i<page->bitmap_max; i++)
	{
		if (page->bitmap[i] != 0) {
			*status = SLAB_BITMAP_PARTIAL;
			break;
		}
	}
	if (i==page->bitmap_max) { //如果bitmap全0，返回空闲页标志
		*status = SLAB_BITMAP_FREE;
	}
}

/********************************************************************************************************
* 函数：void VSlabBlockFree(struct StVSlabMgr *pSlabMgr, void *ptr);
* 描述: 释放某个block块
* 参数:
* [1] pSlabMgr: slab管理对象
* [2] ptr: 释放块的指针
* 返回：返回信息是ptr指针是否在slab的分配器管理范围内，返回值被buddy分配器释放时提供信息
* 注意：无
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
	//查看释放的地址所在的页基址，通过buddy分配器上查找
	if (pSlabMgr->pheap) {
		pPageBase = (u8*)VMemGetPageBaseAddr(pSlabMgr->pheap, ptr);
	}
	//pPageBase = (u8*)VHeapMgrGetPageBaseAddr(ptr, 0, 0);
	if (pPageBase == 0) {
		//kprintf("error: %s, pPageBase==NULL!\r\n", __FUNCTION__);
		goto END_SLAB_BLOCK_FREE;
	}
	//根据页基址，找到页管理区，就在页面的最高地址上
	slab_page = (struct StVSlabPage *)(pPageBase + pSlabMgr->page_size - pSlabMgr->page_header_size);
	//校验magic数
	if (slab_page->magic != VSLAB_MAGIC_NUM) {
		//kprintf("error: %s, slab_page->magic match failed!\r\n", __FUNCTION__);
		goto END_SLAB_BLOCK_FREE;
	}

	//根据页管理区里的block_size块尺寸，计算slab_class索引号地址
	index =  (slab_page->block_size + pSlabMgr->step_size - 1) / pSlabMgr->step_size - 1;

	if (index >= pSlabMgr->class_max) {
		goto END_SLAB_BLOCK_FREE;
	}

	//注意：slab_page->magic == VSLAB_MAGIC_NUM 证明这个是slab分配器里的页，这时ret=1返回地址属于slab管理内容
	ret = 1;

	//校验释放的对象地址是否是对象的首地址，如果是对象身体的地址，释放失败，这样避免地址乱释放
	if(((u8*)ptr - pPageBase)%slab_page->block_size) {
		goto END_SLAB_BLOCK_FREE;
	}
	//计算释放的指针对应页基址的偏移位置
	offset = ((u8*)ptr - pPageBase)/slab_page->block_size;
	//清除页管理区中位图响应的偏移位，这样就释放了某个block, 注意还返回状态，是清除后的状态
	SlabBitMapClr(slab_page, &status, offset);
	//如果刚清除了那位后，发现全部都是空闲block，将从partial链移到free链中
	if (status == SLAB_BITMAP_FREE) {//如果是全空位图，要添加到page_free链表中
		//从head_partial断开链表
		list_del(&slab_page->list);
		pSlabMgr->class_base[index].page_partial_num--;

		//根据设定的空闲页的阈值，来释放页回buddy系统中
		if (pSlabMgr->class_base[index].page_free_num >= VSLAB_FREE_PAGES_THREHOLD) {
			//清除一下magic, 防止随便释放内存地址，碰巧到中一个释放掉的页
			//还有一个原因必须清除magic， 因为VMemFree会尝试先从slab释放
			slab_page->magic = 0;

			if (pSlabMgr->pheap) {
				VSLAB_UNLOCK();
				VMemFree (pSlabMgr->pheap , pPageBase, 1);//指示slab调用释放函数，要buddy释放自己的内存。
				VSLAB_LOCK();
			}
		}
		else {
			//插入到free链表
			head_free = &pSlabMgr->class_base[index].page_free;
			list_add(&slab_page->list, head_free);
			//设置状态
			slab_page->status = SLAB_BITMAP_FREE;
			pSlabMgr->class_base[index].page_free_num++;
		}

	}
	else {//返回SLAB_BITMAP_PARTIAL，要判断slab_page的链表是从满链过来，还是从部分链过来
		if (slab_page->status == SLAB_BITMAP_FULL) {//从满链过过来，还要从满链移到部分链中
		    //从head_full断开链表
			list_del(&slab_page->list);
			pSlabMgr->class_base[index].page_full_num--;

			//添加到部分链中
			head_partial = &pSlabMgr->class_base[index].page_partial;
			list_add(&slab_page->list, head_partial);
			//设置状态
			slab_page->status = SLAB_BITMAP_PARTIAL;
			pSlabMgr->class_base[index].page_partial_num++;
		}
	}

END_SLAB_BLOCK_FREE:

	VSLAB_UNLOCK();

	return ret;
}

/********************************************************************************************************
* 函数：s32 bitmap_iterate(void **iter, u8 val, void *bitmap, s32 nbits);
* 描述: 遍历就绪任务位图
* 参数:
* [1] iter: 迭代变量，记录下一个要查找的位置，如果为0，则从第一个开始查找
* [2] val: 0或1， 如果是0，指示查找到0就返回；如果是1：当遇到1就返回。
* [3] bitmap: 位图数组地址
* [4] nbits: 位图的长度，这里是位（bits)
* 返回：返回偏移位置，如果>0,则返回正常偏移值，如果-1：返回失败
* 注意：无
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
		if ((pos & 0x7) == 0 && p8[pos >> 3] == mask) {//处理一个字节
			pos += 8;
		}
		else {//处理8bit
			do {
				if (val == !!(p8[pos >> 3] & 1 << (pos & 0x7))) {
					*iter = (void*)(pos + 1);
					goto END_ITERATE;
				}
				pos++;
				if (pos == nbits) { //最后末尾可能超过nbits,证明已经搜索结束
					*iter = (void*)(pos);
					break;
				}
			} while (pos & 0x7);
		}
	}
	//找不到，则返回-1
	pos = -1;

END_ITERATE:

	return pos;
}

/********************************************************************************************************
* 函数：s32 VSlabBitMapGetFreeBlocks(u8 *bitmap, s32 nbits);
* 描述: 获取每页中空闲块个数
* 参数:
* [1] bitmap: 页管理中的位图数组
* [2] nbits: 位数(bits)，注意不是数组个数
* 返回：空闲块个数
* 注意：无
*********************************************************************************************************/
s32 VSlabBitMapGetFreeBlocks(u8 *bitmap, s32 nbits)
{
	s32 free_cnts = 0;
	void *iter = 0; //从头遍历,查找空闲块个数
	while (bitmap_iterate(&iter, 0, bitmap, nbits) > 0) {
		free_cnts++;
	}
	return free_cnts;
}

/********************************************************************************************************
* 函数：s32 VSlabInfoShow(struct StVSlabMgr *pSlabMgr);
* 描述: 打印slab分配器信息,被用于shell命令heap
* 参数:
* [1] pSlabMgr: slab对象指针
* 返回：无
* 注意：无
*********************************************************************************************************/
s32 VSlabInfohow(struct StVSlabMgr *pSlabMgr)
{
	s32 i;
	struct list_head *list;
	struct StVSlabClass *pClass = 0;
	struct StVSlabPage *pPage = 0;

	VSLAB_LOCK();
	kprintf(" slab 管理区信息：\r\n");
	kprintf(" slab 名字: \"%s\", slab 头总大小: %d, 页管理区总大小: %d\r\n",
			pSlabMgr->name, pSlabMgr->slab_header_size, pSlabMgr->page_header_size);
	kprintf(" 对齐字节数: %d, 每页大小: %d, 步长字节: %d, 种类划分大小: %d\r\n",
			pSlabMgr->align_bytes, pSlabMgr->page_size,
			pSlabMgr->step_size, pSlabMgr->class_max);

	kprintf(" 页管理区信息：\r\n");
	for (i = 0; i<pSlabMgr->class_max; i++) {
		pClass = &pSlabMgr->class_base[i];
		if (!list_empty(&pClass->page_full)||
			!list_empty(&pClass->page_partial) ||
			!list_empty(&pClass->page_free)) {

			kprintf(" <种类编号: %d>\r\n", i);
			if (!list_empty(&pClass->page_full)) {
				kprintf("   == 全满页链: 块大小<%d>, 页数<%04d>\r\n", (i+1)*pSlabMgr->step_size, pClass->page_full_num);
				list_for_each(list, &pClass->page_full) {
					pPage = list_entry(list, struct StVSlabPage, list);
					kprintf("   [页头信息: 魔数-0x%08x, 页基址-0x%08x, 位图:位数-<%04d>,空闲块数-<%04d>]\r\n",
						pPage->magic, pPage->block_base, pPage->block_max,
						VSlabBitMapGetFreeBlocks(pPage->bitmap, pPage->block_max));//pPage->block_max才是位图有效总位数
				}
			}
			if (!list_empty(&pClass->page_partial)) {
				kprintf("   == 部分页链: 块大小<%d>, 页数<%04d>\r\n", (i + 1)*pSlabMgr->step_size, pClass->page_partial_num);
				list_for_each(list, &pClass->page_partial) {
					pPage = list_entry(list, struct StVSlabPage, list);
					kprintf("   [页头信息: 魔数-0x%08x, 页基址-0x%08x, 位图:位数-<%04d>,空闲块数-<%04d>]\r\n",
						pPage->magic, pPage->block_base, pPage->block_max,
						VSlabBitMapGetFreeBlocks(pPage->bitmap, pPage->block_max));//pPage->block_max才是位图有效总位数
				}
			}
			if (!list_empty(&pClass->page_free)) {
				kprintf("   == 全空页链: 块大小<%d>, 页数<%04d>\r\n", (i + 1)*pSlabMgr->step_size, pClass->page_free_num);
				list_for_each(list, &pClass->page_free) {
					pPage = list_entry(list, struct StVSlabPage, list);
					kprintf("   [页头信息: 魔数-0x%08x, 页基址-0x%08x, 位图:位数-<%04d>,空闲块数-<%04d>]\r\n",
						pPage->magic, pPage->block_base, pPage->block_max,
						VSlabBitMapGetFreeBlocks(pPage->bitmap, pPage->block_max));//pPage->block_max才是位图有效总位数
				}
			}
		}
	}
	VSLAB_UNLOCK();
	return 0;
}

/********************************************************************************************************
* 函数：s32 VSlabInfoDump(struct StVSlabMgr *pSlabMgr);
* 描述: 打印slab分配器信息
* 参数:
* [1] pSlabMgr: slab对象指针
* 返回：无
* 注意：无
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
	//判断pSlabMgr内部所有元素是否正常
	if (pSlabMgr->align_bytes != VSLAB_ALIGN_SIZE) BOUNDARY_ERROR();
	if (pSlabMgr->page_size != VSLAB_PAGE_SIZE) BOUNDARY_ERROR();
	if (pSlabMgr->step_size != VSLAB_STEP_SIZE) BOUNDARY_ERROR();
	if (pSlabMgr->slab_header_size != pSlabMgr->class_max * sizeof(struct StVSlabClass) + sizeof(struct StVSlabMgr)) BOUNDARY_ERROR();
	if (pSlabMgr->page_header_size != VSlabPageHeaderGetSize(VSLAB_PAGE_SIZE, VSLAB_STEP_SIZE, VSLAB_ALIGN_SIZE)) BOUNDARY_ERROR();
	
	//检查页的属性
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
					//检查位图多余的空间肯定为0
					for (j = (pPage->block_max+8-1)/8;  j < pPage->bitmap_max; j++) {
						if (pPage->bitmap[j] != 0) BOUNDARY_ERROR();
					}
					//检查位图，满页应该都置位
					iter = 0; //从头遍历,查找空闲块，返回-1才是正确的
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
					//检查位图多余的空间肯定为0
					for (j = (pPage->block_max + 8 - 1) / 8; j < pPage->bitmap_max; j++) {
						if (pPage->bitmap[j] != 0) BOUNDARY_ERROR();
					}
					//检查位图，满页应该都置位
					iter = 0; //从头遍历,查找空闲块，应该都有
					if (bitmap_iterate(&iter, 0, pPage->bitmap, pPage->block_max) < 0)  BOUNDARY_ERROR();
					iter = 0; //从头遍历,查找分配块，应该都有
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
					//检查位图多余的空间肯定为0
					for (j = (pPage->block_max + 8 - 1) / 8; j < pPage->bitmap_max; j++) {
						if (pPage->bitmap[j] != 0) BOUNDARY_ERROR();
					}
					//检查位图，满页应该都置位
					iter = 0; //从头遍历,查找分配块，返回-1才是正确的
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
