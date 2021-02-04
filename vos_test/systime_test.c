#include "vos.h"

void VOSTaskDelay(u32 ms);
void VOSDelayUs(u32 us);
u32 VOSGetTicks();
u32 VOSGetTimeMs();
void systime_calibration()
{
	u32 cnts = 10;
	kprintf("%d秒后开始人工校准: VOSDelayUs函数\r\n", cnts);
	while (cnts--) {
		kprintf("%d秒后开始人工校准\r\n", cnts);
		VOSTaskDelay(1*1000);
	}
	kprintf("人工校准：VOSDelayUs (10 seconds) ...\r\n");
	VOSDelayUs(10*1000*1000);
	kprintf("人工校准结束：VOSDelayUs!\r\n");

	cnts = 10;
	kprintf("%d秒后开始人工校准: VOSTaskDelay函数\r\n", cnts);
	while (cnts--) {
		kprintf("%d秒后开始人工校准\r\n", cnts);
		VOSTaskDelay(1*1000);
	}
	kprintf("人工校准：VOSTaskDelay (10 seconds) ...\r\n");
	VOSTaskDelay(10*1000);
	kprintf("人工校准结束：VOSTaskDelay!\r\n");

	cnts = 5;
	kprintf("%d秒后开始自动校准: VOSGetTimeMs函数\r\n", cnts);
	while (cnts--) {
		kprintf("%d秒后开始自动校准\r\n", cnts);
		VOSTaskDelay(1*1000);
	}
	kprintf("VOSTaskDelay VS VOSGetTimeMs 校准开始 ( 10 secs)!\r\n");
	u32 mark = VOSGetTimeMs();
	VOSTaskDelay(10*1000);
	kprintf("VOSDelayUs(10 secs), VOSGetTimeMs(%u)!\r\n", VOSGetTimeMs()-mark);

	kprintf("VOSDelayUs VS VOSGetTimeMs 校准开始 ( 10 secs)!\r\n");
	mark = VOSGetTimeMs();
	VOSDelayUs(10*1000*1000);
	kprintf("VOSDelayUs(10 secs), VOSGetTimeMs(%u)!\r\n", VOSGetTimeMs()-mark);

	kprintf("所有校准结束!\r\n");
}
