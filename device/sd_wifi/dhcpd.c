/* Dynamic Host Configuration Protocol: https://tools.ietf.org/html/rfc2131 */
/* DHCP Options and BOOTP Vendor Extensions: https://tools.ietf.org/html/rfc2132 */
/* udhcp-0.9.8.tar.gz: https://udhcp.busybox.net/ */
#include <lwip/etharp.h>
#include <lwip/prot/dhcp.h>
#include <lwip/sys.h>
#include <lwip/timeouts.h>
#include <lwip/udp.h>
#include <string.h>
#include "dhcpd.h"

#if LWIP_IPV4 && LWIP_ARP

static void dhcpd_check_ip_status(void *arg);
static void dhcpd_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static err_t dhcpd_send_ack(struct dhcpd_state *state, struct dhcp_msg *client_packet, struct dhcpd_lease *lease);
static err_t dhcpd_send_inform(struct dhcpd_state *state, struct dhcp_msg *client_packet);
static err_t dhcpd_send_nak(struct dhcpd_state *state, struct dhcp_msg *client_packet);
static err_t dhcpd_send_offer(struct dhcpd_state *state, struct dhcp_msg *client_packet);
static void dhcpd_send_offer_callback_1(struct dhcpd_state *state, struct dhcpd_tentative_lease *tlease);
static void dhcpd_send_offer_callback_2(void *arg, struct netif *netif, const ip4_addr_t *ipaddr, int exists, const struct eth_addr *ethaddr);
static err_t dhcpd_send_offer_callback_3(struct dhcpd_state *state, struct dhcpd_tentative_lease *tlease, int succeeded);
static err_t dhcpd_send_offer_callback_4(struct dhcpd_state *state, struct pbuf *p, const ip4_addr_t *yiaddr);

static struct dhcpd_ip_status *dhcpd_ip_status; // ARP请求队列
static struct dhcpd_state *dhcpd_list;
static struct udp_pcb *dhcpd_upcb;
static uint8_t dhcpd_ip_checking;

/* 记录为客户端分配好的IP地址 */
struct dhcpd_lease *dhcpd_add_lease(struct dhcpd_state *state, const uint8_t *chaddr, const ip4_addr_t *yiaddr, uint32_t lease_time)
{
  struct dhcpd_lease *lease;
  
  dhcpd_clear_lease(state, chaddr, yiaddr);
  lease = dhcpd_find_oldest_expired_lease(state);
  if (lease == NULL)
    lease = mem_malloc(sizeof(struct dhcpd_lease));
  
  if (lease != NULL)
  {
    memset(lease, 0, sizeof(struct dhcpd_lease));
    if (chaddr != NULL)
      memcpy(lease->chaddr, chaddr, DHCP_CHADDR_LEN);
    lease->yiaddr = *yiaddr;
    lease->expiring_time = sys_now() + lease_time;
    lease->next = state->leases;
    
    if (state->leases != NULL)
      state->leases->prev = lease;
    state->leases = lease;
  }
  return lease;
}

/* 在DHCP消息中添加DHCP选项 */
int dhcpd_add_option(struct dhcp_msg *packet, uint8_t code, uint8_t len, const void *data)
{
  int i = 0;
  
  // 找到结束标记的位置
  while (packet->options[i] != DHCP_OPTION_END)
  {
    if (packet->options[i] == DHCP_OPTION_PAD)
      i++;
    else
      i += packet->options[i + 1] + 2;
  }
  
  // 检查剩余空间是否能容纳一个DHCP选项和一个结束标记
  if (i + len + 3 > DHCPD_OPTIONS_LEN)
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: Option 0x%02x did not fit into the packet!\n", __FUNCTION__, code));
    return -1;
  }
  
  // 添加选项
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: adding option 0x%02x\n", __FUNCTION__, code));
  packet->options[i] = code;
  packet->options[i + 1] = len;
  memcpy(packet->options + i + 2, data, len);
  packet->options[i + len + 2] = DHCP_OPTION_END;
  return i; // 返回添加的选项的位置
}

/* 在DHCP消息中添加常用选项 */
void dhcpd_add_common_options(struct dhcpd_state *state, struct dhcp_msg *packet)
{
  ip4_addr_t dns[2];
  uint8_t dns_cnt = 0;
  
  ip4_addr_set(&packet->siaddr, ip_2_ip4(&state->netif->ip_addr)); // DHCP服务器IP地址
  dhcpd_add_option(packet, DHCP_OPTION_SUBNET_MASK, 4, &state->netif->netmask); // 子网掩码
  dhcpd_add_option(packet, DHCP_OPTION_ROUTER, 4, &state->netif->ip_addr); // 默认网关
  
  // DNS服务器IP地址
  if (ip4_addr_isany_val(state->config.dns_servers[0]))
  {
#if DHCPD_ALWAYS_INCLUDE_DNS
    ip4_addr_set(&dns[0], &packet->siaddr);
    dns_cnt++;
#endif
  }
  else
  {
    dns[0] = state->config.dns_servers[0];
    dns_cnt++;
  }
  if (!ip4_addr_isany_val(state->config.dns_servers[1]))
  {
    dns[dns_cnt] = state->config.dns_servers[1];
    dns_cnt++;
  }
  if (dns_cnt != 0)
    dhcpd_add_option(packet, DHCP_OPTION_DNS_SERVER, dns_cnt * sizeof(ip4_addr_t), dns);
}

/* 检查网络中是否存在指定IP地址的主机 */
// 回调函数始终会被调用, 其中的参数exists: -1表示查询失败, 0表示IP未使用, 1表示IP已有主机使用
err_t dhcpd_check_ip(struct netif *netif, const ip4_addr_t *ipaddr, dhcpd_ip_callback callback, void *arg)
{
  const ip4_addr_t *ipret;
  err_t err;
  int ret;
  struct dhcpd_ip_callback_entry *m;
  struct dhcpd_ip_status *p;
  struct eth_addr ethaddr, *ethret;
  
  LWIP_ASSERT("callback != NULL", callback != NULL);
  
  if (!netif_is_link_up(netif))
  {
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: link is down\n", __FUNCTION__));
    callback(arg, netif, ipaddr, -1, NULL);
    return ERR_ARG;
  }
  
  if (ip4_addr_cmp(ipaddr, ip_2_ip4(&netif->ip_addr)))
  {
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP %s is for netif\n", __FUNCTION__, ip4addr_ntoa(ipaddr)));
    memcpy(ethaddr.addr, netif->hwaddr, ETH_HWADDR_LEN);
    callback(arg, netif, ipaddr, 1, &ethaddr);
    return ERR_OK;
  }
  
  ret = etharp_find_addr(netif, ipaddr, &ethret, &ipret);
  if (ret != -1)
  {
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP %s is in ARP cache\n", __FUNCTION__, ip4addr_ntoa(ipaddr)));
    ethaddr = *ethret;
    callback(arg, netif, ipaddr, 1, &ethaddr);
    return ERR_OK;
  }
  
  m = mem_malloc(sizeof(struct dhcpd_ip_callback_entry));
  if (m == NULL)
  {
    callback(arg, netif, ipaddr, -1, NULL);
    return ERR_MEM;
  }
  m->callback = callback;
  m->arg = arg;
  m->next = NULL;
  
  for (p = dhcpd_ip_status; p != NULL; p = p->next)
  {
    if (p->netif == netif && ip4_addr_cmp(ipaddr, &p->ipaddr))
      break;
  }
  
  if (p == NULL)
  {
    p = mem_malloc(sizeof(struct dhcpd_ip_status));
    if (p == NULL)
    {
      mem_free(m);
      callback(arg, netif, ipaddr, -1, NULL);
      return ERR_MEM;
    }
    
    p->netif = netif;
    p->ipaddr = *ipaddr;
    p->time = sys_now();
    p->retry = 3;
    
    p->callbacks = m;
    p->prev = NULL;
    p->next = dhcpd_ip_status;
    
    if (dhcpd_ip_status != NULL)
      dhcpd_ip_status->prev = p;
    dhcpd_ip_status = p;
  }
  else
  {
    m->next = p->callbacks;
    p->callbacks = m;
  }
  
  if (!dhcpd_ip_checking)
  {
    dhcpd_ip_checking = 1;
    sys_timeout(DHCPD_IPCHECK_TIMEOUT, dhcpd_check_ip_status, NULL);
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: DHCPD timer is started\n", __FUNCTION__));
  }
  
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP %s is not in ARP cache. Send ARP request!\n", __FUNCTION__, ip4addr_ntoa(ipaddr)));
  err = etharp_request(netif, ipaddr);
  if (err != ERR_OK)
  {
    callback(arg, netif, ipaddr, -1, NULL);
    return err;
  }
  
  return ERR_INPROGRESS;
}

/* 定时检查是否收到ARP回应 */
static void dhcpd_check_ip_status(void *arg)
{
  const ip4_addr_t *ipret;
  int ret;
  struct dhcpd_ip_callback_entry *m, *n;
  struct dhcpd_ip_status *p, *q;
  struct eth_addr ethaddr, *ethret;
  
  p = dhcpd_ip_status;
  while (p != NULL)
  {
    q = p->next;
    
    ret = etharp_find_addr(p->netif, &p->ipaddr, &ethret, &ipret);
    if (ret == -1)
    {
      ethret = NULL;
      p->retry--;
      if (p->retry != 0)
      {
        LWIP_DEBUGF(DHCPD_DEBUG, ("%s: Resend ARP request for IP %s! retry=%d\n", __FUNCTION__, ip4addr_ntoa(&p->ipaddr), p->retry));
        etharp_request(p->netif, &p->ipaddr);
      }
      else
        LWIP_DEBUGF(DHCPD_DEBUG, ("%s: No ARP response for IP %s!\n", __FUNCTION__, ip4addr_ntoa(&p->ipaddr)));
    }
    else
    {
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: Received ARP response for IP %s!\n", __FUNCTION__, ip4addr_ntoa(&p->ipaddr)));
      ethaddr = *ethret;
      ethret = &ethaddr;
      p->retry = 0;
    }
    
    if (p->retry == 0)
    {
      // 调用所有的回调函数
      m = p->callbacks;
      p->callbacks = NULL;
      while (m != NULL)
      {
        m->callback(m->arg, p->netif, &p->ipaddr, ethret != NULL, ethret);
        
        n = m->next;
        mem_free(m);
        m = n;
      }
      
      // 从ARP请求队列移除
      if (p->prev == NULL)
      {
        LWIP_ASSERT("dhcpd_ip_status == p", dhcpd_ip_status == p);
        dhcpd_ip_status = p->next;
      }
      else
        p->prev->next = p->next;
      if (p->next != NULL)
        p->next->prev = p->prev;
      mem_free(p);
    }
    
    p = q;
  }
  
  if (dhcpd_ip_status == NULL)
  {
    dhcpd_ip_checking = 0;
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: DHCPD timer is stopped\n", __FUNCTION__));
  }
  else
    sys_timeout(DHCPD_IPCHECK_TIMEOUT, dhcpd_check_ip_status, NULL);
}

/* 清空所有跟chaddr和yiaddr有关的记录 */
void dhcpd_clear_lease(struct dhcpd_state *state, const uint8_t *chaddr, const ip4_addr_t *yiaddr)
{
  int i;
  struct dhcpd_lease *p, *q;
  
  if (chaddr != NULL)
  {
    for (i = 0; i < DHCP_CHADDR_LEN; i++)
    {
      if (chaddr[i] != 0)
        break;
    }
    if (i == DHCP_CHADDR_LEN)
      chaddr = NULL;
  }
  
  p = state->leases;
  while (p != NULL)
  {
    if ((chaddr != NULL && memcmp(p->chaddr, chaddr, DHCP_CHADDR_LEN) == 0) || (!ip4_addr_isany(yiaddr) && ip4_addr_cmp(&p->yiaddr, yiaddr)))
    {
      // 删除p
      q = p->next;
      
      if (p->prev == NULL)
      {
        LWIP_ASSERT("state->leases == p", state->leases == p);
        state->leases = p->next;
      }
      else
        p->prev->next = p->next;
      if (p->next != NULL)
        p->next->prev = p->prev;
      mem_free(p);
      
      p = q;
    }
    else
      p = p->next;
  }
}

/* 检查chaddr是否和MAC地址匹配 */
int dhcpd_compare_chaddr(const uint8_t *chaddr, const uint8_t *mac)
{
  int i;
  
  if (chaddr == NULL || mac == NULL)
    return 0;
  
  for (i = 0; i < DHCP_CHADDR_LEN; i++)
  {
    if (i < ETH_HWADDR_LEN)
    {
      if (chaddr[i] != mac[i])
        return 0;
    }
    else
    {
      if (chaddr[i] != 0)
        return 0;
    }
  }
  return 1;
}

/* 设置默认配置 */
void dhcpd_config_init(struct dhcpd_config *config)
{
  memset(config, 0, sizeof(struct dhcpd_config));
  config->lease = 86400;
  config->decline_time = 3600;
  config->conflict_time = 3600;
  config->offer_time = 60;
  config->min_lease = 60;
}

/* 创建空白DHCPD消息 */
struct pbuf *dhcpd_create_msg(u8_t type, struct dhcp_msg *client_packet, const ip4_addr_t *server_ip)
{
  struct pbuf *p;
  
  // dhcp_msg结构体很大, 如果直接在函数中声明局部变量, 则很容易导致栈溢出 (而且不容易发现)
  // 所以改用动态分配比较合适
  p = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + DHCPD_OPTIONS_LEN, PBUF_RAM);
  if (p != NULL)
  {
    LWIP_ASSERT("dhcpd msg is not chained", p->next == NULL);
    dhcpd_init_msg(p, type, client_packet, server_ip);
  }
  else
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: pbuf_alloc() failed\n", __FUNCTION__));
  
  return p;
}

/* 根据MAC地址查找分配记录 */
struct dhcpd_lease *dhcpd_find_lease_by_chaddr(struct dhcpd_state *state, const uint8_t *chaddr)
{
  struct dhcpd_lease *p;
  
  for (p = state->leases; p != NULL; p = p->next)
  {
    if (memcmp(p->chaddr, chaddr, DHCP_CHADDR_LEN) == 0)
      break;
  }
  return p;
}

/* 根据IP地址查找分配记录 */
struct dhcpd_lease *dhcpd_find_lease_by_yiaddr(struct dhcpd_state *state, const ip4_addr_t *yiaddr)
{
  struct dhcpd_lease *p;
  
  for (p = state->leases; p != NULL; p = p->next)
  {
    if (ip4_addr_cmp(&p->yiaddr, yiaddr))
      break;
  }
  return p;
}

/* 获取过期时间最长的记录 */
struct dhcpd_lease *dhcpd_find_oldest_expired_lease(struct dhcpd_state *state)
{
  struct dhcpd_lease *oldest = NULL;
  struct dhcpd_lease *p;
  uint32_t oldest_lease;
  
  oldest_lease = sys_now();
  for (p = state->leases; p != NULL; p = p->next)
  {
    if (oldest_lease > p->expiring_time)
    {
      oldest_lease = p->expiring_time;
      oldest = p;
    }
  }
  return oldest;
}

/* 获取正在为客户端分配的IP地址 */
struct dhcpd_tentative_lease *dhcpd_find_tentative_lease(struct dhcpd_state *state, const uint8_t *chaddr)
{
  struct dhcpd_tentative_lease *p;
  
  for (p = state->tentative_leases; p != NULL; p = p->next)
  {
    if (memcmp(p->chaddr, chaddr, DHCP_CHADDR_LEN) == 0)
      break;
  }
  return p;
}

/* 将DHCP消息中的选项复制到变量中 */
int dhcpd_get_option(struct dhcp_msg *packet, uint8_t code, uint8_t len, void *data)
{
  uint8_t *opt;
  
  opt = dhcpd_get_option_string(packet, code);
  if (opt == NULL || opt[1] > len)
    return -1;
  
  memcpy(data, opt + 2, opt[1]);
  if (opt[1] != len)
    memset((uint8_t *)data + opt[1], 0, len - opt[1]);
  return opt[1]; // 返回选项的实际长度
}

/* 在DHCP消息中查找指定选项 */
uint8_t *dhcpd_get_option_string(struct dhcp_msg *packet, uint8_t code)
{
  int curr = DHCP_OVERLOAD_NONE; // 所在的选项区域
  int i = 0; // 选项区域的位置
  int len = DHCPD_OPTIONS_LEN; // 选项区域长度
  int over = 0; // 扩展选项区域
  uint8_t *optionptr = packet->options; // 选项区域首地址
  
  while (i < len)
  {
    if (optionptr[i] == code)
    {
      // 找到了指定的选项
      if (i + optionptr[i + 1] + 3 > len)
        break; // 如果i后面的剩余空间无法容纳这个选项加一个结束标记, 则认为数据包有问题
      return optionptr + i;
    }
    
    // 处理特殊选项
    switch (optionptr[i])
    {
      case DHCP_OPTION_PAD:
        i++;
        break;
      case DHCP_OPTION_OVERLOAD:
        if (i + optionptr[i + 1] + 1 >= len)
          break;
        over = optionptr[i + 3];
        i += optionptr[1] + 2;
        break;
      case DHCP_OPTION_END:
        if (curr == DHCP_OVERLOAD_NONE && (over & DHCP_OVERLOAD_FILE))
        {
          optionptr = packet->file;
          i = 0;
          len = DHCP_FILE_LEN;
          curr = DHCP_OVERLOAD_FILE;
        }
        else if (curr == DHCP_OVERLOAD_FILE && (over & DHCP_OVERLOAD_SNAME))
        {
          optionptr = packet->sname;
          i = 0;
          len = DHCP_SNAME_LEN;
          curr = DHCP_OVERLOAD_SNAME;
        }
        else
          return NULL;
        break;
      default:
        i += optionptr[1 + i] + 2;
    }
  }
  
  LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: bogus packet, option fields too long.\n", __FUNCTION__));
  return NULL;
}

/* 初始化DHCP服务器 */
err_t dhcpd_init(void)
{
  err_t err;
  struct udp_pcb *upcb;
  
  // 一个UDP端口号只能绑定到一个upcb上
  // 所以需要为DHCPD建立一个公共的upcb, 无法为每个netif建立单独的upcb
  if (dhcpd_upcb != NULL)
    return ERR_OK;
  
#if !LWIP_DHCP
#if !defined(LWIP_IP_ACCEPT_UDP_PORT)
  LWIP_ASSERT("DHCP messages can be received", 0);
#else
  LWIP_ASSERT("DHCP messages can be received", LWIP_IP_ACCEPT_UDP_PORT(DHCP_CLIENT_PORT));
#endif
#endif
  
  upcb = udp_new();
  if (upcb == NULL)
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: udp_new() failed!\n", __FUNCTION__));
    return ERR_MEM;
  }
  
  ip_set_option(upcb, SOF_BROADCAST);
  err = udp_bind(upcb, IP4_ADDR_ANY, DHCP_SERVER_PORT);
  if (err != ERR_OK)
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: udp_bind() failed! err=%d\n", __FUNCTION__, err));
    udp_remove(upcb);
    return err;
  }
  
  udp_recv(upcb, dhcpd_recv, NULL);
  dhcpd_upcb = upcb;
  return ERR_OK;
}

/* 初始化DHCPD消息内容 */
void dhcpd_init_msg(struct pbuf *p, u8_t type, struct dhcp_msg *client_packet, const ip4_addr_t *server_ip)
{
  struct dhcp_msg *packet = p->payload;
  
  memset(packet, 0, sizeof(struct dhcp_msg));
  switch (type)
  {
    case DHCP_DISCOVER:
    case DHCP_REQUEST:
    case DHCP_RELEASE:
    case DHCP_INFORM:
      packet->op = DHCP_BOOTREQUEST;
      break;
    case DHCP_OFFER:
    case DHCP_ACK:
    case DHCP_NAK:
      packet->op = DHCP_BOOTREPLY;
      break;
  }
  packet->htype = DHCP_HTYPE_ETH;
  packet->hlen = ETH_HWADDR_LEN;
  packet->cookie = htonl(DHCP_MAGIC_COOKIE);
  packet->options[0] = DHCP_OPTION_END;
  dhcpd_add_option(packet, DHCP_OPTION_MESSAGE_TYPE, 1, &type);
  
  if (client_packet != NULL)
  {
    packet->xid = client_packet->xid;
    memcpy(packet->chaddr, client_packet->chaddr, DHCP_CHADDR_LEN);
    packet->flags = client_packet->flags;
    packet->giaddr = client_packet->giaddr;
    packet->ciaddr = client_packet->ciaddr;
  }
  if (server_ip != NULL)
    dhcpd_add_option(packet, DHCP_OPTION_SERVER_ID, 4, server_ip);
}

/* 检查IP地址是否已过期 */
int dhcpd_is_expired_lease(struct dhcpd_lease *lease)
{
  return lease->expiring_time < sys_now();
}

/* 检查IP地址是否位于地址池中 */
int dhcpd_is_pool_address(struct dhcpd_state *state, const ip4_addr_t *addr)
{
  uint32_t value, start, end;
  
  if (ip4_addr_cmp(addr, ip_2_ip4(&state->netif->ip_addr)))
    return 0;
  
  value = ntohl(ip4_addr_get_u32(addr));
  start = ntohl(ip4_addr_get_u32(&state->config.start));
  end = ntohl(ip4_addr_get_u32(&state->config.end));
  return value >= start && value <= end;
}

/* 处理收到的DHCPD数据包 */
static void dhcpd_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  err_t err;
  int ret;
  struct dhcp_msg *packet;
  struct dhcpd_lease *lease;
  struct dhcpd_state *state;
  struct netif *netif;
  uint8_t type;
  ip4_addr_t requested_ip, server_ip;
  
  netif = ip_current_input_netif(); // 获取接收到当前UDP数据包的网络接口
  state = netif_dhcpd_data(netif);
  if (state == NULL)
    goto end;
  
  if (p->next != NULL)
  {
    p = pbuf_coalesce(p, PBUF_RAW);
    if (p->next == NULL)
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: coalesced pbuf data!\n", __FUNCTION__));
    else
    {
      LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: pbuf_coalesce() failed!\n", __FUNCTION__));
      goto end;
    }
  }
  
  packet = p->payload;
  LWIP_DEBUGF(DHCPD_DEBUG, ("[DHCP client] addr=%s, port=%d, len=%d, xid=%#x\n", ipaddr_ntoa(addr), port, p->len, packet->xid));
  if (packet->cookie != PP_HTONL(DHCP_MAGIC_COOKIE))
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: received bogus message, ignoring\n", __FUNCTION__));
    goto end;
  }
  
  // DHCP报文类型
  ret = dhcpd_get_option(packet, DHCP_OPTION_MESSAGE_TYPE, sizeof(type), &type);
  if (ret == -1)
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: couldn't get option from packet, ignoring\n", __FUNCTION__));
    goto end;
  }
  
  lease = dhcpd_find_lease_by_chaddr(state, packet->chaddr);
  switch (type)
  {
    case DHCP_DISCOVER:
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: received DISCOVER\n", __FUNCTION__));
      
      err = dhcpd_send_offer(state, packet);
      if (err != ERR_OK && err != ERR_INPROGRESS)
        LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: failed to send OFFER\n", __FUNCTION__));
      break;
    case DHCP_REQUEST:
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: received REQUEST\n", __FUNCTION__));
    
      ret = dhcpd_get_option(packet, DHCP_OPTION_REQUESTED_IP, sizeof(requested_ip), &requested_ip);
      if (ret == -1)
        ip4_addr_set_any(&requested_ip);
      
      ret = dhcpd_get_option(packet, DHCP_OPTION_SERVER_ID, sizeof(server_ip), &server_ip);
      if (ret == -1)
        ip4_addr_set_any(&server_ip);
      
      if (lease != NULL)
      {
        if (!ip4_addr_isany_val(server_ip))
        {
          /* SELECTING State */
          LWIP_DEBUGF(DHCPD_DEBUG, ("%s: server_ip = %s\n", __FUNCTION__, ip4addr_ntoa(&server_ip)));
          if (ip4_addr_cmp(&server_ip, ip_2_ip4(&netif->ip_addr)) && !ip4_addr_isany_val(requested_ip) && ip4_addr_cmp(&requested_ip, &lease->yiaddr))
            dhcpd_send_ack(state, packet, lease);
        }
        else
        {
          if (!ip4_addr_isany_val(requested_ip))
          {
            /* INIT-REBOOT State */
            if (ip4_addr_cmp(&requested_ip, &lease->yiaddr))
              dhcpd_send_ack(state, packet, lease);
            else
              dhcpd_send_nak(state, packet);
          }
          else
          {
            /* RENEWING or REBINDING State */
            if (ip4_addr_cmp(&packet->ciaddr, &lease->yiaddr))
              dhcpd_send_ack(state, packet, lease);
            else
              dhcpd_send_nak(state, packet);
          }
        }
      }
      else if (!ip4_addr_isany_val(server_ip))
      {
        /* SELECTING State */
      }
      else if (!ip4_addr_isany_val(requested_ip))
      {
        /* INIT-REBOOT State */
        // 客户端已重启, 需要验证旧地址是否还能继续用
        lease = dhcpd_find_lease_by_yiaddr(state, &requested_ip); // 检查旧地址有没有被占用
        if (lease != NULL)
        {
          if (dhcpd_is_expired_lease(lease))
          {
            // 地址已过期, 不发送回应, 让客户端等待一段时间后重新开始DISCOVER过程获取新地址
            // 这期间暂时用旧地址
            LWIP_DEBUGF(DHCPD_DEBUG, ("%s: requested IP address %s has expired!\n", __FUNCTION__, ip4addr_ntoa(&requested_ip)));
            memset(lease->chaddr, 0, DHCP_CHADDR_LEN);
          }
          else
          {
            // 地址已有其他客户端使用, 必须立即更换
            LWIP_DEBUGF(DHCPD_DEBUG, ("%s: requested IP address %s is already in use by another client!\n", __FUNCTION__, ip4addr_ntoa(&requested_ip)));
            dhcpd_send_nak(state, packet);
          }
        }
        else if (!dhcpd_is_pool_address(state, &requested_ip))
        {
          // 客户端使用的是不合法的旧地址, 不能继续使用
          // 必须立即重新开始DISCOVER过程, 获取正确的地址
          LWIP_DEBUGF(DHCPD_DEBUG, ("%s: requested IP address %s is not in the pool!\n", __FUNCTION__, ip4addr_ntoa(&requested_ip)));
          dhcpd_send_nak(state, packet);
        }
        else
        {
          // 请求的IP地址是空闲地址, 暂时先不发送回应, 等待客户端重新开始DISCOVER过程
          // 这期间客户端可以继续使用这个地址
          LWIP_DEBUGF(DHCPD_DEBUG, ("%s: requested IP address %s is in the pool but not yet allocated, remaining silent!\n", __FUNCTION__, ip4addr_ntoa(&requested_ip)));
        }
      } else {
         /* RENEWING or REBINDING State */
      }
      break;
    case DHCP_DECLINE:
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: received DECLINE\n", __FUNCTION__));
      if (lease != NULL)
      {
        // 将IP地址设为不可用 (一段时间内)
        memset(lease->chaddr, 0, DHCP_CHADDR_LEN);
        lease->expiring_time = sys_now() + state->config.decline_time;
      }
      break;
    case DHCP_RELEASE:
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: received RELEASE\n", __FUNCTION__));
      if (lease != NULL)
      {
        // 保留客户端与IP的绑定关系, 只是将租约设为已过期, 而不删除租约记录
        lease->expiring_time = sys_now();
      }
      break;
    case DHCP_INFORM:
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: received INFORM\n", __FUNCTION__));
      dhcpd_send_inform(state, packet);
      break;
    default:
      LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: unsupported DHCP message (%#x) -- ignoring\n", __FUNCTION__, type));
  }

end:
  pbuf_free(p);
}

/* 告诉客户端, IP地址可以继续使用 */
static err_t dhcpd_send_ack(struct dhcpd_state *state, struct dhcp_msg *client_packet, struct dhcpd_lease *lease)
{
  err_t err;
  int ret;
  struct dhcp_msg *packet;
  struct pbuf *p;
  uint32_t lease_time;
  
  p = dhcpd_create_msg(DHCP_ACK, client_packet, ip_2_ip4(&state->netif->ip_addr));
  if (p == NULL)
    return ERR_MEM;
  packet = p->payload;
  ip4_addr_set(&packet->yiaddr, &lease->yiaddr);
  
  // 计算租期
  ret = dhcpd_get_option(client_packet, DHCP_OPTION_LEASE_TIME, sizeof(lease_time), &lease_time); // 客户端请求的租期
  if (ret != -1)
  {
    lease_time = ntohl(lease_time);
    if (lease_time < state->config.min_lease || lease_time > state->config.lease)
      lease_time = state->config.lease;
  }
  else
    lease_time = state->config.lease;
  
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: lease time %u\n", __FUNCTION__, lease_time));
  lease_time = htonl(lease_time);
  dhcpd_add_option(packet, DHCP_OPTION_LEASE_TIME, sizeof(lease_time), &lease_time);
  
  dhcpd_add_common_options(state, packet);
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: sending ACK to %s\n", __FUNCTION__, ip4addr_ntoa(&lease->yiaddr)));
  
  err = dhcpd_send_packet(state->netif, p);
  if (err == ERR_OK)
    lease->expiring_time = sys_now() + ntohl(lease_time);
  
  pbuf_free(p);
  return err;
}

/* 将网络信息发给客户端, 不分配IP地址 */
static err_t dhcpd_send_inform(struct dhcpd_state *state, struct dhcp_msg *client_packet)
{
  err_t err;
  struct pbuf *p;
  
  p = dhcpd_create_msg(DHCP_ACK, client_packet, ip_2_ip4(&state->netif->ip_addr));
  if (p == NULL)
    return ERR_MEM;
  
  dhcpd_add_common_options(state, p->payload);
  err = dhcpd_send_packet(state->netif, p);
  pbuf_free(p);
  return err;
}

/* 告诉客户端, IP地址不能使用 */
static err_t dhcpd_send_nak(struct dhcpd_state *state, struct dhcp_msg *client_packet)
{
  err_t err;
  struct pbuf *p;
  
  p = dhcpd_create_msg(DHCP_NAK, client_packet, ip_2_ip4(&state->netif->ip_addr));
  if (p == NULL)
    return ERR_MEM;
  
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: sending NAK\n", __FUNCTION__));
  err = dhcpd_send_packet(state->netif, p);
  pbuf_free(p);
  return err;
}

/* 将分配的IP地址发送给客户端 */
static err_t dhcpd_send_offer(struct dhcpd_state *state, struct dhcp_msg *client_packet)
{
  int ret;
  ip4_addr_t reqaddr;
  struct dhcpd_lease *lease;
  struct dhcpd_tentative_lease *tlease;
  struct pbuf *resp;
  uint32_t lease_time, req_lease_time;
  
  // 准备好待发送的DHCP回应 (resp)
  tlease = dhcpd_find_tentative_lease(state, client_packet->chaddr);
  if (tlease != NULL)
  {
    // 如果客户端重传了DHCP请求, 而之前的请求还没处理完, 则根据新请求修改回应的内容 (比如xid号)
    resp = tlease->resp;
    dhcpd_init_msg(resp, DHCP_OFFER, client_packet, ip_2_ip4(&state->netif->ip_addr));
  }
  else
  {
    // 如果是新客户端的DHCP请求, 则需要分配新缓冲区
    resp = dhcpd_create_msg(DHCP_OFFER, client_packet, ip_2_ip4(&state->netif->ip_addr));
    if (resp == NULL)
      return ERR_MEM;
  }
  
  // 计算租期
  lease = dhcpd_find_lease_by_chaddr(state, client_packet->chaddr);
  if (lease != NULL && !dhcpd_is_expired_lease(lease))
    lease_time = lease->expiring_time - sys_now(); // 当前剩余租期
  else
    lease_time = state->config.lease; // 默认租期
  ret = dhcpd_get_option(client_packet, DHCP_OPTION_LEASE_TIME, sizeof(req_lease_time), &req_lease_time); // 客户端请求的租期
  if (ret != -1)
  {
    lease_time = ntohl(req_lease_time);
    if (lease_time > state->config.lease)
      lease_time = state->config.lease;
  }
  if (lease_time < state->config.min_lease)
    lease_time = state->config.lease;
  if (lease != NULL)
    lease->expiring_time = sys_now() + lease_time;
  
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: lease time %u\n", __FUNCTION__, lease_time));
  lease_time = htonl(lease_time);
  dhcpd_add_option(resp->payload, DHCP_OPTION_LEASE_TIME, sizeof(lease_time), &lease_time);
  
  dhcpd_add_common_options(state, resp->payload);
  if (tlease != NULL)
  {
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: busy allocating IP address\n", __FUNCTION__));
    return ERR_INPROGRESS; // 如果之前的请求还未完成, 则不再建立新请求, 更新完回应的内容后就在这里退出
  }
  if (lease != NULL)
  {
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP %s belongs to the client\n", __FUNCTION__, ip4addr_ntoa(&lease->yiaddr)));
    return dhcpd_send_offer_callback_4(state, resp, &lease->yiaddr); // 如果已经分配了IP地址, 则直接发给客户端
  }
  
  // 分配新IP地址
  tlease = mem_malloc(sizeof(struct dhcpd_tentative_lease));
  if (tlease == NULL)
  {
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: failed to allocate tlease\n", __FUNCTION__));
    pbuf_free(resp);
    return ERR_MEM;
  }
  memset(tlease, 0, sizeof(struct dhcpd_tentative_lease));
  
  tlease->type = DHCPD_TLEASE_TYPE_UNUSED;
  memcpy(tlease->chaddr, client_packet->chaddr, DHCP_CHADDR_LEN);
  tlease->yiaddr = state->config.start;
  tlease->resp = resp;
  
  tlease->next = state->tentative_leases;
  if (state->tentative_leases != NULL)
    state->tentative_leases->prev = tlease;
  state->tentative_leases = tlease;
  
  // 尝试使用用户请求的IP地址
  ret = dhcpd_get_option(client_packet, DHCP_OPTION_REQUESTED_IP, sizeof(reqaddr), &reqaddr);
  if (ret != -1 && dhcpd_is_pool_address(state, &reqaddr))
  {
    lease = dhcpd_find_lease_by_yiaddr(state, &reqaddr);
    if (lease == NULL || dhcpd_is_expired_lease(lease))
    {
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: client requested to use IP %s\n", __FUNCTION__, ip4addr_ntoa(&reqaddr)));
      tlease->type = DHCPD_TLEASE_TYPE_REQUESTED;
      tlease->yiaddr = reqaddr;
    }
  }
  
  dhcpd_send_offer_callback_1(state, tlease);
  return ERR_INPROGRESS;
}

// 地址搜索
static void dhcpd_send_offer_callback_1(struct dhcpd_state *state, struct dhcpd_tentative_lease *tlease)
{
  struct dhcpd_lease *lease;
  uint32_t value, start, end;
  
  if (tlease->type != DHCPD_TLEASE_TYPE_REQUESTED)
  {
    // 在value和end区间内搜索可用地址
    start = ntohl(ip4_addr_get_u32(&state->config.start));
    end = ntohl(ip4_addr_get_u32(&state->config.end));
    value = ntohl(ip4_addr_get_u32(&tlease->yiaddr));
    while (value <= end)
    {
      ip4_addr_set_u32(&tlease->yiaddr, htonl(value));
      if (ip4_addr_cmp(&tlease->yiaddr, ip_2_ip4(&state->netif->ip_addr)))
        continue;
      
      lease = dhcpd_find_lease_by_yiaddr(state, &tlease->yiaddr); // 检查是否已被占用
      if (lease == NULL || (tlease->type == DHCPD_TLEASE_TYPE_EXPIRED && dhcpd_is_expired_lease(lease)))
      {
        LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP %s is selected\n", __FUNCTION__, ip4addr_ntoa(&tlease->yiaddr)));
        break; // 如果未被占用或者已过期, 则选定
      }
      
      if (value == end && tlease->type == DHCPD_TLEASE_TYPE_UNUSED)
      {
        // 所有的地址都已分配完毕, 检查有没有过期了的地址可以利用
        LWIP_DEBUGF(DHCPD_DEBUG, ("%s: no unused IP address! try to find an expired one\n", __FUNCTION__));
        tlease->type = DHCPD_TLEASE_TYPE_EXPIRED;
        value = start;
      }
      else
        value++;
    }
    
    if (value > end)
    {
      // 未找到可用地址
      dhcpd_send_offer_callback_3(state, tlease, 0);
      return;
    }
  }
  
  // 在局域网上检查是否有手工配置了相同IP地址的主机
  dhcpd_check_ip(state->netif, &tlease->yiaddr, dhcpd_send_offer_callback_2, tlease);
}

// 处理地址检查结果
static void dhcpd_send_offer_callback_2(void *arg, struct netif *netif, const ip4_addr_t *ipaddr, int exists, const struct eth_addr *ethaddr)
{
  struct dhcpd_lease *lease;
  struct dhcpd_state *state;
  struct dhcpd_tentative_lease *tlease = arg;
  uint32_t newaddr;
  
  state = netif_dhcpd_data(netif);
  if (exists && dhcpd_compare_chaddr(tlease->chaddr, ethaddr->addr))
    exists = 0;
  
  if (exists == 0)
  {
    // 地址可用, 再次检查是否已被分配
    lease = dhcpd_find_lease_by_yiaddr(state, ipaddr);
    if (lease == NULL || dhcpd_is_expired_lease(lease))
    {
      // 分配成功
      LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP address %s is available\n", __FUNCTION__, ip4addr_ntoa(ipaddr)));
      dhcpd_send_offer_callback_3(state, tlease, 1);
      return;
    }
    
    // 否则地址不可用
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP address %s has been preempted\n", __FUNCTION__, ip4addr_ntoa(ipaddr)));
  }
  else if (exists == -1)
  {
    // 无法检查IP, 直接认为IP分配失败
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: failed to check IP address %s\n", __FUNCTION__, ip4addr_ntoa(ipaddr)));
    dhcpd_send_offer_callback_3(state, tlease, 0);
    return;
  }
  
  // 地址不可用, 继续搜索
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: IP address %s belongs to someone, reserving it for %u seconds\n", __FUNCTION__, 
    ip4addr_ntoa(ipaddr), state->config.conflict_time));
  dhcpd_add_lease(state, NULL, ipaddr, state->config.conflict_time);
  
  if (tlease->type == DHCPD_TLEASE_TYPE_REQUESTED)
  {
    tlease->type = DHCPD_TLEASE_TYPE_UNUSED;
    tlease->yiaddr = state->config.start;
  }
  else
  {
    newaddr = ntohl(ip4_addr_get_u32(&tlease->yiaddr));
    newaddr = htonl(newaddr + 1);
    ip4_addr_set_u32(&tlease->yiaddr, newaddr);
  }
  dhcpd_send_offer_callback_1(state, tlease);
}

// 添加租约记录并发送回应
// 函数调用后, 指针tlease以及缓冲区tlease->resp不能再使用
static err_t dhcpd_send_offer_callback_3(struct dhcpd_state *state, struct dhcpd_tentative_lease *tlease, int succeeded)
{
  err_t err;
  struct dhcpd_lease *lease;
  struct pbuf *p = tlease->resp;
  
  // 添加租约记录
  if (succeeded)
    lease = dhcpd_add_lease(state, tlease->chaddr, &tlease->yiaddr, state->config.offer_time);
  
  // 删除tlease
  if (tlease->prev == NULL)
  {
    LWIP_ASSERT("state->tentative_leases == tlease", state->tentative_leases == tlease);
    state->tentative_leases = tlease->next;
  }
  else
    tlease->prev->next = tlease->next;
  if (tlease->next != NULL)
    tlease->next->prev = tlease->prev;
  mem_free(tlease);
  tlease = NULL;
  
  // 发送回应
  if (succeeded)
  {
    if (lease != NULL)
    {
      err = dhcpd_send_offer_callback_4(state, p, &lease->yiaddr);
      p = NULL;
    }
    else
    {
      LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: lease pool is full -- OFFER abandoned\n", __FUNCTION__));
      err = ERR_MEM;
    }
  }
  else
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: no IP addresses to give -- OFFER abandoned\n", __FUNCTION__));
    err = ERR_OK;
  }
  
  if (p != NULL)
    pbuf_free(p);
  return err;
}

// 发送回应
// 函数调用后, 指针p不能再使用
static err_t dhcpd_send_offer_callback_4(struct dhcpd_state *state, struct pbuf *p, const ip4_addr_t *yiaddr)
{
  err_t err;
  struct dhcp_msg *packet = p->payload;
  
  ip4_addr_set(&packet->yiaddr, yiaddr);
  LWIP_DEBUGF(DHCPD_DEBUG, ("%s: sending OFFER of %s\n", __FUNCTION__, ip4addr_ntoa(yiaddr)));
  err = dhcpd_send_packet(state->netif, p);
  pbuf_free(p);
  return err;
}

/* 发送DHCP消息 */
err_t dhcpd_send_packet(struct netif *netif, struct pbuf *p)
{
  err_t err;
  ip_addr_t addr;
  struct dhcp_msg *packet = p->payload;
  
  if (ip4_addr_isany_val(packet->giaddr))
  {
    // 直接发送
    err = udp_sendto_if(dhcpd_upcb, p, IP_ADDR_BROADCAST, DHCP_CLIENT_PORT, netif);
  }
  else
  {
    // 中转发送
    LWIP_DEBUGF(DHCPD_DEBUG, ("%s: Forwarding packet to relay\n", __FUNCTION__));
    ip_addr_copy_from_ip4(addr, packet->giaddr);
    err = udp_sendto(dhcpd_upcb, p, &addr, DHCP_SERVER_PORT);
  }

  if (err != ERR_OK)
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: udp_sendto() failed! err=%d\n", __FUNCTION__, err));
  return err;
}

/* 在网络接口上启动DHCP服务器 */
err_t dhcpd_start(struct netif *netif, const struct dhcpd_config *config)
{
  err_t err;
  struct dhcpd_state *state;
  
  if (dhcpd_upcb == NULL)
  {
    err = dhcpd_init();
    if (err != ERR_OK)
      return err;
  }
  
  if (netif_dhcpd_data(netif) != NULL)
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: DHCP server is already running on the interface!\n", __FUNCTION__));
    return ERR_ARG;
  }
  else if (ip_addr_isany_val(netif->ip_addr))
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: netif does not have an IP address!\n", __FUNCTION__));
    return ERR_ARG;
  }
  
  state = mem_malloc(sizeof(struct dhcpd_state));
  if (state == NULL)
  {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_LEVEL_WARNING, ("%s: mem_malloc() failed!\n", __FUNCTION__));
    return ERR_MEM;
  }
  
  memset(state, 0, sizeof(struct dhcpd_state));
  state->config = *config;
  state->netif = netif;
  state->next = dhcpd_list;
  dhcpd_list = state;
  return ERR_OK;
}

/* 获取网络接口的DHCPD状态信息 */
struct dhcpd_state *netif_dhcpd_data(struct netif *netif)
{
  struct dhcpd_state *state;
  
  for (state = dhcpd_list; state != NULL; state = state->next)
  {
    if (state->netif == netif)
      break;
  }
  return state;
}

#endif
