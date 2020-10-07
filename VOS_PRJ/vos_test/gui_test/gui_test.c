#include "stm32f4xx.h"
#include "vos.h"
#include "2D_DISPLAY/2ddisplay.h"
#include "EMWINFONT/ttffontcreate.h"
#include "vmisc.h"
#include "ffconf.h"
#include "ff.h"
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
#if 1
	GUI_TTF_SetCacheSize(1, 1, 16*1024);
	result=Create_TTFFont("0:/msyh3500a.ttf");
	if(result) {
		kprintf("TTF font build failed!\r\n");
		return;
	}
	GUI_UC_SetEncodeUTF8();
	GUI_SetFont(&TTF36_Font);
#endif
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetColor(GUI_DARKBLUE);

	GUI_SetTextMode(GUI_TM_TRANS);

	GUI_Clear();

	jpegdisplay_demo();
	//bmpdisplay_demo();

#endif
}

void fatfs_sd_card();
static long long emwindemo_stack[1024];
void emWinTest()
{
 	LCD_Init();
 	fatfs_sd_card();
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC,ENABLE);
 	GUI_Init();

  	s32 task_id;
 	task_id = VOSTaskCreate(emwindemo_task, 0, emwindemo_stack, sizeof(emwindemo_stack), TASK_PRIO_NORMAL, "emwindemo_task");
}
