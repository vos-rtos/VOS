#include "vos.h"
#include "usbh_custom.h"

enum
{
    NORMAL, // 直接访问模式，不存在USB睡眠逻辑
    SLEEP, // 睡眠模式，即读逻辑通过专用的睡眠线程实现
    STOP
};

typedef struct StUsbModem
{
    USBH_HandleTypeDef *pHandle;
    u32 dwJobs; // 允许多个任务同时进行
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
// 功能：注销USB设备
// 参数：
// 返回：
// 备注：
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
// 功能：直接访问AT通道（读）
// 参数：pBuf -- 读缓冲；dwLen -- 读长度；dwTimeout -- 读超时。
// 返回：实际读长度。
// 备注：
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
// 功能：通过DEBUG接口发送数据
// 参数：pBuf -- 命令；dwLen -- 命令字节长；dwOffset（未用）；dwTimeout -- 等待完成时间。
// 返回：实际发送字节数；
// 备注：dwTimeout为零，则采用默认的等待时间。
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
// 功能：通过DEBUG接口接收数据
// 参数：pBuf -- 数据缓冲；dwLen -- 数据字节长；dwOffset（未用）；dwTimeout -- 等待完成时间。
// 返回：实际接收字节数；
// 备注：dwTimeout为零，则采用默认的等待时间。
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


