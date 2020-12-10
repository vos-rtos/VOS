#ifndef __DHCPD_H
#define __DHCPD_H

#if LWIP_IPV4 && LWIP_ARP

#ifndef DHCPD_DEBUG
#define DHCPD_DEBUG LWIP_DBG_OFF
#endif

#ifndef DHCPD_ALWAYS_INCLUDE_DNS
#define DHCPD_ALWAYS_INCLUDE_DNS 1 // ���û����config�ṹ��������DNS��ַ, ��DNS��ַΪ�ӿڵ�ַ
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

// IP��ַ����˳��
enum
{
  DHCPD_TLEASE_TYPE_REQUESTED = 0, // �ͻ��������IP��ַ
  DHCPD_TLEASE_TYPE_UNUSED = 1, // δʹ�õ�IP��ַ
  DHCPD_TLEASE_TYPE_EXPIRED = 2 // ��ʹ�õ��ѹ��ڵ�IP��ַ
};

struct dhcpd_config
{
  ip4_addr_t start; // ��ַ���еĵ�һ����ַ
  ip4_addr_t end; // ��ַ���е����һ����ַ
  ip4_addr_t dns_servers[2]; // DNS��������ַ
  uint32_t lease; // Ĭ������
  uint32_t decline_time; // �����ܾ�ʹ��ĳIP��ַ (DHCP DECLINE), �೤ʱ�䲻���������ַ
  uint32_t conflict_time; // �����ַʱ��⵽��ַ����������ռ��, �೤ʱ�䲻���������ַ
  uint32_t offer_time; // ��ַͨ��OFFER��Ϣ����������, ����ͨ��REQUEST��Ϣȷ��ʹ��ǰ, ��ַ�����೤ʱ��
  uint32_t min_lease; // �ͻ��˿�������������
};

// DHCP lease
struct dhcpd_lease
{
  uint8_t chaddr[16]; // �ͻ���MAC��ַ
  ip4_addr_t yiaddr; // ���䵽��IP��ַ
  uint32_t expiring_time; // ����ʱ��
  struct dhcpd_lease *prev, *next;
};

struct dhcpd_tentative_lease
{
  uint8_t type; // IP��ַ����
  uint8_t chaddr[16]; // �ͻ���MAC��ַ
  ip4_addr_t yiaddr; // �����IP��ַ
  struct pbuf *resp; // �����͵�DHCP��Ӧ
  struct dhcpd_tentative_lease *prev, *next;
};

struct dhcpd_state
{
  struct dhcpd_config config; // DHCP����������
  struct netif *netif; // ����ӿ�
  struct dhcpd_lease *leases; // �����IP��ַ
  struct dhcpd_tentative_lease *tentative_leases; // �����е�IP��ַ
  struct dhcpd_state *next;
};

typedef void (*dhcpd_ip_callback)(void *arg, struct netif *netif, const ip4_addr_t *ipaddr, int exists, const struct eth_addr *ethaddr);

struct dhcpd_ip_callback_entry
{
  dhcpd_ip_callback callback;
  void *arg;
  struct dhcpd_ip_callback_entry *next;
};

// MAC��ַ��ѯ
struct dhcpd_ip_status
{
  struct netif *netif; // ���������ڵ�����ӿ�
  ip4_addr_t ipaddr; // ��ѯ��IP��ַ
  uint32_t time; // ��ѯ��ʼʱ��
  int retry; // ʣ�����Դ���
  
  struct dhcpd_ip_callback_entry *callbacks; // �ص������б�
  struct dhcpd_ip_status *prev, *next; // ��һ������һ����ѯ
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
