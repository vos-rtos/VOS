#ifndef __RANDOM_HW_H__
#define __RANDOM_HW_H__
#include "vos.h"

void HwRandomInit();
u32 HwRandomBuild();
u32 HwRandomBuildRang(u32 from, u32 to);
#endif

