#include "vos.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"

#include "ethernetif.h"

void Lan8720ResetPinInit();
struct netif gVosNetIf;

void lwip_packet_handler()
{
	ethernetif_input(&gVosNetIf);
}

struct netif *GetNetIfPtr()
{
	return &gVosNetIf;
}


s32 net_init()
{
  	u32 irq_save = 0;
  	struct netif *new_netif;
  	struct ip4_addr ipaddr;
  	struct ip4_addr netmask;
  	struct ip4_addr gw;

  	if (LAN8720_Init())	return -2;
  	tcpip_init(NULL, NULL);

  	ipaddr.addr 	= 0;
  	netmask.addr 	= 0;
  	gw.addr 		= 0;

  	irq_save = __vos_irq_save();
  	new_netif = netif_add(&gVosNetIf, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  	__vos_irq_restore(irq_save);
  	if(new_netif) {
  		netif_set_default(&gVosNetIf);
  		netif_set_up(&gVosNetIf);
  	}
}

s32 DhcpClientCheck(struct netif *pNetIf)
{
	return !!pNetIf->ip_addr.addr;
}

void NetAddrInfoPrt(struct netif *pNetIf)
{
	u8 ip[4];
	u8 netmask[4];
	u8 gateway[4];
	if (pNetIf) {
		kprintf("MAC地址:  %02x-%02x-%02x-%02x-%02x-%02x\r\n",
				pNetIf->hwaddr[0], pNetIf->hwaddr[1], pNetIf->hwaddr[2],
				pNetIf->hwaddr[3], pNetIf->hwaddr[4], pNetIf->hwaddr[5]);

		*(u32*)ip = pNetIf->ip_addr.addr;
		kprintf("IP地址:  %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);

		*(u32*)netmask = pNetIf->netmask.addr;
		kprintf("子网掩码: %d.%d.%d.%d\r\n",
				netmask[0], netmask[1], netmask[2], netmask[3]);

		*(u32*)gateway = pNetIf->gw.addr;
		kprintf("默认网关: %d.%d.%d.%d\r\n",
				gateway[0], gateway[1], gateway[2], gateway[3]);
	}
}

