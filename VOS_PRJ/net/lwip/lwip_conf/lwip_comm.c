#include "lwip_comm.h" 
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h" 
#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/tcpip.h" 
#include "usart.h"  
#include <stdio.h>
 
#define printf kprintf

__lwip_dev lwipdev;						//lwip���ƽṹ�� 
struct netif lwip_netif;				//����һ��ȫ�ֵ�����ӿ�
 
extern sys_mbox_t mbox;  				//��Ϣ���� ȫ�ֱ���(��tcpip.c���涨��)
extern u32 memp_get_memorysize(void);	//��memp.c���涨��
extern u8_t *memp_memory;				//��memp.c���涨��.
extern u8_t *ram_heap;					//��mem.c���涨��.
extern sys_mbox_t mbox;  				//��Ϣ���� ȫ�ֱ���(��tcpip.c���涨��)

extern u8_t netif_num;                  //netif.c�ļ�
extern struct tcp_seg inseg;            //tcp_in.c�ļ�
extern struct tcp_hdr *tcphdr;          //tcp_in.c�ļ�
extern struct ip_hdr *iphdr;			//tcp_in.c�ļ�
extern u32_t seqno, ackno;				//tcp_in.c�ļ�
extern u16_t tcplen;					//tcp_in.c�ļ�
extern u8_t recv_flags;					//tcp_in.c�ļ�
extern struct pbuf *recv_data;			//tcp_in.c�ļ�
extern u8_t etharp_cached_entry;        //etharp.c�ļ�
extern u16_t ip_id;                     //ip.c�ļ�
extern struct ip_reassdata *reassdatagrams; //ip_frag.c�ļ�
extern u16_t ip_reass_pbufcount;        //ip_frag.c�ļ�
extern u8_t *ram;						//mem.c�ļ�
extern struct mem *ram_end;				//mem.c�ļ�
extern struct mem *lfree;				//mem.c�ļ�
//extern sys_mutex_t mem_mutex;			//mem.c�ļ�

extern void ip_reass_timer(void *arg);  //timers.c�ļ�
extern void arp_timer(void *arg);		//timers.c�ļ�
extern void dhcp_timer_coarse(void *arg);//timers.c�ļ�
extern void dhcp_timer_fine(void *arg); //timers.c�ļ�
extern struct sys_timeo *next_timeout;  //timers.c�ļ�
extern int tcpip_tcp_timer_active;      //timers.c�ļ�
/////////////////////////////////////////////////////////////////////////////////
//lwip����������(�ں������DHCP����)

//lwip�ں������ջ(���ȼ��Ͷ�ջ��С��lwipopts.h������) 
//OS_STK * TCPIP_THREAD_TASK_STK;

//lwip DHCP����
//�����������ȼ�
#define LWIP_DHCP_TASK_PRIO       		150
//���������ջ��С
#define LWIP_DHCP_STK_SIZE  		    2048
//�����ջ�������ڴ����ķ�ʽ��������	
//OS_STK * LWIP_DHCP_TASK_STK;
//������
void lwip_dhcp_task(void *pdata); 


//������̫���жϵ���
void lwip_packet_handler(void)
{
	ethernetif_input(&lwip_netif);
}
//lwip�ں˲���,�ڴ�����
//����ֵ:0,�ɹ�;
//    ����,ʧ��
u8 lwip_comm_mem_malloc(void)
{
	u32 mempsize;
	u32 ramheapsize; 
	mempsize = memp_get_memorysize();			//�õ�memp_memory�����С
	memp_memory = vmalloc(mempsize);	//Ϊmemp_memory�����ڴ�
	ramheapsize = LWIP_MEM_ALIGN_SIZE(MEM_SIZE)+2*LWIP_MEM_ALIGN_SIZE(4*3)+MEM_ALIGNMENT;//�õ�ram heap��С
	//ram_heap=vmalloc(ramheapsize);	//Ϊram_heap�����ڴ�
//	TCPIP_THREAD_TASK_STK = vmalloc(TCPIP_THREAD_STACKSIZE*4);//���ں����������ջ
//	LWIP_DHCP_TASK_STK = vmalloc(SRAMIN,LWIP_DHCP_STK_SIZE*4);		//��dhcp�����ջ�����ڴ�ռ�
	if(!memp_memory)//������ʧ�ܵ�
	{
		lwip_comm_mem_free();
		return 1;
	}
	memset(memp_memory,0,mempsize);
//	memset(ram_heap,0,ramheapsize);
	//mymemset(TCPIP_THREAD_TASK_STK,0,TCPIP_THREAD_STACKSIZE*4);
	return 0;	
}
//lwip�ں˲���,�ڴ��ͷ�
void lwip_comm_mem_free(void)
{ 	
	vfree(memp_memory);
	//vfree(ram_heap);
//	vfree(TCPIP_THREAD_TASK_STK);
//	vfree(LWIP_DHCP_TASK_STK);
}
//lwip Ĭ��IP����
//lwipx:lwip���ƽṹ��ָ��
void lwip_comm_default_ip_set(__lwip_dev *lwipx)
{
	u32 sn0;
	sn0=*(vu32*)(0x1FFF7A10);//��ȡSTM32��ΨһID��ǰ24λ��ΪMAC��ַ�����ֽ�
	//Ĭ��Զ��IPΪ:192.168.1.100
	lwipx->remoteip[0]=192;	
	lwipx->remoteip[1]=168;
	lwipx->remoteip[2]=2;
	lwipx->remoteip[3]=101;
	//MAC��ַ����(�����ֽڹ̶�Ϊ:2.0.0,�����ֽ���STM32ΨһID)
	lwipx->mac[0]=2;//�����ֽ�(IEEE��֮Ϊ��֯ΨһID,OUI)��ַ�̶�Ϊ:2.0.0
	lwipx->mac[1]=0;
	lwipx->mac[2]=0;
	lwipx->mac[3]=(sn0>>16)&0XFF;//�����ֽ���STM32��ΨһID
	lwipx->mac[4]=(sn0>>8)&0XFFF;;
	lwipx->mac[5]=sn0&0XFF; 
	//Ĭ�ϱ���IPΪ:192.168.1.30
	lwipx->ip[0]=192;	
	lwipx->ip[1]=168;
	lwipx->ip[2]=2;
	lwipx->ip[3]=103;
	//Ĭ����������:255.255.255.0
	lwipx->netmask[0]=255;	
	lwipx->netmask[1]=255;
	lwipx->netmask[2]=255;
	lwipx->netmask[3]=0;
	//Ĭ������:192.168.1.1
	lwipx->gateway[0]=192;	
	lwipx->gateway[1]=168;
	lwipx->gateway[2]=2;
	lwipx->gateway[3]=101;
	lwipx->dhcpstatus=0;//û��DHCP	
} 
//LWIP��ʼ��(LWIP������ʱ��ʹ��)
//����ֵ:0,�ɹ�
//      1,�ڴ����
//      2,LAN8720��ʼ��ʧ��
//      3,�������ʧ��.
void Lan8720ResetPinInit();
u8 lwip_comm_init(void)
{
//	OS_CPU_SR cpu_sr;
	u32 irq_save = 0;
	struct netif *Netif_Init_Flag;		//����netif_add()����ʱ�ķ���ֵ,�����ж������ʼ���Ƿ�ɹ�
	struct ip_addr ipaddr;  			//ip��ַ
	struct ip_addr netmask; 			//��������
	struct ip_addr gw;      			//Ĭ������ 
	if(ETH_Mem_Malloc())return 1;		//�ڴ�����ʧ��
	if(lwip_comm_mem_malloc())return 1;	//�ڴ�����ʧ��
	if(LAN8720_Init())return 2;			//��ʼ��LAN8720ʧ��
	tcpip_init(NULL,NULL);				//��ʼ��tcp ip�ں�,�ú�������ᴴ��tcpip_thread�ں�����
	lwip_comm_default_ip_set(&lwipdev);	//����Ĭ��IP����Ϣ
#if LWIP_DHCP		//ʹ�ö�̬IP
	ipaddr.addr = 0;
	netmask.addr = 0;
	gw.addr = 0;
#else				//ʹ�þ�̬IP
	IP4_ADDR(&ipaddr,lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
	IP4_ADDR(&netmask,lwipdev.netmask[0],lwipdev.netmask[1] ,lwipdev.netmask[2],lwipdev.netmask[3]);
	IP4_ADDR(&gw,lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
#endif
	//OS_ENTER_CRITICAL();  //�����ٽ���
	irq_save = __vos_irq_save();
	Netif_Init_Flag=netif_add(&lwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&tcpip_input);//�������б������һ������
	//OS_EXIT_CRITICAL();  //�˳��ٽ���
	__vos_irq_restore(irq_save);
	if(Netif_Init_Flag==NULL)return 3;//�������ʧ�� 
	else//������ӳɹ���,����netifΪĬ��ֵ,���Ҵ�netif����
	{
		netif_set_default(&lwip_netif); //����netifΪĬ������
		netif_set_up(&lwip_netif);		//��netif����
	}
	return 0;//����OK.
}   
//���ʹ����DHCP
#if LWIP_DHCP

s32 task_dhcp_id = -1;
u8 *pdhcpstack = 0;
//����DHCP����
void lwip_comm_dhcp_creat(void)
{
	s32 stacksize = LWIP_DHCP_STK_SIZE*4;
	pdhcpstack = (u8*)vmalloc(LWIP_DHCP_STK_SIZE*4);
	task_dhcp_id = VOSTaskCreate(lwip_dhcp_task, 0, pdhcpstack, stacksize, LWIP_DHCP_TASK_PRIO, "dhcp_lwip");
}
//ɾ��DHCP����
void lwip_comm_dhcp_delete(void)
{
	dhcp_stop(&lwip_netif); 		//�ر�DHCP
	if (task_dhcp_id >= 0) {
		VOSTaskDelete(task_dhcp_id);
		task_dhcp_id = -1;
	}
	if (pdhcpstack) {
		vfree(pdhcpstack);
		pdhcpstack = 0;
	}
}
//DHCP��������
void lwip_dhcp_task(void *pdata)
{
	u32 ip=0,netmask=0,gw=0;
	dhcp_start(&lwip_netif);//����DHCP 
	lwipdev.dhcpstatus=0;	//����DHCP
	lwipdev.dhcpstatus=1;	//����DHCP��ȡ��
	while(1)
	{ 
		printf("���ڻ�ȡ��ַ...\r\n");
		ip=lwip_netif.ip_addr.addr;		//��ȡ��IP��ַ
		netmask=lwip_netif.netmask.addr;//��ȡ��������
		gw=lwip_netif.gw.addr;			//��ȡĬ������ 
		if(ip!=0)   					//����ȷ��ȡ��IP��ַ��ʱ��
		{
 			printf("����en��MAC��ַΪ:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
			//������ͨ��DHCP��ȡ����IP��ַ
			lwipdev.ip[3]=(uint8_t)(ip>>24); 
			lwipdev.ip[2]=(uint8_t)(ip>>16);
			lwipdev.ip[1]=(uint8_t)(ip>>8);
			lwipdev.ip[0]=(uint8_t)(ip);
			printf("ͨ��DHCP��ȡ��IP��ַ..............%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			//����ͨ��DHCP��ȡ�������������ַ
			lwipdev.netmask[3]=(uint8_t)(netmask>>24);
			lwipdev.netmask[2]=(uint8_t)(netmask>>16);
			lwipdev.netmask[1]=(uint8_t)(netmask>>8);
			lwipdev.netmask[0]=(uint8_t)(netmask);
			printf("ͨ��DHCP��ȡ����������............%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			//������ͨ��DHCP��ȡ����Ĭ������
			lwipdev.gateway[3]=(uint8_t)(gw>>24);
			lwipdev.gateway[2]=(uint8_t)(gw>>16);
			lwipdev.gateway[1]=(uint8_t)(gw>>8);
			lwipdev.gateway[0]=(uint8_t)(gw);
			printf("ͨ��DHCP��ȡ����Ĭ������..........%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			lwipdev.dhcpstatus=2;	//DHCP�ɹ�
			break;
		}else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //ͨ��DHCP�����ȡIP��ַʧ��,�ҳ�������Դ���
		{  
			lwipdev.dhcpstatus=0XFF;//DHCPʧ��.
			//ʹ�þ�̬IP��ַ
			IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			IP4_ADDR(&(lwip_netif.netmask),lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			IP4_ADDR(&(lwip_netif.gw),lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			printf("DHCP����ʱ,ʹ�þ�̬IP��ַ!\r\n");
			printf("����en��MAC��ַΪ:................%d.%d.%d.%d.%d.%d\r\n",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);
			printf("��̬IP��ַ........................%d.%d.%d.%d\r\n",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);
			printf("��������..........................%d.%d.%d.%d\r\n",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);
			printf("Ĭ������..........................%d.%d.%d.%d\r\n",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);
			break;
		}  
		VOSTaskDelay(1000);//delay_ms(500); //��ʱ250ms
	}
	lwip_comm_dhcp_delete();//ɾ��DHCP���� 
}
#endif 
//�ر�LWIP,���ͷ��ڴ�
//�˳�LWIPʱʹ��
void lwip_comm_destroy(void)
{
	u8 err;
#if LWIP_DHCP
	lwip_comm_dhcp_delete();		//dhcp����ɾ�� 
#endif
	ETH_DeInit();  					//��λ��̫��
	//OSTaskDel(TCPIP_THREAD_PRIO); 	//ɾ��LWIP�ں��߳�
	//VOSTaskDelete();
 	sys_mbox_free(&mbox);  			//ɾ��mbox��Ϣ����(��tcpip.c���涨��)
	lwip_comm_delete_next_timeout();//ɾ����ʱ�¼������һ���¼� 
	netif_remove(&lwip_netif);  	//ɾ��lwip_netif����
	//���tcp_pcb���ĸ���������(��tcp.c�ļ�����) 
	tcp_ticks=0;
	tcp_bound_pcbs=NULL;
	tcp_listen_pcbs.pcbs=NULL;
	tcp_active_pcbs=NULL;
	tcp_tw_pcbs=NULL;	
	//ɾ�������б�(netif.c�ļ���ȫ�ֱ���)
	netif_default=NULL; //Ĭ����������
	netif_list=NULL;  	//�����������
	netif_num=0;      	//���netif_num����
	//���tcp_in.c�ļ���ȫ�ֱ���
	memset(&inseg,0,sizeof(struct tcp_seg));
	tcphdr=NULL;
	iphdr=NULL;
	seqno=0;
	ackno=0;
	tcplen=0;
	recv_flags=0;	
	recv_data=NULL;
	tcp_input_pcb=NULL;
	//���etharp.c��ȫ�ֱ���
	etharp_cached_entry=0;
	//���ip.c��ȫ�ֱ���
	memset(current_netif,0,sizeof(struct netif));
	current_netif = NULL;
	memset((void*)current_header,0,sizeof(struct ip_hdr));
	current_header=NULL;
	memset(&current_iphdr_src, 0,sizeof(ip_addr_t));
	memset(&current_iphdr_dest,0,sizeof(ip_addr_t));
	ip_id=0;
	//���ip_frag.c��ȫ�ֱ���
	memset(reassdatagrams,0,sizeof(struct ip_reassdata));
	reassdatagrams=NULL;
	ip_reass_pbufcount=0;
	//���mem.c��ȫ�ֱ���
	//ram=NULL;
	//ram_end=NULL;
	//lfree=NULL;
	//OSSemDel(mem_mutex,OS_DEL_ALWAYS,&err);//ɾ�������ź���.
	//VOSSemDelete(mem_mutex);
	lwip_comm_mem_free();	//�ͷ��ڴ�.
 	ETH_Mem_Free();			//�ͷ��ڴ�
	
    //PCF8574_WriteBit(ETH_RESET_IO,1);       //Ӳ����λLAN8720A
	GPIO_AF_Set(GPIOB,11,7);				//PB11,AF7,�ָ�Ϊ����3 RX����.    
} 
//ɾ��next_timeout()���ݽṹ(������times.c�ļ�)
void lwip_comm_delete_next_timeout(void)
{
#if IP_REASSEMBLY   //IP_PREASSEMBLY = 1
	sys_untimeout(ip_reass_timer,NULL); 
#endif 
#if LWIP_ARP		//LWIP_ARP = 1
	sys_untimeout(arp_timer,NULL); 
#endif  
#if LWIP_DHCP      	//LWIP_DHCP = 1
	sys_untimeout(dhcp_timer_coarse,NULL); 
	sys_untimeout(dhcp_timer_fine,NULL); 
#endif   
#if LWIP_IGMP      	//LWIP_IGMP = 1
	sys_untimeout(igmp_timer,NULL);
#endif  
#if LWIP_DNS       	//LWIP_DNS = 1
	sys_untimeout(dns_timer,NULL);
#endif  
	if(next_timeout!=NULL)next_timeout=NULL;
	tcpip_tcp_timer_active=0;
}



























