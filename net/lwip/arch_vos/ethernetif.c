#if 1
#include "stm32f4xx_hal.h"
#include "ethernetif.h"
#include "lan8720.h"

#include "netif/etharp.h"  
#include "string.h"  

extern ETH_HandleTypeDef ETH_Handler;

//#define ETH_MAC "\x08\x00\x00\x22\x33\x44"
 
#if defined   (__CC_ARM) /*!< ARM Compiler */
  __align(4)
   ETH_DMADESCTypeDef  DMARxDscrTab[ETH_RXBUFNB];/* Ethernet Rx MA Descriptor */
  __align(4)
   ETH_DMADESCTypeDef  DMATxDscrTab[ETH_TXBUFNB];/* Ethernet Tx DMA Descriptor */
  __align(4)
   uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE]; /* Ethernet Receive Buffer */
  __align(4)
   uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE]; /* Ethernet Transmit Buffer */

#elif defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4
   ETH_DMADESCTypeDef  DMARxDscrTab[ETH_RXBUFNB];/* Ethernet Rx MA Descriptor */
  #pragma data_alignment=4
   ETH_DMADESCTypeDef  DMATxDscrTab[ETH_TXBUFNB];/* Ethernet Tx DMA Descriptor */
  #pragma data_alignment=4
   uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE]; /* Ethernet Receive Buffer */
  #pragma data_alignment=4
   uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE]; /* Ethernet Transmit Buffer */

#elif defined (__GNUC__) /*!< GNU Compiler */
	ETH_DMADescTypeDef  DMARxDscrTab[ETH_RXBUFNB] __attribute__ ((aligned (4))); /* Ethernet Rx DMA Descriptor */
	ETH_DMADescTypeDef  DMATxDscrTab[ETH_TXBUFNB] __attribute__ ((aligned (4))); /* Ethernet Tx DMA Descriptor */
	uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __attribute__ ((aligned (4))); /* Ethernet Receive Buffer */
	uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __attribute__ ((aligned (4))); /* Ethernet Transmit Buffer */

#elif defined  (__TASKING__) /*!< TASKING Compiler */
  __align(4)
   ETH_DMADESCTypeDef  DMARxDscrTab[ETH_RXBUFNB];/* Ethernet Rx MA Descriptor */
  __align(4)
   ETH_DMADESCTypeDef  DMATxDscrTab[ETH_TXBUFNB];/* Ethernet Tx DMA Descriptor */
  __align(4)
   uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE]; /* Ethernet Receive Buffer */
  __align(4)
   uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE]; /* Ethernet Transmit Buffer */

#endif /* __CC_ARM */


static err_t low_level_init(struct netif *netif)
{
#if 0
#ifdef CHECKSUM_BY_HARDWARE
	int i; 
#endif 
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	memcpy(netif->hwaddr, ETH_MAC, ETHARP_HWADDR_LEN);
	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;
	
	ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr);
	ETH_DMATxDescChainInit(DMATxDscrTab, Tx_Buff, ETH_TXBUFNB);
	ETH_DMARxDescChainInit(DMARxDscrTab, Rx_Buff, ETH_RXBUFNB);

#ifdef CHECKSUM_BY_HARDWARE 	//使用硬件帧校验
	for(i=0;i<ETH_TXBUFNB;i++)	//使能TCP,UDP和ICMP的发送帧校验,TCP,UDP和ICMP的接收帧校验在DMA中配置了
	{
		ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i], ETH_DMATxDesc_ChecksumTCPUDPICMPFull);
	}
#endif
	ETH_Start(); //开启MAC和DMA
#else
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	memcpy(netif->hwaddr, ETH_MAC, ETHARP_HWADDR_LEN);
	netif->mtu=1500;

	netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;

    HAL_ETH_DMATxDescListInit(&ETH_Handler,DMATxDscrTab,Tx_Buff,ETH_TXBUFNB);
    HAL_ETH_DMARxDescListInit(&ETH_Handler,DMARxDscrTab,Rx_Buff,ETH_RXBUFNB);
	HAL_ETH_Start(&ETH_Handler);
	return ERR_OK;
#endif
	return ERR_OK;
} 

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
#if 0
	u8 res;
	struct pbuf *q;
	int l = 0;
	u8 *buffer = (u8 *)ETH_GetCurrentTxBuffer();
	for(q=p; q!=NULL; q=q->next)
	{
		memcpy((u8_t*)&buffer[l], q->payload, q->len);
		l = l+q->len;
	} 
	res = ETH_Tx_Packet(l);
	if(res == ETH_ERROR)return ERR_MEM;//返回错误状态
	return ERR_OK;
#else
    err_t errval;
    struct pbuf *q;
    uint8_t *buffer=(uint8_t *)(ETH_Handler.TxDesc->Buffer1Addr);
    __IO ETH_DMADescTypeDef *DmaTxDesc;
    uint32_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;

    DmaTxDesc = ETH_Handler.TxDesc;
    bufferoffset = 0;

    for(q=p;q!=NULL;q=q->next)
    {
        if((DmaTxDesc->Status&ETH_DMATXDESC_OWN)!=(uint32_t)RESET)
        {
            errval=ERR_USE;
            goto error;
        }
        byteslefttocopy=q->len;
        payloadoffset=0;

        while((byteslefttocopy+bufferoffset)>ETH_TX_BUF_SIZE )
        {
            memcpy((uint8_t*)((uint8_t*)buffer+bufferoffset),(uint8_t*)((uint8_t*)q->payload+payloadoffset),(ETH_TX_BUF_SIZE-bufferoffset));

            DmaTxDesc=(ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);

            if((DmaTxDesc->Status&ETH_DMATXDESC_OWN)!=(uint32_t)RESET)
            {
                errval = ERR_USE;
                goto error;
            }
            buffer=(uint8_t *)(DmaTxDesc->Buffer1Addr);
            byteslefttocopy=byteslefttocopy-(ETH_TX_BUF_SIZE-bufferoffset);
            payloadoffset=payloadoffset+(ETH_TX_BUF_SIZE-bufferoffset);
            framelength=framelength+(ETH_TX_BUF_SIZE-bufferoffset);
            bufferoffset=0;
        }

        memcpy( (uint8_t*)((uint8_t*)buffer+bufferoffset),(uint8_t*)((uint8_t*)q->payload+payloadoffset),byteslefttocopy );
        bufferoffset=bufferoffset+byteslefttocopy;
        framelength=framelength+byteslefttocopy;
    }

    HAL_ETH_TransmitFrame(&ETH_Handler,framelength);
    errval = ERR_OK;
error:

    if((ETH_Handler.Instance->DMASR&ETH_DMASR_TUS)!=(uint32_t)RESET)
    {

        ETH_Handler.Instance->DMASR = ETH_DMASR_TUS;

        ETH_Handler.Instance->DMATPDR=0;
    }
    return errval;
#endif
} 

static struct pbuf * low_level_input(struct netif *netif)
{  
#if 0
	struct pbuf *p, *q;
	u16_t len;
	int l =0;
	FrameTypeDef frame;
	u8 *buffer;
	p = NULL;
	frame = ETH_Rx_Packet();
	len = frame.length;
	buffer = (u8 *)frame.buffer;
	p = pbuf_alloc(PBUF_RAW,len,PBUF_POOL);
	if(p) {
		for(q=p; q!=NULL; q=q->next) {
			memcpy((u8_t*)q->payload,(u8_t*)&buffer[l], q->len);
			l = l+q->len;
		}    
	}
	frame.descriptor->Status = ETH_DMARxDesc_OWN;	//设置Rx描述符OWN位,buffer重归ETH DMA
	if((ETH->DMASR&ETH_DMASR_RBUS) != (u32)RESET) {	//当Rx Buffer不可用位(RBUS)被设置的时候,重置它.恢复传输
		ETH->DMASR = ETH_DMASR_RBUS;				//重置ETH DMA RBUS位
		ETH->DMARPDR = 0;							//恢复DMA接收
	}
	return p;
#else
	struct pbuf *p = NULL;
	    struct pbuf *q;
	    uint16_t len;
	    uint8_t *buffer;
	    __IO ETH_DMADescTypeDef *dmarxdesc;
	    uint32_t bufferoffset=0;
	    uint32_t payloadoffset=0;
	    uint32_t byteslefttocopy=0;
	    uint32_t i=0;

	    if(HAL_ETH_GetReceivedFrame(&ETH_Handler)!=HAL_OK)
	    return NULL;

	    len=ETH_Handler.RxFrameInfos.length;
	    buffer=(uint8_t *)ETH_Handler.RxFrameInfos.buffer;

	    if(len>0) p=pbuf_alloc(PBUF_RAW,len,PBUF_POOL);
	    if(p!=NULL)
	    {
	        dmarxdesc=ETH_Handler.RxFrameInfos.FSRxDesc;
	        bufferoffset = 0;
	        for(q=p;q!=NULL;q=q->next)
	        {
	            byteslefttocopy=q->len;
	            payloadoffset=0;

	            while((byteslefttocopy+bufferoffset)>ETH_RX_BUF_SIZE )
	            {

	                memcpy((uint8_t*)((uint8_t*)q->payload+payloadoffset),(uint8_t*)((uint8_t*)buffer+bufferoffset),(ETH_RX_BUF_SIZE-bufferoffset));

	                dmarxdesc=(ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);

	                buffer=(uint8_t *)(dmarxdesc->Buffer1Addr);

	                byteslefttocopy=byteslefttocopy-(ETH_RX_BUF_SIZE-bufferoffset);
	                payloadoffset=payloadoffset+(ETH_RX_BUF_SIZE-bufferoffset);
	                bufferoffset=0;
	            }
	            memcpy((uint8_t*)((uint8_t*)q->payload+payloadoffset),(uint8_t*)((uint8_t*)buffer+bufferoffset),byteslefttocopy);
	            bufferoffset=bufferoffset+byteslefttocopy;
	        }
	    }
	    dmarxdesc=ETH_Handler.RxFrameInfos.FSRxDesc;
	    for(i=0;i<ETH_Handler.RxFrameInfos.SegCount; i++)
	    {
	        dmarxdesc->Status|=ETH_DMARXDESC_OWN;
	        dmarxdesc=(ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
	    }
	    ETH_Handler.RxFrameInfos.SegCount =0;
	    if((ETH_Handler.Instance->DMASR&ETH_DMASR_RBUS)!=(uint32_t)RESET)
	    {
	        ETH_Handler.Instance->DMASR = ETH_DMASR_RBUS;
	        ETH_Handler.Instance->DMARPDR=0;
	    }
	    return p;
#endif
} 

err_t ethernetif_input(struct netif *netif)
{
#if 0
	err_t err;
	struct pbuf *p;
	p=low_level_input(netif);
	if(p==NULL) return ERR_MEM;
	err=netif->input(p, netif);
	if(err!=ERR_OK)
	{
		LWIP_DEBUGF(NETIF_DEBUG,("ethernetif_input: IP input error\n"));
		pbuf_free(p);
		p = NULL;
	} 
	return err;
#else
	err_t err;
	struct pbuf *p;
	p=low_level_input(netif);
	if(p==NULL) return ERR_MEM;
	err=netif->input(p, netif);
	if(err!=ERR_OK)
	{
		LWIP_DEBUGF(NETIF_DEBUG,("ethernetif_input: IP input error\n"));
		pbuf_free(p);
		p = NULL;
	}
	return err;
#endif
} 

err_t ethernetif_init(struct netif *netif)
{
#if LWIP_NETIF_HOSTNAME
	netif->hostname = " vos_lwip";
#endif 
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	netif->output = etharp_output;
	netif->linkoutput = low_level_output;
	low_level_init(netif);
	return ERR_OK;
}
#endif
