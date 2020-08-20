//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-20: initial by vincent.
//------------------------------------------------------

#include "vtype.h"
#include "vbitmap.h"

//ŒªÕº≤‚ ‘
#define MAX_TASK_NUM 10;
u8 bitmap[10];
int bitmap_prinf(u8 *bitmap, s32 num)
{
	s32 line_num = 0;
	u32 n = 0;
	bitmap_for_each(n, num)
	{
		//printf("%d ", bitmap_get(n, bitmap) ? 1 : 0);
		printf("%d ", bitmap_get(n, bitmap));
		line_num++;
		if (line_num % 8 == 0) printf("\r\n");
	}
	printf("\r\n");
}

int bitmap_test() {
	u32 n = 0;
	memset(bitmap, 0, sizeof(bitmap));
	bitmap_set(2, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_clear(2, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(7, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(8, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(15, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));
	bitmap_set(16, bitmap);
	bitmap_prinf(bitmap, sizeof(bitmap));

#if 0
	bitmap_for_each(n, bitmap) {
		if (bitmap_check(n, bitmap)) {

		}
	}
#endif
}
