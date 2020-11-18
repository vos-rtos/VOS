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

s32 CUSTOM_WriteMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout)
{
    USBH_HandleTypeDef *host = gUsbModem.pHandle;
    u8 *buf = pBuf;
    u32 len = dwLen;
    u32 timeout = dwTimeout;

    if(!host)
    {
        printf("\r\n: warn : usb    : modem device(channel) is not register.");
        return (0);
    }


    USBH_CUSTOM_MODEM_Transmit(host, buf, &len, timeout);

    return (dwLen - len);
}

s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout)
{
    USBH_HandleTypeDef *host = gUsbModem.pHandle;
    u32 len = dwLen;

    if(!host){
        return (0);
    }

	USBH_CUSTOM_MODEM_Receive(host, pBuf, &len, dwTimeout);
	return dwLen - len;
}


// ============================================================================
// ���ܣ�ͨ��DEBUG�ӿڷ�������
// ������pBuf -- ���dwLen -- �����ֽڳ���dwOffset��δ�ã���dwTimeout -- �ȴ����ʱ�䡣
// ���أ�ʵ�ʷ����ֽ�����
// ��ע��dwTimeoutΪ�㣬�����Ĭ�ϵĵȴ�ʱ�䡣
// ============================================================================
s32 CUSTOM_WriteDEBUG(u8 *pBuf, u32 dwLen, u32 dwTimeout)
{
    USBH_HandleTypeDef *host = gUsbModem.pHandle;
    u8 *command = pBuf;
    u32 len = dwLen;
    u32 timeout = dwTimeout;

    if(!host)
    {
        printf("\r\n: warn : usb: custom debug device(channel) is not register.\r\n");
        return (0);
    }
    USBH_CUSTOM_DEBUG_Transmit(host, command, &len, timeout);

    return (dwLen - len);
}

// ============================================================================
// ���ܣ�ͨ��DEBUG�ӿڽ�������
// ������pBuf -- ���ݻ��壻dwLen -- �����ֽڳ���dwOffset��δ�ã���dwTimeout -- �ȴ����ʱ�䡣
// ���أ�ʵ�ʽ����ֽ�����
// ��ע��dwTimeoutΪ�㣬�����Ĭ�ϵĵȴ�ʱ�䡣
// ============================================================================
s32 CUSTOM_ReadDEBUG(u8 *pBuf, u32 dwLen, u32 dwTimeout)
{
    USBH_HandleTypeDef *host = gUsbModem.pHandle;
    u8 *command = pBuf;
    u32 len = dwLen;
    u32 timeout = dwTimeout;

    if(!host)
    {
        printf("\r\n: warn : usb    : \"modem\" device channel is not register.");
        return (0);
    }

    USBH_CUSTOM_DEBUG_Receive(host, command, &len, timeout);

    return (dwLen - len);
}


