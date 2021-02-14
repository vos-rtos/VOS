#ifdef STM32F429xx
#include "vringbuf.h"
#include "sai.h"
#include "stm32F4xx_hal.h"

#define SAI_RING_TX_DEFAULT (16*1024) //(32*1024)
#define SAI_RING_RX_DEFAULT (128*1024) //(32*1024)
#define SAI_TX_BUF_DEFAULT (8*1024) //(16*1024)
#define SAI_RX_BUF_DEFAULT (8*1024) //(16*1024)

typedef struct StSaiBus {
	s32 mode; //0: player, 1: recorder
	SAI_HandleTypeDef handle;
	StVOSRingBuf *ring_rx; //接收环形缓冲
	StVOSRingBuf *ring_tx; //发送环形缓冲
	s32 rx_ring_max;
	s32 tx_ring_max;
	u8 *rx_buf0;
	u8 *rx_buf1;
	s32 rx_len;
	u8 *tx_buf0;
	u8 *tx_buf1;
	s32 tx_len;

	volatile s32 is_dma_tx_working;
	StVOSTimer *timer; //定时器如果没满buffer， 就超时直接发送

	DMA_HandleTypeDef hdma_tx;
	DMA_HandleTypeDef hdma_rx;

	s32 is_inited; //是否已经初始化
}StSaiBus;


struct StSaiBus gSaiBus[3];

void sai_tx_timer(void *ptimer, void *parg)
{
	static s32 count = 0;
	StVOSTimer *p = (StVOSTimer*)ptimer;
	struct StSaiBus *pSaiBus = (struct StSaiBus *)parg;
	if (pSaiBus->handle.Instance == SAI1_Block_A) {
		if (!pSaiBus->is_dma_tx_working) {
			__HAL_DMA_ENABLE(&pSaiBus->hdma_tx);
			pSaiBus->is_dma_tx_working = 1;
		}
	}
	//kprintf("%s->%s count=%d running!\r\n", __FUNCTION__, p->name, count++);
}

struct StSaiBus *SaiBusPtr(SAI_HandleTypeDef *sai)
{
	int i = 0;
	for (i=0; i<sizeof(gSaiBus)/sizeof(gSaiBus[0]); i++) {
		if (gSaiBus[i].handle.Instance == sai) {
			return &gSaiBus[i];
		}
	}
	return 0;
}
void sai_mode_set(s32 port, s32 mode)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	pSaiBus->mode = mode;
}


s32 sai_open(s32 port, u32 SAI_Mode, u32 SAI_Clock_Polarity, u32 SAI_DataFormat)
{
	s32 err = 0;
	struct StSaiBus *pSaiBus = &gSaiBus[port];

	if (pSaiBus->is_inited == 0) {
		pSaiBus->ring_rx = VOSRingBufCreate(pSaiBus->rx_ring_max =
				(pSaiBus->rx_ring_max?pSaiBus->rx_ring_max:(SAI_RING_RX_DEFAULT+sizeof(StVOSRingBuf))));
		if (pSaiBus->ring_rx == 0) {
			err = -1;
			goto END;
		}

		pSaiBus->rx_buf0 = vmalloc(pSaiBus->rx_len = (pSaiBus->rx_len ? pSaiBus->rx_len : SAI_RX_BUF_DEFAULT));
		if (pSaiBus->rx_buf0 == 0) {
			err = -2;
			goto END;
		}
		pSaiBus->rx_buf1 = vmalloc(pSaiBus->rx_len = (pSaiBus->rx_len ? pSaiBus->rx_len : SAI_RX_BUF_DEFAULT));
		if (pSaiBus->rx_buf1 == 0) {
			err = -3;
			goto END;
		}

		pSaiBus->ring_tx = VOSRingBufCreate(pSaiBus->tx_ring_max =
				(pSaiBus->tx_ring_max?pSaiBus->tx_ring_max:(SAI_RING_TX_DEFAULT+sizeof(StVOSRingBuf))));
		if (pSaiBus->ring_tx == 0) {
			err = -4;
			goto END;
		}

		pSaiBus->tx_buf0 = vmalloc(pSaiBus->tx_len = (pSaiBus->tx_len ? pSaiBus->tx_len : SAI_TX_BUF_DEFAULT));
		if (pSaiBus->tx_buf0 == 0) {
			err = -5;
			goto END;
		}

		pSaiBus->tx_buf1 = vmalloc(pSaiBus->tx_len = (pSaiBus->tx_len ? pSaiBus->tx_len : SAI_TX_BUF_DEFAULT));
		if (pSaiBus->tx_buf1 == 0) {
			err = -6;
			goto END;
		}
		//if (pSaiBus->mode == MODE_I2S_RECORDER) {//录音器，master， 推数据为收数据。
			memset(pSaiBus->tx_buf0, 0, pSaiBus->tx_len);
			memset(pSaiBus->tx_buf1, 0, pSaiBus->tx_len);
		//}


		//创建软件定时器，在第一次数据小于ring buf max时，设定定时器发送
		pSaiBus->timer = VOSTimerCreate(VOS_TIMER_TYPE_ONE_SHOT, 500, sai_tx_timer, pSaiBus, "timer");
		if (pSaiBus->timer == 0) {
			err = -7;
			goto END;
		}

	}
	if (port == 0) { //从设备，同步接收

		HAL_SAI_DeInit(&pSaiBus->handle);                          //清除以前的配置
		pSaiBus->handle.Instance=SAI1_Block_B;                     //SAI1 Bock B
		pSaiBus->handle.Init.AudioMode=SAI_Mode;                       //设置SAI1工作模式
		pSaiBus->handle.Init.Synchro=SAI_SYNCHRONOUS;             //音频模块同步
		pSaiBus->handle.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;   //立即驱动音频模块输出
		pSaiBus->handle.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;   //使能主时钟分频器(MCKDIV)
		pSaiBus->handle.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF;  //设置FIFO阈值,1/4 FIFO
		pSaiBus->handle.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;     //SIA时钟源为PLL2S
		pSaiBus->handle.Init.MonoStereoMode=SAI_STEREOMODE;        //立体声模式
		pSaiBus->handle.Init.Protocol=SAI_FREE_PROTOCOL;           //设置SAI1协议为:自由协议(支持I2S/LSB/MSB/TDM/PCM/DSP等协议)
		pSaiBus->handle.Init.DataSize=SAI_DataFormat;                     //设置数据大小
		pSaiBus->handle.Init.FirstBit=SAI_FIRSTBIT_MSB;            //数据MSB位优先
		pSaiBus->handle.Init.ClockStrobing=SAI_Clock_Polarity;                   //数据在时钟的上升/下降沿选通

		//帧设置
		pSaiBus->handle.FrameInit.FrameLength=64;                  //设置帧长度为64,左通道32个SCK,右通道32个SCK.
		pSaiBus->handle.FrameInit.ActiveFrameLength=32;            //设置帧同步有效电平长度,在I2S模式下=1/2帧长.
		pSaiBus->handle.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS信号为SOF信号+通道识别信号
		pSaiBus->handle.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;    //FS低电平有效(下降沿)
		pSaiBus->handle.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT;  //在slot0的第一位的前一位使能FS,以匹配飞利浦标准

		//SLOT设置
		pSaiBus->handle.SlotInit.FirstBitOffset=0;                 //slot偏移(FBOFF)为0
		pSaiBus->handle.SlotInit.SlotSize=SAI_SLOTSIZE_32B;        //slot大小为32位
		pSaiBus->handle.SlotInit.SlotNumber=2;                     //slot数为2个
		pSaiBus->handle.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//使能slot0和slot1

		HAL_SAI_Init(&pSaiBus->handle);
		SAIB_DMA_Enable();
		__HAL_SAI_ENABLE(&pSaiBus->handle);                       //使能SAI
	}
	else if (port == 1) { //主设备，异步发送

		HAL_SAI_DeInit(&pSaiBus->handle);                          //清除以前的配置
		pSaiBus->handle.Instance=SAI1_Block_A;                     //SAI1 Bock A
		pSaiBus->handle.Init.AudioMode=SAI_Mode;                       //设置SAI1工作模式
		pSaiBus->handle.Init.Synchro=SAI_ASYNCHRONOUS;             //音频模块异步
		pSaiBus->handle.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;   //立即驱动音频模块输出
		pSaiBus->handle.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;   //使能主时钟分频器(MCKDIV)
		pSaiBus->handle.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF;  //设置FIFO阈值,1/4 FIFO
		pSaiBus->handle.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;     //SIA时钟源为PLL2S
		pSaiBus->handle.Init.MonoStereoMode=SAI_STEREOMODE;        //立体声模式
		pSaiBus->handle.Init.Protocol=SAI_FREE_PROTOCOL;           //设置SAI1协议为:自由协议(支持I2S/LSB/MSB/TDM/PCM/DSP等协议)
		pSaiBus->handle.Init.DataSize=SAI_DataFormat;                     //设置数据大小
		pSaiBus->handle.Init.FirstBit=SAI_FIRSTBIT_MSB;            //数据MSB位优先
		pSaiBus->handle.Init.ClockStrobing=SAI_Clock_Polarity;                   //数据在时钟的上升/下降沿选通

		//帧设置
		pSaiBus->handle.FrameInit.FrameLength=64;                  //设置帧长度为64,左通道32个SCK,右通道32个SCK.
		pSaiBus->handle.FrameInit.ActiveFrameLength=32;            //设置帧同步有效电平长度,在I2S模式下=1/2帧长.
		pSaiBus->handle.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS信号为SOF信号+通道识别信号
		pSaiBus->handle.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;    //FS低电平有效(下降沿)
		pSaiBus->handle.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT;  //在slot0的第一位的前一位使能FS,以匹配飞利浦标准

		//SLOT设置
		pSaiBus->handle.SlotInit.FirstBitOffset=0;                 //slot偏移(FBOFF)为0
		pSaiBus->handle.SlotInit.SlotSize=SAI_SLOTSIZE_32B;        //slot大小为32位
		pSaiBus->handle.SlotInit.SlotNumber=2;                     //slot数为2个
		pSaiBus->handle.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//使能slot0和slot1

		HAL_SAI_Init(&pSaiBus->handle);
		__HAL_SAI_ENABLE(&pSaiBus->handle);                       //使能SAI
	}

	if (pSaiBus->is_inited==0) {
		pSaiBus->is_inited = 1;
	}
END:
	return err;

}
//
//SAI_HandleTypeDef SAI1A_Handler;        //SAI1 Block A句柄
//SAI_HandleTypeDef SAI1B_Handler;        //SAI1 Block B句柄
//DMA_HandleTypeDef SAI1_TXDMA_Handler;   //DMA发送句柄
//DMA_HandleTypeDef SAI1_RXDMA_Handler;   //DMA接收句柄

//SAI Block A初始化,I2S,飞利浦标准
//mode:工作模式,可以设置:SAI_MODEMASTER_TX/SAI_MODEMASTER_RX/SAI_MODESLAVE_TX/SAI_MODESLAVE_RX
//cpol:数据在时钟的上升/下降沿选通，可以设置：SAI_CLOCKSTROBING_FALLINGEDGE/SAI_CLOCKSTROBING_RISINGEDGE
//datalen:数据大小,可以设置：SAI_DATASIZE_8/10/16/20/24/32
//void SAIA_Init(u32 mode,u32 cpol,u32 datalen)
//{
//    HAL_SAI_DeInit(&SAI1A_Handler);                          //清除以前的配置
//    SAI1A_Handler.Instance=SAI1_Block_A;                     //SAI1 Bock A
//    SAI1A_Handler.Init.AudioMode=mode;                       //设置SAI1工作模式
//    SAI1A_Handler.Init.Synchro=SAI_ASYNCHRONOUS;             //音频模块异步
//    SAI1A_Handler.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;   //立即驱动音频模块输出
//    SAI1A_Handler.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;   //使能主时钟分频器(MCKDIV)
//    SAI1A_Handler.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF;  //设置FIFO阈值,1/4 FIFO
//    SAI1A_Handler.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;     //SIA时钟源为PLL2S
//    SAI1A_Handler.Init.MonoStereoMode=SAI_STEREOMODE;        //立体声模式
//    SAI1A_Handler.Init.Protocol=SAI_FREE_PROTOCOL;           //设置SAI1协议为:自由协议(支持I2S/LSB/MSB/TDM/PCM/DSP等协议)
//    SAI1A_Handler.Init.DataSize=datalen;                     //设置数据大小
//    SAI1A_Handler.Init.FirstBit=SAI_FIRSTBIT_MSB;            //数据MSB位优先
//    SAI1A_Handler.Init.ClockStrobing=cpol;                   //数据在时钟的上升/下降沿选通
//
//    //帧设置
//    SAI1A_Handler.FrameInit.FrameLength=64;                  //设置帧长度为64,左通道32个SCK,右通道32个SCK.
//    SAI1A_Handler.FrameInit.ActiveFrameLength=32;            //设置帧同步有效电平长度,在I2S模式下=1/2帧长.
//    SAI1A_Handler.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS信号为SOF信号+通道识别信号
//    SAI1A_Handler.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;    //FS低电平有效(下降沿)
//    SAI1A_Handler.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT;  //在slot0的第一位的前一位使能FS,以匹配飞利浦标准
//
//    //SLOT设置
//    SAI1A_Handler.SlotInit.FirstBitOffset=0;                 //slot偏移(FBOFF)为0
//    SAI1A_Handler.SlotInit.SlotSize=SAI_SLOTSIZE_32B;        //slot大小为32位
//    SAI1A_Handler.SlotInit.SlotNumber=2;                     //slot数为2个
//    SAI1A_Handler.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//使能slot0和slot1
//
//    HAL_SAI_Init(&SAI1A_Handler);
//    __HAL_SAI_ENABLE(&SAI1A_Handler);                       //使能SAI
//}
//
////SAI Block B初始化,I2S,飞利浦标准
////mode:工作模式,可以设置:SAI_MODEMASTER_TX/SAI_MODEMASTER_RX/SAI_MODESLAVE_TX/SAI_MODESLAVE_RX
////cpol:数据在时钟的上升/下降沿选通，可以设置：SAI_CLOCKSTROBING_FALLINGEDGE/SAI_CLOCKSTROBING_RISINGEDGE
////datalen:数据大小,可以设置：SAI_DATASIZE_8/10/16/20/24/32
//void SAIB_Init(u32 mode,u32 cpol,u32 datalen)
//{
//    HAL_SAI_DeInit(&SAI1B_Handler);                         //清除以前的配置
//    SAI1B_Handler.Instance=SAI1_Block_B;                    //SAI1 Bock B
//    SAI1B_Handler.Init.AudioMode=mode;                      //设置SAI1工作模式
//    SAI1B_Handler.Init.Synchro=SAI_SYNCHRONOUS;             //音频模块同步
//    SAI1B_Handler.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;  //立即驱动音频模块输出
//    SAI1B_Handler.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;  //使能主时钟分频器(MCKDIV)
//    SAI1B_Handler.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF; //设置FIFO阈值,1/4 FIFO
//    SAI1B_Handler.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;    //SIA时钟源为PLL2S
//    SAI1B_Handler.Init.MonoStereoMode=SAI_STEREOMODE;       //立体声模式
//    SAI1B_Handler.Init.Protocol=SAI_FREE_PROTOCOL;          //设置SAI1协议为:自由协议(支持I2S/LSB/MSB/TDM/PCM/DSP等协议)
//    SAI1B_Handler.Init.DataSize=datalen;                    //设置数据大小
//    SAI1B_Handler.Init.FirstBit=SAI_FIRSTBIT_MSB;           //数据MSB位优先
//    SAI1B_Handler.Init.ClockStrobing=cpol;                  //数据在时钟的上升/下降沿选通
//
//    //帧设置
//    SAI1B_Handler.FrameInit.FrameLength=64;                 //设置帧长度为64,左通道32个SCK,右通道32个SCK.
//    SAI1B_Handler.FrameInit.ActiveFrameLength=32;           //设置帧同步有效电平长度,在I2S模式下=1/2帧长.
//    SAI1B_Handler.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS信号为SOF信号+通道识别信号
//    SAI1B_Handler.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;   //FS低电平有效(下降沿)
//    SAI1B_Handler.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT; //在slot0的第一位的前一位使能FS,以匹配飞利浦标准
//
//    //SLOT设置
//    SAI1B_Handler.SlotInit.FirstBitOffset=0;                //slot偏移(FBOFF)为0
//    SAI1B_Handler.SlotInit.SlotSize=SAI_SLOTSIZE_32B;       //slot大小为32位
//    SAI1B_Handler.SlotInit.SlotNumber=2;                    //slot数为2个
//    SAI1B_Handler.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//使能slot0和slot1
//
//    HAL_SAI_Init(&SAI1B_Handler);
//    SAIB_DMA_Enable();                                      //使能SAI的DMA功能
//    __HAL_SAI_ENABLE(&SAI1B_Handler);                       //使能SAI
//}

//SAI底层驱动，引脚配置，时钟使能
//此函数会被HAL_SAI_Init()调用
//hsdram:SAI句柄
void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef GPIO_Initure;
	struct StSaiBus *pSaiBus = SaiBusPtr(hsai->Instance);


    __HAL_RCC_SAI1_CLK_ENABLE();                //使能SAI1时钟
    __HAL_RCC_GPIOE_CLK_ENABLE();               //使能GPIOE时钟

    //初始化PE2,3,4,5,6Sai
    GPIO_Initure.Pin=GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;          //推挽复用
    GPIO_Initure.Pull=GPIO_PULLUP;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //高速
    GPIO_Initure.Alternate=GPIO_AF6_SAI1;       //复用为SAI
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);         //初始化

    if (hsai->Instance == SAI1_Block_A) {

        //发送DMA
         __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟
         __HAL_LINKDMA(&pSaiBus->handle,hdmatx,pSaiBus->hdma_tx);        //将DMA与SAI联系起来
         pSaiBus->hdma_tx.Instance=DMA2_Stream3;                       //DMA2数据流3
         pSaiBus->hdma_tx.Init.Channel=DMA_CHANNEL_0;                  //通道0
         pSaiBus->hdma_tx.Init.Direction=DMA_MEMORY_TO_PERIPH;         //存储器到外设模式
         pSaiBus->hdma_tx.Init.PeriphInc=DMA_PINC_DISABLE;             //外设非增量模式
         pSaiBus->hdma_tx.Init.MemInc=DMA_MINC_ENABLE;                 //存储器增量模式
         pSaiBus->hdma_tx.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;           //外设数据长度:16/32位
         pSaiBus->hdma_tx.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD;              //存储器数据长度:16/32位
         pSaiBus->hdma_tx.Init.Mode=DMA_CIRCULAR;                      //使用循环模式
         pSaiBus->hdma_tx.Init.Priority=DMA_PRIORITY_HIGH;             //高优先级
         pSaiBus->hdma_tx.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //不使用FIFO

         pSaiBus->hdma_tx.Init.MemBurst=DMA_MBURST_SINGLE;             //存储器单次突发传输
         pSaiBus->hdma_tx.Init.PeriphBurst=DMA_PBURST_SINGLE;          //外设突发单次传输
         HAL_DMA_DeInit(&pSaiBus->hdma_tx);                            //先清除以前的设置
         HAL_DMA_Init(&pSaiBus->hdma_tx);	                            //初始化DMA

         HAL_DMAEx_MultiBufferStart(&pSaiBus->hdma_tx,(u32)pSaiBus->tx_buf0,
        		 (u32)&SAI1_Block_A->DR, (u32)pSaiBus->tx_buf1, pSaiBus->tx_len/2);//开启双缓冲
         __HAL_DMA_DISABLE(&pSaiBus->hdma_tx);                         //先关闭发送DMA
         VOSDelayUs(10);                                                   //10us延时，防止-O2优化出问题
         __HAL_DMA_ENABLE_IT(&pSaiBus->hdma_tx, DMA_IT_TC);             //开启传输完成中断
         __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_tx, DMA_FLAG_TCIF3_7);     //清除DMA传输完成中断标志位
         HAL_NVIC_SetPriority(DMA2_Stream3_IRQn,0,0);                    //DMA中断优先级
         HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    }
    else if (hsai->Instance == SAI1_Block_B) {
         //接收DMA
           __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟
           __HAL_LINKDMA(&pSaiBus->handle,hdmarx,pSaiBus->hdma_rx);        //将DMA与SAI联系起来
           pSaiBus->hdma_rx.Instance=DMA2_Stream5;                       //DMA2数据流5
           pSaiBus->hdma_rx.Init.Channel=DMA_CHANNEL_0;                  //通道0
           pSaiBus->hdma_rx.Init.Direction=DMA_PERIPH_TO_MEMORY;         //外设到存储器模式
           pSaiBus->hdma_rx.Init.PeriphInc=DMA_PINC_DISABLE;             //外设非增量模式
           pSaiBus->hdma_rx.Init.MemInc=DMA_MINC_ENABLE;                 //存储器增量模式
           pSaiBus->hdma_rx.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;           //外设数据长度:16/32位
           pSaiBus->hdma_rx.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD;              //存储器数据长度:16/32位
           pSaiBus->hdma_rx.Init.Mode=DMA_CIRCULAR;                      //使用循环模式
           pSaiBus->hdma_rx.Init.Priority=DMA_PRIORITY_MEDIUM;           //中等优先级
           pSaiBus->hdma_rx.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //不使用FIFO
           pSaiBus->hdma_rx.Init.MemBurst=DMA_MBURST_SINGLE;             //存储器单次突发传输
           pSaiBus->hdma_rx.Init.PeriphBurst=DMA_PBURST_SINGLE;          //外设突发单次传输
           HAL_DMA_DeInit(&pSaiBus->hdma_rx);                            //先清除以前的设置
           HAL_DMA_Init(&pSaiBus->hdma_rx);	                            //初始化DMA

           HAL_DMAEx_MultiBufferStart(&pSaiBus->hdma_rx,(u32)&SAI1_Block_B->DR,
        		   (u32)pSaiBus->rx_buf0,(u32)pSaiBus->rx_buf1,pSaiBus->rx_len/2);//开启双缓冲
           __HAL_DMA_DISABLE(&pSaiBus->hdma_rx);                         //先关闭接收DMA
           VOSDelayUs(10);                                                   //10us延时，防止-O2优化出问题
           __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_rx,DMA_FLAG_TCIF1_5);     //清除DMA传输完成中断标志位
           __HAL_DMA_ENABLE_IT(&pSaiBus->hdma_rx,DMA_IT_TC);             //开启传输完成中断

           HAL_NVIC_SetPriority(DMA2_Stream5_IRQn,0,1);                    //DMA中断优先级
           HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    }
}

//SAI Block A采样率设置
//采样率计算公式:
//MCKDIV!=0: Fs=SAI_CK_x/[512*MCKDIV]
//MCKDIV==0: Fs=SAI_CK_x/256
//SAI_CK_x=(HSE/pllm)*PLLI2SN/PLLI2SQ/(PLLI2SDIVQ+1)
//一般HSE=25Mhz
//pllm:在Stm32_Clock_Init设置的时候确定，一般是25
//PLLI2SN:一般是192~432
//PLLI2SQ:2~15
//PLLI2SDIVQ:0~31
//MCKDIV:0~15
//SAI A分频系数表@pllm=8,HSE=25Mhz,即vco输入频率为1Mhz
const u16 SAI_PSC_TBL[][5]=
{
	{800 ,344,7,0,12},	//8Khz采样率
	{1102,429,2,18,2},	//11.025Khz采样率
	{1600,344,7, 0,6},	//16Khz采样率
	{2205,429,2,18,1},	//22.05Khz采样率
	{3200,344,7, 0,3},	//32Khz采样率
	{4410,429,2,18,0},	//44.1Khz采样率
	{4800,344,7, 0,2},	//48Khz采样率
	{8820,271,2, 2,1},	//88.2Khz采样率
	{9600,344,7, 0,1},	//96Khz采样率
	{17640,271,2,2,0},	//176.4Khz采样率
	{19200,344,7,0,0},	//192Khz采样率
};

//开启SAIB的DMA功能,HAL库没有提供此函数
//因此我们需要自己操作寄存器编写一个
void SAIA_DMA_Enable(void)
{
    u32 tempreg=0;
    tempreg=SAI1_Block_A->CR1;          //先读出以前的设置
	tempreg|=1<<17;					    //使能DMA
	SAI1_Block_A->CR1=tempreg;		    //写入CR1寄存器中
}

//开启SAIB的DMA功能,HAL库没有提供此函数
//因此我们需要自己操作寄存器编写一个
void SAIB_DMA_Enable(void)
{
    u32 tempreg=0;
    tempreg=SAI1_Block_B->CR1;          //先读出以前的设置
	tempreg|=1<<17;					    //使能DMA
	SAI1_Block_B->CR1=tempreg;		    //写入CR1寄存器中
}
//设置SAIA的采样率(@MCKEN)
//samplerate:采样率,单位:Hz
//返回值:0,设置成功;1,无法设置.
u8 SAIA_SampleRate_Set(s32 port, u32 samplerate)
{
    u8 i=0;
	struct StSaiBus *pSaiBus = &gSaiBus[port];
    RCC_PeriphCLKInitTypeDef RCCSAI1_Sture;
	for(i=0;i<(sizeof(SAI_PSC_TBL)/10);i++)//看看改采样率是否可以支持
	{
		if((samplerate/10)==SAI_PSC_TBL[i][0])break;
	}
    if(i==(sizeof(SAI_PSC_TBL)/10))return 1;//搜遍了也找不到
    RCCSAI1_Sture.PeriphClockSelection=RCC_PERIPHCLK_SAI_PLLI2S;    //外设时钟源选择
    RCCSAI1_Sture.PLLI2S.PLLI2SN=(u32)SAI_PSC_TBL[i][1];            //设置PLLI2SN
    RCCSAI1_Sture.PLLI2S.PLLI2SQ=(u32)SAI_PSC_TBL[i][2];            //设置PLLI2SQ
    //设置PLLI2SDivQ的时候SAI_PSC_TBL[i][3]要加1，因为HAL库中会在把PLLI2SDivQ赋给寄存器DCKCFGR的时候减1
    RCCSAI1_Sture.PLLI2SDivQ=SAI_PSC_TBL[i][3]+1;                   //设置PLLI2SDIVQ
    HAL_RCCEx_PeriphCLKConfig(&RCCSAI1_Sture);                      //设置时钟

    __HAL_RCC_SAI_BLOCKACLKSOURCE_CONFIG(RCC_SAIACLKSOURCE_PLLI2S); //设置SAI1时钟来源为PLLI2SQ

    __HAL_SAI_DISABLE(&pSaiBus->handle);                              //关闭SAI
    pSaiBus->handle.Init.AudioFrequency=samplerate;                   //设置播放频率
    HAL_SAI_Init(&pSaiBus->handle);                                   //初始化SAI
    SAIA_DMA_Enable();                                              //开启SAI的DMA功能
    __HAL_SAI_ENABLE(&pSaiBus->handle);                               //开启SAI
    return 0;
}
////SAIA TX DMA配置
////设置为双缓冲模式,并开启DMA传输完成中断
////buf0:M0AR地址.
////buf1:M1AR地址.
////num:每次传输数据量
////width:位宽(存储器和外设,同时设置),0,8位;1,16位;2,32位;
//void SAIA_TX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width)
//{
//    u32 memwidth=0,perwidth=0;      //外设和存储器位宽
//    switch(width)
//    {
//        case 0:         //8位
//            memwidth=DMA_MDATAALIGN_BYTE;
//            perwidth=DMA_PDATAALIGN_BYTE;
//            break;
//        case 1:         //16位
//            memwidth=DMA_MDATAALIGN_HALFWORD;
//            perwidth=DMA_PDATAALIGN_HALFWORD;
//            break;
//        case 2:         //32位
//            memwidth=DMA_MDATAALIGN_WORD;
//            perwidth=DMA_PDATAALIGN_WORD;
//            break;
//
//    }
//    __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟
//    __HAL_LINKDMA(&SAI1A_Handler,hdmatx,SAI1_TXDMA_Handler);        //将DMA与SAI联系起来
//    SAI1_TXDMA_Handler.Instance=DMA2_Stream3;                       //DMA2数据流3
//    SAI1_TXDMA_Handler.Init.Channel=DMA_CHANNEL_0;                  //通道0
//    SAI1_TXDMA_Handler.Init.Direction=DMA_MEMORY_TO_PERIPH;         //存储器到外设模式
//    SAI1_TXDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE;             //外设非增量模式
//    SAI1_TXDMA_Handler.Init.MemInc=DMA_MINC_ENABLE;                 //存储器增量模式
//    SAI1_TXDMA_Handler.Init.PeriphDataAlignment=perwidth;           //外设数据长度:16/32位
//    SAI1_TXDMA_Handler.Init.MemDataAlignment=memwidth;              //存储器数据长度:16/32位
//    SAI1_TXDMA_Handler.Init.Mode=DMA_CIRCULAR;                      //使用循环模式
//    SAI1_TXDMA_Handler.Init.Priority=DMA_PRIORITY_HIGH;             //高优先级
//    SAI1_TXDMA_Handler.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //不使用FIFO
//
//}
//
////SAIA TX DMA配置
////设置为双缓冲模式,并开启DMA传输完成中断
////buf0:M0AR地址.
////buf1:M1AR地址.
////num:每次传输数据量
////width:位宽(存储器和外设,同时设置),0,8位;1,16位;2,32位;
//void SAIA_RX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width)
//{
//    u32 memwidth=0,perwidth=0;      //外设和存储器位宽
//    switch(width)
//    {
//        case 0:         //8位
//            memwidth=DMA_MDATAALIGN_BYTE;
//            perwidth=DMA_PDATAALIGN_BYTE;
//            break;
//        case 1:         //16位
//            memwidth=DMA_MDATAALIGN_HALFWORD;
//            perwidth=DMA_PDATAALIGN_HALFWORD;
//            break;
//        case 2:         //32位
//            memwidth=DMA_MDATAALIGN_WORD;
//            perwidth=DMA_PDATAALIGN_WORD;
//            break;
//
//    }
//    __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟
//    __HAL_LINKDMA(&SAI1B_Handler,hdmarx,SAI1_RXDMA_Handler);        //将DMA与SAI联系起来
//    SAI1_RXDMA_Handler.Instance=DMA2_Stream5;                       //DMA2数据流5
//    SAI1_RXDMA_Handler.Init.Channel=DMA_CHANNEL_0;                  //通道0
//    SAI1_RXDMA_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;         //外设到存储器模式
//    SAI1_RXDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE;             //外设非增量模式
//    SAI1_RXDMA_Handler.Init.MemInc=DMA_MINC_ENABLE;                 //存储器增量模式
//    SAI1_RXDMA_Handler.Init.PeriphDataAlignment=perwidth;           //外设数据长度:16/32位
//    SAI1_RXDMA_Handler.Init.MemDataAlignment=memwidth;              //存储器数据长度:16/32位
//    SAI1_RXDMA_Handler.Init.Mode=DMA_CIRCULAR;                      //使用循环模式
//    SAI1_RXDMA_Handler.Init.Priority=DMA_PRIORITY_MEDIUM;           //中等优先级
//    SAI1_RXDMA_Handler.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //不使用FIFO
//    SAI1_RXDMA_Handler.Init.MemBurst=DMA_MBURST_SINGLE;             //存储器单次突发传输
//    SAI1_RXDMA_Handler.Init.PeriphBurst=DMA_PBURST_SINGLE;          //外设突发单次传输
//    HAL_DMA_DeInit(&SAI1_RXDMA_Handler);                            //先清除以前的设置
//    HAL_DMA_Init(&SAI1_RXDMA_Handler);	                            //初始化DMA
//
//    HAL_DMAEx_MultiBufferStart(&SAI1_RXDMA_Handler,(u32)&SAI1_Block_B->DR,(u32)buf0,(u32)buf1,num);//开启双缓冲
//    __HAL_DMA_DISABLE(&SAI1_RXDMA_Handler);                         //先关闭接收DMA
//    VOSDelayUs(10);                                                   //10us延时，防止-O2优化出问题
//    __HAL_DMA_CLEAR_FLAG(&SAI1_RXDMA_Handler,DMA_FLAG_TCIF1_5);     //清除DMA传输完成中断标志位
//    __HAL_DMA_ENABLE_IT(&SAI1_RXDMA_Handler,DMA_IT_TC);             //开启传输完成中断
//
//    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn,0,1);                    //DMA中断优先级
//    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
//}
//
////SAI DMA回调函数指针
//void (*sai_tx_callback)(void);	//TX回调函数
//void (*sai_rx_callback)(void);	//RX回调函数

s32 sai_sends(s32 port, u8 *buf, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	s32 irq_save;
	struct StSaiBus *pSaiBus = &gSaiBus[port];

	if (pSaiBus->handle.Instance) {
		irq_save = __vos_irq_save();
		ret = VOSRingBufSet(pSaiBus->ring_tx, buf, len);
		__vos_irq_restore(irq_save);
		if (VOSRingBufIsFull(pSaiBus->ring_tx)) {//装满
			if (!pSaiBus->is_dma_tx_working) {
				__HAL_DMA_ENABLE(&pSaiBus->hdma_tx);//开启DMA TX传输
				pSaiBus->is_dma_tx_working = 1;
				if (pSaiBus->timer) { //停止定时器
					VOSTimerStop(pSaiBus->timer);
				}
			}
		}
		else {//处理数据小于ring tx buf时，没任何触发发送
			if (!pSaiBus->is_dma_tx_working && pSaiBus->timer) {//如果没启动dma发送，就设定定时器
				VOSTimerStart(pSaiBus->timer); //启动定时器, 重复调用，会返回定时器已启动信息
			}
		}
	}

	return ret;
}


s32 sai_recvs(s32 port, u8 *buf, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	s32 irq_save;
	u32 mark_time=0;
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	mark_time = VOSGetTimeMs();
	while (pSaiBus->handle.Instance) {
		irq_save = __vos_irq_save();
		ret = VOSRingBufGet(pSaiBus->ring_rx, buf, len);
		__vos_irq_restore(irq_save);
		if (ret > 0) {
			break;
		}
		if (VOSGetTimeMs() - mark_time >= timeout_ms) {
			break;
		}
		VOSTaskDelay(5);
	}
	return ret;
}

//DMA2_Stream3中断服务函数Sai
void DMA2_Stream3_IRQHandler(void)
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StSaiBus *pSaiBus = SaiBusPtr(SAI1_Block_A);
	VOSIntEnter();
    if(__HAL_DMA_GET_FLAG(&pSaiBus->hdma_tx,DMA_FLAG_TCIF3_7)!=RESET) //DMA传输完成
    {
        __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_tx,DMA_FLAG_TCIF3_7);     //清除DMA传输完成中断标志位
		if(DMA2_Stream3->CR & (1 << 19)) { //当前使用的是buf1, 证明中断是buf0完成
			pbuf = pSaiBus->tx_buf0;
		}
		else { //当前使用的是buf0, 证明中断是buf1完成
			pbuf = pSaiBus->tx_buf1;
		}
		if (pSaiBus->mode == MODE_SAI_PLAYER) { //播放器才输出有效的数据，否者输出0数据
			/* 从发送ring缓冲区获取数据到dma buf */
			ret = VOSRingBufGet(pSaiBus->ring_tx, pbuf,  pSaiBus->tx_len);
			if (ret == 0) { //没数据，直接关闭dma, 下次发送时自x, pbuf, pI2sB动开动dma
				__HAL_DMA_DISABLE(&pSaiBus->hdma_tx);
				pSaiBus->is_dma_tx_working = 0;
			}
			else if (ret <= pSaiBus->tx_len) {//这里证明源头数据太慢，这里直接填充0，部分静音
				memset(pbuf, 0x00, pSaiBus->tx_len-ret);
			}
		}
    }
	VOSIntExit ();
}
//DMA2_Stream5中断服务函数 (接收）
void DMA2_Stream5_IRQHandler(void)
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StSaiBus *pSaiBus = SaiBusPtr(SAI1_Block_B);
	VOSIntEnter();
    if(__HAL_DMA_GET_FLAG(&pSaiBus->hdma_rx,DMA_FLAG_TCIF1_5)!=RESET) //DMA传输完成
    {
        __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_rx,DMA_FLAG_TCIF1_5);     //清除DMA传输完成中断标志位
        if(DMA2_Stream5->CR & (1 << 19)) {//rx buf0 full
        	pbuf = pSaiBus->rx_buf0;
		}
        else { //rx buf1 full
        	pbuf = pSaiBus->rx_buf1;
		}
    	if (pSaiBus->ring_rx) {
    		ret = VOSRingBufSet(pSaiBus->ring_rx, pbuf, pSaiBus->rx_len);
    		if (ret != pSaiBus->rx_len) {
    			kprintf("warning: RingBuf recv overflow!\r\n");
    		}
    	}
    }
    VOSIntExit ();
}
////SAI开始播放
//void SAI_Play_Start(void)
//{
//    __HAL_DMA_ENABLE(&SAI1_TXDMA_Handler);//开启DMA TX传输
//}
////关闭I2S播放
//void SAI_Play_Stop(void)
//{
//    __HAL_DMA_DISABLE(&SAI1_TXDMA_Handler);  //结束播放
//}
////SAI开始录音
//void SAI_Rec_Start(void)
//{
//    __HAL_DMA_ENABLE(&SAI1_RXDMA_Handler);//开启DMA RX传输
//}
////关闭SAI录音
//void SAI_Rec_Stop(void)
//{
//    __HAL_DMA_DISABLE(&SAI1_RXDMA_Handler);//结束录音
//}

//I2S开始播放
void sai_tx_dma_start(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_ENABLE(&pSaiBus->hdma_tx);//开启DMA TX传输
}

//关闭I2S播放
void sai_tx_dma_stop(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_DISABLE(&pSaiBus->hdma_tx);//结束播放;
}

//I2S开始录音
void sai_rx_dma_start(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_ENABLE(&pSaiBus->hdma_rx); //开启DMA RX传输
}

//关闭I2S录音
void sai_rx_dma_stop(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_DISABLE(&pSaiBus->hdma_rx); //结束录音
}

#endif
