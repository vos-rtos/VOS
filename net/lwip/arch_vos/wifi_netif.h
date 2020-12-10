// lwip 2.0.3�汾�е�ethernetif.c�ļ�λ��lwip-2.0.3.zipѹ������src/netif�ļ�����
// lwip 2.1.0�汾�е�ethernetif.c�ļ����ƶ�����contrib-2.1.0.zipѹ������examples/ethernetif�ļ���������, ������һЩϸ΢���޸�
#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#define WIFI_DISPLAY_PACKET_SIZE 1 // ��ʾ�շ������ݰ��Ĵ�С
#define WIFI_DISPLAY_PACKET_RX 0 // ��ʾ�յ������ݰ�����
#define WIFI_DISPLAY_PACKET_RXRATES 0 // ��ʾ���ݰ��Ľ�������
#define WIFI_DISPLAY_PACKET_TX 0 // ��ʾ���͵����ݰ�����

typedef uint8_t bss_t;
typedef int8_t port_t;

// lwip��struct netif��state��Աָ����û��Զ�������
// application state����˼�������������(��ṹ��)�йص�Ӧ�ó����Զ�������
struct ethernetif {
  struct eth_addr *ethaddr;
  
  bss_t bss; // ����ӿ����ڵ�BSS��
  const void *data; // �յ�������
  port_t port; // �յ����������ڵ�ͨ����
};

err_t ethernetif_init(struct netif *netif);
void ethernetif_input(struct netif *netif);

#endif
