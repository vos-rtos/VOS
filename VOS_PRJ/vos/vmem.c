/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vmem.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200905：创建文件, 实现buddy内存动态分配算法，速度理论上很快，申请，释放都不用查找链表
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
	u8 *mem_end; //内存对齐后的结束地址，用来判断释放是否越界
	s32 page_counts; //暂时没什么用
	s32 page_size; //每页大小，单位字节，通常1024,2048等等
	struct StVMemCtrlBlock *pMCB_base; //内存控制块基地址，与page_base页地址是平行数组，偏移量一致
	u8 *page_base; //数据页基地址，与pMCB_base控制块是平行数组，偏移量一致
	struct list_head page_class[MAX_PAGE_CLASS_MAX]; //page_class[0] 指向2^0个page的链表；page_class[1]指向2^1个page的链表；
}StVMemHeap;

//每个页对应着一个内存控制块，例如1个页1k字节，估计花销就是sizeof(StVMemCtrlBlock)/1k
//这里StVMemCtrlBlock数组和所有页是平行数组，偏移值一致，这样设计是为了malloc的输入，写穿后不影响管理书籍
//平行数组也是为了加速释放内存时，合并相邻的内存，不用遍历链表
typedef struct StVMemCtrlBlock{
	struct list_head list;
	u16 used_num; //当前使用了多少字节
	u16 page_max; //最大分配了多少页, 2^n
	u32 status; //VMEM_STATUS_FREE,VMEM_STATUS_USED,VMEM_STATUS_UNKNOWN
}StVMemCtrlBlock;

#define ALIGN_UP(mem, align) 	((u32)(mem) & ~((align)-1))
#define ALIGN_DOWN(mem, align) 	(((u32)(mem)+(align)-1) & ~((align)-1))

/********************************************************************************************************
* 函数：u32 CutPageClassSize(s32 size, s32 page_size, s32 *index, s32 *count);
* 描述:  内存初始化时，需要计算空闲链表的尺寸，然后把他们链接一起
* 参数:
* [1] size: 用户需要划分存储空间大小
* [2] page_size: 每页空间大小，通常为1024，2048，等等
* [3] index: 输出参数，这里的index就是page_class[index]索引，其实就是2^n中的n值
* [4] count: 返回有多少个整数倍的2^n返回值，如果传入的size大于2^n不止一倍，则这里返回倍数，
*                         返回值也是倍数后的数值
* 返回：最大存储的字节数（例如：传入参数为1025， 返回1024）
* 注意：如果参数size的尺寸不能完整存放到最大的page_class[MAX_PAGE_CLASS_MAX-1]里，则剩余部分存放到
*      page_class[x < MAX_PAGE_CLASS_MAX]里，直到最少的page_size的1倍粒度
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
* 函数：s32 GetPageClassIndex(s32 size, s32 page_size);
* 描述:  计算申请size的需要对齐2^n的索引号，例如申请100, 返回0，每页1024字节，2^0等于1就是需要申请的空间
* 参数:
* [1] size: 用户需要申请内存的大小
* [2] page_size: 每页空间大小，通常为1024，2048，等等
* 返回：返回page_class[n]的索引号
* 注意：无
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
* 函数：void VMemDebug (StVMemHeap *pheap);
* 描述:  调试内存
* 参数:
* [1] pheap: 堆对象指针
* 返回：无
* 注意：无
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
* 函数：StVMemHeap *VMemCreate(u8 *mem, s32 len, s32 page_size, s32 align_bytes);
* 描述:  堆创建
* 参数:
* [1] mem: 用户提供用来划分堆的内存
* [2] len: 用户提供内存的大小，单位：字节
* [3] page_size: 页尺寸，1024,2048等
* [4] align_bytes: 字节对齐，例如8，就是8字节对齐
* 返回：堆管理对象指针
* 注意：如果提供的内存超出了page_class[n]最大空间，就page_class[n]最大值里链表里链接多个相邻的数据块
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

	//初始化page头，各个元素都是一个环形双向链表
	for (i=0; i<MAX_PAGE_CLASS_MAX; i++)
	{
		INIT_LIST_HEAD(&pheap->page_class[i]); //自己指向自己
	}

	//初始化链表
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
		//当cnts > 1, 证明page[MAX_PAGE_CLASS_MAX-1]不能一次性容纳整块，可以增加MAX_PAGE_CLASS_MAX
		//否则，将会把内存连续的放到page[MAX_PAGE_CLASS_MAX-1]链表中
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
* 函数：void *VMemMalloc(StVMemHeap *pheap, s32 size);
* 描述:  指定堆里申请size空间内存
* 参数:
* [1] pheap: 指定的堆
* [2] size: 申请的大小，单位字节
* 返回：申请的内存空间起始地址
* 注意：buddy算法，如果申请内存块对应的链表为空，则往上申请，直接申请成功或失败。
*********************************************************************************************************/
void *VMemMalloc(StVMemHeap *pheap, s32 size)
{
	void *pObj = 0;
	s32 i;
	s32 index;
	s32 offset;
	s32 index_top;//这层是有内存空间可以分配
	StVMemCtrlBlock *pMCB = 0;//分裂，左边那个
	StVMemCtrlBlock *pMCBRight = 0; //分裂，右边那个
	if (size > pheap->page_size * (1<<(MAX_PAGE_CLASS_MAX-1))) {//申请大小 大于 最大块
		return 0;
	}
	index = GetPageClassIndex(size, pheap->page_size);
	if (index < 0) goto END_VMEMMALLOC;

	//往大块找内存
	index_top = -1;
	for (i=index; i<MAX_PAGE_CLASS_MAX; i++)
	{
		if (!list_empty(&pheap->page_class[i])) {
			//pheap->page_class[index]
			index_top = i;
			break;
		}
	}
	if (index_top >= 0) { //可能需要大块分裂
		//先取出上层大块的数据块
		pMCB = (StVMemCtrlBlock *)list_entry(pheap->page_class[index_top].next, struct StVMemCtrlBlock, list);
		list_del(&pMCB->list);//取出第一个内存块

		while (index_top != index) {
			//一分为二
			pMCBRight = pMCB + (1 << index_top) / 2;

			pMCBRight->page_max = pMCB->page_max = pMCB->page_max/2;

			pMCBRight->used_num = pMCB->used_num = 0;

			pMCBRight->status = pMCB->status = VMEM_STATUS_FREE;

			index_top--;
			//插入到page_class[n]中， 插入有右边的，左边继续分配
			list_add_tail(&pMCBRight->list, &pheap->page_class[index_top]);
		}
		//取出分配到的内存
		pMCB->status = VMEM_STATUS_USED;
		pMCB->used_num = size;
		pMCB->page_max = 1<<index_top; //page_class[n]对应的最大页数 ，释放时需要用到。
		INIT_LIST_HEAD(&pMCB->list); //自己指向自己吧

		//算出页地址
		pObj = pheap->page_base + (pMCB - pheap->pMCB_base) * pheap->page_size;
	}

END_VMEMMALLOC:

	return pObj;
}

/********************************************************************************************************
* 函数：VMemFree (StVMemHeap *pheap, void *p);
* 描述:  释放指定堆里申请内存
* 参数:
* [1] pheap: 指定的堆
* [2] p: 释放的地址，这个地址必须是申请时的起始地址
* 返回：无
* 注意：buddy算法，如果内存释放后邻居也空闲，则合并到上一级的page_class[n]的链表中，直到最顶链表。
*********************************************************************************************************/
void VMemFree (StVMemHeap *pheap, void *p)
{
	s32 size = 0;
	s32 offset = 0;
	s32 page_max = 0;
	s32 index = 0;
	struct StVMemCtrlBlock *pMCB = 0;
	struct StVMemCtrlBlock *pMCBTemp = 0;
	if ((u8*)p < pheap->page_base || (u8*)p >= pheap->mem_end) return; //地址范围溢出
	//判断p地址必须是页面对齐，否者不释放
	size = (u8*)p - pheap->page_base;
	if (size % pheap->page_size) return; //地址距离必须是page_size倍数

	offset = size / pheap->page_size;
	pMCB = &pheap->pMCB_base[offset];
	page_max = pMCB->page_max;

	while (1) {
		if (page_max >= 1<<(MAX_PAGE_CLASS_MAX-1)) { //超出的page_class[n]最大的尺寸，跳出
			break;
		}
		if (offset % (page_max << 1) == 0) {//当前块是合并块的左块
			//查找合并块右块是否空闲 (buddy)
			pMCBTemp = &pheap->pMCB_base[offset+page_max];
			if (pMCBTemp->status == VMEM_STATUS_FREE && //右块是空闲
					pMCBTemp->page_max == pMCB->page_max )  { //右块和左块同一个链表中
				//释放右边
				list_del(&pMCBTemp->list);
				//清空右边
				pMCBTemp->used_num = 0;
				pMCBTemp->status = 0;
				pMCBTemp->page_max = 0;
				pMCBTemp->list.prev = 0;
				pMCBTemp->list.next = 0;
				//合并右边
				pMCB->page_max <<= 1;
				pMCB->used_num = 0;
				pMCB->status = VMEM_STATUS_FREE;
				//往上层合并
				//pMCB = pMCB;
				page_max = pMCB->page_max;
				offset = pMCB - pheap->pMCB_base;
			}
			else {
				break;
			}
		}
		else {//当前块是合并块的右块
			//查找合并块左块是否空闲 (buddy)
			pMCBTemp = &pheap->pMCB_base[offset-page_max];
			if (pMCBTemp->status == VMEM_STATUS_FREE && //左块是空闲
					pMCBTemp->page_max == pMCB->page_max )  { //右块和左块同一个链表中
				//释放左边
				list_del(&pMCBTemp->list);
				//清空右边
				pMCB->used_num = 0;
				pMCB->status = 0;
				pMCB->page_max = 0;
				pMCB->list.prev = 0;
				pMCB->list.next = 0;
				//合并右边
				pMCBTemp->page_max <<= 1;
				pMCBTemp->used_num = 0;
				pMCBTemp->status = VMEM_STATUS_FREE;
				//往上层合并
				pMCB = pMCBTemp;
				page_max = pMCB->page_max;
				offset = pMCB - pheap->pMCB_base;
			}
			else {
				break;
			}
		}
	}
	//把MCB插入到指定page_class[n]链表尾部
	index = 0;
	while (pMCB->page_max != 1<<index) index++;
	if (index < MAX_PAGE_CLASS_MAX) {
		pMCB->status = VMEM_STATUS_FREE;
		pMCB->used_num = 0;
		list_add_tail(&pMCB->list, &pheap->page_class[index]);//添加到空闲链表末尾
	}
	return;
}

u8 arr_heap[1024*10+10];
/********************************************************************************************************
* 函数：void vmem_test();
* 描述:  测试
* 参数:  无
* 返回：无
* 注意：无
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


