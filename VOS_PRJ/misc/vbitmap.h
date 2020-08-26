//----------------------------------------------------
// Copyright (c) 2020, VOS Open source. All rights reserved.
// Author: 156439848@qq.com; vincent_cws2008@gmail.com
// History:
//	     2020-08-20: initial by vincent.
//------------------------------------------------------

#ifndef __V_BITMAP_H__
#define __V_BITMAP_H__


#define MAX_COUNTS(arr) (sizeof(arr)/sizeof(arr[0]))

/*
  * 位图操作宏定义
 */
#define bitmap_for_each(pos, bitmap_byte) \
	for (pos = 0; pos < bitmap_byte*8; pos++)

#define bitmap_get(n, bitmap)		(!!(((u8*)(bitmap))[(n)>>3] & 1<<((n)&0x07)))
#define bitmap_clr(n, bitmap)		(((u8*)(bitmap))[(n)>>3] &= ~(1<<((n)&0x07)))
#define bitmap_set(n, bitmap)		(((u8*)(bitmap))[(n)>>3] |= 1<<((n)&0x07))

s32 TaskHighestPrioGet(u32 *bitmap, s32 num);

#endif
