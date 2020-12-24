// lwip 2.0.3版本中的ethernetif.c文件位于lwip-2.0.3.zip压缩包的src/netif文件夹下
// lwip 2.1.0版本中的ethernetif.c文件则被移动到了contrib-2.1.0.zip压缩包的examples/ethernetif文件夹里面了, 里面有一些细微的修改
#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#define WIFI_DISPLAY_PACKET_SIZE 1 // 显示收发的数据包的大小
#define WIFI_DISPLAY_PACKET_RX 0 // 显示收到的数据包内容
#define WIFI_DISPLAY_PACKET_RXRATES 0 // 显示数据包的接收速率
#define WIFI_DISPLAY_PACKET_TX 0 // 显示发送的数据包内容

typedef uint8_t bss_t;
typedef int8_t port_t;

// lwip中struct netif的state成员指向的用户自定义数据
// application state的意思就是与这个对象(或结构体)有关的应用程序自定义数据
struct ethernetif {
  struct eth_addr *ethaddr;
  
  bss_t bss; // 网络接口所在的BSS号
  const void *data; // 收到的数据
  port_t port; // 收到的数据所在的通道号
};

err_t ethernetif_init(struct netif *netif);
void ethernetif_input(struct netif *netif);

#endif
