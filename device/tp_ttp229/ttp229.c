#include "ttp229.h"
#include "vos.h"

uint16_t ttp229_read();

void ttp229_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOC_CLK_ENABLE();


	GPIO_InitStructure.Pin=GPIO_PIN_2;
	GPIO_InitStructure.Mode=GPIO_MODE_OUTPUT_PP;
	//GPIO_InitStructure.Pull=GPIO_PULLUP;
	GPIO_InitStructure.Speed=GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	KEY_SDA_OUT();             //SDO设置为输出
	TTP_SDO = 0;
	TTP_SCL = 0;
	ttp229_read();
}


uint16_t ttp229_read()
{
	u8 i;
	u16 re_val = 0;
	KEY_SDA_OUT();
	TTP_SDO = 1;
	VOSDelayUs(100);
	TTP_SDO = 0;
	VOSDelayUs(20);

	KEY_SDA_IN();

	for (i = 0; i < 16; i++)
	{
		TTP_SCL = 1;
		VOSDelayUs(100);
		TTP_SCL = 0;
		if (TTP_SDI==1)
		{
		   re_val |= (1 << i);
		}
		VOSDelayUs(100);
	}
	VOSTaskDelay(35);    //根据时序图延时2ms， 不然容易出现按键串扰现象
	return re_val;
}

u16 PreKeyNum;
u16 NowKeyNum;

uint8_t ttp229_scan()
{
	u8 key_num = 0xFF;
	NowKeyNum=ttp229_read();
	if((NowKeyNum & 0x0001)&& !(PreKeyNum & 0x0001))
	{
	key_num=1;
	}
	if((NowKeyNum & 0x0002)&& !(PreKeyNum & 0x0002))
	{
	key_num=2;
	}
	if((NowKeyNum & 0x0004)&& !(PreKeyNum & 0x0004))
	{
	key_num=3;
	}
	if((NowKeyNum & 0x0008)&& !(PreKeyNum & 0x0008))
	{
	key_num=4;
	}
	if((NowKeyNum & 0x0010)&& !(PreKeyNum & 0x0010))
	{
	key_num=5;
	}
	if((NowKeyNum & 0x0020)&& !(PreKeyNum & 0x0020))
	{
	key_num=6;
	}
	if((NowKeyNum & 0x0040)&& !(PreKeyNum & 0x0040))
	{
	key_num=7;
	}
	if((NowKeyNum & 0x0080)&& !(PreKeyNum & 0x0080))
	{
	key_num=8;
	}
	if((NowKeyNum & 0x0100)&& !(PreKeyNum & 0x0100))
	{
	key_num=9;
	}
	if((NowKeyNum & 0x0200)&& !(PreKeyNum & 0x0200))
	{
	key_num=10;
	}
	if((NowKeyNum & 0x0400)&& !(PreKeyNum & 0x0400))
	{
	key_num=11;
	}
	if((NowKeyNum & 0x0800)&& !(PreKeyNum & 0x0800))
	{
	key_num=12;
	}
	if((NowKeyNum & 0x1000)&& !(PreKeyNum & 0x1000))
	{
	key_num=13;
	}
	if((NowKeyNum & 0x2000)&& !(PreKeyNum & 0x2000))
	{
	key_num=14;
	}
	if((NowKeyNum & 0x4000)&& !(PreKeyNum & 0x4000))
	{
	key_num=15;
	}
	if((NowKeyNum & 0x8000)&& !(PreKeyNum & 0x8000))
	{
	key_num=16;
	}
	PreKeyNum=NowKeyNum;
	return key_num;

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
