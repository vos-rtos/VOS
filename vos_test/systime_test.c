#include "vos.h"

void VOSTaskDelay(u32 ms);
void VOSDelayUs(u32 us);
u32 VOSGetTicks();
u32 VOSGetTimeMs();
void systime_calibration()
{
	u32 cnts = 10;
	kprintf("%d���ʼ�˹�У׼: VOSDelayUs����\r\n", cnts);
	while (cnts--) {
		kprintf("%d���ʼ�˹�У׼\r\n", cnts);
		VOSTaskDelay(1*1000);
	}
	kprintf("�˹�У׼��VOSDelayUs (10 seconds) ...\r\n");
	VOSDelayUs(10*1000*1000);
	kprintf("�˹�У׼������VOSDelayUs!\r\n");

	cnts = 10;
	kprintf("%d���ʼ�˹�У׼: VOSTaskDelay����\r\n", cnts);
	while (cnts--) {
		kprintf("%d���ʼ�˹�У׼\r\n", cnts);
		VOSTaskDelay(1*1000);
	}
	kprintf("�˹�У׼��VOSTaskDelay (10 seconds) ...\r\n");
	VOSTaskDelay(10*1000);
	kprintf("�˹�У׼������VOSTaskDelay!\r\n");

	cnts = 5;
	kprintf("%d���ʼ�Զ�У׼: VOSGetTimeMs����\r\n", cnts);
	while (cnts--) {
		kprintf("%d���ʼ�Զ�У׼\r\n", cnts);
		VOSTaskDelay(1*1000);
	}
	kprintf("VOSTaskDelay VS VOSGetTimeMs У׼��ʼ ( 10 secs)!\r\n");
	u32 mark = VOSGetTimeMs();
	VOSTaskDelay(10*1000);
	kprintf("VOSDelayUs(10 secs), VOSGetTimeMs(%u)!\r\n", VOSGetTimeMs()-mark);

	kprintf("VOSDelayUs VS VOSGetTimeMs У׼��ʼ ( 10 secs)!\r\n");
	mark = VOSGetTimeMs();
	VOSDelayUs(10*1000*1000);
	kprintf("VOSDelayUs(10 secs), VOSGetTimeMs(%u)!\r\n", VOSGetTimeMs()-mark);

	kprintf("����У׼����!\r\n");
}
