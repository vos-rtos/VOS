#include "lan8720.h"
#include "stm32f4x7_eth.h"
#include "usart.h"  

void VOSDelayUs(u32 us);

void Lan8720ResetPinInit()
{
  GPIO_InitTypeDef  GPIO_InitStructure;
#if 0
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_SetBits(GPIOB, GPIO_Pin_0);
#else
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
   GPIO_Init(GPIOD, &GPIO_InitStructure);

   GPIO_SetBits(GPIOD, GPIO_Pin_3);
#endif
}

void Lan8720Reset()
{
#if 0
	GPIO_ResetBits(GPIOB,GPIO_Pin_0);
	VOSDelayUs(50*1000);
	GPIO_SetBits(GPIOB, GPIO_Pin_0);
#else
	GPIO_ResetBits(GPIOD,GPIO_Pin_3);
	VOSDelayUs(50*1000);
	GPIO_SetBits(GPIOD, GPIO_Pin_3);
#endif
}

//LAN8720初始化
//返回值:0,成功;
//    其他,失败
u8 LAN8720_Init(void)
{
	u8 rval=0;
	//ETH IO接口初始化
 	RCC->AHB1ENR|=1<<0;     //使能PORTA时钟 
 	RCC->AHB1ENR|=1<<1;     //使能PORTB时钟 
 	RCC->AHB1ENR|=1<<2;     //使能PORTC时钟  
	RCC->AHB1ENR|=1<<6;     //使能PORTG时钟 
 	RCC->APB2ENR|=1<<14;   	//使能SYSCFG时钟
	SYSCFG->PMC|=1<<23;		//使用RMII PHY接口.

	GPIO_Set(GPIOA, GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_7, GPIO_Mode_AF,GPIO_OType_PP, GPIO_Speed_100MHz, GPIO_PuPd_UP);		//PA1,2,7复用输出
	GPIO_Set(GPIOC, GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5, GPIO_Mode_AF,GPIO_OType_PP, GPIO_Speed_100MHz, GPIO_PuPd_UP);		//PC1,4,5复用输出
	GPIO_Set(GPIOG, GPIO_Pin_13|GPIO_Pin_14, GPIO_Mode_AF,GPIO_OType_PP, GPIO_Speed_100MHz, GPIO_PuPd_UP);		//PG13,14复用输出
	GPIO_Set(GPIOB, GPIO_Pin_11, GPIO_Mode_AF, GPIO_OType_PP, GPIO_Speed_100MHz, GPIO_PuPd_UP);				//PB11复用输出
 
	GPIO_AF_Set(GPIOA,1,11);	//PA1,AF11
 	GPIO_AF_Set(GPIOA,2,11);	//PA2,AF11
 	GPIO_AF_Set(GPIOA,7,11);	//PA7,AF11
	
  	GPIO_AF_Set(GPIOB,11,11);	//PB11,AF11
 
	GPIO_AF_Set(GPIOC,1,11);	//PC1,AF11
 	GPIO_AF_Set(GPIOC,4,11);	//PC4,AF11
 	GPIO_AF_Set(GPIOC,5,11);	//PC5,AF11
 
  	GPIO_AF_Set(GPIOG,13,11);	//PG13,AF11
 	GPIO_AF_Set(GPIOG,14,11);	//PG14,AF11  
	
 	Lan8720ResetPinInit();
 	Lan8720Reset();

 	MY_NVIC_Init(0,0,ETH_IRQn,2);//配置ETH中的分组
	rval=ETH_MACDMA_Config();
	return !rval;				//ETH的规则为:0,失败;1,成功;所以要取反一下 
}
//得到8720的速度模式
//返回值:
//001:10M半双工
//101:10M全双工
//010:100M半双工
//110:100M全双工
//其他:错误.
u8 LAN8720_Get_Speed(void)
{
	u8 speed;
	speed=((ETH_ReadPHYRegister(0x00,31)&0x1C)>>2); //从LAN8720的31号寄存器中读取网络速度和双工模式
	return speed;
}

//初始化ETH MAC层及DMA配置
//返回值:ETH_ERROR,发送失败(0)
//		ETH_SUCCESS,发送成功(1)
u8 ETH_MACDMA_Config(void)
{
	u8 rval;
	ETH_InitTypeDef ETH_InitStructure; 
	
	//使能以太网时钟
	RCC->AHB1ENR|=7<<25;//使能ETH MAC/MAC_Tx/MAC_Rx时钟
	
	ETH_DeInit();  								//AHB总线重启以太网
	ETH_SoftwareReset();  						//软件重启网络
	while (ETH_GetSoftwareResetStatus() == SET);//等待软件重启网络完成 
	ETH_StructInit(&ETH_InitStructure); 	 	//初始化网络为默认值

	///网络MAC参数设置
	//ETH_InitStructure.ETH_Speed = ETH_Speed_100M;
	ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;   			//开启网络自适应功能
	ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;					//关闭反馈
	ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Enable; 		//关闭重传功能kp
	ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable; 	//关闭自动去除PDA/CRC功能
	ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;						//关闭接收所有的帧
	ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;//允许接收所有广播帧
	ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;			//关闭混合模式的地址过滤
	ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;//对于组播地址使用完美地址过滤
	ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;	//对单播地址使用完美地址过滤
#ifdef CHECKSUM_BY_HARDWARE
	ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable; 			//开启ipv4和TCP/UDP/ICMP的帧校验和卸载
#endif
	//当我们使用帧校验和卸载功能的时候，一定要使能存储转发模式,存储转发模式中要保证整个帧存储在FIFO中,
	//这样MAC能插入/识别出帧校验值,当真校验正确的时候DMA就可以处理帧,否则就丢弃掉该帧
	ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; //开启丢弃TCP/IP错误帧
	ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;     //开启接收数据的存储转发模式
	ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;   //开启发送数据的存储转发模式

	ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;     	//禁止转发错误帧
	ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;	//不转发过小的好帧
	ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;  		//打开处理第二帧功能
	ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;  	//开启DMA传输的地址对齐功能
	ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;            			//开启固定突发功能
	ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;     		//DMA发送的最大突发长度为32个节拍
	ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;			//DMA接收的最大突发长度为32个节拍
	ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
	ETH_InitStructure.Sys_Clock_Freq = 168;//系统时钟频率为168Mhz
	rval=ETH_Init(&ETH_InitStructure, LAN8720_PHY_ADDRESS);//配置ETH
	if(rval==ETH_SUCCESS)//配置成功
	{
		ETH_DMAITConfig(ETH_DMA_IT_NIS|ETH_DMA_IT_R,ENABLE);//使能以太网接收中断
	}
	return rval;
}
extern void lwip_packet_handler(void);		//在lwip_comm.c里面定义

int dumphex(const unsigned char *buf, int size)
{
	int i;
	for(i=0; i<size; i++)
		kprintf("%02x,%s", buf[i], (i+1)%16?"":"\r\n");
	return 0;
}


void lwip_packet_handler();

void ETH_IRQHandler(void)
{ 
	int len = 0;
	VOSIntEnter();
	while((len=ETH_GetRxPktSize(DMARxDescToGet))!=0) 	//检测是否收到数据包
	{ 		
//		kprintf("------------------------------------\r\n");
//		dumphex((unsigned char*)(DMARxDescToGet->Buffer1Addr), len);
		lwip_packet_handler();
		//ETH_Rx_Packet();
	} 
	ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
	ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
	VOSIntExit ();
}  
//接收一个网卡数据包
//返回值:网络数据包帧结构体
FrameTypeDef ETH_Rx_Packet(void)
{ 
	u32 framelength=0;
	FrameTypeDef frame={0,0};   
	//检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
	if((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) != (u32)RESET)
	{	
		frame.length=ETH_ERROR; 
		if ((ETH->DMASR&ETH_DMASR_RBUS) != (u32)RESET)
		{ 
			ETH->DMASR = ETH_DMASR_RBUS;//清除ETH DMA的RBUS位 
			ETH->DMARPDR = 0;//恢复DMA接收
		}
		return frame;//错误,OWN位被设置了
	}  
	if( ((DMARxDescToGet->Status & ETH_DMARxDesc_ES)==(u32)RESET)&&
		((DMARxDescToGet->Status & ETH_DMARxDesc_LS)!=(u32)RESET)&&
		((DMARxDescToGet->Status & ETH_DMARxDesc_FS)!=(u32)RESET))
	{       
		framelength = ((DMARxDescToGet->Status&ETH_DMARxDesc_FL) >> ETH_DMARxDesc_FrameLengthShift)-4;//得到接收包帧长度(不包含4字节CRC)
 		frame.buffer = DMARxDescToGet->Buffer1Addr;//得到包数据所在的位置
	}
	else
		framelength = ETH_ERROR;//错误
	frame.length = framelength;
	frame.descriptor = DMARxDescToGet;
	//更新ETH DMA全局Rx描述符为下一个Rx描述符
	//为下一次buffer读取设置下一个DMA Rx描述符
	DMARxDescToGet = (ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr);
	return frame;  
}
//发送一个网卡数据包
//FrameLength:数据包长度
//返回值:ETH_ERROR,发送失败(0)
//		ETH_SUCCESS,发送成功(1)
u8 ETH_Tx_Packet(u16 FrameLength)
{   
	//检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
	if((DMATxDescToSet->Status&ETH_DMATxDesc_OWN)!=(u32)RESET)return ETH_ERROR;//错误,OWN位被设置了 
 	DMATxDescToSet->ControlBufferSize=(FrameLength&ETH_DMATxDesc_TBS1);//设置帧长度,bits[12:0]
	DMATxDescToSet->Status|=ETH_DMATxDesc_LS|ETH_DMATxDesc_FS;//设置最后一个和第一个位段置位(1个描述符传输一帧)
  	DMATxDescToSet->Status|=ETH_DMATxDesc_OWN;//设置Tx描述符的OWN位,buffer重归ETH DMA
	if((ETH->DMASR&ETH_DMASR_TBUS)!=(u32)RESET)//当Tx Buffer不可用位(TBUS)被设置的时候,重置它.恢复传输
	{ 
		ETH->DMASR=ETH_DMASR_TBUS;//重置ETH DMA TBUS位 
		ETH->DMATPDR=0;//恢复DMA发送
	} 
	//更新ETH DMA全局Tx描述符为下一个Tx描述符
	//为下一次buffer发送设置下一个DMA Tx描述符 
	DMATxDescToSet=(ETH_DMADESCTypeDef*)(DMATxDescToSet->Buffer2NextDescAddr);    
	return ETH_SUCCESS;   
}

u32 ETH_GetCurrentTxBuffer(void)
{  
  return DMATxDescToSet->Buffer1Addr;
}






















