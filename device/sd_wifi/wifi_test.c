#include <lwip/dns.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <string.h>
#include "common.h"

#define DUMP 0

#define printf kprintf


/*** [DNS TEST] ***/
static err_t test_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  uintptr_t count = (uintptr_t)arg;
#if DUMP
  struct pbuf *q;
#endif
  
  if (p != NULL)
  {
    printf("%d bytes received!\n", p->tot_len);
#if DUMP
    for (q = p; q != NULL; q = q->next)
      printf("%.*s\n", q->len, q->payload);
#endif

    count += p->tot_len;
    tcp_arg(tpcb, (void *)count);

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
  }
  else
  {
    err = tcp_close(tpcb);
    printf("TCP socket is closed! err=%d, count=%d\n", err, count);
  }
  return ERR_OK;
}


static err_t test_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  char *request = "GET /news/?group=lwip HTTP/1.1\r\nHost: savannah.nongnu.org\r\n\r\n";
  
  printf("TCP socket is connected! err=%d\n", err);
  tcp_recv(tpcb, test_recv);
  
  tcp_write(tpcb, request, strlen(request), 0);
  tcp_output(tpcb);
  return ERR_OK;
}

static void test_err(void *arg, err_t err)
{
  printf("TCP socket error! err=%d\n", err);
}

void connect_test(const ip_addr_t *ipaddr)
{
  err_t err;
  struct tcp_pcb *tpcb;
  
  tpcb = tcp_new();
  if (tpcb == NULL)
  {
    printf("\atcp_new failed!\n");
    return;
  }
  tcp_err(tpcb, test_err);
  
  printf("TCP socket is connecting to %s...\n", ipaddr_ntoa(ipaddr));
  err = tcp_connect(tpcb, ipaddr, 80, test_connected);
  if (err != ERR_OK)
  {
    tcp_close(tpcb);
    printf("TCP socket connection failed! err=%d\n", err);
  }
}

#if LWIP_DNS
static void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
  if (ipaddr != NULL)
  {
    printf("DNS Found IP of %s: %s\n", name, ipaddr_ntoa(ipaddr));
    connect_test(ipaddr);
  }
  else
    printf("DNS Not Found IP of %s!\n", name);
}

void dns_test(int ipver)
{
  char *domain = "savannah.nongnu.org";
  err_t err;
  ip_addr_t dnsip;

  if (ipver == 4)
    err = dns_gethostbyname(domain, &dnsip, dns_found, NULL);
  else
    err = dns_gethostbyname_addrtype(domain, &dnsip, dns_found, NULL, LWIP_DNS_ADDRTYPE_IPV6_IPV4);
  
  if (err == ERR_OK)
  {
    printf("%s: IP of %s is in cache: %s\n", __FUNCTION__, domain, ipaddr_ntoa(&dnsip));
    connect_test(&dnsip);
  }
  else if (err == ERR_INPROGRESS)
    printf("%s: IP of %s is not in cache!\n", __FUNCTION__, domain);
  else
    printf("%s: dns_gethostbyname failed! err=%d\n", __FUNCTION__, err);
}
#endif

/*** [TCP SPEED TEST] ***/
static uint8_t tcp_tester_buffer[1500];

static err_t tcp_tester_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static void tcp_tester_err(void *arg, err_t err);
static err_t tcp_tester_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_tester_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_tester_sent2(void *arg, struct tcp_pcb *tpcb, u16_t len);

static err_t tcp_tester_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  printf("TCP tester accepted [%s]:%d!\n", ipaddr_ntoa(&newpcb->remote_ip), newpcb->remote_port);
  tcp_err(newpcb, tcp_tester_err);
  tcp_recv(newpcb, tcp_tester_recv);
  tcp_sent(newpcb, tcp_tester_sent);
  
  tcp_tester_sent(NULL, newpcb, 0);
  return ERR_OK;
}

static void tcp_tester_err(void *arg, err_t err)
{
  printf("TCP tester error! err=%d\n", err);
}

static err_t tcp_tester_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if (p != NULL)
  {

    printf("TCP tester received %d bytes from [%s]:%d!\n", p->tot_len, ipaddr_ntoa(&tpcb->remote_ip), tpcb->remote_port);
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    tcp_sent(tpcb, tcp_tester_sent2);
  }
  else
  {
    tcp_sent(tpcb, NULL);
    //tcp_err(tpcb, NULL);
    err = tcp_close(tpcb);
    printf("TCP tester client [%s]:%d closed! err=%d\n", ipaddr_ntoa(&tpcb->remote_ip), tpcb->remote_port, err);
  }
  return ERR_OK;
}

static err_t tcp_tester_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  uint16_t size;
  
  size = tcp_sndbuf(tpcb);
  if (size > sizeof(tcp_tester_buffer))
    size = sizeof(tcp_tester_buffer);
  
  tcp_write(tpcb, tcp_tester_buffer, size, 0);
  return ERR_OK;
}

static err_t tcp_tester_sent2(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  err_t err;
  
  tcp_sent(tpcb, NULL);
  err = tcp_shutdown(tpcb, 0, 1);
  
  printf("TCP tester stopped sending data! err=%d\n", err);
  return ERR_OK;
}

void tcp_tester_init(void)
{
  err_t err;
  struct tcp_pcb *tpcb, *temp;
  
  tpcb = tcp_new();
  if (tpcb == NULL)
  {
    printf("%s: tcp_new failed!\n", __FUNCTION__);
    return;
  }
  
  err = tcp_bind(tpcb, IP_ANY_TYPE, 24001);
  if (err != ERR_OK)
  {
    tcp_close(tpcb);
    printf("%s: tcp_bind failed! err=%d\n", __FUNCTION__, err);
    return;
  }
  
  temp = tcp_listen(tpcb);
  if (temp == NULL)
  {
    tcp_close(tpcb);
    printf("%s: tcp_listen failed!\n", __FUNCTION__);
    return;
  }
  tpcb = temp;
  
  tcp_accept(tpcb, tcp_tester_accept);
}

/*** [UDP TEST] ***/
struct test
{
  uint32_t id;
  uint32_t count;
};

static void udp_tester_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  struct pbuf *q;
  struct test *t = (struct test *)tcp_tester_buffer;
  
  if (p != NULL)
  {
    pbuf_free(p);
    p = NULL;
    
    q = pbuf_alloc(PBUF_TRANSPORT, 1300, PBUF_REF);
    if (q != NULL)
    {
      printf("Sending UDP packets...\n");
      q->payload = t;
      
      t->count = 1024;
      for (t->id = 0; t->id < t->count; t->id++)
        udp_sendto(pcb, q, addr, port);
      
      pbuf_free(q);
    }
  }
}

void udp_tester_init(void)
{
  err_t err;
  struct udp_pcb *upcb;
  
  upcb = udp_new();
  if (upcb == NULL)
  {
    printf("%s: udp_new failed!\n", __FUNCTION__);
    return;
  }
  
  err = udp_bind(upcb, IP_ANY_TYPE, 24002);
  if (err != ERR_OK)
  {
    udp_remove(upcb);
    printf("%s: udp_bind failed! err=%d\n", __FUNCTION__, err);
    return;
  }
  
  udp_recv(upcb, udp_tester_recv, NULL);
}
