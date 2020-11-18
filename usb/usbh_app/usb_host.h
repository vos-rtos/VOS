#ifndef __USB_HOST__H__
#define __USB_HOST__H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "vos.h"

#include "vconf.h"

#include "usbh_def.h"


typedef enum {
  APP_STATUS_IDLE = 0,
  APP_STATUS_START,
  APP_STATUS_READY,
  APP_STATUS_DISCONNECT
};

typedef s32 (*FUN_USBH_STATE_DO)(s32 status);

typedef struct StUsbHostApp {

	USBH_ClassTypeDef* pclass;
	volatile s32 status;
	FUN_USBH_STATE_DO pfStateDo;
	s32 is_inited;
}StUsbHostApp;

s32 RegisterUsbhApp(StUsbHostApp *pApp);

#ifdef __cplusplus
}
#endif

#endif
