#include "IDWD1016b.h"

void DetectFinger()
{
	int		w_nRet, w_nDetect;

	Run_SLEDControl(1);

	Sleep(50);

	w_nRet = Run_FingerDetect(&w_nDetect);

	if (w_nRet != ERR_SUCCESS)
	{
		Run_SLEDControl(0);
		kprintf("error: %s!\r\n", GetErrorMsg(w_nRet));
		return;
	}

	Run_SLEDControl(0);

	if (w_nDetect == 0) {
		kprintf("Result : Success\r\nFinger is not detected!\r\n");
	}
	else {
		kprintf("Result : Success\r\nFinger is detected\r\n");
	}
}


void xxxx()
{
	char buf[256];
	InitConnection(2, 115200);

	Run_TestConnection();
	Run_GetDeviceInfo(buf);
	kprintf("%s\r\n", buf);

	while (1) {
		Run_GetDeviceInfo(buf);
		kprintf("%s\r\n", buf);
		//DetectFinger();
		VOSTaskDelay(1000);
	}
}
