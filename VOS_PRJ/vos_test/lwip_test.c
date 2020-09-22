#include "lwip/sockets.h"
#include "lwip_comm.h"
#include "vos.h"

void lwip_test_init()
{
	s32 res = 0;
	res=lwip_comm_init();
	if(res==0)
	{
		lwip_comm_dhcp_creat();
		while(lwipdev.dhcpstatus==0||lwipdev.dhcpstatus==1)
		{
			VOSTaskDelay(100);
		}
	}
	//lwip_comm_destroy();
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
		if ((ret = get_socket_err(sockfd)) < 0) {
			kprintf("get_socket_err: %d, \"%s\"!\r\n", ret, lwip_strerr(ret));
		}
		VOSTaskDelay(1*1000);
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

