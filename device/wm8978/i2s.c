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
	StVOSRingBuf *ring_rx; //���ջ��λ���
	StVOSRingBuf *ring_tx; //���ͻ��λ���
	s32 rx_ring_max;
	s32 tx_ring_max;
	u8 *rx_buf0;
	u8 *rx_buf1;
	s32 rx_len;
	u8 *tx_buf0;
	u8 *tx_buf1;
	s32 tx_len;

	volatile s32 is_dma_tx_working;
	StVOSTimer *timer; //��ʱ�����û��buffer�� �ͳ�ʱֱ�ӷ���

	DMA_HandleTypeDef hdma_tx;
	DMA_HandleTypeDef hdma_rx;

	s32 is_inited; //�Ƿ��Ѿ���ʼ��
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
		//if (pI2sBus->mode == MODE_I2S_RECORDER) {//¼������master�� ������Ϊ�����ݡ�
			memset(pI2sBus->tx_buf0, 0, pI2sBus->tx_len);
			memset(pI2sBus->tx_buf1, 0, pI2sBus->tx_len);
		//}


		//���������ʱ�����ڵ�һ������С��ring buf maxʱ���趨��ʱ������
		pI2sBus->timer = VOSTimerCreate(VOS_TIMER_TYPE_ONE_SHOT, 500, i2s_tx_timer, pI2sBus, "timer");
		if (pI2sBus->timer == 0) {
			err = -7;
			goto END;
		}

	}

	if (port == 1) {
		pI2sBus->handle.Instance = SPI2;
		pI2sBus->handle.Init.Mode = I2S_Mode;					//IISģʽ
		pI2sBus->handle.Init.Standard = I2S_Standard;			//IIS��׼
		pI2sBus->handle.Init.DataFormat = I2S_DataFormat;		//IIS���ݳ���
		pI2sBus->handle.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;	//��ʱ�����ʹ��
		pI2sBus->handle.Init.AudioFreq = I2S_AUDIOFREQ_DEFAULT;	//IISƵ������
		pI2sBus->handle.Init.CPOL = I2S_Clock_Polarity;			//����״̬ʱ�ӵ�ƽ
		pI2sBus->handle.Init.ClockSource = I2S_CLOCK_PLL;		//IISʱ��ԴΪPLL
		pI2sBus->handle.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;	//IISȫ˫��
		HAL_I2S_Init(&pI2sBus->handle);

		SPI2->CR2 |= 1<<1;									//SPI2 TX DMA����ʹ��.
		I2S2ext->CR2 |= 1<<0;									//I2S2ext RX DMA����ʹ��.
		__HAL_I2S_ENABLE(&pI2sBus->handle);					//ʹ��I2S2
		__HAL_I2SEXT_ENABLE(&pI2sBus->handle);					//ʹ��I2S2ext

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

		__HAL_RCC_SPI2_CLK_ENABLE();        		//ʹ��SPI2/I2S2ʱ��
		__HAL_RCC_GPIOB_CLK_ENABLE();               //ʹ��GPIOBʱ��
		__HAL_RCC_GPIOC_CLK_ENABLE();               //ʹ��GPIOCʱ��

		//��ʼ��PB12/13
		GPIO_Initure.Pin = GPIO_PIN_12|GPIO_PIN_13;
		GPIO_Initure.Mode = GPIO_MODE_AF_PP;          //���츴��
		GPIO_Initure.Pull = GPIO_PULLUP;              //����
		GPIO_Initure.Speed = GPIO_SPEED_HIGH;         //����
		GPIO_Initure.Alternate = GPIO_AF5_SPI2;       //����ΪSPI/I2S
		HAL_GPIO_Init(GPIOB, &GPIO_Initure);         //��ʼ��

		//��ʼ��PC3/PC6
		GPIO_Initure.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6;
		HAL_GPIO_Init(GPIOC, &GPIO_Initure);         //��ʼ��

		//��ʼ��PC2
		GPIO_Initure.Pin = GPIO_PIN_2;
		GPIO_Initure.Alternate = GPIO_AF6_I2S2ext;  	//����ΪI2Sext
		HAL_GPIO_Init(GPIOC, &GPIO_Initure);         //��ʼ��

	    //����DMA����
	    __HAL_RCC_DMA1_CLK_ENABLE();                                    		//ʹ��DMA1ʱ��
	     __HAL_LINKDMA(&pI2sBus->handle, hdmatx, pI2sBus->hdma_tx);         		//��DMA��I2S��ϵ����

	     pI2sBus->hdma_tx.Instance = DMA1_Stream4;                       		//DMA1������4
	     pI2sBus->hdma_tx.Init.Channel = DMA_CHANNEL_0;                  		//ͨ��0
	     pI2sBus->hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;         		//�洢��������ģʽ
	     pI2sBus->hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;             		//���������ģʽ
	     pI2sBus->hdma_tx.Init.MemInc = DMA_MINC_ENABLE;                 		//�洢������ģʽ
	     pI2sBus->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   	//�������ݳ���:16λ
	     pI2sBus->hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;    	//�洢�����ݳ���:16λ
	     pI2sBus->hdma_tx.Init.Mode = DMA_CIRCULAR;                      		//ʹ��ѭ��ģʽ
	     pI2sBus->hdma_tx.Init.Priority = DMA_PRIORITY_HIGH;             		//�����ȼ�
	     pI2sBus->hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;          		//��ʹ��FIFO
	     pI2sBus->hdma_tx.Init.MemBurst = DMA_MBURST_SINGLE;             		//�洢������ͻ������
	     pI2sBus->hdma_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;          		//����ͻ�����δ���
	     HAL_DMA_DeInit(&pI2sBus->hdma_tx);                            		//�������ǰ������
	     HAL_DMA_Init(&pI2sBus->hdma_tx);	                            		//��ʼ��DMA

	     HAL_DMAEx_MultiBufferStart(&pI2sBus->hdma_tx, (u32)pI2sBus->tx_buf0,
	    		 (u32)&SPI2->DR, (u32)pI2sBus->tx_buf1, pI2sBus->tx_len/2);//����˫����
	     __HAL_DMA_DISABLE(&pI2sBus->hdma_tx);                         		//�ȹر�DMA
	     VOSDelayUs(10);                                                  		//10us��ʱ����ֹ-O2�Ż�������
	     __HAL_DMA_ENABLE_IT(&pI2sBus->hdma_tx, DMA_IT_TC);             		//������������ж�
	     __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_tx, DMA_FLAG_TCIF0_4);     		//���DMA��������жϱ�־λ
	     HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);                    		//DMA�ж����ȼ�
	     HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

		//����DMA����
		__HAL_RCC_DMA1_CLK_ENABLE();                                    		//ʹ��DMA1ʱ��
	    __HAL_LINKDMA(&pI2sBus->handle, hdmarx, pI2sBus->hdma_rx);         		//��DMA��I2S��ϵ����

	    pI2sBus->hdma_rx.Instance = DMA1_Stream3;                       		//DMA1������3
	    pI2sBus->hdma_rx.Init.Channel = DMA_CHANNEL_3;                  		//ͨ��3
	    pI2sBus->hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;         		//���赽�洢��ģʽ
	    pI2sBus->hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;             		//���������ģʽ
	    pI2sBus->hdma_rx.Init.MemInc = DMA_MINC_ENABLE;                 		//�洢������ģʽ
	    pI2sBus->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   	//�������ݳ���:16λ
	    pI2sBus->hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;    	//�洢�����ݳ���:16λ
	    pI2sBus->hdma_rx.Init.Mode = DMA_CIRCULAR;                      		//ʹ��ѭ��ģʽ
	    pI2sBus->hdma_rx.Init.Priority = DMA_PRIORITY_MEDIUM;             		//�е����ȼ�
	    pI2sBus->hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;          		//��ʹ��FIFO
	    pI2sBus->hdma_rx.Init.MemBurst = DMA_MBURST_SINGLE;             		//�洢������ͻ������
	    pI2sBus->hdma_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;          		//����ͻ�����δ���
	    HAL_DMA_DeInit(&pI2sBus->hdma_rx);                            		//�������ǰ������
	    HAL_DMA_Init(&pI2sBus->hdma_rx);	                            		//��ʼ��DMA

	    HAL_DMAEx_MultiBufferStart(&pI2sBus->hdma_rx,(u32)&I2S2ext->DR,
	    		(u32)pI2sBus->rx_buf0,(u32)pI2sBus->rx_buf1, pI2sBus->rx_len/2);//����˫����
	    __HAL_DMA_DISABLE(&pI2sBus->hdma_rx);                         		//�ȹر�DMA
	    //VOSDelayUs(10);                                                   		//10us��ʱ����ֹ-O2�Ż�������
	    __HAL_DMA_ENABLE_IT(&pI2sBus->hdma_rx, DMA_IT_TC);             		//������������ж�
	    __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_rx, DMA_FLAG_TCIF3_7);     		//���DMA��������жϱ�־λ

		HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 1);  //��ռ1�������ȼ�1����2
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

//�����ʼ��㹫ʽ:Fs=I2SxCLK/[256*(2*I2SDIV+ODD)]
//I2SxCLK=(HSE/pllm)*PLLI2SN/PLLI2SR
//һ��HSE=8Mhz
//pllm:��Sys_Clock_Set���õ�ʱ��ȷ����һ����8
//PLLI2SN:һ����192~432
//PLLI2SR:2~7
//I2SDIV:2~255
//ODD:0/1
//I2S��Ƶϵ����@pllm=8,HSE=8Mhz,��vco����Ƶ��Ϊ1Mhz
//���ʽ:������/10,PLLI2SN,PLLI2SR,I2SDIV,ODD
const u16 I2S_PSC_TBL[][5]=
{
	{800 ,256,5,12,1},		//8Khz������
	{1102,429,4,19,0},		//11.025Khz������
	{1600,213,2,13,0},		//16Khz������
	{2205,429,4, 9,1},		//22.05Khz������
	{3200,213,2, 6,1},		//32Khz������
	{4410,271,2, 6,0},		//44.1Khz������
	{4800,258,3, 3,1},		//48Khz������
	{8820,316,2, 3,1},		//88.2Khz������
	{9600,344,2, 3,1},  	//96Khz������
	{17640,361,2,2,0},  	//176.4Khz������
	{19200,393,2,2,0},  	//192Khz������
};

//����I2S��DMA����,HAL��û���ṩ�˺���
//���������Ҫ�Լ������Ĵ�����дһ��
//void I2S_DMA_Enable(void)
//{
//    u32 tempreg=0;
//    tempreg=SPI2->CR2;    	//�ȶ�����ǰ������
//	tempreg|=1<<1;			//ʹ��DMA
//	SPI2->CR2=tempreg;		//д��CR1�Ĵ�����
//}

//����SAIA�Ĳ�����(@MCKEN)
//samplerate:������,��λ:Hz
//����ֵ:0,���óɹ�;1,�޷�����.
u8 I2S2_SampleRate_Set(u32 samplerate)
{
    u8 i=0;
	u32 tempreg=0;
    RCC_PeriphCLKInitTypeDef RCCI2S2_ClkInitSture;

	for(i=0;i<(sizeof(I2S_PSC_TBL)/10);i++)//�����Ĳ������Ƿ����֧��
	{
		if((samplerate/10)==I2S_PSC_TBL[i][0])break;
	}
    if(i==(sizeof(I2S_PSC_TBL)/10))return 1;//�ѱ���Ҳ�Ҳ���

    RCCI2S2_ClkInitSture.PeriphClockSelection=RCC_PERIPHCLK_I2S;	//����ʱ��Դѡ��
    RCCI2S2_ClkInitSture.PLLI2S.PLLI2SN=(u32)I2S_PSC_TBL[i][1];    	//����PLLI2SN
    RCCI2S2_ClkInitSture.PLLI2S.PLLI2SR=(u32)I2S_PSC_TBL[i][2];    	//����PLLI2SR
    HAL_RCCEx_PeriphCLKConfig(&RCCI2S2_ClkInitSture);             	//����ʱ��

	RCC->CR|=1<<26;					//����I2Sʱ��
	while((RCC->CR&1<<27)==0);		//�ȴ�I2Sʱ�ӿ����ɹ�.
	tempreg=I2S_PSC_TBL[i][3]<<0;	//����I2SDIV
	tempreg|=I2S_PSC_TBL[i][4]<<8;	//����ODDλ
	tempreg|=1<<9;					//ʹ��MCKOEλ,���MCK
	SPI2->I2SPR=tempreg;			//����I2SPR�Ĵ���
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
		if (VOSRingBufIsFull(pI2sBus->ring_tx)) {//װ��
			if (!pI2sBus->is_dma_tx_working) {
				__HAL_DMA_ENABLE(&pI2sBus->hdma_tx);//����DMA TX����
				pI2sBus->is_dma_tx_working = 1;
				if (pI2sBus->timer) { //ֹͣ��ʱ��
					VOSTimerStop(pI2sBus->timer);
				}
			}
		}
		else {//��������С��ring tx bufʱ��û�κδ�������
			if (!pI2sBus->is_dma_tx_working && pI2sBus->timer) {//���û����dma���ͣ����趨��ʱ��
				VOSTimerStart(pI2sBus->timer); //������ʱ��, �ظ����ã��᷵�ض�ʱ����������Ϣ
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

//����dma�ж�
void DMA1_Stream4_IRQHandler()
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StI2sBus *pI2sBus = I2sBusPtr(SPI2);
	VOSIntEnter();
    if(pI2sBus && __HAL_DMA_GET_FLAG(&pI2sBus->hdma_tx, DMA_FLAG_TCIF0_4) != RESET) //DMA�������
    {
        __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_tx, DMA_FLAG_TCIF0_4);     //���DMA��������жϱ�־λ
    	if(DMA1_Stream4->CR & (1 << 19)) { //��ǰʹ�õ���buf1, ֤���ж���buf0���
    		pbuf = pI2sBus->tx_buf0;
    	}
    	else { //��ǰʹ�õ���buf0, ֤���ж���buf1���
    		pbuf = pI2sBus->tx_buf1;
    	}
    	if (pI2sBus->mode == MODE_I2S_PLAYER) { //�������������Ч�����ݣ��������0����
			/* �ӷ���ring��������ȡ���ݵ�dma buf */
			ret = VOSRingBufGet(pI2sBus->ring_tx, pbuf,  pI2sBus->tx_len);
			if (ret == 0) { //û���ݣ�ֱ�ӹر�dma, �´η���ʱ��x, pbuf, pI2sB������dma
				__HAL_DMA_DISABLE(&pI2sBus->hdma_tx);
				pI2sBus->is_dma_tx_working = 0;
			}
			else if (ret <= pI2sBus->tx_len) {//����֤��Դͷ����̫��������ֱ�����0�����־���
				memset(pbuf, 0x00, pI2sBus->tx_len-ret);
			}
    	}
    }
	VOSIntExit ();
}

//����dma�ж�
void DMA1_Stream3_IRQHandler()
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StI2sBus *pI2sBus = I2sBusPtr(SPI2);

	VOSIntEnter();
    if(pI2sBus && __HAL_DMA_GET_FLAG(&pI2sBus->hdma_rx, DMA_FLAG_TCIF3_7) != RESET) //DMA�������
    {
        __HAL_DMA_CLEAR_FLAG(&pI2sBus->hdma_rx, DMA_FLAG_TCIF3_7);     //���DMA��������жϱ�־λ
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


//I2S��ʼ����
void i2s_tx_dma_start(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_ENABLE(&pI2sBus->hdma_tx);//����DMA TX����
}

//�ر�I2S����
void i2s_tx_dma_stop(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_DISABLE(&pI2sBus->hdma_tx);//��������;
}

//I2S��ʼ¼��
void i2s_rx_dma_start(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_ENABLE(&pI2sBus->hdma_rx); //����DMA RX����
}

//�ر�I2S¼��
void i2s_rx_dma_stop(s32 port)
{
	struct StI2sBus *pI2sBus = &gI2sBus[port];
	__HAL_DMA_DISABLE(&pI2sBus->hdma_rx); //����¼��
}




