/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vbitmap.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/
#include "vtype.h"
#include "vbitmap.h"

const u8 TabTaskPrio[] =
{
	0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
};

/********************************************************************************************************
* 函数：s32 TaskHighestPrioGet(u32 *bitmap, s32 num);
* 描述: 获取任务最高优先级。
* 参数:
* [1] bitmap: 优先级位图，u32类型的数组指针
* [2] num: u32数据项的个数
* 返回：最高优先级（最低位）的第n位。2^n
* 注意：无
*********************************************************************************************************/
s32 TaskHighestPrioGet(u32 *bitmap, s32 num)
{
	s32 i = 0;
	u32 tmp = 0;
	s32 ret = -1;
	for (i = 0; i < num; i++) {
		if (bitmap[i] != 0) {
			tmp = bitmap[i];
			if (tmp & 0x000000FF) {
				ret = i * 32 + TabTaskPrio[(u8)tmp];
			}
			else if (tmp & 0x0000FF00) {
				ret = i * 32 + 8 + TabTaskPrio[(u8)(tmp >> 8)];
			}
			else if (tmp & 0x00FF0000) {
				ret = i * 32 + 16 + TabTaskPrio[(u8)(tmp >> 16)];
			}
			else {
				ret = i * 32 + 24 + TabTaskPrio[(u8)(tmp >> 24)];
			}
			break;
		}
	}
	return ret;
}

/********************************************************************************************************
* 函数：int bitmap_prinf(u8 *bitmap, s32 num);
* 描述: 遍历位图， 按位来读取位图并打印出来。
* 参数:
* [1] bitmap: 优先级位图，u32类型的数组指针
* [2] num: u32数据项的个数
* 返回：最高优先级（最低位）的第n位。2^n
* 注意：无
*********************************************************************************************************/
int bitmap_prinf(u8 *bitmap, s32 num)
{
	s32 line_num = 0;
	u32 n = 0;
	bitmap_for_each(n, num)
	{
		//printf("%d ", bitmap_get(n, bitmap) ? 1 : 0);
		kprintf("%d ", bitmap_get(n, bitmap));
		line_num++;
		if (line_num % 32 == 0) kprintf("\r\n");
	}
	kprintf("\r\n");
}


u32 bitmap[10];//4*10 bits
/********************************************************************************************************
* 函数：s32 bitmap_iterate(void **iter);
* 描述: 遍历位图，按字节来读取位图，速度更快。
* 参数:
* [1] iter: 记录迭代的指针，每次都记录下一个查找位置
* 返回：最高优先级（最低位）的第n位。2^n
* 注意：无
*********************************************************************************************************/
//s32 bitmap_iterate(void **iter)
//{
//	u32 pos = (u32)*iter;
//	u8 *p8 = (u8*)bitmap;
//	while (pos < sizeof(bitmap) * 8) {
//		if ((pos & 0x7) == 0 && p8[pos>>3] == 0) {//处理一个字节
//			pos += 8;
//		}
//		else {//处理8bit
//			do {
//				if (p8[pos>>3] & 1 << (pos & 0x7)) {
//					*iter = (void*)(pos + 1);
//					goto END_ITERATE;
//				}
//				pos++;
//			} while (pos & 0x7);
//		}
//	}
//	return -1;
//
//END_ITERATE:
//	return pos;
//}

/********************************************************************************************************
* 函数：int bitmap_test();
* 描述: 测试例子。
* 参数: 无
* 返回：无
* 注意：无
*********************************************************************************************************/
int bitmap_test()
{
	u32 n = 0;
	memset(bitmap, 0, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_set(32*10-1, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_clr(2, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_set(7, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_set(8, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_set(1, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_set(0, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_clr(0, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	bitmap_set(16, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	kprintf("TaskHighestPrioGet=%d\r\n", TaskHighestPrioGet(bitmap, MAX_COUNTS(bitmap)));
	void *iter = 0; //从头遍历
	s32 prio;
	while (-1 != (prio = bitmap_iterate(&iter))) {
		kprintf("======>prio=%d<======\r\n", prio);
	}
}

