#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "netif/ppp/pppapi.h"

#define printf kprintf

#define false  0
#define true   1

s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);
s32 CUSTOM_WriteMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);

#define MODEM_WRITE CUSTOM_WriteMODEM//__CUSTOM_DirectWriteAT
#define MODEM_READ CUSTOM_ReadMODEM//__CUSTOM_DirectReadAT

err_t ppposnetif_init(struct netif* netif);
err_t tcp_Client_connected(void* arg, struct tcp_pcb* pcb, err_t err);

ppp_pcb*     ppp = NULL;
struct netif ppp_netif;


s32 ppp_conn_success = false;


//void PRINT(bool recv, uint8_t* data, uint16_t len)
//{
//    if (recv)
//    {
//        printf("recv:");
//    }
//    else
//    {
//        printf("send:");
//    }
//    for (uint16_t i = 0; i < len; i++)
//    {
//        printf("%02x ", data[i]);
//    }
//    printf("\r\n");
//}

uint32_t output_cb(ppp_pcb* pcb, u8_t* data, uint32_t len, void* ctx)
{
	s32 writed = 0;
    writed = MODEM_WRITE(data, len, 2000);
    return writed;
}

void status_cb(ppp_pcb* pcb, int err_code, void* ctx)
{
    struct netif* pppif = ppp_netif(pcb);
    LWIP_UNUSED_ARG(ctx);

    switch (err_code)
    {
        case PPPERR_NONE: {
#if LWIP_DNS
            const ip_addr_t* ns;
#endif /* LWIP_DNS */
            printf("status_cb: Connected\n");
#if PPP_IPV4_SUPPORT
            printf("   our_ipaddr  = %s\n", ipaddr_ntoa(&pppif->ip_addr));
            printf("   his_ipaddr  = %s\n", ipaddr_ntoa(&pppif->gw));
            printf("   netmask     = %s\n", ipaddr_ntoa(&pppif->netmask));
            ppp_conn_success = true;
#if LWIP_DNS
            ns = ( ip_addr_t* )dns_getserver(0);
            printf("   dns1        = %s\n", ipaddr_ntoa(ns));
            ns = ( ip_addr_t* )dns_getserver(1);
            printf("   dns2        = %s\n", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#endif /* PPP_IPV4_SUPPORT */
#if PPP_IPV6_SUPPORT
            printf("   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
            break;
        }
        case PPPERR_PARAM: {
            printf("status_cb: Invalid parameter\n");
            break;
        }
        case PPPERR_OPEN: {
            printf("status_cb: Unable to open PPP session\n");
            break;
        }
        case PPPERR_DEVICE: {
            printf("status_cb: Invalid I/O device for PPP\n");
            break;
        }
        case PPPERR_ALLOC: {
            printf("status_cb: Unable to allocate resources\n");
            break;
        }
        case PPPERR_USER: {
            printf("status_cb: User interrupt\n");
            break;
        }
        case PPPERR_CONNECT: {
            printf("status_cb: Connection lost\n");
            break;
        }
        case PPPERR_AUTHFAIL: {
            printf("status_cb: Failed authentication challenge\n");
            break;
        }
        case PPPERR_PROTOCOL: {
            printf("status_cb: Failed to meet protocol\n");
            break;
        }
        case PPPERR_PEERDEAD: {
            printf("status_cb: Connection timeout\n");
            break;
        }
        case PPPERR_IDLETIMEOUT: {
            printf("status_cb: Idle Timeout\n");
            break;
        }
        case PPPERR_CONNECTTIME: {
            printf("status_cb: Max connect time reached\n");
            break;
        }
        case PPPERR_LOOPBACK: {
            printf("status_cb: Loopback detected\n");
            break;
        }
        default: {
            printf("status_cb: Unknown error code %d\n", err_code);
            break;
        }
    }

    /*
     * This should be in the switch case, this is put outside of the switch
     * case for example readability.
     */

    if (err_code == PPPERR_NONE)
    {
        return;
    }

    /* ppp_close() was previously called, don't reconnect */
    if (err_code == PPPERR_USER)
    {
        /* ppp_free(); -- can be called here */
        return;
    }
    ppp_conn_success = false;
    /*
     * Try to reconnect in 30 seconds, if you need a modem chatscript you have
     * to do a much better signaling here ;-)
     */
    ppp_connect(pcb, 30);
    /* OR ppp_listen(pcb); */
}





void TaskPppInput(void *param)
{
	u32 irq_save = 0;
	u8 buf[1024] = {0};
	s32 readed = 0;
    for (;;)
    {
        if (ppp)
        {
        	readed = MODEM_READ(buf, sizeof(buf), 10);
        	if (readed > 0) {
				irq_save = __local_irq_save();
				pppos_input_tcpip(ppp, buf, readed);  // 0x7e
				__local_irq_restore(irq_save);
        	}
        }
		VOSTaskDelay(10);
    }
}

static long long ppp_input_stack[1024];

uint8_t lwip_comm_init(void)
{

    uint8_t ctx = 0;

	s32 task_id;
	task_id = VOSTaskCreate(TaskPppInput, 0, ppp_input_stack, sizeof(ppp_input_stack), TASK_PRIO_NORMAL, "task_ppp_input");

	VOSTaskDelay(10);
    output_cb(ppp, ( uint8_t* )"ATD*99#\r\n", 9, &ctx);
    VOSTaskDelay(10);

    tcpip_init(NULL, NULL);

    ppp = pppos_create(&ppp_netif, output_cb, status_cb, &ctx);

    if (ppp_connect(ppp, 0) != ERR_OK)
    {
        printf("ppp connect failed\r\n");
        while (1)
            ;
    }
    else
    {
        printf("ppp connect success\r\n");
    }
    ppp_set_default(ppp);

    return ctx;
}

err_t tcp_Client_connected(void* arg, struct tcp_pcb* pcb, err_t err)
{
    printf("hello\r\n");
    return ERR_OK;
}

struct tcp_pcb* pcb;

//void create_tcp(void)
//{
//
//    ip4_addr_t ipaddr;
//    IP4_ADDR(&ipaddr, 125, 123, 139, 21);
//    pcb = tcp_new();
//    if (pcb)
//    {
//        printf("connect to TCP");
//        tcp_bind(pcb, IP_ADDR_ANY, 0);
//        extern err_t tcp_Client_connected(void* arg, struct tcp_pcb* pcb, err_t err);
//        tcp_connect(pcb, &ipaddr, 1082, tcp_Client_connected);
//    }
//    for (uint16_t i = 0; i < 5; i++)
//    {
//        if (pcb->state == 4)
//        {
//            break;
//        }
//        osDelay(1000);
//    }
//}

s32 PppCheck()
{
	return ppp_conn_success;
}

//void connect_to_server(void const* argument)
//{
//
//    while (!ppp_conn_success)
//    {
//        osDelay(10);
//    }
//
//    //    void mqtt_init( void );
//    //    mqtt_init();
//    //    for( ;; ){
//    //        osDelay( 500 );
//    //        #include "mqtt.h"
//    //        void example_publish( void );
//    //        example_publish( );
//    //    }
//
//    create_tcp();
//    char data[32] = { 0 };
//    for (;;)
//    {
//        printf("------------------------------TCP STATE:%d----------------\r\n", pcb->state);
//        if (ppp_conn_success && (pcb->state == 0 || pcb == NULL))
//        {
//            if (pcb != NULL)
//            {
//                tcp_close(pcb);
//            }
//            create_tcp();
//        }
//        if (!ppp_conn_success && pcb != NULL)
//        {
//            if (pcb != NULL)
//            {
//                tcp_close(pcb);
//                pcb = NULL;
//            }
//        }
//
//        if (pcb != NULL && pcb->state == 4)
//        {
//            static uint32_t i = 0;
//            memset(data, 0, 32);
//            if (sprintf(data, "%d\r\n", i++) > 0)
//            {
//                while (ERR_OK != tcp_write(pcb, data, strlen(data), TCP_WRITE_FLAG_COPY))
//                    osDelay(10);
//                while (ERR_OK != tcp_write(pcb, "123456789abcdefghijklmnopqrstuvwxyz\r\n", strlen("123456789abcdefghijklmnopqrstuvwxyz\r\n"), TCP_WRITE_FLAG_COPY))
//                    osDelay(10);
//                printf("%s\r\n", data);
//            }
//            osDelay(5000);
//        }
//        osDelay(1000);
//    }
//}

//void tcp_connect_init(void)
//{
//    osThreadDef(tcp, connect_to_server, osPriorityNormal, 1, 512);
//    TCP_CLIENT_TASK_HANDLE = osThreadCreate(osThread(tcp), NULL);
//}
