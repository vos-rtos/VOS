#include "stm32f4xx.h"
#include "vos.h"
#include "2D_DISPLAY/2ddisplay.h"
#include "EMWINFONT/ttffontcreate.h"

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
#else

	int result;
	result=Create_TTFFont("0:/msyh3500a.ttf");
	if(result) {
		printf("TTF font build failed!\r\n");
		return;
	}

	GUI_CURSOR_Show();
	GUI_SetBkColor(GUI_LIGHTGREEN);
	GUI_SetColor(GUI_BLUE);
	GUI_Clear();

	GUI_UC_SetEncodeUTF8();
	GUI_SetFont(&TTF24_Font);

	GUI_DispStringAt("你好?123,very good!", 5, 10);

	while(1)
	{
		 GUI_Delay(100);
	}

#endif
}

void fatfs_sd_card();
static long long emwindemo_stack[2048/*1024*/];
void emWinTest()
{
 	LCD_Init();
 	fatfs_sd_card();
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC,ENABLE);
 	GUI_Init();

  	s32 task_id;
 	task_id = VOSTaskCreate(emwindemo_task, 0, emwindemo_stack, sizeof(emwindemo_stack), TASK_PRIO_NORMAL, "emwindemo_task");
}
