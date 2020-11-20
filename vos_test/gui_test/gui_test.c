#include "stm32f4xx.h"
#include "vos.h"
#include "2D_DISPLAY/2ddisplay.h"
#include "EMWINFONT/ttffontcreate.h"
#include "vmisc.h"
#include "ffconf.h"
#include "ff.h"
#include "lcd_dev/lcd.h"
#include "tp_dev/touch.h"
#include "GUI.h"
#include "FRAMEWIN.h"

void emwindemo_task(void *p_arg)
{

#if 0
	GUI_CURSOR_Show();
	while(1)
	{
		display_2d();
		GUI_Delay(2000);
		alpha_display();
		GUI_Delay(2000);
		draw_polygon();
		GUI_Delay(2000);
	}
#elif 0
	int result;
	result=Create_TTFFont("0:/msyh3500a.ttf");
	if(result) {
		kprintf("TTF font build failed!\r\n");
		return;
	}

	//GUI_CURSOR_Show();
	GUI_SetBkColor(GUI_ORANGE);
	GUI_SetColor(GUI_BLUE);
	GUI_Clear();

	GUI_UC_SetEncodeUTF8();
	GUI_SetFont(&TTF24_Font);

	GUI_DispStringAt(GB2312_TO_UTF8_LOCAL("ÄãºÃ?123,very good!"), 5, 10);

	while(1)
	{
		 GUI_Delay(100);
	}
#else

	void jpegdisplay_demo(void);
	void bmpdisplay_demo(void);

	int result;

	GUI_TTF_SetCacheSize(1, 1, 16*1024);
	result=Create_TTFFont("0:/msyh3500a.ttf");
	if(result) {
		kprintf("TTF font build failed!\r\n");
		return;
	}
	GUI_UC_SetEncodeUTF8();
	GUI_SetFont(&TTF36_Font);

	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_RED);

	GUI_SetTextMode(LCD_DRAWMODE_XOR);

	GUI_Clear();
#if 0
	WM_SetCreateFlags(WM_CF_MEMDEV);
	CreateFramewin();
	while(1) {
		GUI_Exec();
		GUI_TOUCH_Exec();
	}
#endif
	jpegdisplay_demo();
	//bmpdisplay_demo();

#endif
}

void fatfs_sd_card();
static long long emwindemo_stack[1024*2];
void emWinTest()
{

 	LCD_Init();
 	tp_dev.init();
 	fatfs_sd_card();
 	__HAL_RCC_CRC_CLK_ENABLE();

 	GUI_Init();

  	s32 task_id;
 	task_id = VOSTaskCreate(emwindemo_task, 0, emwindemo_stack, sizeof(emwindemo_stack), TASK_PRIO_NORMAL-1, "emwindemo_task");
}
