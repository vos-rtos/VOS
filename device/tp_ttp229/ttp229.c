#include "ttp229.h"
#include "vos.h"

u16 ttp229_read();

void ttp229_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitStructure.Pin = GPIO_PIN_2;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	KEY_SDA_OUT();             //SDO设置为输出
	TTP_SDO = 0;
	TTP_SCL = 0;
	ttp229_read();
}


u16 ttp229_read()
{
	u8 i;
	u16 val = 0;
	KEY_SDA_OUT();
	TTP_SDO = 1;
	VOSDelayUs(80);
	TTP_SDO = 0;
	VOSDelayUs(20);

	KEY_SDA_IN();

	for (i = 0; i < 16; i++) {
		TTP_SCL = 1;
		VOSDelayUs(80);
		TTP_SCL = 0;
		if (TTP_SDI==1){
		   val |= (1 << i);
		}
		VOSDelayUs(80);
	}
	VOSTaskDelay(35);    //根据时序图延时2ms， 不然容易出现按键串扰现象
	return val;
}

u8 ttp229_scan()
{
	s32 i = 0;
	u8 ret = 0xFF;
	u16 key;

	static u16 pre_key = 0;

	key = ttp229_read();

	if (key == 0) {
		pre_key = 0;
		return ret;
	}
	for (i = 0; i < 16; i++) {
		if ((1<<i) & key) {
			if (pre_key != key) {
				pre_key = key;
				ret = i+1;
			}
			break;
		}
	}
	return ret;
}

int test_ttp229()
{
	u16 num=0;
	ttp229_init();
	while(1)
	{
		num=ttp229_scan();
		if (num != 0xFF) {
			kprintf("num=%d\r\n",num);
		}
	}
}
