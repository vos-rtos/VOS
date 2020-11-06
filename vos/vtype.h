/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vtype.h
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#if 0
#define u8 	unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long

#define s8 	char
#define s16 short
#define s32 int
#define s64 long long
#else

//#ifndef __CORE_CM4_H_GENERIC

typedef	unsigned char u8;
typedef	unsigned short u16;
typedef	unsigned int u32;
typedef	char s8;
typedef	short s16;
typedef	int s32;
//#endif

typedef	unsigned long long u64;
typedef	long long s64;

#endif
