#include "i2s.h"
#include "vringbuf.h"
#include "stm32f4xx_hal.h"

#define I2S_RING_TX_DEFAULT (16*1024) //(32*1024)
#define I2S_RING_RX_DEFAULT (128*1024) //(32*1024)
#define I2S_TX_BUF_DEFAULT (8*1024) //(16*1024)
#define I2S_RX_BUF_DEFAULT (8*1024) //(16*1024)

void i2s_tx_timer(void *ptimer, void *parg);

typedef struct StI2sBus {
	s32 mode; //0: player, 1: recorder
	I2S_HandleTypeDef handle;
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
}StI2sBus;

struct StI2sBus gI2sBus[3];

void i2s_mode_set(s32 port, s32 mode)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	pI2sBus->mode = mode;
}

s32 i2s_open(s32 port, u32 I2S_Standard, u32 I2S_Mode, u32 I2S_Clock_Polarity, u32 I2S_DataFormat)
{
	s32 err = 0;
	struct StI2sBus *pI2sBus = &gI2sBus[port];

	if (pI2sBus->is_inited == 0) {
		pI2sBus->ring_rx = VOSRingBufCreate(pI2sBus->rx_ring_max =
				(pI2sBus->rx_ring_max?pI2sBus->rx_ring_max:(I2S_RING_RX_DEFAULT+sizeof(StVOSRingBuf))));
		if (pI2sBus->ring_rx == 0) {
			err = -1;
			goto END;
		}

		pI2sBus->rx_buf0 = vmalloc(pI2sBus->rx_len = (pI2sBus->rx_len ? pI2sBus->rx_len : I2S_RX_BUF_DEFAULT));
		if (pI2sBus->rx_buf0 == 0) {
			err = -2;
			goto END;
		}
		pI2sBus->rx_buf1 = vmalloc(pI2sBus->rx_len = (pI2sBus->rx_len ? pI2sBus->rx_len : I2S_RX_BUF_DEFAULT));
		if (pI2sBus->rx_buf1 == 0) {
			err = -3;
			goto END;
		}

		pI2sBus->ring_tx = VOSRingBufCreate(pI2sBus->tx_ring_max =
				(pI2sBus->tx_ring_max?pI2sBus->tx_ring_max:(I2S_RING_TX_DEFAULT+sizeof(StVOSRingBuf))));
		if (pI2sBus->ring_tx == 0) {
			err = -4;
			goto END;
		}

		pI2sBus->tx_buf0 = vmalloc(pI2sBus->tx_len = (pI2sBus->tx_len ? pI2sBus->tx_len : I2S_TX_BUF_DEFAULT));
		if (pI2sBus->tx_buf0 == 0) {
			err = -5;
			goto END;
		}

		pI2sBus->tx_buf1 = vmalloc(pI2sBus->tx_len = (pI2sBus->tx_len ? pI2sBus->tx_len : I2S_TX_BUF_DEFAULT));
		if (pI2sBus->tx_buf1 == 0) {
			err = -6;
			goto END;
		}
		//if (pI2sBus->mode == MODE_I2S_RECORDER) {//录音器，master， 推数据为收数据。
			memset(pI2sBus->tx_buf0, 0, pI2sBus->tx_len);
			memset(pI2sBus->tx_buf1, 0, pI2sBus->tx_len);
		//}


		//创建软件定时器，在第一次数据小于ring buf max时，设定定时器发送
		pI2sBus->timer = VOSTimerCreate(VOS_TIMER_TYPE_ONE_SHOT, 500, i2s_tx_timer, pI2sBus, "timer");
		if (pI2sBus->timer == 0) {
			err = -7;
			goto END;
		}

	}

	if (port == 1) {
		pI2sBus->handle.Instance = SPI2;
		pI2sBus->handle.Init.Mode = I2S_Mode;					//IIS模式
		pI2sBus->handle.Init.Standard = I2S_Standard;			//IIS标准
		pI2sBus->handle.Init.DataFormat = I2S_DataFormat;		//IIS数据长度
		pI2sBus->handle.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;	//主时钟输出使能
		pI2sBus->handle.Init.AudioFreq = I2S_AUDIOFREQ_DEFAULT;	//IIS频率设置
		pI2sBus->handle.Init.CPOL = I2S_Clock_Polarity;			//空闲状态时钟电平
		pI2sBus->handle.Init.ClockSource = I2S_CLOCK_PLL;		//IIS时钟源为PLL
		pI2sBus->handle.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;	//IIS全双工
		HAL_I2S_Init(&pI2sBus->handle);

		SPI2->CR2 |= 1<<1;									//SPI2 TX DMA请求使能.
		I2S2ext->CR2 |= 1<<0;									//I2S2ext RX DMA请求使能.
		__HAL_I2S_ENABLE(&pI2sBus->handle);					//使能I2S2
		__HAL_I2SEXT_ENABLE(&pI2sBus->handle);					//使能I2S2ext

	}

	if (pI2sBus->is_inited==0) {
		pI2sBus->is_inited = 1;
	}
END:
	return err;

}

void i2s_close(int port)
{

}

struct StI2sBus *I2sBusPtr(SPI_TypeDef *spi)
{
	int i = 0;
	for (i=0; i<sizeof(gI2sBus)/sizeof(gI2sBus[0]); i++) {
		if (gI2sBus[i].handle.Instance == spi) {
			return &gI2sBus[i];
		}
	}
	return 0;
}

void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
	GPIO_InitTypeDef GPIO_Initure;
	struct StI2sBus *pI2sBus = I2sBusPtr(hi2s->Instance);
	if (hi2s->Instance == SPI2) {

		__HAL_RCC_SPI2_CLK_ENABLE();        		//使能SPI2/I2S2时钟
		__HAL_RCC_GPIOB_CLK_ENABLE();               //使能GPIOB时钟
		__HAL_RCC_GPIOC_CLK_ENABLE();               //使能GPIOC时钟

		//初始化PB12/13
		GPIO_Initure.Pin = GPIO_PIN_12|GPIO_PIN_13;
		GPIO_Initure.Mode = GPIO_MODE_AF_PP;          //推挽复用
		GPIO_Initure.Pull = GPIO_PULLUP;              //上拉
		GPIO_Initure.Speed = GPIO_SPEED_HIGH;         //高速
		GPIO_Initure.Alternate = GPIO_AF5_SPI2;       //复用为SPI/I2S
		HAL_GPIO_Init(GPIOB, &GPIO_Initure);         //初始化

		//初始化PC3/PC6
		GPIO_Initure.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6;
		HAL_GPIO_Init(GPIOC, &GPIO_Initure);         //初始化

		//初始化PC2
		GPIO_Initure.Pin = GPIO_PIN_2;
		GPIO_Initure.Alternate = GPIO_AF6_I2S2ext;  	//复用为I2Sext
		HAL_GPIO_Init(GPIOC, &GPIO_Initure);         //初始化

	    //发送DMA设置
	    __HAL_RCC_DMA1_CLK_ENABLE();                                    		//使能DMA1时钟
	     __HAL_LINKDMA(&pI2sBus->handle, hdmatx, pI2sBus->hdma_tx);         		//将DMA与I2S联系起来

	     pI2sBus->hdma_tx.Instance = DMA1_Stream4;                       		//DMA1数据流4
	     pI2sBus->hdma_tx.Init.Channel = DMA_CHANNEL_0;                  		//通道0
	     pI2sBus->hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;         		//存储器到外设模式
	     pI2sBus->hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;             		//外设非增量模式
	     pI2sBus->hdma_tx.Init.MemInc = DMA_MINC_ENABLE;                 		//存储器增量模式
	     pI2sBus->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   	//外设数据长度:16位
	     pI2sBus->hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;    	//存储器数据长度:16位
	     pI2sBus->hdma_tx.Init.Mode = DMA_CIRCULAR;                      		//使用循环模式
	     pI2sBus->hdma_tx.Init.Priority = DMA_PRIORITY_HIGH;             		//高优先级
	     pI2sBus->hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;          		//不使用FIFO
	     pI2sBus->hdma_tx.Init.MemBurst = DMA_MBURST_SINGLE;             		//存储器单次突发传输
	     pI2sBus->hdma_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;          		//外设突发单次传输
	     HAL_DMA_DeInit(&pI2sBus->hdma_tx);                            		//先清除以前的设置
	     HAL_DMA_Init(&pI2sBus->hdma_tx);	                            		//初始化DMA

	     HAL_DMAEx_MultiBufferStart(&pI2sBus->hdma_tx, (u32)pI2sBus->tx_buf0,
	    		 (u32)&SPI2->DR, (u32)pI2sBus->tx_buf1, pI2sBus->tx_len/2);//开启双缓冲
	     __HAL_DMA_DISABLE(&pI2sBus->hdma_tx);                         		//先关闭DMA
	     VOSDelayUs(10);                                                  		//10us延时，防止-O2优化出问题
	     __HAL_DMA_ENABLE_IT(&pI2sBus->hdma_tx, DMA_IT_TC);             		//开启传输完成中断
	     __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_tx, DMA_FLAG_TCIF0_4);     		//清除DMA传输完成中断标志位
	     HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);                    		//DMA中断优先级
	     HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

		//接收DMA设置
		__HAL_RCC_DMA1_CLK_ENABLE();                                    		//使能DMA1时钟
	    __HAL_LINKDMA(&pI2sBus->handle, hdmarx, pI2sBus->hdma_rx);         		//将DMA与I2S联系起来

	    pI2sBus->hdma_rx.Instance = DMA1_Stream3;                       		//DMA1数据流3
	    pI2sBus->hdma_rx.Init.Channel = DMA_CHANNEL_3;                  		//通道3
	    pI2sBus->hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;         		//外设到存储器模式
	    pI2sBus->hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;             		//外设非增量模式
	    pI2sBus->hdma_rx.Init.MemInc = DMA_MINC_ENABLE;                 		//存储器增量模式
	    pI2sBus->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   	//外设数据长度:16位
	    pI2sBus->hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;    	//存储器数据长度:16位
	    pI2sBus->hdma_rx.Init.Mode = DMA_CIRCULAR;                      		//使用循环模式
	    pI2sBus->hdma_rx.Init.Priority = DMA_PRIORITY_MEDIUM;             		//中等优先级
	    pI2sBus->hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;          		//不使用FIFO
	    pI2sBus->hdma_rx.Init.MemBurst = DMA_MBURST_SINGLE;             		//存储器单次突发传输
	    pI2sBus->hdma_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;          		//外设突发单次传输
	    HAL_DMA_DeInit(&pI2sBus->hdma_rx);                            		//先清除以前的设置
	    HAL_DMA_Init(&pI2sBus->hdma_rx);	                            		//初始化DMA

	    HAL_DMAEx_MultiBufferStart(&pI2sBus->hdma_rx,(u32)&I2S2ext->DR,
	    		(u32)pI2sBus->rx_buf0,(u32)pI2sBus->rx_buf1, pI2sBus->rx_len/2);//开启双缓冲
	    __HAL_DMA_DISABLE(&pI2sBus->hdma_rx);                         		//先关闭DMA
	    //VOSDelayUs(10);                                                   		//10us延时，防止-O2优化出问题
	    __HAL_DMA_ENABLE_IT(&pI2sBus->hdma_rx, DMA_IT_TC);             		//开启传输完成中断
	    __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_rx, DMA_FLAG_TCIF3_7);     		//清除DMA传输完成中断标志位

		HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 1);  //抢占1，子优先级1，组2
	    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
	}
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s)
{
	struct StI2sBus *pI2sBus = I2sBusPtr(hi2s->Instance);

	if (hi2s->Instance == SPI2) {
		__HAL_RCC_SPI2_FORCE_RESET();
		__HAL_RCC_SPI2_RELEASE_RESET();

		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2);
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3);
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6);

		HAL_DMA_DeInit(&pI2sBus->hdma_rx);
		HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn);

		HAL_DMA_DeInit(&pI2sBus->hdma_tx);
		HAL_NVIC_DisableIRQ(DMA1_Stream4_IRQn);
	}
}

//采样率计算公式:Fs=I2SxCLK/[256*(2*I2SDIV+ODD)]
//I2SxCLK=(HSE/pllm)*PLLI2SN/PLLI2SR
//一般HSE=8Mhz
//pllm:在Sys_Clock_Set设置的时候确定，一般是8
//PLLI2SN:一般是192~432
//PLLI2SR:2~7
//I2SDIV:2~255
//ODD:0/1
//I2S分频系数表@pllm=8,HSE=8Mhz,即vco输入频率为1Mhz
//表格式:采样率/10,PLLI2SN,PLLI2SR,I2SDIV,ODD
const u16 I2S_PSC_TBL[][5]=
{
	{800 ,256,5,12,1},		//8Khz采样率
	{1102,429,4,19,0},		//11.025Khz采样率
	{1600,213,2,13,0},		//16Khz采样率
	{2205,429,4, 9,1},		//22.05Khz采样率
	{3200,213,2, 6,1},		//32Khz采样率
	{4410,271,2, 6,0},		//44.1Khz采样率
	{4800,258,3, 3,1},		//48Khz采样率
	{8820,316,2, 3,1},		//88.2Khz采样率
	{9600,344,2, 3,1},  	//96Khz采样率
	{17640,361,2,2,0},  	//176.4Khz采样率
	{19200,393,2,2,0},  	//192Khz采样率
};

//开启I2S的DMA功能,HAL库没有提供此函数
//因此我们需要自己操作寄存器编写一个
//void I2S_DMA_Enable(void)
//{
//    u32 tempreg=0;
//    tempreg=SPI2->CR2;    	//先读出以前的设置
//	tempreg|=1<<1;			//使能DMA
//	SPI2->CR2=tempreg;		//写入CR1寄存器中
//}

//设置SAIA的采样率(@MCKEN)
//samplerate:采样率,单位:Hz
//返回值:0,设置成功;1,无法设置.
u8 I2S2_SampleRate_Set(u32 samplerate)
{
    u8 i=0;
	u32 tempreg=0;
    RCC_PeriphCLKInitTypeDef RCCI2S2_ClkInitSture;

	for(i=0;i<(sizeof(I2S_PSC_TBL)/10);i++)//看看改采样率是否可以支持
	{
		if((samplerate/10)==I2S_PSC_TBL[i][0])break;
	}
    if(i==(sizeof(I2S_PSC_TBL)/10))return 1;//搜遍了也找不到

    RCCI2S2_ClkInitSture.PeriphClockSelection=RCC_PERIPHCLK_I2S;	//外设时钟源选择
    RCCI2S2_ClkInitSture.PLLI2S.PLLI2SN=(u32)I2S_PSC_TBL[i][1];    	//设置PLLI2SN
    RCCI2S2_ClkInitSture.PLLI2S.PLLI2SR=(u32)I2S_PSC_TBL[i][2];    	//设置PLLI2SR
    HAL_RCCEx_PeriphCLKConfig(&RCCI2S2_ClkInitSture);             	//设置时钟

	RCC->CR|=1<<26;					//开启I2S时钟
	while((RCC->CR&1<<27)==0);		//等待I2S时钟开启成功.
	tempreg=I2S_PSC_TBL[i][3]<<0;	//设置I2SDIV
	tempreg|=I2S_PSC_TBL[i][4]<<8;	//设置ODD位
	tempreg|=1<<9;					//使能MCKOE位,输出MCK
	SPI2->I2SPR=tempreg;			//设置I2SPR寄存器
	return 0;
}


void i2s_tx_timer(void *ptimer, void *parg)
{
	static s32 count = 0;
	StVOSTimer *p = (StVOSTimer*)ptimer;
	struct StI2sBus *pI2sBus = (struct StI2sBus *)parg;
	if (pI2sBus->handle.Instance == SPI2) {
		if (!pI2sBus->is_dma_tx_working) {
			__HAL_DMA_ENABLE(&pI2sBus->hdma_tx);
			pI2sBus->is_dma_tx_working = 1;
		}
	}
	//kprintf("%s->%s count=%d running!\r\n", __FUNCTION__, p->name, count++);
}

s32 i2s_sends(s32 port, u8 *buf, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	s32 irq_save;
	struct StI2sBus *pI2sBus = &gI2sBus[port];

	if (pI2sBus->handle.Instance) {
		irq_save = __vos_irq_save();
		ret = VOSRingBufSet(pI2sBus->ring_tx, buf, len);
		__vos_irq_restore(irq_save);
		if (VOSRingBufIsFull(pI2sBus->ring_tx)) {//装满
			if (!pI2sBus->is_dma_tx_working) {
				__HAL_DMA_ENABLE(&pI2sBus->hdma_tx);//开启DMA TX传输
				pI2sBus->is_dma_tx_working = 1;
				if (pI2sBus->timer) { //停止定时器
					VOSTimerStop(pI2sBus->timer);
				}
			}
		}
		else {//处理数据小于ring tx buf时，没任何触发发送
			if (!pI2sBus->is_dma_tx_working && pI2sBus->timer) {//如果没启动dma发送，就设定定时器
				VOSTimerStart(pI2sBus->timer); //启动定时器, 重复调用，会返回定时器已启动信息
			}
		}
	}

	return ret;
}

s32 i2s_recvs(s32 port, u8 *buf, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	s32 irq_save;
	u32 mark_time=0;
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	mark_time = VOSGetTimeMs();
	while (pI2sBus->handle.Instance) {
		irq_save = __vos_irq_save();
		ret = VOSRingBufGet(pI2sBus->ring_rx, buf, len);
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

//发送dma中断
void DMA1_Stream4_IRQHandler()
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StI2sBus *pI2sBus = I2sBusPtr(SPI2);
	VOSIntEnter();
    if(pI2sBus && __HAL_DMA_GET_FLAG(&pI2sBus->hdma_tx, DMA_FLAG_TCIF0_4) != RESET) //DMA传输完成
    {
        __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_tx, DMA_FLAG_TCIF0_4);     //清除DMA传输完成中断标志位
    	if(DMA1_Stream4->CR & (1 << 19)) { //当前使用的是buf1, 证明中断是buf0完成
    		pbuf = pI2sBus->tx_buf0;
    	}
    	else { //当前使用的是buf0, 证明中断是buf1完成
    		pbuf = pI2sBus->tx_buf1;
    	}
    	if (pI2sBus->mode == MODE_I2S_PLAYER) { //播放器才输出有效的数据，否者输出0数据
			/* 从发送ring缓冲区获取数据到dma buf */
			ret = VOSRingBufGet(pI2sBus->ring_tx, pbuf,  pI2sBus->tx_len);
			if (ret == 0) { //没数据，直接关闭dma, 下次发送时自x, pbuf, pI2sB动开动dma
				__HAL_DMA_DISABLE(&pI2sBus->hdma_tx);
				pI2sBus->is_dma_tx_working = 0;
			}
			else if (ret <= pI2sBus->tx_len) {//这里证明源头数据太慢，这里直接填充0，部分静音
				memset(pbuf, 0x00, pI2sBus->tx_len-ret);
			}
    	}
    }
	VOSIntExit ();
}

//接收dma中断
void DMA1_Stream3_IRQHandler()
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StI2sBus *pI2sBus = I2sBusPtr(SPI2);

	VOSIntEnter();
    if(pI2sBus && __HAL_DMA_GET_FLAG(&pI2sBus->hdma_rx, DMA_FLAG_TCIF3_7) != RESET) //DMA传输完成
    {
        __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_rx, DMA_FLAG_TCIF3_7);     //清除DMA传输完成中断标志位
        if(DMA1_Stream3->CR & (1 << 19)) {//rx buf0 full
        	pbuf = pI2sBus->rx_buf0;
		}
        else { //rx buf1 full
        	pbuf = pI2sBus->rx_buf1;
		}
    	if (pI2sBus->ring_rx) {
    		ret = VOSRingBufSet(pI2sBus->ring_rx, pbuf, pI2sBus->rx_len);
    		if (ret != pI2sBus->rx_len) {
    			kprintf("warning: RingBuf recv overflow!\r\n");
    		}
    	}
    }
    VOSIntExit ();
}


//I2S开始播放
void i2s_tx_dma_start(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_ENABLE(&pI2sBus->hdma_tx);//开启DMA TX传输
}

//关闭I2S播放
void i2s_tx_dma_stop(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_DISABLE(&pI2sBus->hdma_tx);//结束播放;
}

//I2S开始录音
void i2s_rx_dma_start(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_ENABLE(&pI2sBus->hdma_rx); //开启DMA RX传输
}

//关闭I2S录音
void i2s_rx_dma_stop(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_DISABLE(&pI2sBus->hdma_rx); //结束录音
}




