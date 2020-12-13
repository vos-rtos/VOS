
#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__
#include "lwip/err.h"
#include "lwip/netif.h"

#if !LWIP_IPV6

#define IFNAME0 'e'
#define IFNAME1 'n'
 
err_t ethernetif_init(struct netif *netif);
err_t ethernetif_input(struct netif *netif);
#endif

#endif

