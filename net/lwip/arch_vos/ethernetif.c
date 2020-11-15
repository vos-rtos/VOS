#if 1
#include "stm32f4xx_hal.h"
#include "ethernetif.h"
#include "lan8720.h"

#include "netif/etharp.h"  
#include "string.h"  

extern ETH_HandleTypeDef ETH_Handler;


ETH_DMADescTypeDef  DMARxDscrTab[ETH_RXBUFNB] __attribute__ ((aligned (4))); /* Ethernet Rx DMA Descriptor */
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TXBUFNB] __attribute__ ((aligned (4))); /* Ethernet Tx DMA Descriptor */
uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __attribute__ ((aligned (4))); /* Ethernet Receive Buffer */
uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __attribute__ ((aligned (4))); /* Ethernet Transmit Buffer */



static err_t low_level_init(struct netif *netif)
{
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	memcpy(netif->hwaddr, ETH_MAC, ETHARP_HWADDR_LEN);
	netif->mtu=1500;

	netif->flags = NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;

    HAL_ETH_DMATxDescListInit(&ETH_Handler,DMATxDscrTab,Tx_Buff,ETH_TXBUFNB);
    HAL_ETH_DMARxDescListInit(&ETH_Handler,DMARxDscrTab,Rx_Buff,ETH_RXBUFNB);
	HAL_ETH_Start(&ETH_Handler);
	return ERR_OK;
} 

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
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

} 

static struct pbuf * low_level_input(struct netif *netif)
{  
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
} 

err_t ethernetif_input(struct netif *netif)
{
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
