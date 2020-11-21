#include "vconf.h"
#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_audio.h"
#include "usbh_cdc.h"
#include "usbh_msc.h"
#include "usbh_hid.h"
#include "usbh_mtp.h"
#include "usbh_custom.h"
#include "vos.h"
#include "usbh_modem.h"
#include "usbh_udisk.h"

USBH_HandleTypeDef hUsbHostFS;


struct  StUsbHostApp *arr_usbh_app_ptr[USBH_MAX_NUM_SUPPORTED_CLASS];

void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

StUsbHostApp *UsbhAppGet(USBH_ClassTypeDef *pActClass)
{
	s32 i = 0;
	for (i=0; i<sizeof(arr_usbh_app_ptr)/sizeof(arr_usbh_app_ptr[0]); i++){
		if (arr_usbh_app_ptr[i]->pclass == pActClass){
			return arr_usbh_app_ptr[i];
		}
	}
	return 0;
}

s32 RegisterUsbhApp(StUsbHostApp *pApp)
{
	s32 ret = -1;
	s32 i = 0;
	if (pApp == 0) return ret;

	for (i=0; i<sizeof(arr_usbh_app_ptr)/sizeof(arr_usbh_app_ptr[0]); i++){
		if (arr_usbh_app_ptr[i] == pApp) {
			ret = 0; //ÒÑ¾­×¢²á¹ý
			break;
		}
		if (arr_usbh_app_ptr[i] == 0 && pApp) {
			arr_usbh_app_ptr[i] = pApp;
			ret = 0;
			break;
		}
	}
	return ret;
}


void USBH_UserProcess  (USBH_HandleTypeDef *phost, uint8_t id)
{

	StUsbHostApp *pUsbhApp = UsbhAppGet(phost->pActiveClass);
	if (pUsbhApp == 0) return;

	switch(id)
	{
		case HOST_USER_SELECT_CONFIGURATION:
			pUsbhApp->status = APP_STATUS_START;
			if (pUsbhApp->pfStateDo) {
				pUsbhApp->pfStateDo(pUsbhApp->status);
			}
			kprintf("USBH_UserProcess->HOST_USER_SELECT_CONFIGURATION!\r\n");
			break;

		case HOST_USER_CLASS_SELECTED:
			pUsbhApp->status = APP_STATUS_READY;
			if (pUsbhApp->pfStateDo) {
				pUsbhApp->pfStateDo(pUsbhApp->status);
			}
			kprintf("USBH_UserProcess->HOST_USER_CLASS_SELECTED!\r\n");
			break;

		case HOST_USER_DISCONNECTION:
			pUsbhApp->status = APP_STATUS_DISCONNECT;
			if (pUsbhApp->pfStateDo) {
				pUsbhApp->pfStateDo(pUsbhApp->status);
			}
			pUsbhApp->status = APP_STATUS_IDLE;
			kprintf("USBH_UserProcess->HOST_USER_DISCONNECTION!\r\n");
			break;

		case HOST_USER_CLASS_ACTIVE:
			pUsbhApp->status = APP_STATUS_READY;
			if (pUsbhApp->pfStateDo) {
				pUsbhApp->pfStateDo(pUsbhApp->status);
			}
			kprintf("USBH_UserProcess->HOST_USER_CLASS_ACTIVE!\r\n");
			break;

		case HOST_USER_CONNECTION:
			pUsbhApp->status = APP_STATUS_START;
			if (pUsbhApp->pfStateDo) {
				pUsbhApp->pfStateDo(pUsbhApp->status);
			}
			kprintf("USBH_UserProcess->HOST_USER_CONNECTION!\r\n");
			break;

		default:
			pUsbhApp->status = APP_STATUS_START;
			kprintf("USBH_UserProcess->unkonwn!\r\n");
			break;
	}

}

long long usbh_stack[1024];

void task_usbh_server(void *p)
{
	s32 i = 0;
	USBH_Start(&hUsbHostFS);
	while (1)
	{
		USBH_Process(&hUsbHostFS);
		VOSTaskDelay(10);
	}
}

void usb_host_init()
{
	s32 i = 0;
	s32 task_id;

	SetUSBWorkMode(USB_WORK_AS_HOST);

	USBH_Init(&hUsbHostFS, USBH_UserProcess, HOST_FS);

	USBH_RegisterClass(&hUsbHostFS, USBH_CUSTOM_CLASS);
	USBH_RegisterClass(&hUsbHostFS, USBH_HID_CLASS);
	USBH_RegisterClass(&hUsbHostFS, USBH_MSC_CLASS);
	USBH_RegisterClass(&hUsbHostFS, USBH_MTP_CLASS);
	USBH_RegisterClass(&hUsbHostFS, USBH_AUDIO_CLASS);
	USBH_RegisterClass(&hUsbHostFS, USBH_CDC_CLASS);

	task_id = VOSTaskCreate(task_usbh_server, 0, usbh_stack, sizeof(usbh_stack), TASK_PRIO_NORMAL, "usbh_server");

}

