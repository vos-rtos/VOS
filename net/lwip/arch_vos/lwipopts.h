#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include "lwip/debug.h"


extern void *vmalloc(unsigned int size);
extern void vfree(void *ptr);
extern void *vrealloc(void *ptr, unsigned int size);
extern void *vcalloc(unsigned int nitems, unsigned int size);

#define SOCKETS_DEBUG		LWIP_DBG_ON|LWIP_DBG_TYPES_ON
#define IP_DEBUG 			LWIP_DBG_ON|LWIP_DBG_TYPES_ON
#define TCP_DEBUG			LWIP_DBG_ON|LWIP_DBG_TYPES_ON
#define TCP_INPUT_DEBUG 	LWIP_DBG_ON|LWIP_DBG_TYPES_ON
#define TCP_OUTPUT_DEBUG	LWIP_DBG_ON|LWIP_DBG_TYPES_ON

//#define LWIP_TCPIP_CORE_LOCKING 0

#define LWIP_RAW 1

#define LWIP_DNS 1

#define PING_USE_SOCKETS 0

#define LWIP_RAND() rand()

#define LWIP_TIMEVAL_PRIVATE	0

#define mem_clib_free vfree

#define mem_clib_malloc vmalloc

#define mem_clib_calloc vcalloc

#define MEM_LIBC_MALLOC 1

#define sys_msleep(ms) VOSTaskDelay(ms)

#define LWIP_NETIF_HOSTNAME 	1

#define LWIP_AUTOIP 			1

#define ERRNO	1



#define LWIP_PROVIDE_ERRNO 		1

#define LWIP_TCPIP_CORE_LOCKING  0//1

#define LWIP_COMPAT_MUTEX_ALLOWED 1



#ifndef TCPIP_THREAD_PRIO
#define TCPIP_THREAD_PRIO		11	//�����ں���������ȼ�Ϊ5
#endif
#undef  DEFAULT_THREAD_PRIO
#define DEFAULT_THREAD_PRIO		2


#define SYS_LIGHTWEIGHT_PROT    1

//NO_SYS==1:��ʹ�ò���ϵͳ
#define NO_SYS                  0  // ʹ��VOS����ϵͳ

//ʹ��4�ֽڶ���ģʽ
#define MEM_ALIGNMENT           4  

//MEM_SIZE:heap�ڴ�Ĵ�С,�����Ӧ�����д������ݷ��͵Ļ����ֵ������ô�һ�� 
#define MEM_SIZE                500*1024//25600 //�ڴ�Ѵ�С

//MEMP_NUM_PBUF:memp�ṹ��pbuf����,���Ӧ�ô�ROM���߾�̬�洢�����ʹ�������ʱ,���ֵӦ�����ô�һ��
#define MEMP_NUM_PBUF           256

//MEMP_NUM_UDP_PCB:UDPЭ����ƿ�(PCB)����.ÿ�����UDP"����"��Ҫһ��PCB.
#define MEMP_NUM_UDP_PCB        6

//MEMP_NUM_TCP_PCB:ͬʱ���������TCP����
#define MEMP_NUM_TCP_PCB        10

//MEMP_NUM_TCP_PCB_LISTEN:�ܹ�������TCP��������
#define MEMP_NUM_TCP_PCB_LISTEN 6

//MEMP_NUM_SYS_TIMEOUT:�ܹ�ͬʱ�����timeout����
#define MEMP_NUM_SYS_TIMEOUT    9



#define LWIP_TCP                1  //Ϊ1��ʹ��TCP
#define TCP_TTL                 255//����ʱ��

#define MAX_QUEUE_ENTRIES 		100//20//50

#define PPP_SUPPORT				1
#define PAP_SUPPORT				1
#define CHAP_SUPPORT			1
#define PPP_IPV4_SUPPORT		1

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#undef TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE         MAX_QUEUE_ENTRIES

/**
 * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#undef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE       MAX_QUEUE_ENTRIES


/**
 * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created.
 */
#undef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE         MAX_QUEUE_ENTRIES

/*��TCP�����ݶγ�������ʱ�Ŀ���λ,���豸���ڴ��С��ʱ�����ӦΪ0*/
#define TCP_QUEUE_OOSEQ         1//0

//���TCP�ֶ�
#define TCP_MSS                 (1500 - 40)	  //TCP_MSS = (MTU - IP��ͷ��С - TCP��ͷ��С


/* ˵����
 * MEMP_MEM_MALLOC==1: vmalloc��Ϊ�ڴ��Դ�������ٶ����200KB/s, MCU 97%
 * MEMP_MEM_MALLOC==0: ��������ʼ��ʱ����ã������ٶ���� 270KB/s MCU 97%
 */
#define MEMP_MEM_MALLOC 1

/*****���ͻ������趨******MAX[270KB/s]***********************/
//MEMP_NUM_TCP_SEG:���ͬʱ�ڶ����е�TCP������
#define MEMP_NUM_TCP_SEG        224
//TCP���ͻ�������С(bytes).
#define TCP_SND_BUF             (28*TCP_MSS)
//TCP_SND_QUEUELEN: TCP���ͻ�������С(pbuf).���ֵ��СΪ(2 * TCP_SND_BUF/TCP_MSS) */
#define TCP_SND_QUEUELEN        (8* TCP_SND_BUF/TCP_MSS)
/*************************************************************/

/*****���մ����趨**********MAX[130KB/s]**********************/
//PBUF_POOL_SIZE:pbuf�ڴ�ظ���.
#define PBUF_POOL_SIZE          50//100//200//120//100
//PBUF_POOL_BUFSIZE:ÿ��pbuf�ڴ�ش�С.
#define PBUF_POOL_BUFSIZE       512//512//512
//TCP���մ���
#define TCP_WND                 (20*TCP_MSS)//(43*TCP_MSS)//(2*TCP_MSS)
/*************************************************************/

/* ---------- ICMPѡ��---------- */
#define LWIP_ICMP                 1 //ʹ��ICMPЭ��


/* ---------- DHCPѡ��---------- */
//��ʹ��DHCPʱ��λӦ��Ϊ1,LwIP 0.5.1�汾��û��DHCP����.
#define LWIP_DHCP               1


/* ---------- UDPѡ�� ---------- */
#define LWIP_UDP                1 //ʹ��UDP����
#define UDP_TTL                 255 //UDP���ݰ�����ʱ��


/* ---------- Statistics options ---------- */
#define LWIP_STATS 				1
#define LWIP_PROVIDE_ERRNO 		1

//STM32F4x7����ͨ��Ӳ��ʶ��ͼ���IP,UDP��ICMP��֡У���

//#define CHECKSUM_BY_HARDWARE //����CHECKSUM_BY_HARDWARE,ʹ��Ӳ��֡У��

#ifdef CHECKSUM_BY_HARDWARE
  //CHECKSUM_GEN_IP==0: Ӳ������IP���ݰ���֡У���
  #define CHECKSUM_GEN_IP                 0
  //CHECKSUM_GEN_UDP==0: Ӳ������UDP���ݰ���֡У���
  #define CHECKSUM_GEN_UDP                0
  //CHECKSUM_GEN_TCP==0: Ӳ������TCP���ݰ���֡У���
  #define CHECKSUM_GEN_TCP                0 
  //CHECKSUM_CHECK_IP==0: Ӳ����������IP���ݰ�֡У���
  #define CHECKSUM_CHECK_IP               0
  //CHECKSUM_CHECK_UDP==0: Ӳ����������UDP���ݰ�֡У���
  #define CHECKSUM_CHECK_UDP              0
  //CHECKSUM_CHECK_TCP==0: Ӳ����������TCP���ݰ�֡У���
  #define CHECKSUM_CHECK_TCP              0
#else
  /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 1
  /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                1
  /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                1
  /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               1
  /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              1
  /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              1
#endif


//LWIP_NETCONN==1:ʹ��NETCON����(Ҫ��ʹ��api_lib.c)
#define LWIP_NETCONN                    1

//LWIP_SOCKET==1:ʹ��Sicket API(Ҫ��ʹ��sockets.c)
#define LWIP_SOCKET                     1

#define LWIP_COMPAT_MUTEX               1

#define LWIP_SO_RCVTIMEO                1 //��ֹ�����߳�

#define TCPIP_THREAD_STACKSIZE          (4*1024)//1500	//�ں������ջ��С
#define DEFAULT_UDP_RECVMBOX_SIZE       1000
#define DEFAULT_THREAD_STACKSIZE        512

#define LWIP_TCP_SACK_OUT 1 // ��������ȷ��

#if 1
// ����DHCP
#define LWIP_DHCP 1
#define LWIP_NETIF_HOSTNAME 1

// ����DNS
#define LWIP_DNS 1

// ����DHCPD
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 1) // DHCPD��Ҫ�õ�1��timeout��ʱ��
#if !LWIP_DHCP
// �ڲ�����DHCP�������Ҳ��ʹ��DHCPD, ������ҪΪIP��ַ0.0.0.0����DHCP UDP�˿ں�
#define LWIP_IP_ACCEPT_UDP_PORT(port) ((port) == LWIP_IANA_PORT_DHCP_CLIENT)
#endif

// �㲥��������
// ����������������, ��ô����Ҫ���׽���������SOF_BROADCASTѡ������շ��㲥���ݰ�
//#define IP_SOF_BROADCAST 1
//#define IP_SOF_BROADCAST_RECV 1

// ����IPv6
#define LWIP_IPV6 0
#define LWIP_ND6_RDNSS_MAX_DNS_SERVERS LWIP_DNS // ����SLAAC��ȡDNS�������ĵ�ַ

// ����������ӿڼ�ת�����ݰ�
// 88W8801��STA��uAP���ܿ���ͬʱ��, �����Ҫ�ֻ����������88W8801�������ȵ��������, ����Ҫ����������ѡ��
// ����/�ֻ� <---> 88W8801 <---> ��������·���� <---> Internet
// ѡ�����, ����Ҫ��·����������·�ɱ����, ��Ϊ��ʱ88W8801Ҳ�൱��һ��·����
#define IP_FORWARD 1
#define LWIP_IPV6_FORWARD 1

// ����������Ϣ���
#define LWIP_DEBUG
#define DHCPD_DEBUG LWIP_DBG_ON
#define ND6D_DEBUG LWIP_DBG_ON


#define LWIP_DEBUG                     0

#define ICMP_DEBUG                      LWIP_DBG_OFF
#endif
#endif
