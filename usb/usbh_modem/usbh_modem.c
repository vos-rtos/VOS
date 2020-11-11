#include "vos.h"
#include "usbh_custom.h"

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

StUsbModem gUsbModem;

s32 USBH_SetHostName(USBH_HandleTypeDef *pHandle, char *pName)
{
	kprintf("USBH_SetHostName!\r\n");
    return (0);
}

void USBH_SetCUSTOM(USBH_HandleTypeDef *pHost)
{
    gUsbModem.pHandle = pHost;
    gUsbModem.curMode = STOP;
}

// ============================================================================
// ���ܣ�ע��USB�豸
// ������
// ���أ�
// ��ע��
// ============================================================================
void USBH_ResetCUSTOM(USBH_HandleTypeDef *pHost)
{
    if(pHost == gUsbModem.pHandle)
    {
    	gUsbModem.pHandle = NULL;
    	gUsbModem.curMode = STOP;
    }
}

u32 __CUSTOM_DirectWriteAT(u8 *pBuf, u32 dwLen, u32 dwTimeout)
{
    USBH_HandleTypeDef *host = gUsbModem.pHandle;
    u32 len = dwLen;

    USBH_CUSTOM_AT_Transmit(host, pBuf, &len, dwTimeout);

    return (dwLen - len);
}

// ============================================================================
// ���ܣ�ֱ�ӷ���ATͨ��������
// ������pBuf -- �����壻dwLen -- �����ȣ�dwTimeout -- ����ʱ��
// ���أ�ʵ�ʶ����ȡ�
// ��ע��
// ============================================================================
u32 __CUSTOM_DirectReadAT(u8 *pBuf, u32 dwLen, u32 dwTimeout)
{
    USBH_HandleTypeDef *host = gUsbModem.pHandle;
    u32 len = dwLen;

    USBH_CUSTOM_AT_Receive(host, pBuf, &len, dwTimeout);

    return (dwLen - len);
}
