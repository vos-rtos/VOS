#ifndef __TTP229_H__
#define	__TTP229_H__
#include "stm32f4xx_hal.h"
#include "common.h"

#define	KEY_SDA_IN()	{GPIOC->MODER &= ~(3 << (3*2)); GPIOC->MODER |= (0 << (3*2));}
#define	KEY_SDA_OUT()	{GPIOC->MODER &= ~(3 << (3*2)); GPIOC->MODER |= (1 << (3*2));}

#define	TTP_SCL		PCout(2)
#define	TTP_SDO		PCout(3)
#define	TTP_SDI		PCin(3)

void ttp229_init();
u8 ttp229_scan();

#endif
