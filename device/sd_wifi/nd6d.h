#ifndef _ND6D_H
#define _ND6D_H

#if LWIP_IPV6 && LWIP_RAW
#ifndef ND6D_DEBUG
#define ND6D_DEBUG LWIP_DBG_OFF
#endif

#ifndef ND6D_ALWAYS_INCLUDE_DNS
#define ND6D_ALWAYS_INCLUDE_DNS 1 // 如果没有在config结构体中配置DNS地址, 则DNS地址为接口地址
#endif

struct nd6d_config
{
  ip6_addr_t dns_server;
};

struct nd6d_state
{
  struct nd6d_config config;
  struct raw_pcb *rpcb;
  struct netif *netif;
  struct nd6d_state *next;
};

err_t nd6d_start(struct netif *netif, const struct nd6d_config *config);

#endif
#endif
