/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: ex_sram.h
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200915：创建文件
*********************************************************************************************************/
#ifndef __EX_SRAM_H__
#define __EX_SRAM_H__

#include "vtype.h"


void ExSRamInit();

u32 ExSRamGetBaseAddr();

u32 ExSRamGetTotalSize();


#endif

