#ifndef __USBH_MODEM_H__
#define __USBH_MODEM_H__

#include "vos.h"
#include "usbh_custom.h"
#include "usb_host.h"
enum
{
    NORMAL, // ֱ�ӷ���ģʽ��������USB˯���߼�
    SLEEP, // ˯��ģʽ�������߼�ͨ��ר�õ�˯���߳�ʵ��
    STOP
};

typedef struct StUsbModem
{
    USBH_HandleTypeDef *pHandle;
    u32 dwJobs; // ����������ͬʱ����
    u32 dwSleepCycle;
    u32 dwWakeCycle;
    u32 dwBackup;
    void *pSleep;
    void *pLock;
    enum {
        TASK_OFF,
        TASK_ON,
        BLOCK
    }status;
    s32 toMode;
    s32 curMode;
}StUsbModem;


s32 USBH_SetHostName(USBH_HandleTypeDef *pHandle, char *pName);

void USBH_SetCUSTOM(USBH_HandleTypeDef *pHost);

void USBH_ResetCUSTOM(USBH_HandleTypeDef *pHost);

u32 __CUSTOM_DirectWriteAT(u8 *pBuf, u32 dwLen, u32 dwTimeout);

u32 __CUSTOM_DirectReadAT(u8 *pBuf, u32 dwLen, u32 dwTimeout);

s32 CUSTOM_WriteMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);

s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);

s32 CUSTOM_WriteDEBUG(u8 *pBuf, u32 dwLen, u32 dwTimeout);

s32 CUSTOM_ReadDEBUG(u8 *pBuf, u32 dwLen, u32 dwTimeout);

s32 CUSTOM_DoStatus(s32 status);


StUsbHostApp *usbh_modem_ptr();

#endif


