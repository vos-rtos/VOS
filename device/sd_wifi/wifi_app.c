#include "inttypes.h"
#include "cmsis_gcc.h"

#include <stm32f4xx.h>
#include "wifi.h"
#include <lwip/apps/httpd.h>
#include <lwip/apps/netbiosns.h>
#include <lwip/dhcp.h>
#include <lwip/dns.h>
#include <lwip/init.h>
#include <lwip/netif.h>
#include <lwip/timeouts.h>
#include <wifi_netif.h>

#include "dhcpd.h"
#include "nd6d.h"

#define NULL 0

#define printf kprintf

void associate_example(void);
static struct netif wifi_88w8801_sta;
static struct netif wifi_88w8801_uap;

void WiFi_EventHandler(bss_t bss, const WiFi_EventHeader *event)
{
  if (WiFi_IsConnectionEstablishedEvent(event->event_id))
  {
    if (bss == BSS_STA)
      netif_set_link_up(&wifi_88w8801_sta);
    else if (bss == BSS_UAP)
      netif_set_link_up(&wifi_88w8801_uap);
  }
  else if (WiFi_IsConnectionLostEvent(event->event_id))
  {
    if (bss == BSS_STA)
    {
      netif_set_link_down(&wifi_88w8801_sta);

      //printf("Reconnecting...\n");
      //associate_example();
    }
    else if (bss == BSS_UAP)
      netif_set_link_down(&wifi_88w8801_uap);
  }
}

void WiFi_PacketHandler(const WiFi_DataRx *data, port_t port)
{
  bss_t bss;
  struct ethernetif *state;
  struct netif *netif;

  bss = WIFI_BSS(data->bss_type, data->bss_num);
  if (bss == BSS_STA)
    netif = &wifi_88w8801_sta;
  else if (bss == BSS_UAP)
    netif = &wifi_88w8801_uap;
  else
  {
    printf("Dropped a packet of %d bytes on port %d! bss=0x%02x\n", data->rx_packet_length, port, bss);
    return;
  }

  state = netif->state;
  state->data = data;
  state->port = port;
  ethernetif_input(netif);
}

#if LWIP_DHCP || LWIP_IPV6
static void display_dns(int ipver)
{
#if LWIP_DNS
  const ip_addr_t *addr;

  addr = dns_getserver(0);
  if (ip_addr_isany(addr))
    return;
  printf("DNS Server: %s", ipaddr_ntoa(addr));

  addr = dns_getserver(1);
  if (!ip_addr_isany(addr))
    printf(" %s", ipaddr_ntoa(addr));

  printf("\n");
  //dns_test(ipver);
#endif
}
#endif

static void display_ip(void)
{
#if LWIP_DHCP
  static u8 ip_displayed = 0;
#endif
#if LWIP_IPV6
  static u8 ip6_displayed = 0;
  int i, ip6_valid = 0;
#endif

#if LWIP_DHCP
  if (dhcp_supplied_address(&wifi_88w8801_sta))
  {
    if (ip_displayed == 0)
    {
      ip_displayed = 1;

      printf("DHCP supplied address!\n");
      printf("IP address: %s\n", ipaddr_ntoa(&wifi_88w8801_sta.ip_addr));
      printf("Subnet mask: %s\n", ipaddr_ntoa(&wifi_88w8801_sta.netmask));
      printf("Default gateway: %s\n", ipaddr_ntoa(&wifi_88w8801_sta.gw));
      display_dns(4);
    }
  }
  else
    ip_displayed = 0;
#endif

#if LWIP_IPV6
  for (i = 1; i < LWIP_IPV6_NUM_ADDRESSES; i++)
  {
    if (ip6_addr_isvalid(netif_ip6_addr_state(&wifi_88w8801_sta, i)))
    {
      if ((ip6_displayed & _BV(i)) == 0)
      {
        ip6_displayed |= _BV(i);
        printf("IPv6 address %d: %s\n", i, ipaddr_ntoa(netif_ip_addr6(&wifi_88w8801_sta, i)));
        ip6_valid = 1;
      }
    }
    else
      ip6_displayed &= ~_BV(i);
  }
  if (ip6_valid)
    display_dns(6);
#endif
}

static void associate_callback(void *arg, void *data, WiFi_Status status)
{
  switch (status)
  {
    case WIFI_STATUS_OK:
      printf("Associated!\n");
      break;
    case WIFI_STATUS_NOTFOUND:
      printf("Cannot find the SSID!\n");
      break;
    case WIFI_STATUS_FAIL:
      printf("Association failed!\n");
      break;
    case WIFI_STATUS_INPROGRESS:
      printf("Waiting for authentication!\n");
      break;
    case WIFI_STATUS_INVALID:
      printf("Invalid condition!\n");
      break;
    default:
      printf("Unknown error! status=%d\n", status);
  }
}

void associate_example(void)
{
  WiFi_Connection conn;

#if 0
  conn.bss = BSS_STA;
  conn.auth_type = WIFI_AUTH_MODE_OPEN;
  conn.sec_type = WIFI_SECURITYTYPE_NONE;
  strcpy(conn.ssid, "vivo Y29L");
  WiFi_Associate(&conn, -1, associate_callback, NULL);
#endif

#if 0
  conn.bss = BSS_STA;
  conn.auth_type = WIFI_AUTH_MODE_OPEN;
  //conn.auth_type = WIFI_AUTH_MODE_SHARED;
  conn.sec_type = WIFI_SECURITYTYPE_WEP;
  strcpy(conn.ssid, "Oct1158");
  conn.password.wep.index = 0;
  strcpy(conn.password.wep.keys[0], "1234567890123");
  WiFi_Associate(&conn, -1, associate_callback, NULL);
#endif

#if 1
  conn.bss = BSS_STA;
  conn.auth_type = WIFI_AUTH_MODE_OPEN;
  conn.sec_type = WIFI_SECURITYTYPE_WPA;
  strcpy(conn.ssid, "visa");
  strcpy(conn.password.wpa, "120503Aa");
  WiFi_Associate(&conn, -1, associate_callback, NULL);
#endif
}

static void ap_callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
    printf("AP is created!\n");
  else
    printf("Failed to create AP!\n");

  associate_example();
}

void ap_example(void)
{
  WiFi_AccessPoint ap;

#if 0
  ap.bss = BSS_UAP;
  ap.auth_type = WIFI_AUTH_MODE_OPEN;
  ap.sec_type = WIFI_SECURITYTYPE_NONE;
  strcpy(ap.ssid, "Hello World");
  WiFi_StartMicroAP(&ap, WIFI_MICROAP_BROADCASTSSID, ap_callback, NULL);
#endif

#if 1
  ap.bss = BSS_UAP;
  ap.auth_type = WIFI_AUTH_MODE_OPEN;
  ap.sec_type = WIFI_SECURITYTYPE_WPA2;
  strcpy(ap.ssid, "Hello World");
  strcpy(ap.password.wpa, "12345678");
  WiFi_StartMicroAP(&ap, WIFI_MICROAP_BROADCASTSSID, ap_callback, NULL);
#endif
}

static void scan_callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
    printf("Scan finished!\n");
  else
    printf("Scan failed!\n");

  ap_example();
}

static void mac_address_callback(void *arg, void *data, WiFi_Status status)
{
  bss_t bss;
  ip4_addr_t ip4addr, netmask, gw;
  struct dhcpd_config dhcpd;
#if LWIP_IPV6
  ip6_addr_t ip6addr;
#endif
#if !LWIP_DHCP && LWIP_DNS
  ip_addr_t ipaddr;
#endif

  if (status != WIFI_STATUS_OK)
  {
    printf("Failed to get MAC address!\n");
    return;
  }
  WiFi_Scan(scan_callback, NULL);

  printf("MAC Addr: %02X:%02X:%02X:%02X:%02X:%02X\n", wifi_88w8801_sta.hwaddr[0], wifi_88w8801_sta.hwaddr[1],
    wifi_88w8801_sta.hwaddr[2], wifi_88w8801_sta.hwaddr[3], wifi_88w8801_sta.hwaddr[4], wifi_88w8801_sta.hwaddr[5]);

#if IP_FORWARD
  printf("IP_FORWARD is enabled!\n");
#else
  printf("IP_FORWARD is disabled!\n");
#endif
#if LWIP_IPV6_FORWARD
  printf("LWIP_IPV6_FORWARD is enabled!\n");
#else
  printf("LWIP_IPV6_FORWARD is disabled!\n");
#endif

  bss = BSS_STA;
#if LWIP_DHCP
  netif_add_noaddr(&wifi_88w8801_sta, &bss, ethernetif_init, netif_input);
#else
  IP4_ADDR(&ip4addr, 192, 168, 2, 15);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 192, 168, 2, 1);
  netif_add(&wifi_88w8801_sta, &ip4addr, &netmask, &gw, &bss, ethernetif_init, netif_input);

#if LWIP_DNS
  IP_ADDR4(&ipaddr, 8, 8, 8, 8);
  dns_setserver(0, &ipaddr);
  IP_ADDR4(&ipaddr, 8, 8, 4, 4);
  dns_setserver(1, &ipaddr);
#endif
#endif
  netif_set_default(&wifi_88w8801_sta);
  netif_set_up(&wifi_88w8801_sta);
#if LWIP_DHCP
  dhcp_start(&wifi_88w8801_sta);
#endif

#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&wifi_88w8801_sta, 1);
  printf("IPv6 link-local address: %s\n", ipaddr_ntoa(netif_ip_addr6(&wifi_88w8801_sta, 0)));

  netif_set_ip6_autoconfig_enabled((struct netif *)(uintptr_t)&wifi_88w8801_sta, 1);
#endif

  memcpy(wifi_88w8801_uap.hwaddr, wifi_88w8801_sta.hwaddr, WIFI_MACADDR_LEN);

  bss = BSS_UAP;
  IP4_ADDR(&ip4addr, 192, 168, 20, 1);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  ip4_addr_set_zero(&gw);
  netif_add(&wifi_88w8801_uap, &ip4addr, &netmask, &gw, &bss, ethernetif_init, netif_input);
  netif_set_up(&wifi_88w8801_uap);

  dhcpd_config_init(&dhcpd);
  IP4_ADDR(&dhcpd.start, 192, 168, 20, 100);
  IP4_ADDR(&dhcpd.end, 192, 168, 20, 200);
  IP4_ADDR(&dhcpd.dns_servers[0], 8, 8, 8, 8);
  IP4_ADDR(&dhcpd.dns_servers[1], 8, 8, 4, 4);
  dhcpd_start(&wifi_88w8801_uap, &dhcpd);

#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&wifi_88w8801_uap, 1);
  printf("IPv6 link-local address for micro AP: %s\n", ipaddr_ntoa(netif_ip_addr6(&wifi_88w8801_uap, 0)));

  IP6_ADDR(&ip6addr, PP_HTONL(0xfd200000), 0, 0, PP_HTONL(1));
  printf("IPv6 address for micro AP: %s\n", ip6addr_ntoa(&ip6addr));
  netif_add_ip6_address(&wifi_88w8801_uap, &ip6addr, NULL);

  nd6d_start(&wifi_88w8801_uap, NULL);
#endif
}

static void stop_callback(void *arg, void *data, WiFi_Status status)
{
  char *s1 = arg;
  char *s2;
  WiFi_CommandHeader *header = data;

  s2 = s1 + strlen(s1) + 1;
  if (status == WIFI_STATUS_OK)
  {
    printf("%s\n", s1);

    if (header->bss == BSS_STA)
      netif_set_link_down(&wifi_88w8801_sta);
    else if (header->bss == BSS_UAP)
      netif_set_link_down(&wifi_88w8801_uap);
  }
  else
    printf("%s\n", s2);
}

static void ap_deauth_first_callback(void *arg, void *data, WiFi_Status status)
{
  u16 code;
  WiFi_APCmdResponse_STAList *resp = data;

  code = WiFi_GetCommandCode(data);
  switch (code)
  {
    case APCMD_STA_LIST:
      if (status != WIFI_STATUS_OK)
        printf("Failed to retrieve client list! status=%d\n", status);
      else if (resp->sta_count == 0)
        printf("No client to deauthenticate!\n");
      else
      {
        printf("Deauthenticating %02X:%02X:%02X:%02X:%02X:%02X...\n", resp->sta_list[0].mac_address[0], resp->sta_list[0].mac_address[1],
          resp->sta_list[0].mac_address[2], resp->sta_list[0].mac_address[3],
          resp->sta_list[0].mac_address[4], resp->sta_list[0].mac_address[5]);
        WiFi_DeauthenticateClient(BSS_UAP, resp->sta_list[0].mac_address, NOT_AUTHENTICATED, ap_deauth_first_callback, NULL);
      }
      break;
    case APCMD_STA_DEAUTH:
      if (status == WIFI_STATUS_OK)
        printf("Deauthenticated the client!\n");
      else
        printf("Failed to deauthenticated the client! status=%d\n", status);
      break;
  }
}

static void ap_sysinfo_callback(void *arg, void *data, WiFi_Status status)
{
  WiFi_ShowAPClientList(BSS_UAP, NULL, NULL);
}

void usart_process(char ch)
{
  switch (ch)
  {
    case 'a':
      ap_example();
      break;
    case 'A':
      WiFi_StopMicroAP(BSS_UAP, stop_callback, "AP is stopped!\0Failed to stop AP!");
      break;
    case 'b':
      WiFi_ShowAPClientList(BSS_UAP, ap_deauth_first_callback, NULL);
      break;
    case 'c':
      printf("Connecting...\n");
      associate_example();
      break;
    case 'C':
      WiFi_Deauthenticate(BSS_STA, LEAVING_NETWORK_DISASSOC, stop_callback, "Disconnected from the network!\0Failed to disconnect from the network!");
      break;
    case 'd':
      WiFi_DiscardData();
      break;
    case 'I':
      WiFi_ShowAPSysInfo(BSS_UAP, ap_sysinfo_callback, NULL);
      break;
    case 'k':
      WiFi_ShowKeyMaterials(BSS_STA);
      break;
#if LWIP_DNS
    case 'n':
      dns_test(4);
      break;
    case 'N':
      dns_test(6);
      break;
#endif
    case 's':
      printf("SDIO->STA=0x%08x, DMA2->LISR=0x%08x\n", SDIO->STA, DMA2->LISR);
      printf("CARDSTATUS=%d, INTSTATUS=%d, ", WiFi_LowLevel_ReadReg(1, WIFI_CARDSTATUS), WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS));
      printf("RDBITMAP=0x%04x, WRBITMAP=0x%04x\n", WiFi_GetReadPortStatus(), WiFi_GetWritePortStatus());
      break;
    case 'S':
      WiFi_Scan(NULL, NULL);
      break;
    case 't':
      printf("sys_now()=%d\n", sys_now());
      break;
    case 'T':
      WiFi_PrintTxPacketStatus(BSS_STA);
      break;
  }
}

void task_wifi(void *param)
{
  WiFi_Init();
  WiFi_MACAddr(BSS_STA, wifi_88w8801_sta.hwaddr, WIFI_ACT_GET, mac_address_callback, NULL);
  //lwip_init();
  tcpip_init(NULL, NULL);

  netbiosns_init();
  netbiosns_set_name("STM32F407VE");

//  httpd_init();
  //tcp_tester_init();
//  udp_tester_init();
  while (1)
  {

    if (WiFi_LowLevel_GetITStatus(1)) {
      WiFi_Input();
    }
    else {
      WiFi_CheckTimeout();
      VOSTaskDelay(2);
    }

    sys_check_timeouts();


    display_ip();
    //VOSTaskDelay(10);
  }
}

static long long task_wifi_stack[1024];
void wifi_test()
{
	s32 task_id;
	task_id = VOSTaskCreate(task_wifi, 0, task_wifi_stack, sizeof(task_wifi_stack), TASK_PRIO_NORMAL, "task_wifi");
}



