#ifndef __TTP229_H__
#define	__TTP229_H__
#include "stm32f4xx_hal.h"
#include "common.h"

#define	KEY_SDA_IN()	GPIOx_IN(GPIOD,6)
#define	KEY_SDA_OUT()	GPIOx_OUT(GPIOD,6)

#define	TTP_SCL		PCout(2)
#define	TTP_SDO		PDout(6)
#define	TTP_SDI		PDin(6)

void ttp229_init();
u8 ttp229_scan();

#endif
