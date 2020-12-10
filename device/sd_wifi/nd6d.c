#include <lwip/icmp6.h>
#include <lwip/inet_chksum.h>
#include <lwip/mld6.h>
#include <lwip/prot/nd6.h>
#include <lwip/raw.h>
#include <string.h>
#include "nd6d.h"

#if LWIP_IPV6 && LWIP_RAW
struct ra_options
{
  struct lladdr_option lladdr;
  struct mtu_option mtu;
  struct prefix_option prefix;
  struct rdnss_option dns;
};

static u8_t nd6d_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr);

static struct nd6d_state *nd6d_list;

// 如果这个函数返回0, 则不需要释放p
// 如果返回1, 则必须释放p
static u8_t nd6d_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
  err_t err;
  int ralen;
  ip6_addr_t prefix;
  struct ip6_hdr *ip6hdr = p->payload;
  struct lladdr_option *lladdr_opt;
  struct nd6d_state *state = arg;
  struct pbuf *q;
  struct ra_header *rahdr;
  struct rs_header *rshdr = (struct rs_header *)(ip6hdr + 1);
  struct ra_options *raopts;
  
  if (p->next != NULL)
  {
    LWIP_DEBUGF(ND6D_DEBUG, ("%s: ignoring a chained packet\n", __FUNCTION__));
    return 0;
  }
  else if (p->len < sizeof(struct ip6_hdr) + sizeof(struct rs_header))
  {
    LWIP_DEBUGF(ND6D_DEBUG, ("%s: ignoring an incomplete packet\n", __FUNCTION__));
    return 0;
  }
  
  // 检查报文类型是否为RS报文
  LWIP_DEBUGF(ND6D_DEBUG, ("%s: received an ICMP packet of type %d from %s\n", __FUNCTION__, rshdr->type, ipaddr_ntoa(addr)));
  if (rshdr->type != ICMP6_TYPE_RS)
    return 0;
  LWIP_DEBUGF(ND6D_DEBUG, ("%s: received router solicitation\n", __FUNCTION__));
  if (!ip6_addr_islinklocal(ip_2_ip6(addr)))
  {
    LWIP_DEBUGF(ND6D_DEBUG, ("%s: source IPv6 address is not link-local\n", __FUNCTION__));
    return 0;
  }
  
  // 客户端MAC地址
  lladdr_opt = (struct lladdr_option *)(rshdr + 1);
  if (p->len >= sizeof(struct ip6_hdr) + sizeof(struct rs_header) + sizeof(struct lladdr_option))
  {
    if (lladdr_opt->type == ND6_OPTION_TYPE_SOURCE_LLADDR && lladdr_opt->length == 1)
      LWIP_DEBUGF(ND6D_DEBUG, ("%s: source link-layer address is %02X:%02X:%02X:%02X:%02X:%02X\n", __FUNCTION__, 
        lladdr_opt->addr[0], lladdr_opt->addr[1], lladdr_opt->addr[2], lladdr_opt->addr[3], lladdr_opt->addr[4], lladdr_opt->addr[5]));
  }
  
  // 生成网络前缀
  if (state->netif->ip6_autoconfig_enabled)
  {
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: autoconfig should not be enabled!\n", __FUNCTION__));
    return 0;
  }
  else if (!ip6_addr_isvalid(netif_ip6_addr_state(state->netif, 1)))
  {
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: netif IPv6 address 1 is not valid!\n", __FUNCTION__));
    return 0;
  }
  prefix = *netif_ip6_addr(state->netif, 1);
  prefix.addr[2] = 0;
  prefix.addr[3] = 0;
  
  // 发送RA回应报文
  ralen = sizeof(struct ra_header) + sizeof(struct ra_options);
#if !ND6D_ALWAYS_INCLUDE_DNS
  if (ip6_addr_isany_val(state->config.dns_server))
    ralen -= sizeof(struct rdnss_option);
#endif
  q = pbuf_alloc(PBUF_IP, ralen, PBUF_RAM);
  if (q == NULL)
  {
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: pbuf_alloc() failed!\n", __FUNCTION__));
    return 0;
  }
  LWIP_ASSERT("q->next == NULL", q->next == NULL);
  memset(q->payload, 0, q->len);
  
  rahdr = q->payload;
  rahdr->type = ICMP6_TYPE_RA;
  rahdr->current_hop_limit = IP_DEFAULT_TTL;
  rahdr->flags = ND6_RA_PREFERENCE_HIGH;
  rahdr->router_lifetime = PP_HTONS(3600);
  
  raopts = (struct ra_options *)(rahdr + 1);
  raopts->lladdr.type = ND6_OPTION_TYPE_SOURCE_LLADDR;
  raopts->lladdr.length = 1;
  memcpy(raopts->lladdr.addr, state->netif->hwaddr, state->netif->hwaddr_len);
  
  raopts->mtu.type = ND6_OPTION_TYPE_MTU;
  raopts->mtu.length = 1;
  raopts->mtu.mtu = PP_HTONL(netif_mtu6(state->netif));
  
  raopts->prefix.type = ND6_OPTION_TYPE_PREFIX_INFO;
  raopts->prefix.length = 4;
  raopts->prefix.prefix_length = 64;
  raopts->prefix.flags = ND6_PREFIX_FLAG_ON_LINK | ND6_PREFIX_FLAG_AUTONOMOUS;
  raopts->prefix.valid_lifetime = PP_HTONL(3600);
  raopts->prefix.preferred_lifetime = PP_HTONL(3600);
  ip6_addr_copy_to_packed(raopts->prefix.prefix, prefix);
  
#if !ND6D_ALWAYS_INCLUDE_DNS
  if (!ip6_addr_isany_val(state->config.dns_server))
#endif
  {
    raopts->dns.type = ND6_OPTION_TYPE_RDNSS;
    raopts->dns.length = 3;
    raopts->dns.lifetime = PP_HTONL(3600);
    
#if ND6D_ALWAYS_INCLUDE_DNS
    if (ip6_addr_isany_val(state->config.dns_server))
      ip6_addr_copy_to_packed(raopts->dns.rdnss_address[0], *netif_ip6_addr(state->netif, 0));
    else
#endif
      ip6_addr_copy_to_packed(raopts->dns.rdnss_address[0], state->config.dns_server);
  }
  
#if CHECKSUM_GEN_ICMP6
  IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_ICMP6)
  {
    rahdr->chksum = ip6_chksum_pseudo(q, IP6_NEXTH_ICMP6, q->len, netif_ip6_addr(state->netif, 0), ip_2_ip6(addr));
  }
#endif
  
  err = raw_sendto(pcb, q, addr);
  if (err == ERR_OK)
    LWIP_DEBUGF(ND6D_DEBUG, ("%s: sent RA packet with prefix %s/64\n", __FUNCTION__, ip6addr_ntoa(&prefix)));
  else
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: raw_sendto() failed! err=%d\n", __FUNCTION__, err));
  pbuf_free(q);
  return 0;
}

err_t nd6d_start(struct netif *netif, const struct nd6d_config *config)
{
  err_t err, err2;
  int joined = 0;
  ip_addr_t addr;
  struct nd6d_state *state;
  
  state = mem_malloc(sizeof(struct nd6d_state));
  if (state == NULL)
  {
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: mem_malloc() failed!\n", __FUNCTION__));
    return ERR_MEM;
  }
  memset(state, 0, sizeof(struct nd6d_state));
  if (config != NULL)
    state->config = *config;
  state->netif = netif;
  
  state->rpcb = raw_new_ip_type(IPADDR_TYPE_V6, IP6_NEXTH_ICMP6);
  if (state->rpcb == NULL)
  {
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: raw_new_ip_type() failed!\n", __FUNCTION__));
    err = ERR_MEM;
    goto err;
  }
  
  IP_ADDR6_HOST(&addr, 0xff020000, 0, 0, 0x2);
  ip6_addr_assign_zone(ip_2_ip6(&addr), IP6_MULTICAST, netif); // 必须设置zone, 否则收不到数据
  
  err = mld6_joingroup_netif(netif, ip_2_ip6(&addr));
  if (err != ERR_OK)
  {
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: mld6_joingroup_netif() failed! err=%d\n", __FUNCTION__, err));
    goto err;
  }
  joined = 1;
  
  err = raw_bind(state->rpcb, &addr);
  if (err != ERR_OK)
  {
    LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: raw_bind() failed! err=%d\n", __FUNCTION__, err));
    goto err;
  }
  
  raw_bind_netif(state->rpcb, netif);
  raw_recv(state->rpcb, nd6d_recv, state);
  
  state->next = nd6d_list;
  nd6d_list = state;
  return ERR_OK;

err:
  if (state != NULL)
  {
    if (state->rpcb != NULL)
      raw_remove(state->rpcb);
    mem_free(state);
  }
  if (joined)
  {
    err2 = mld6_leavegroup_netif(netif, ip_2_ip6(&addr));
    if (err2 != ERR_OK)
      LWIP_DEBUGF(ND6D_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: mld6_leavegroup_netif() failed! err=%d\n", __FUNCTION__, err2));
  }
  return err;
}

#endif
