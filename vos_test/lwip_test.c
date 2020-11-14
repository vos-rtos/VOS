#include "lwip/sockets.h"
#include "vos.h"

struct netif;
s32 net_init();
struct netif *GetNetIfPtr();
void NetAddrInfoPrt(struct netif *pNetIf);
s32 NetDhcpClient(u32 timeout)
{
	s32 ret = 0;
	u32 spend_ms = 0;
	ret = net_init();
	if (ret < 0) {
		kprintf("error: net_init failed!\r\n");
		return ret;
	}
	ret = 0;
	if (GetNetIfPtr()) {
		dhcp_start(GetNetIfPtr());
		while (DhcpClientCheck(GetNetIfPtr()) == 0) {
			VOSTaskDelay(10);
			spend_ms += 10;
			if (spend_ms > timeout) {
				kprintf("error: dhcp client timeout %d (ms)!\r\n", timeout);
				ret = -2;
				break;
			}
		}
		if (ret == 0) { //打印IP地址信息
			NetAddrInfoPrt(GetNetIfPtr());
		}
	}
	return ret;
}

s32 SetNetWorkInfo (s8 *ipAddr, s8 *netMask, s8 *gateWay)
{
	s32 ret = 0;
	ip4_addr_t addr;
	ip4_addr_t mask;
	ip4_addr_t gw;
	ret = net_init();
	if (ret < 0) {
		kprintf("error: net_init failed!\r\n");
		return ret;
	}
    if (inet_aton(ipAddr, &addr.addr) <= 0 ||
    	inet_aton(netMask, &mask.addr) <= 0 ||
		inet_aton(gateWay, &gw.addr) <= 0 )
    {
    	return -1;
    }
    netif_set_addr(GetNetIfPtr(), &addr, &mask, &gw);
	return ret;
}


int get_socket_err(int s);
void  sock_tcp_test()
{
	int ret = 0;
    int sockfd, n;
    struct sockaddr_in    servaddr;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	kprintf("ERROR!\r\n");
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(55750);
    //if( inet_aton("103.46.128.49", &servaddr.sin_addr) <= 0){
    if( inet_aton("192.168.2.101", &servaddr.sin_addr) <= 0){
    	kprintf("ERROR!\r\n");
    }
    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    	kprintf("ERROR!\r\n");
    }
    kprintf("send msg to server: \n");
    s8 send_buf[] = "hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!hello world!";
    //s8 send_buf[] = "hello world!";
    while(1)
    {
		if( send(sockfd, send_buf, strlen(send_buf), 0) < 0)
		{
			kprintf("ERROR!\r\n");
			//break;
		}
//		if ((ret = get_socket_err(sockfd)) < 0) {
//			kprintf("get_socket_err: %d, \"%s\"!\r\n", ret, lwip_strerr(ret));
//		}
		//VOSTaskDelay(1*1000);
    }
    close(sockfd);
}
void lwip_test_task(void *arg)
{
	s32 ret = 0;
	ret = DhcpClient(10*1000);
	if (ret < 0) {
		kprintf("dhcp error!\r\n");
	}
	sock_tcp_test();
	while(1) {
		VOSTaskDelay(1*1000);
	}
}

void lan8720Regs()
{
	u32 data = 0;
	u32 eth_addr = 0;
	u8 binData[17];

	data = ETH_ReadPHYRegister(eth_addr, 0x00);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x00 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x01);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x01 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x02);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x02 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x03);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x03 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x04);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x04 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x05);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x05 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x06);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x06 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x09);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x09 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x11);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x11 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x12);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x12 = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(200);
	data = ETH_ReadPHYRegister(eth_addr, 0x1F);
	memset(binData, 0, sizeof(binData));
	itoa(data, binData, 2);
	kprintf("0x1F = 0x%04x, %04s\r\n", data, binData);
	VOSTaskDelay(3000);
}


void sockets_stresstest_init_loopback(int addr_family);
void sockets_stresstest_init_server(int addr_family, u16_t server_port);
void sockets_stresstest_init_client(const char *remote_ip, u16_t remote_port);
void lwip_inner_test()
{
	sockets_stresstest_init_client("192.168.2.100", 5555);
}



static long long lwip_stack[2048];
void lwip_test()
{
	kprintf("lwip_test!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(lwip_test_task, 0, lwip_stack, sizeof(lwip_stack), TASK_PRIO_NORMAL, "lwip_test_task");
}

