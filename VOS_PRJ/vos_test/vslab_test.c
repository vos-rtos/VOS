#include "vos.h"
#include "vslab.h"
#include "vmem.h"

#define VSLAB_ALIGN_SIZE	8
#define VSLAB_STEP_SIZE		8
#define VSLAB_PAGE_SIZE		1024

extern struct StVSlabMgr;


extern struct StVMemHeap;
struct StVMemHeap *gpheap = 0;
void *VMemMalloc(struct StVMemHeap *pheap, u32 size);
#define vmalloc(x)  VMemMalloc(gpheap, (x))
#define kprintf printf


s32 VSlabInfoDump(struct StVSlabMgr *pSlabMgr);

s32 VSlabBoudaryCheck(struct StVSlabMgr *pSlabMgr);

/********************************************************************************************************
* 函数：void vslab_test_by_man();
* 描述: 手动测试, 手动设定参数，结合调试判断是否合法
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void vslab_test_by_man()
{
	u8 *pnew[100];
	static u8 arr_heap[1024 * 10 + 10];
	u8 *p = 0;
	s32 i = 0;
	s32 malloc_size = SlabMgrHeaderGetSize(1024, 8, 8);

	gpheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 1024, VSLAB_ALIGN_SIZE, VHEAP_ATTR_SYS, "test_heap", 1);
	p = (u8*)VMemMalloc(gpheap, malloc_size);
	struct StVSlabMgr *pslab = VSlabBuild(p, malloc_size, 1024, VSLAB_ALIGN_SIZE, VSLAB_STEP_SIZE, "test_slab", gpheap);
	memset(pnew, 0, sizeof(pnew) / sizeof(pnew[0]));
	for (i = 0; i < 84; i++) {
		pnew[i] = VSlabBlockAlloc(pslab, 50);
		if (pnew[i] == 0) {
			break;
		}
	}
	VSlabInfoDump(pslab);
	VSlabBoudaryCheck(pslab);
	VSlabBlockFree(pslab, pnew[20]);
	VSlabInfoDump(pslab);
	VSlabBoudaryCheck(pslab);

	VSlabBlockFree(pslab, pnew[50]);
	VSlabInfoDump(pslab);

	VSlabBlockFree(pslab, pnew[70]);
	VSlabInfoDump(pslab);

	for (i = 0; i < 84; i++) {
		VSlabBlockFree(pslab, pnew[i]);
	}
	VSlabInfoDump(pslab);
	VSlabBoudaryCheck(pslab);
}

/********************************************************************************************************
* 函数：void vslab_test_random();
* 描述: 随机产生参数进行压力测试
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
void vslab_test_random()
{
	s32 counts = 0;
	s32 rand_num = 0;
	s32 total = 0;
	s32 malloc_size = 0;
	u8 *pnew[1000];
	static u8 arr_heap[1024 * 1024 + 10];
	u8 *p = 0;
	s32 i = 0;

	malloc_size = SlabMgrHeaderGetSize(1024, VSLAB_ALIGN_SIZE, VSLAB_STEP_SIZE);

	gpheap = VMemBuild(&arr_heap[0], sizeof(arr_heap), 1024, VSLAB_ALIGN_SIZE, VHEAP_ATTR_SYS, "test_heap", 1);
	p = (u8*)VMemMalloc(gpheap, malloc_size);
	struct StVSlabMgr *pslab = VSlabBuild(p, malloc_size, 1024, VSLAB_ALIGN_SIZE, VSLAB_STEP_SIZE, "test_slab", gpheap);


	malloc_size = 0;
	rand_num = rand() % 1000;
	memset(pnew, 0, sizeof(pnew) / sizeof(pnew[0]));
	//申请满
	for (i = 0; i < sizeof(pnew) / sizeof(pnew[0]); i++) {
		while (pnew[i]) {
			malloc_size = rand() % 1000;
			pnew[i] = VSlabBlockAlloc(pslab, malloc_size);
			if (pnew[i]) memset(pnew[i], 0x00, malloc_size);
			VSlabBoudaryCheck(pslab);
		}
	}
	VSlabInfoDump(pslab);

	//全部释放
	for (i = 0; i < sizeof(pnew) / sizeof(pnew[0]); i++) {
		VSlabBlockFree(pslab, pnew[i]);
		VSlabBoudaryCheck(pslab);
	}
	VSlabInfoDump(pslab);

	//申请后立即释放
	for (i = 0; i < 10*10000; i++) {
		pnew[0] = VSlabBlockAlloc(pslab, malloc_size);
		if (pnew[0]) memset(pnew[0], 0x00, malloc_size);
		VSlabBoudaryCheck(pslab);
		VSlabBlockFree(pslab, pnew[0]);
		VSlabBoudaryCheck(pslab);
	}
	VSlabInfoDump(pslab);

	//随机申请随机释放
	counts = 0;
	rand_num = 0;
	total = 0;
	while (total++ < 100000) {
		counts = rand() % (sizeof(pnew) / sizeof(pnew[0]));
		//随机申请
		for (i = 0; i < counts; i++) {
			rand_num = rand() % (sizeof(pnew) / sizeof(pnew[0]));
			if (pnew[i]) {
				VSlabBlockFree(pslab, pnew[i]);
				VSlabBoudaryCheck(pslab);
				pnew[i] = 0;
			}
			while (pnew[rand_num]) {
				malloc_size = rand() % 1000;
				pnew[rand_num] = VSlabBlockAlloc(pslab, malloc_size);
				if (pnew[rand_num]) memset(pnew[rand_num], 0x00, malloc_size);
				VSlabBoudaryCheck(pslab);
			}
		}
		VSlabInfoDump(pslab);

		//随机释放
		counts = rand() % (sizeof(pnew) / sizeof(pnew[0]));
		for (i = 0; i < counts; i++) {
			rand_num = rand() % (sizeof(pnew) / sizeof(pnew[0]));
			VSlabBlockFree(pslab, pnew[rand_num]);
			VSlabBoudaryCheck(pslab);
			pnew[rand_num] = 0;
		}
		VSlabInfoDump(pslab);
	}

}

void VSlabTest()
{
	//vslab_test_random();
	vslab_test_by_man();
}
