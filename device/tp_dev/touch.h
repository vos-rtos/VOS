#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "common.h"

#define TP_PRES_DOWN 0x80
#define TP_CATH_PRES 0x40
#define CT_MAX_TOUCH 10

typedef struct
{
	u8 (*init)(void);
	u8 (*scan)(u8);
	void (*adjust)(void);
	u16 x[CT_MAX_TOUCH];
	u16 y[CT_MAX_TOUCH];
	u16 sta;

	float xfac;
	float yfac;
	short xoff;
	short yoff;
	u8 touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev;

//#define PEN     PHin(7)    //T_PEN
//#define DOUT    PGin(3)    //T_MISO
//#define TDIN    PIout(3)   //T_MOSI
//#define TCLK    PHout(6)   //T_SCK
//#define TCS     PIout(8)   //T_CS

//电阻屏芯片连接引脚
#define PEN  		PBin(1)  	//T_PEN
#define DOUT 		PBin(2)   	//T_MISO
#define TDIN 		PFout(11)  	//T_MOSI
#define TCLK 		PBout(0)  	//T_SCK
#define TCS  		PCout(13)  	//T_CS


void TP_Write_Byte(u8 num);
u16 TP_Read_AD(u8 CMD);
u16 TP_Read_XOY(u8 xy);
u8 TP_Read_XY(u16 *x,u16 *y);
u8 TP_Read_XY2(u16 *x,u16 *y);
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color);
void TP_Draw_Big_Point(u16 x,u16 y,u16 color);
void TP_Save_Adjdata(void);
u8 TP_Get_Adjdata(void);
void TP_Adjust(void);
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac);

u8 TP_Scan(u8 tp);
u8 TP_Init(void);

#endif

















