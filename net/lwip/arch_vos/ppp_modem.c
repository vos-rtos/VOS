#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "netif/ppp/pppapi.h"
#include "vringbuf.h"
#include "vos.h"

#include "ping.h"

#define printf kprintf

#define false  0
#define true   1

s32 CUSTOM_ReadMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);
s32 CUSTOM_WriteMODEM(u8 *pBuf, u32 dwLen, u32 dwTimeout);

#define MODEM_WRITE CUSTOM_WriteMODEM//__CUSTOM_DirectWriteAT
#define MODEM_READ CUSTOM_ReadMODEM//__CUSTOM_DirectReadAT

err_t ppposnetif_init(struct netif* netif);
err_t tcp_Client_connected(void* arg, struct tcp_pcb* pcb, err_t err);

void hex_str_dump (const char *desc, const void *addr, const int len);
#define hex_str_dump
ppp_pcb*     ppp = NULL;
struct netif ppp_netif;

enum {
	STATUS_PPP_AT_MODE = 0,
	STATUS_PPP_DATA_MODE,
};

#define PPP_RECV_MAX	1024
#define MAX_ITEMS(arr) (sizeof(arr)/sizeof(arr[0]))
typedef struct StPppNetDev {
	struct StVOSRingBuf *pRecvRing;
	s32 	status; //AT״̬������״̬
}StPppNetDev;

struct StPppNetDev gPppNetDev;

uint8_t lwip_comm_init();
s32 PppModemInit()
{
	void *pRing = 0;
	struct StPppNetDev *pPppNetDev = &gPppNetDev;

	pRing = vmalloc(sizeof(struct StVOSRingBuf) + PPP_RECV_MAX);
	if(pRing == 0) return -1;

	pPppNetDev->pRecvRing = VOSRingBufBuild(pRing, sizeof(struct StVOSRingBuf) + PPP_RECV_MAX);
	pPppNetDev->status = STATUS_PPP_AT_MODE;

	lwip_comm_init();

	return 0;
}

s32 ppp_conn_success = false;

s32 PppModemSend(u8 *buf, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	if (buf && len>0) {
		ret = MODEM_WRITE(buf, len, timeout_ms);
	}
	return ret;
}

s32 PppModemRecv(u8 *buf, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	u32 timemark = 0;
	struct StPppNetDev *pPppNetDev = &gPppNetDev;

	if (pPppNetDev->pRecvRing) {
		timemark = VOSGetTimeMs();

		while (1) {
			ret = VOSRingBufGet(pPppNetDev->pRecvRing, buf, len);
			if (ret > 0) {
				break;
			}
			if (VOSGetTimeMs()-timemark > timeout_ms) {
				break;
			}
			VOSTaskDelay(100);
		}
	}
	return ret;
}


s32 PppModemAtCmdRsp(u8 *cmd, u8 *rsp, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	s32 min = 0;
	u32 timemark = 0;
	timemark = VOSGetTimeMs();

	kprintf("AT SEND: %s\r\n", cmd);
	ret = PppModemSend(cmd, strlen(cmd), timeout_ms);
	if (ret == strlen(cmd)) {
		ret = PppModemRecv(rsp, len-1, timeout_ms);
		if (ret > 0 && rsp && len > 0) {
			min = ret < (len-1) ? ret : (len-1);
			rsp[min] = 0;
			ret = min;
			kprintf("AT RECV: %s\r\n", rsp);
		}
		else {//�����ʱ����0
			ret = 0;
		}
	}
	return ret;
}

typedef void (*FUNC_AT_CB)(s8 *cmd, void *priv);
#define FUNC_NULL	0

typedef struct StAtOption {
	u8 *cmd;		//AT����
	u8 *rsp;		//�ڴ���Ӧ
	s32 retry_cnts; //���Դ���
	u32 timeout_ms; //ÿ�γ�ʱʱ��
	FUNC_AT_CB pfunc;
}StAtOption;

void atd_connect_cb(s8 *cmd, void *priv)
{
	struct StPppNetDev *pPppNetDev = &gPppNetDev;
	if (strstr(cmd, "atd")) {
		if (*(s32*)priv == 0) {//����ʧ��
			pPppNetDev->status = STATUS_PPP_AT_MODE;
		}
		else{//���ӳɹ�
			pPppNetDev->status = STATUS_PPP_DATA_MODE;
		}
	}
}

StAtOption gArrAtOpts_HWMe909[] = {
		{"AT\r\n", 								"OK", 		3, 2000, atd_connect_cb},
		{"ATE0\r\n", 							"OK", 		3, 2000, atd_connect_cb},
		{"at+cgmi\r\n", 						"OK", 		3, 2000, atd_connect_cb},
		{"at+cgmm\r\n", 						"OK", 		3, 2000, atd_connect_cb},
		{"at+cgmr\r\n", 						"OK", 		3, 2000, atd_connect_cb},
		{"at+cgsn\r\n", 						"OK", 		3, 2000, atd_connect_cb},
		{"at+cpin?\r\n", 						"OK", 		3, 2000, atd_connect_cb},
		{"at+cimi\r\n", 						"OK", 		3, 2000, atd_connect_cb},
		{"at+cgreg?\r\n", 						"OK", 		3, 2000, atd_connect_cb},
		{"at+cgdcont=1,\"ip\",\"3gnet\"\r\n", 	"OK", 		3, 2000, atd_connect_cb},
		{"atd*99#\r\n",							"CONNECT", 	3, 2000, atd_connect_cb},
};



s32 PppModemAtProcess(StAtOption *pArrAtOpt, s32 max_num)
{
	s32 ret = 0;
	u8 cmd[100];
	u8 rsp[512];
	s32 retry_cnts = 0;
	s32 i, j;
	s32 priv = 0;
	for (i=0; i< max_num; i++) {
		if (pArrAtOpt[i].cmd[0]) {
			for (j=0; j<pArrAtOpt[i].retry_cnts; j++) {
				ret = PppModemAtCmdRsp(pArrAtOpt[i].cmd, rsp, sizeof(rsp), pArrAtOpt[i].timeout_ms);
				if (ret > 0) {
					if (strstr(rsp, pArrAtOpt[i].rsp)) {
						if (i == max_num-1) {//"atd*99#\r\n", ���ӳɹ�����������ģʽ
							priv = 1;
						}
						if (pArrAtOpt[i].pfunc) {
							pArrAtOpt[i].pfunc(pArrAtOpt[i].cmd, &priv);
						}
						break;
					}
				}
				VOSTaskDelay(1000);
			}
		}
	}
	return 0;
}

uint32_t output_cb(ppp_pcb* pcb, u8_t* data, uint32_t len, void* ctx)
{
	s32 writed = 0;
    writed = MODEM_WRITE(data, len, 2000);
    if (writed > 0) {
    	hex_str_dump ("MODEM SEND: ", data, writed);
    }
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


#define printf kprintf
#define hex_str_dump
//void hex_str_dump (const char *desc, const void *addr, const int len)
//{
//    int i;
//    unsigned char buff[17];
//    unsigned char *pc = (unsigned char*)addr;
//    // Output description if given.
//    if (desc != NULL)
//        printf ("%s:\r\n", desc);
//    // Process every byte in the data.
//    for (i = 0; i < len; i++) {
//        // Multiple of 16 means new line (with line offset).
//        if ((i % 16) == 0) {
//            // Just don't print ASCII for the zeroth line.
//            if (i != 0)
//                printf ("  %s\r\n", buff);
//            // Output the offset.
//            printf ("  %04x ", i);
//        }
//        // Now the hex code for the specific character.
//        printf (" %02x", pc[i]);
//        // And store a printable ASCII character for later.
//        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
//            buff[i % 16] = '.';
//        else
//            buff[i % 16] = pc[i];
//        buff[(i % 16) + 1] = '\0';
//    }
//    // Pad out last line if not exactly 16 characters.
//    while ((i % 16) != 0) {
//        printf ("   ");
//        i++;
//    }
//    // And print the final ASCII bit.
//    printf ("  %s\r\n", buff);
//}

void TaskPppInput(void *param)
{
	u32 irq_save = 0;
	u8 buf[1024] = {0};
	s32 readed = 0;
	struct StPppNetDev *pPppNetDev = &gPppNetDev;

    while (1) {
		readed = MODEM_READ(buf, sizeof(buf), 10);
		if (readed > 0) {
			if (pPppNetDev->status == STATUS_PPP_DATA_MODE) {
				hex_str_dump ("MODEM RECV: ", buf, readed);
				if (ppp) {
					pppos_input_tcpip(ppp, buf, readed);  // 0x7e
				}
			}else {
				if (pPppNetDev->pRecvRing) {
					if (readed != VOSRingBufSet(pPppNetDev->pRecvRing, buf, readed)) {
						kprintf("warnning: PPP recv ring buf overflow!\r\n");
					}
				}
			}
		}
		else {
			VOSTaskDelay(10);
		}
    }
}

static long long ppp_input_stack[1024];

uint8_t lwip_comm_init(void)
{
	struct StPppNetDev *pPppNetDev = &gPppNetDev;

    uint8_t ctx = 0;

	s32 task_id;
	task_id = VOSTaskCreate(TaskPppInput, 0, ppp_input_stack, sizeof(ppp_input_stack), TASK_PRIO_NORMAL, "task_ppp_input");

	while(1) {
		PppModemAtProcess(gArrAtOpts_HWMe909, MAX_ITEMS(gArrAtOpts_HWMe909));
		if (pPppNetDev->status == STATUS_PPP_DATA_MODE) break;
		VOSTaskDelay(1000);
	}

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

    ping_init();
    dns_init();
//	void lwip_ping(char *ip_str);
//	while (1) {
//	lwip_ping("221.122.82.30");
//	VOSTaskDelay(2000);
//	}
    return ctx;
}

s32 PppCheck()
{
	return ppp_conn_success;
}
