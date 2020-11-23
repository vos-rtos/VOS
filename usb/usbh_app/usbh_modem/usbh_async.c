
#include "usbh_def.h"
#include "vringbuf.h"

#include "vos.h"

#define NULL 0
#define BUFFER_SIZE                128

USBH_StatusTypeDef __USBH_LL_EnAsync(USBH_HandleTypeDef *pHost, uint8_t pipe);
USBH_StatusTypeDef __USBH_LL_DisAsync(USBH_HandleTypeDef *pHost, uint8_t pipe);

typedef struct StChannel{
    struct StVOSStVOSRingBuf *pRing;
    struct channel *pNext;
    u8 bPipe;
    struct StVOSSemaphore *pSemp;
}StChannel;

StChannel *gChannels;

static StChannel *__FindChannel(u8 bPipe)
{
	StChannel *pChannel = gChannels;

    for(;;)
    {
        if(!pChannel)
            return (NULL); // 结束，未找到

        if(bPipe == pChannel->bPipe)
            return (pChannel); // 找到

        pChannel = pChannel->pNext;
    }

    return (NULL);
}

static void __AddChannel(StChannel *pChannel)
{

    if(!gChannels)
    {
    	gChannels = pChannel; // 队列本身为空，则将本成员作为第一个
    }
    else
    {
        // 将本成员插入第一个的后面
    	pChannel->pNext = gChannels->pNext;
        gChannels->pNext = pChannel;
    }
}

static StChannel *__DelChannel(u8 bPipe)
{
	StChannel *pChannel = gChannels;
	StChannel *pre = NULL;

    for(;;)
    {
        if(!pChannel)
            return (NULL); // 结束，未找到

        if(bPipe == pChannel->bPipe)
        {
            if(pre)
                pre->pNext = pChannel->pNext; // 将本成员从队列中剔除
            else
            	gChannels = pChannel->pNext; // 本成员是队列中第一个

            return (pChannel);
        }

        pre = pChannel;
        pChannel = pChannel->pNext;
    }

    return (NULL);
}

static StChannel *__CreateChannel(u8 bPipe)
{
    //u32 size;
    StChannel *res;
    struct StVOSRingBuf *ring = NULL;
    u8 *space = NULL;
    struct StVOSSemaphore *semp = NULL;

    res = vmalloc(sizeof(StChannel));
    if(!res) {
        goto FAIL;
    }
    memset(res, 0x0, sizeof(StChannel));

    ring = vmalloc(sizeof(struct StVOSRingBuf)+BUFFER_SIZE);
    if(!ring)
        goto FAIL;

    semp = VOSSemCreate(1, 0, "usb_channel");
    if(!semp) {
        goto FAIL;
    }
    res->pRing = VOSRingBufBuild(ring, sizeof(struct StVOSRingBuf)+BUFFER_SIZE);
    res->bPipe = bPipe;
    res->pSemp = semp;

    return (res);

FAIL:
    if(res)
        vfree(res);

    if(ring)
        vfree(ring);

    if(space)
        vfree(space);

    if(semp)
    	VOSSemRelease(semp);

    return (NULL);
}

static void __DestroyChannel(StChannel *pChannel)
{
    if(pChannel->pRing)
    {
       vfree(pChannel->pRing);
    }
    VOSSemDelete(pChannel->pSemp);
    vfree(pChannel);
}


USBH_StatusTypeDef USBH_LL_EnAsync(USBH_HandleTypeDef *pHost, uint8_t pipe)
{

    HCD_HandleTypeDef *hhcd = pHost->pData;

    hhcd->hc[pipe].async = 1;

    return (USBH_OK);
}


USBH_StatusTypeDef USBH_LL_DisAsync(USBH_HandleTypeDef *pHost, uint8_t pipe)
{
    HCD_HandleTypeDef *hhcd = pHost->pData;

    hhcd->hc[pipe].async = 0;

    return (USBH_OK);
}

s32 USBH_OpenAsync(USBH_HandleTypeDef *pHost, u8 bPipe)
{
	StChannel *channel;

    channel = __FindChannel(bPipe);
    if(channel)
        return (0); // pipe已经存在

    channel = __CreateChannel(bPipe);
    if(!channel)
        return (-1);

    __AddChannel(channel);

    USBH_LL_EnAsync(pHost, bPipe);
    return (0);
}

s32 USBH_CloseAsync(USBH_HandleTypeDef *pHost, u8 bPipe)
{
	StChannel *channel;

    channel = __DelChannel(bPipe);
    if(!channel)
        return (0); // pipe不存在

    USBH_LL_DisAsync(pHost, bPipe);
    __DestroyChannel(channel);
    return (0);
}

void USBH_Store(u8 bPipe, u8 *pData, u32 dwLen)
{
	StChannel *pChannel;
    u32 res;

    pChannel = __FindChannel(bPipe);
    if(!pChannel)
        return; // pipe不存在
    res = VOSRingBufSet(pChannel->pRing, pData, dwLen);
    if(res != dwLen)
    {
        USBH_UsrLog("\r\nUSB Module : error : channel %d buffer overflow.\r\n", bPipe);
    }
    VOSSemRelease(pChannel->pSemp);
}

u32 USBH_Fetch(u8 bPipe, u8 *pBuffer, u32 dwLen)
{
    u32 len;
    StChannel *pChannel;

    pChannel = __FindChannel(bPipe);
    if(!pChannel)
        return (0); // pipe不存在
    len = VOSRingBufGet(pChannel->pRing, pBuffer, dwLen);
    if(!VOSRingBufIsEmpty(pChannel->pRing)) {
    	VOSSemRelease(pChannel->pSemp);
    }
    return (len);
}


void USBH_Wait(u8 bPipe, u32 dwTimeout)
{
	StChannel *pChannel;

	pChannel = __FindChannel(bPipe);
    if(!pChannel)
        return ;

    VOSSemWait(pChannel->pSemp, dwTimeout);
}

