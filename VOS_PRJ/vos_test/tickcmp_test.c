/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: vos.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "vos.h"
#define vos_assert(exp) while(!(exp))

void tickcmp()
{

	u32 ticks_cur = 0x1;
	u32 ticks_alert = 0x1;
	u32 ticks_start = 0x1;
	s32 ret = TICK_CMP(ticks_cur,ticks_alert,ticks_start);
	vos_assert(ret==0);

	ticks_cur = 0x3;
	ticks_alert = 0x3;
	ticks_start = 0x1;
	ret = TICK_CMP(ticks_cur,ticks_alert,ticks_start);
	vos_assert(ret==0);

	ticks_cur = 0x4;
	ticks_alert = 0x3;
	ticks_start = 0x1;
	ret = TICK_CMP(ticks_cur,ticks_alert,ticks_start);
	vos_assert(ret==1);

	ticks_cur = 0x3;
	ticks_alert = 0x5;
	ticks_start = 0x1;
	ret = TICK_CMP(ticks_cur,ticks_alert,ticks_start);
	vos_assert(ret==-1);

	ticks_cur = 0xFFFFFFFF;
	ticks_alert = 0x2;
	ticks_start = 0xFFFFFFFE;
	ret = TICK_CMP(ticks_cur,ticks_alert,ticks_start);
	vos_assert(ret==-1);

	ticks_cur = 0x2;
	ticks_alert = 0x2;
	ticks_start = 0xFFFFFFFE;
	ret = TICK_CMP(ticks_cur,ticks_alert,ticks_start);
	vos_assert(ret==0);

	ticks_cur = 0x3;
	ticks_alert = 0x2;
	ticks_start = 0xFFFFFFFE;
	ret = TICK_CMP(ticks_cur,ticks_alert,ticks_start);
	vos_assert(ret==1);
	kprintf("test tickcmp done!!\r\n");
}

void tickcmp_test()
{
	kprintf("test tickcmp!\r\n");
	tickcmp();
	while (TestExitFlagGet() == 0) {
		VOSTaskDelay(1*1000);
	}
}
