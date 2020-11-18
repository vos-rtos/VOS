#ifndef __USBH_UDISK_H__
#define __USBH_UDISK_H__

#include "usbh_core.h"
#include "usbh_msc.h"

#include "usbd_uart.h"
#include "usbd_def.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "fatfs.h"
#include "usb_host.h"


u32 __CUSTOM_DirectWriteAT(u8 *pBuf, u32 dwLen, u32 dwTimeout);
u32 __CUSTOM_DirectReadAT(u8 *pBuf, u32 dwLen, u32 dwTimeout);

s32 usbh_udisk_do_status(s32 status);
s32 usbh_udisk_init();
StUsbHostApp *usbh_udisk_ptr();

#endif
