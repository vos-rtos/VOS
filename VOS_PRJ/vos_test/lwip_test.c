#include "lwip/sockets.h"
#include "vos.h"

struct netif;
void net_init();
struct netif *GetNetIfPtr();
void NetAddrInfoPrt(struct netif *pNetIf);
void lwip_test_init()
{
	net_init();

	dhcp_start(GetNetIfPtr());
	while (DhcpClientCheck(GetNetIfPtr()) == 0) {
		VOSTaskDelay(10);
	}
	NetAddrInfoPrt(GetNetIfPtr());
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
    servaddr.sin_port = htons(6666);
    if( inet_aton("192.168.2.104", &servaddr.sin_addr) <= 0){
    	kprintf("ERROR!\r\n");
    }
    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    	kprintf("ERROR!\r\n");
    }
    kprintf("send msg to server: \n");
    while(1)
    {
		if( send(sockfd, "hello world!", strlen("hello world!"), 0) < 0)
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
	lwip_test_init();
	sock_tcp_test();
	while(1) {
		VOSTaskDelay(1*1000);
	}
}



static long long lwip_stack[2048];
void lwip_test()
{
	kprintf("lwip_test!\r\n");
	s32 task_id;
	task_id = VOSTaskCreate(lwip_test_task, 0, lwip_stack, sizeof(lwip_stack), TASK_PRIO_NORMAL, "lwip_test_task");
}

