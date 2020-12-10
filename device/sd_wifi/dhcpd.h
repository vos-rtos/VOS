#ifndef __DHCPD_H
#define __DHCPD_H

#if LWIP_IPV4 && LWIP_ARP

#ifndef DHCPD_DEBUG
#define DHCPD_DEBUG LWIP_DBG_OFF
#endif

#ifndef DHCPD_ALWAYS_INCLUDE_DNS
#define DHCPD_ALWAYS_INCLUDE_DNS 1 // 如果没有在config结构体中配置DNS地址, 则DNS地址为接口地址
#endif

#define DHCPD_BROADCAST_FLAG 0x8000
#define DHCPD_IPCHECK_TIMEOUT 100
#define DHCPD_OPTIONS_LEN 308

#ifdef LWIP_HDR_PROT_DHCP_H
#ifndef DHCP_HTYPE_ETH
#include <lwip/prot/iana.h>

#define DHCP_HTYPE_ETH LWIP_IANA_HWTYPE_ETHERNET
#define DHCP_CLIENT_PORT LWIP_IANA_PORT_DHCP_CLIENT
#define DHCP_SERVER_PORT LWIP_IANA_PORT_DHCP_SERVER
#endif
#else
struct dhcp_msg;
#endif

// IP地址分配顺序
enum
{
  DHCPD_TLEASE_TYPE_REQUESTED = 0, // 客户端请求的IP地址
  DHCPD_TLEASE_TYPE_UNUSED = 1, // 未使用的IP地址
  DHCPD_TLEASE_TYPE_EXPIRED = 2 // 已使用但已过期的IP地址
};

struct dhcpd_config
{
  ip4_addr_t start; // 地址池中的第一个地址
  ip4_addr_t end; // 地址池中的最后一个地址
  ip4_addr_t dns_servers[2]; // DNS服务器地址
  uint32_t lease; // 默认租期
  uint32_t decline_time; // 主机拒绝使用某IP地址 (DHCP DECLINE), 多长时间不重试这个地址
  uint32_t conflict_time; // 分配地址时检测到地址被其他主机占用, 多长时间不重试这个地址
  uint32_t offer_time; // 地址通过OFFER消息发给主机后, 主机通过REQUEST消息确认使用前, 地址保留多长时间
  uint32_t min_lease; // 客户端可请求的最短租期
};

// DHCP lease
struct dhcpd_lease
{
  uint8_t chaddr[16]; // 客户端MAC地址
  ip4_addr_t yiaddr; // 分配到的IP地址
  uint32_t expiring_time; // 到期时间
  struct dhcpd_lease *prev, *next;
};

struct dhcpd_tentative_lease
{
  uint8_t type; // IP地址类型
  uint8_t chaddr[16]; // 客户端MAC地址
  ip4_addr_t yiaddr; // 分配的IP地址
  struct pbuf *resp; // 待发送的DHCP回应
  struct dhcpd_tentative_lease *prev, *next;
};

struct dhcpd_state
{
  struct dhcpd_config config; // DHCP服务器配置
  struct netif *netif; // 网络接口
  struct dhcpd_lease *leases; // 分配的IP地址
  struct dhcpd_tentative_lease *tentative_leases; // 检验中的IP地址
  struct dhcpd_state *next;
};

typedef void (*dhcpd_ip_callback)(void *arg, struct netif *netif, const ip4_addr_t *ipaddr, int exists, const struct eth_addr *ethaddr);

struct dhcpd_ip_callback_entry
{
  dhcpd_ip_callback callback;
  void *arg;
  struct dhcpd_ip_callback_entry *next;
};

// MAC地址查询
struct dhcpd_ip_status
{
  struct netif *netif; // 局域网所在的网络接口
  ip4_addr_t ipaddr; // 查询的IP地址
  uint32_t time; // 查询开始时间
  int retry; // 剩余重试次数
  
  struct dhcpd_ip_callback_entry *callbacks; // 回调函数列表
  struct dhcpd_ip_status *prev, *next; // 上一条和下一条查询
};

struct dhcpd_lease *dhcpd_add_lease(struct dhcpd_state *state, const uint8_t *chaddr, const ip4_addr_t *yiaddr, uint32_t lease_time);
int dhcpd_add_option(struct dhcp_msg *packet, uint8_t code, uint8_t len, const void *data);
void dhcpd_add_common_options(struct dhcpd_state *state, struct dhcp_msg *packet);
err_t dhcpd_check_ip(struct netif *netif, const ip4_addr_t *ipaddr, dhcpd_ip_callback callback, void *arg);
void dhcpd_clear_lease(struct dhcpd_state *state, const uint8_t *chaddr, const ip4_addr_t *yiaddr);
int dhcpd_compare_chaddr(const uint8_t *chaddr, const uint8_t *mac);
void dhcpd_config_init(struct dhcpd_config *config);
struct pbuf *dhcpd_create_msg(u8_t type, struct dhcp_msg *client_packet, const ip4_addr_t *server_ip);
struct dhcpd_lease *dhcpd_find_lease_by_chaddr(struct dhcpd_state *state, const uint8_t *chaddr);
struct dhcpd_lease *dhcpd_find_lease_by_yiaddr(struct dhcpd_state *state, const ip4_addr_t *yiaddr);
struct dhcpd_lease *dhcpd_find_oldest_expired_lease(struct dhcpd_state *state);
struct dhcpd_tentative_lease *dhcpd_find_tentative_lease(struct dhcpd_state *state, const uint8_t *chaddr);
int dhcpd_get_option(struct dhcp_msg *packet, uint8_t code, uint8_t len, void *data);
uint8_t *dhcpd_get_option_string(struct dhcp_msg *packet, uint8_t code);
err_t dhcpd_init(void);
void dhcpd_init_msg(struct pbuf *p, u8_t type, struct dhcp_msg *client_packet, const ip4_addr_t *server_ip);
int dhcpd_is_expired_lease(struct dhcpd_lease *lease);
int dhcpd_is_pool_address(struct dhcpd_state *state, const ip4_addr_t *addr);
err_t dhcpd_send_packet(struct netif *netif, struct pbuf *p);
err_t dhcpd_start(struct netif *netif, const struct dhcpd_config *config);
struct dhcpd_state *netif_dhcpd_data(struct netif *netif);

#endif
#endif
