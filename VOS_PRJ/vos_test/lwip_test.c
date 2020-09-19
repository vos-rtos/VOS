
#include "lwip_comm.h"
#include "vos.h"
#if 1
void lwip_test()
{
	s32 res = 0;
	res=lwip_comm_init();
	if(res==0)
	{
		lwip_comm_dhcp_creat();
		while(lwipdev.dhcpstatus==0||lwipdev.dhcpstatus==1)
		{
			VOSTaskDelay(10*1000);
		}
	}
	//lwip_comm_destroy();
}
#endif
