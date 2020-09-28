/********************************************************************************************************
* 版    权: Copyright (c) 2020, VOS Open source. All rights reserved.
* 文    件: main.c
* 作    者: 156439848@qq.com; vincent_cws2008@gmail.com
* 版    本: VOS V1.0
* 历    史：
* --20200801：创建文件
* --20200828：添加注释
*********************************************************************************************************/

#include "stm32f4xx.h"
#include "usart.h"
#include "vtype.h"
#include "vos.h"

int kprintf(char* format, ...);
#define printf kprintf

void lwip_test();

#include "lcd_dev/lcd.h"
#include "tp_dev/touch.h"
const u16 POINT_COLOR_TBL[OTT_MAX_TOUCH]={RED,GREEN,BLUE,BROWN,GRED};

#define LED1 PFout(9)	  // D1
#define LED2 PFout(10)	// D2
#define KEY0_PRES 	1

void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);
 	POINT_COLOR=BLUE;
	LCD_ShowString(lcddev.width-24,0,200,16,16,"RST");
  	POINT_COLOR=RED;
}
void rtp_test(void)
{
	u8 key;
	u8 i=0;
	while(1)
	{
		tp_dev.scan(0);
		if(tp_dev.sta&TP_PRES_DOWN)
		{
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{
				if(tp_dev.x[0]>(lcddev.width-24)&&tp_dev.y[0]<16)Load_Drow_Dialog();
				else TP_Draw_Big_Point(tp_dev.x[0],tp_dev.y[0],RED);
			}
		}
		else
			VOSTaskDelay(10);
		if(0)
		{
			LCD_Clear(WHITE);
		    TP_Adjust();
			TP_Save_Adjdata();
			Load_Drow_Dialog();
		}
		i++;
		//if(i%20==0)LED1=!LED1;
	}
}

s32 NetDhcpClient(u32 timeout);
void main(void *param)
{
	s32 res;

//	VSlabTest();
//	return ;
// 	LCD_Init();
//	tp_dev.init();
//	rtp_test();
	//lwip_test();
	u8 *ppp = (u8*)vmalloc(18);
	vfree(ppp);
	NetDhcpClient(30*1000);
	//SetNetWorkInfo ("192.168.2.150", "255.255.255.0", "192.168.2.101");
	//lwip_inner_test();

//	kprintf("total: %d, free: %d!\r\n", total, free);
	dma_printf("hello world!\r\n");
	kprintf("main function!\r\n");
	while (1) {
//		kprintf(".");
		VOSTaskDelay(1*1000);
	}
}
