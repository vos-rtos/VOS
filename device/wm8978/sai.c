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
		//if (pSaiBus->mode == MODE_I2S_RECORDER) {//¼������master�� ������Ϊ�����ݡ�
			memset(pSaiBus->tx_buf0, 0, pSaiBus->tx_len);
			memset(pSaiBus->tx_buf1, 0, pSaiBus->tx_len);
		//}


		//���������ʱ�����ڵ�һ������С��ring buf maxʱ���趨��ʱ������
		pSaiBus->timer = VOSTimerCreate(VOS_TIMER_TYPE_ONE_SHOT, 500, sai_tx_timer, pSaiBus, "timer");
		if (pSaiBus->timer == 0) {
			err = -7;
			goto END;
		}

	}
	if (port == 0) { //���豸��ͬ������

		HAL_SAI_DeInit(&pSaiBus->handle);                          //�����ǰ������
		pSaiBus->handle.Instance=SAI1_Block_B;                     //SAI1 Bock B
		pSaiBus->handle.Init.AudioMode=SAI_Mode;                       //����SAI1����ģʽ
		pSaiBus->handle.Init.Synchro=SAI_SYNCHRONOUS;             //��Ƶģ��ͬ��
		pSaiBus->handle.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;   //����������Ƶģ�����
		pSaiBus->handle.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;   //ʹ����ʱ�ӷ�Ƶ��(MCKDIV)
		pSaiBus->handle.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF;  //����FIFO��ֵ,1/4 FIFO
		pSaiBus->handle.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;     //SIAʱ��ԴΪPLL2S
		pSaiBus->handle.Init.MonoStereoMode=SAI_STEREOMODE;        //������ģʽ
		pSaiBus->handle.Init.Protocol=SAI_FREE_PROTOCOL;           //����SAI1Э��Ϊ:����Э��(֧��I2S/LSB/MSB/TDM/PCM/DSP��Э��)
		pSaiBus->handle.Init.DataSize=SAI_DataFormat;                     //�������ݴ�С
		pSaiBus->handle.Init.FirstBit=SAI_FIRSTBIT_MSB;            //����MSBλ����
		pSaiBus->handle.Init.ClockStrobing=SAI_Clock_Polarity;                   //������ʱ�ӵ�����/�½���ѡͨ

		//֡����
		pSaiBus->handle.FrameInit.FrameLength=64;                  //����֡����Ϊ64,��ͨ��32��SCK,��ͨ��32��SCK.
		pSaiBus->handle.FrameInit.ActiveFrameLength=32;            //����֡ͬ����Ч��ƽ����,��I2Sģʽ��=1/2֡��.
		pSaiBus->handle.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS�ź�ΪSOF�ź�+ͨ��ʶ���ź�
		pSaiBus->handle.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;    //FS�͵�ƽ��Ч(�½���)
		pSaiBus->handle.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT;  //��slot0�ĵ�һλ��ǰһλʹ��FS,��ƥ������ֱ�׼

		//SLOT����
		pSaiBus->handle.SlotInit.FirstBitOffset=0;                 //slotƫ��(FBOFF)Ϊ0
		pSaiBus->handle.SlotInit.SlotSize=SAI_SLOTSIZE_32B;        //slot��СΪ32λ
		pSaiBus->handle.SlotInit.SlotNumber=2;                     //slot��Ϊ2��
		pSaiBus->handle.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//ʹ��slot0��slot1

		HAL_SAI_Init(&pSaiBus->handle);
		SAIB_DMA_Enable();
		__HAL_SAI_ENABLE(&pSaiBus->handle);                       //ʹ��SAI
	}
	else if (port == 1) { //���豸���첽����

		HAL_SAI_DeInit(&pSaiBus->handle);                          //�����ǰ������
		pSaiBus->handle.Instance=SAI1_Block_A;                     //SAI1 Bock A
		pSaiBus->handle.Init.AudioMode=SAI_Mode;                       //����SAI1����ģʽ
		pSaiBus->handle.Init.Synchro=SAI_ASYNCHRONOUS;             //��Ƶģ���첽
		pSaiBus->handle.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;   //����������Ƶģ�����
		pSaiBus->handle.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;   //ʹ����ʱ�ӷ�Ƶ��(MCKDIV)
		pSaiBus->handle.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF;  //����FIFO��ֵ,1/4 FIFO
		pSaiBus->handle.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;     //SIAʱ��ԴΪPLL2S
		pSaiBus->handle.Init.MonoStereoMode=SAI_STEREOMODE;        //������ģʽ
		pSaiBus->handle.Init.Protocol=SAI_FREE_PROTOCOL;           //����SAI1Э��Ϊ:����Э��(֧��I2S/LSB/MSB/TDM/PCM/DSP��Э��)
		pSaiBus->handle.Init.DataSize=SAI_DataFormat;                     //�������ݴ�С
		pSaiBus->handle.Init.FirstBit=SAI_FIRSTBIT_MSB;            //����MSBλ����
		pSaiBus->handle.Init.ClockStrobing=SAI_Clock_Polarity;                   //������ʱ�ӵ�����/�½���ѡͨ

		//֡����
		pSaiBus->handle.FrameInit.FrameLength=64;                  //����֡����Ϊ64,��ͨ��32��SCK,��ͨ��32��SCK.
		pSaiBus->handle.FrameInit.ActiveFrameLength=32;            //����֡ͬ����Ч��ƽ����,��I2Sģʽ��=1/2֡��.
		pSaiBus->handle.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS�ź�ΪSOF�ź�+ͨ��ʶ���ź�
		pSaiBus->handle.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;    //FS�͵�ƽ��Ч(�½���)
		pSaiBus->handle.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT;  //��slot0�ĵ�һλ��ǰһλʹ��FS,��ƥ������ֱ�׼

		//SLOT����
		pSaiBus->handle.SlotInit.FirstBitOffset=0;                 //slotƫ��(FBOFF)Ϊ0
		pSaiBus->handle.SlotInit.SlotSize=SAI_SLOTSIZE_32B;        //slot��СΪ32λ
		pSaiBus->handle.SlotInit.SlotNumber=2;                     //slot��Ϊ2��
		pSaiBus->handle.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//ʹ��slot0��slot1

		HAL_SAI_Init(&pSaiBus->handle);
		__HAL_SAI_ENABLE(&pSaiBus->handle);                       //ʹ��SAI
	}

	if (pSaiBus->is_inited==0) {
		pSaiBus->is_inited = 1;
	}
END:
	return err;

}
//
//SAI_HandleTypeDef SAI1A_Handler;        //SAI1 Block A���
//SAI_HandleTypeDef SAI1B_Handler;        //SAI1 Block B���
//DMA_HandleTypeDef SAI1_TXDMA_Handler;   //DMA���;��
//DMA_HandleTypeDef SAI1_RXDMA_Handler;   //DMA���վ��

//SAI Block A��ʼ��,I2S,�����ֱ�׼
//mode:����ģʽ,��������:SAI_MODEMASTER_TX/SAI_MODEMASTER_RX/SAI_MODESLAVE_TX/SAI_MODESLAVE_RX
//cpol:������ʱ�ӵ�����/�½���ѡͨ���������ã�SAI_CLOCKSTROBING_FALLINGEDGE/SAI_CLOCKSTROBING_RISINGEDGE
//datalen:���ݴ�С,�������ã�SAI_DATASIZE_8/10/16/20/24/32
//void SAIA_Init(u32 mode,u32 cpol,u32 datalen)
//{
//    HAL_SAI_DeInit(&SAI1A_Handler);                          //�����ǰ������
//    SAI1A_Handler.Instance=SAI1_Block_A;                     //SAI1 Bock A
//    SAI1A_Handler.Init.AudioMode=mode;                       //����SAI1����ģʽ
//    SAI1A_Handler.Init.Synchro=SAI_ASYNCHRONOUS;             //��Ƶģ���첽
//    SAI1A_Handler.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;   //����������Ƶģ�����
//    SAI1A_Handler.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;   //ʹ����ʱ�ӷ�Ƶ��(MCKDIV)
//    SAI1A_Handler.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF;  //����FIFO��ֵ,1/4 FIFO
//    SAI1A_Handler.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;     //SIAʱ��ԴΪPLL2S
//    SAI1A_Handler.Init.MonoStereoMode=SAI_STEREOMODE;        //������ģʽ
//    SAI1A_Handler.Init.Protocol=SAI_FREE_PROTOCOL;           //����SAI1Э��Ϊ:����Э��(֧��I2S/LSB/MSB/TDM/PCM/DSP��Э��)
//    SAI1A_Handler.Init.DataSize=datalen;                     //�������ݴ�С
//    SAI1A_Handler.Init.FirstBit=SAI_FIRSTBIT_MSB;            //����MSBλ����
//    SAI1A_Handler.Init.ClockStrobing=cpol;                   //������ʱ�ӵ�����/�½���ѡͨ
//
//    //֡����
//    SAI1A_Handler.FrameInit.FrameLength=64;                  //����֡����Ϊ64,��ͨ��32��SCK,��ͨ��32��SCK.
//    SAI1A_Handler.FrameInit.ActiveFrameLength=32;            //����֡ͬ����Ч��ƽ����,��I2Sģʽ��=1/2֡��.
//    SAI1A_Handler.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS�ź�ΪSOF�ź�+ͨ��ʶ���ź�
//    SAI1A_Handler.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;    //FS�͵�ƽ��Ч(�½���)
//    SAI1A_Handler.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT;  //��slot0�ĵ�һλ��ǰһλʹ��FS,��ƥ������ֱ�׼
//
//    //SLOT����
//    SAI1A_Handler.SlotInit.FirstBitOffset=0;                 //slotƫ��(FBOFF)Ϊ0
//    SAI1A_Handler.SlotInit.SlotSize=SAI_SLOTSIZE_32B;        //slot��СΪ32λ
//    SAI1A_Handler.SlotInit.SlotNumber=2;                     //slot��Ϊ2��
//    SAI1A_Handler.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//ʹ��slot0��slot1
//
//    HAL_SAI_Init(&SAI1A_Handler);
//    __HAL_SAI_ENABLE(&SAI1A_Handler);                       //ʹ��SAI
//}
//
////SAI Block B��ʼ��,I2S,�����ֱ�׼
////mode:����ģʽ,��������:SAI_MODEMASTER_TX/SAI_MODEMASTER_RX/SAI_MODESLAVE_TX/SAI_MODESLAVE_RX
////cpol:������ʱ�ӵ�����/�½���ѡͨ���������ã�SAI_CLOCKSTROBING_FALLINGEDGE/SAI_CLOCKSTROBING_RISINGEDGE
////datalen:���ݴ�С,�������ã�SAI_DATASIZE_8/10/16/20/24/32
//void SAIB_Init(u32 mode,u32 cpol,u32 datalen)
//{
//    HAL_SAI_DeInit(&SAI1B_Handler);                         //�����ǰ������
//    SAI1B_Handler.Instance=SAI1_Block_B;                    //SAI1 Bock B
//    SAI1B_Handler.Init.AudioMode=mode;                      //����SAI1����ģʽ
//    SAI1B_Handler.Init.Synchro=SAI_SYNCHRONOUS;             //��Ƶģ��ͬ��
//    SAI1B_Handler.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;  //����������Ƶģ�����
//    SAI1B_Handler.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;  //ʹ����ʱ�ӷ�Ƶ��(MCKDIV)
//    SAI1B_Handler.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF; //����FIFO��ֵ,1/4 FIFO
//    SAI1B_Handler.Init.ClockSource=SAI_CLKSOURCE_PLLI2S;    //SIAʱ��ԴΪPLL2S
//    SAI1B_Handler.Init.MonoStereoMode=SAI_STEREOMODE;       //������ģʽ
//    SAI1B_Handler.Init.Protocol=SAI_FREE_PROTOCOL;          //����SAI1Э��Ϊ:����Э��(֧��I2S/LSB/MSB/TDM/PCM/DSP��Э��)
//    SAI1B_Handler.Init.DataSize=datalen;                    //�������ݴ�С
//    SAI1B_Handler.Init.FirstBit=SAI_FIRSTBIT_MSB;           //����MSBλ����
//    SAI1B_Handler.Init.ClockStrobing=cpol;                  //������ʱ�ӵ�����/�½���ѡͨ
//
//    //֡����
//    SAI1B_Handler.FrameInit.FrameLength=64;                 //����֡����Ϊ64,��ͨ��32��SCK,��ͨ��32��SCK.
//    SAI1B_Handler.FrameInit.ActiveFrameLength=32;           //����֡ͬ����Ч��ƽ����,��I2Sģʽ��=1/2֡��.
//    SAI1B_Handler.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS�ź�ΪSOF�ź�+ͨ��ʶ���ź�
//    SAI1B_Handler.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;   //FS�͵�ƽ��Ч(�½���)
//    SAI1B_Handler.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT; //��slot0�ĵ�һλ��ǰһλʹ��FS,��ƥ������ֱ�׼
//
//    //SLOT����
//    SAI1B_Handler.SlotInit.FirstBitOffset=0;                //slotƫ��(FBOFF)Ϊ0
//    SAI1B_Handler.SlotInit.SlotSize=SAI_SLOTSIZE_32B;       //slot��СΪ32λ
//    SAI1B_Handler.SlotInit.SlotNumber=2;                    //slot��Ϊ2��
//    SAI1B_Handler.SlotInit.SlotActive=SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1;//ʹ��slot0��slot1
//
//    HAL_SAI_Init(&SAI1B_Handler);
//    SAIB_DMA_Enable();                                      //ʹ��SAI��DMA����
//    __HAL_SAI_ENABLE(&SAI1B_Handler);                       //ʹ��SAI
//}

//SAI�ײ��������������ã�ʱ��ʹ��
//�˺����ᱻHAL_SAI_Init()����
//hsdram:SAI���
void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef GPIO_Initure;
	struct StSaiBus *pSaiBus = SaiBusPtr(hsai->Instance);


    __HAL_RCC_SAI1_CLK_ENABLE();                //ʹ��SAI1ʱ��
    __HAL_RCC_GPIOE_CLK_ENABLE();               //ʹ��GPIOEʱ��

    //��ʼ��PE2,3,4,5,6Sai
    GPIO_Initure.Pin=GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;          //���츴��
    GPIO_Initure.Pull=GPIO_PULLUP;              //����
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //����
    GPIO_Initure.Alternate=GPIO_AF6_SAI1;       //����ΪSAI
    HAL_GPIO_Init(GPIOE,&GPIO_Initure);         //��ʼ��

    if (hsai->Instance == SAI1_Block_A) {

        //����DMA
         __HAL_RCC_DMA2_CLK_ENABLE();                                    //ʹ��DMA2ʱ��
         __HAL_LINKDMA(&pSaiBus->handle,hdmatx,pSaiBus->hdma_tx);        //��DMA��SAI��ϵ����
         pSaiBus->hdma_tx.Instance=DMA2_Stream3;                       //DMA2������3
         pSaiBus->hdma_tx.Init.Channel=DMA_CHANNEL_0;                  //ͨ��0
         pSaiBus->hdma_tx.Init.Direction=DMA_MEMORY_TO_PERIPH;         //�洢��������ģʽ
         pSaiBus->hdma_tx.Init.PeriphInc=DMA_PINC_DISABLE;             //���������ģʽ
         pSaiBus->hdma_tx.Init.MemInc=DMA_MINC_ENABLE;                 //�洢������ģʽ
         pSaiBus->hdma_tx.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;           //�������ݳ���:16/32λ
         pSaiBus->hdma_tx.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD;              //�洢�����ݳ���:16/32λ
         pSaiBus->hdma_tx.Init.Mode=DMA_CIRCULAR;                      //ʹ��ѭ��ģʽ
         pSaiBus->hdma_tx.Init.Priority=DMA_PRIORITY_HIGH;             //�����ȼ�
         pSaiBus->hdma_tx.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //��ʹ��FIFO

         pSaiBus->hdma_tx.Init.MemBurst=DMA_MBURST_SINGLE;             //�洢������ͻ������
         pSaiBus->hdma_tx.Init.PeriphBurst=DMA_PBURST_SINGLE;          //����ͻ�����δ���
         HAL_DMA_DeInit(&pSaiBus->hdma_tx);                            //�������ǰ������
         HAL_DMA_Init(&pSaiBus->hdma_tx);	                            //��ʼ��DMA

         HAL_DMAEx_MultiBufferStart(&pSaiBus->hdma_tx,(u32)pSaiBus->tx_buf0,
        		 (u32)&SAI1_Block_A->DR, (u32)pSaiBus->tx_buf1, pSaiBus->tx_len/2);//����˫����
         __HAL_DMA_DISABLE(&pSaiBus->hdma_tx);                         //�ȹرշ���DMA
         VOSDelayUs(10);                                                   //10us��ʱ����ֹ-O2�Ż�������
         __HAL_DMA_ENABLE_IT(&pSaiBus->hdma_tx, DMA_IT_TC);             //������������ж�
         __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_tx, DMA_FLAG_TCIF3_7);     //���DMA��������жϱ�־λ
         HAL_NVIC_SetPriority(DMA2_Stream3_IRQn,0,0);                    //DMA�ж����ȼ�
         HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    }
    else if (hsai->Instance == SAI1_Block_B) {
         //����DMA
           __HAL_RCC_DMA2_CLK_ENABLE();                                    //ʹ��DMA2ʱ��
           __HAL_LINKDMA(&pSaiBus->handle,hdmarx,pSaiBus->hdma_rx);        //��DMA��SAI��ϵ����
           pSaiBus->hdma_rx.Instance=DMA2_Stream5;                       //DMA2������5
           pSaiBus->hdma_rx.Init.Channel=DMA_CHANNEL_0;                  //ͨ��0
           pSaiBus->hdma_rx.Init.Direction=DMA_PERIPH_TO_MEMORY;         //���赽�洢��ģʽ
           pSaiBus->hdma_rx.Init.PeriphInc=DMA_PINC_DISABLE;             //���������ģʽ
           pSaiBus->hdma_rx.Init.MemInc=DMA_MINC_ENABLE;                 //�洢������ģʽ
           pSaiBus->hdma_rx.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;           //�������ݳ���:16/32λ
           pSaiBus->hdma_rx.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD;              //�洢�����ݳ���:16/32λ
           pSaiBus->hdma_rx.Init.Mode=DMA_CIRCULAR;                      //ʹ��ѭ��ģʽ
           pSaiBus->hdma_rx.Init.Priority=DMA_PRIORITY_MEDIUM;           //�е����ȼ�
           pSaiBus->hdma_rx.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //��ʹ��FIFO
           pSaiBus->hdma_rx.Init.MemBurst=DMA_MBURST_SINGLE;             //�洢������ͻ������
           pSaiBus->hdma_rx.Init.PeriphBurst=DMA_PBURST_SINGLE;          //����ͻ�����δ���
           HAL_DMA_DeInit(&pSaiBus->hdma_rx);                            //�������ǰ������
           HAL_DMA_Init(&pSaiBus->hdma_rx);	                            //��ʼ��DMA

           HAL_DMAEx_MultiBufferStart(&pSaiBus->hdma_rx,(u32)&SAI1_Block_B->DR,
        		   (u32)pSaiBus->rx_buf0,(u32)pSaiBus->rx_buf1,pSaiBus->rx_len/2);//����˫����
           __HAL_DMA_DISABLE(&pSaiBus->hdma_rx);                         //�ȹرս���DMA
           VOSDelayUs(10);                                                   //10us��ʱ����ֹ-O2�Ż�������
           __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_rx,DMA_FLAG_TCIF1_5);     //���DMA��������жϱ�־λ
           __HAL_DMA_ENABLE_IT(&pSaiBus->hdma_rx,DMA_IT_TC);             //������������ж�

           HAL_NVIC_SetPriority(DMA2_Stream5_IRQn,0,1);                    //DMA�ж����ȼ�
           HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    }
}

//SAI Block A����������
//�����ʼ��㹫ʽ:
//MCKDIV!=0: Fs=SAI_CK_x/[512*MCKDIV]
//MCKDIV==0: Fs=SAI_CK_x/256
//SAI_CK_x=(HSE/pllm)*PLLI2SN/PLLI2SQ/(PLLI2SDIVQ+1)
//һ��HSE=25Mhz
//pllm:��Stm32_Clock_Init���õ�ʱ��ȷ����һ����25
//PLLI2SN:һ����192~432
//PLLI2SQ:2~15
//PLLI2SDIVQ:0~31
//MCKDIV:0~15
//SAI A��Ƶϵ����@pllm=8,HSE=25Mhz,��vco����Ƶ��Ϊ1Mhz
const u16 SAI_PSC_TBL[][5]=
{
	{800 ,344,7,0,12},	//8Khz������
	{1102,429,2,18,2},	//11.025Khz������
	{1600,344,7, 0,6},	//16Khz������
	{2205,429,2,18,1},	//22.05Khz������
	{3200,344,7, 0,3},	//32Khz������
	{4410,429,2,18,0},	//44.1Khz������
	{4800,344,7, 0,2},	//48Khz������
	{8820,271,2, 2,1},	//88.2Khz������
	{9600,344,7, 0,1},	//96Khz������
	{17640,271,2,2,0},	//176.4Khz������
	{19200,344,7,0,0},	//192Khz������
};

//����SAIB��DMA����,HAL��û���ṩ�˺���
//���������Ҫ�Լ������Ĵ�����дһ��
void SAIA_DMA_Enable(void)
{
    u32 tempreg=0;
    tempreg=SAI1_Block_A->CR1;          //�ȶ�����ǰ������
	tempreg|=1<<17;					    //ʹ��DMA
	SAI1_Block_A->CR1=tempreg;		    //д��CR1�Ĵ�����
}

//����SAIB��DMA����,HAL��û���ṩ�˺���
//���������Ҫ�Լ������Ĵ�����дһ��
void SAIB_DMA_Enable(void)
{
    u32 tempreg=0;
    tempreg=SAI1_Block_B->CR1;          //�ȶ�����ǰ������
	tempreg|=1<<17;					    //ʹ��DMA
	SAI1_Block_B->CR1=tempreg;		    //д��CR1�Ĵ�����
}
//����SAIA�Ĳ�����(@MCKEN)
//samplerate:������,��λ:Hz
//����ֵ:0,���óɹ�;1,�޷�����.
u8 SAIA_SampleRate_Set(s32 port, u32 samplerate)
{
    u8 i=0;
	struct StSaiBus *pSaiBus = &gSaiBus[port];
    RCC_PeriphCLKInitTypeDef RCCSAI1_Sture;
	for(i=0;i<(sizeof(SAI_PSC_TBL)/10);i++)//�����Ĳ������Ƿ����֧��
	{
		if((samplerate/10)==SAI_PSC_TBL[i][0])break;
	}
    if(i==(sizeof(SAI_PSC_TBL)/10))return 1;//�ѱ���Ҳ�Ҳ���
    RCCSAI1_Sture.PeriphClockSelection=RCC_PERIPHCLK_SAI_PLLI2S;    //����ʱ��Դѡ��
    RCCSAI1_Sture.PLLI2S.PLLI2SN=(u32)SAI_PSC_TBL[i][1];            //����PLLI2SN
    RCCSAI1_Sture.PLLI2S.PLLI2SQ=(u32)SAI_PSC_TBL[i][2];            //����PLLI2SQ
    //����PLLI2SDivQ��ʱ��SAI_PSC_TBL[i][3]Ҫ��1����ΪHAL���л��ڰ�PLLI2SDivQ�����Ĵ���DCKCFGR��ʱ���1
    RCCSAI1_Sture.PLLI2SDivQ=SAI_PSC_TBL[i][3]+1;                   //����PLLI2SDIVQ
    HAL_RCCEx_PeriphCLKConfig(&RCCSAI1_Sture);                      //����ʱ��

    __HAL_RCC_SAI_BLOCKACLKSOURCE_CONFIG(RCC_SAIACLKSOURCE_PLLI2S); //����SAI1ʱ����ԴΪPLLI2SQ

    __HAL_SAI_DISABLE(&pSaiBus->handle);                              //�ر�SAI
    pSaiBus->handle.Init.AudioFrequency=samplerate;                   //���ò���Ƶ��
    HAL_SAI_Init(&pSaiBus->handle);                                   //��ʼ��SAI
    SAIA_DMA_Enable();                                              //����SAI��DMA����
    __HAL_SAI_ENABLE(&pSaiBus->handle);                               //����SAI
    return 0;
}
////SAIA TX DMA����
////����Ϊ˫����ģʽ,������DMA��������ж�
////buf0:M0AR��ַ.
////buf1:M1AR��ַ.
////num:ÿ�δ���������
////width:λ��(�洢��������,ͬʱ����),0,8λ;1,16λ;2,32λ;
//void SAIA_TX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width)
//{
//    u32 memwidth=0,perwidth=0;      //����ʹ洢��λ��
//    switch(width)
//    {
//        case 0:         //8λ
//            memwidth=DMA_MDATAALIGN_BYTE;
//            perwidth=DMA_PDATAALIGN_BYTE;
//            break;
//        case 1:         //16λ
//            memwidth=DMA_MDATAALIGN_HALFWORD;
//            perwidth=DMA_PDATAALIGN_HALFWORD;
//            break;
//        case 2:         //32λ
//            memwidth=DMA_MDATAALIGN_WORD;
//            perwidth=DMA_PDATAALIGN_WORD;
//            break;
//
//    }
//    __HAL_RCC_DMA2_CLK_ENABLE();                                    //ʹ��DMA2ʱ��
//    __HAL_LINKDMA(&SAI1A_Handler,hdmatx,SAI1_TXDMA_Handler);        //��DMA��SAI��ϵ����
//    SAI1_TXDMA_Handler.Instance=DMA2_Stream3;                       //DMA2������3
//    SAI1_TXDMA_Handler.Init.Channel=DMA_CHANNEL_0;                  //ͨ��0
//    SAI1_TXDMA_Handler.Init.Direction=DMA_MEMORY_TO_PERIPH;         //�洢��������ģʽ
//    SAI1_TXDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE;             //���������ģʽ
//    SAI1_TXDMA_Handler.Init.MemInc=DMA_MINC_ENABLE;                 //�洢������ģʽ
//    SAI1_TXDMA_Handler.Init.PeriphDataAlignment=perwidth;           //�������ݳ���:16/32λ
//    SAI1_TXDMA_Handler.Init.MemDataAlignment=memwidth;              //�洢�����ݳ���:16/32λ
//    SAI1_TXDMA_Handler.Init.Mode=DMA_CIRCULAR;                      //ʹ��ѭ��ģʽ
//    SAI1_TXDMA_Handler.Init.Priority=DMA_PRIORITY_HIGH;             //�����ȼ�
//    SAI1_TXDMA_Handler.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //��ʹ��FIFO
//
//}
//
////SAIA TX DMA����
////����Ϊ˫����ģʽ,������DMA��������ж�
////buf0:M0AR��ַ.
////buf1:M1AR��ַ.
////num:ÿ�δ���������
////width:λ��(�洢��������,ͬʱ����),0,8λ;1,16λ;2,32λ;
//void SAIA_RX_DMA_Init(u8* buf0,u8 *buf1,u16 num,u8 width)
//{
//    u32 memwidth=0,perwidth=0;      //����ʹ洢��λ��
//    switch(width)
//    {
//        case 0:         //8λ
//            memwidth=DMA_MDATAALIGN_BYTE;
//            perwidth=DMA_PDATAALIGN_BYTE;
//            break;
//        case 1:         //16λ
//            memwidth=DMA_MDATAALIGN_HALFWORD;
//            perwidth=DMA_PDATAALIGN_HALFWORD;
//            break;
//        case 2:         //32λ
//            memwidth=DMA_MDATAALIGN_WORD;
//            perwidth=DMA_PDATAALIGN_WORD;
//            break;
//
//    }
//    __HAL_RCC_DMA2_CLK_ENABLE();                                    //ʹ��DMA2ʱ��
//    __HAL_LINKDMA(&SAI1B_Handler,hdmarx,SAI1_RXDMA_Handler);        //��DMA��SAI��ϵ����
//    SAI1_RXDMA_Handler.Instance=DMA2_Stream5;                       //DMA2������5
//    SAI1_RXDMA_Handler.Init.Channel=DMA_CHANNEL_0;                  //ͨ��0
//    SAI1_RXDMA_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;         //���赽�洢��ģʽ
//    SAI1_RXDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE;             //���������ģʽ
//    SAI1_RXDMA_Handler.Init.MemInc=DMA_MINC_ENABLE;                 //�洢������ģʽ
//    SAI1_RXDMA_Handler.Init.PeriphDataAlignment=perwidth;           //�������ݳ���:16/32λ
//    SAI1_RXDMA_Handler.Init.MemDataAlignment=memwidth;              //�洢�����ݳ���:16/32λ
//    SAI1_RXDMA_Handler.Init.Mode=DMA_CIRCULAR;                      //ʹ��ѭ��ģʽ
//    SAI1_RXDMA_Handler.Init.Priority=DMA_PRIORITY_MEDIUM;           //�е����ȼ�
//    SAI1_RXDMA_Handler.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          //��ʹ��FIFO
//    SAI1_RXDMA_Handler.Init.MemBurst=DMA_MBURST_SINGLE;             //�洢������ͻ������
//    SAI1_RXDMA_Handler.Init.PeriphBurst=DMA_PBURST_SINGLE;          //����ͻ�����δ���
//    HAL_DMA_DeInit(&SAI1_RXDMA_Handler);                            //�������ǰ������
//    HAL_DMA_Init(&SAI1_RXDMA_Handler);	                            //��ʼ��DMA
//
//    HAL_DMAEx_MultiBufferStart(&SAI1_RXDMA_Handler,(u32)&SAI1_Block_B->DR,(u32)buf0,(u32)buf1,num);//����˫����
//    __HAL_DMA_DISABLE(&SAI1_RXDMA_Handler);                         //�ȹرս���DMA
//    VOSDelayUs(10);                                                   //10us��ʱ����ֹ-O2�Ż�������
//    __HAL_DMA_CLEAR_FLAG(&SAI1_RXDMA_Handler,DMA_FLAG_TCIF1_5);     //���DMA��������жϱ�־λ
//    __HAL_DMA_ENABLE_IT(&SAI1_RXDMA_Handler,DMA_IT_TC);             //������������ж�
//
//    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn,0,1);                    //DMA�ж����ȼ�
//    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
//}
//
////SAI DMA�ص�����ָ��
//void (*sai_tx_callback)(void);	//TX�ص�����
//void (*sai_rx_callback)(void);	//RX�ص�����

s32 sai_sends(s32 port, u8 *buf, s32 len, u32 timeout_ms)
{
	s32 ret = 0;
	s32 irq_save;
	struct StSaiBus *pSaiBus = &gSaiBus[port];

	if (pSaiBus->handle.Instance) {
		irq_save = __vos_irq_save();
		ret = VOSRingBufSet(pSaiBus->ring_tx, buf, len);
		__vos_irq_restore(irq_save);
		if (VOSRingBufIsFull(pSaiBus->ring_tx)) {//װ��
			if (!pSaiBus->is_dma_tx_working) {
				__HAL_DMA_ENABLE(&pSaiBus->hdma_tx);//����DMA TX����
				pSaiBus->is_dma_tx_working = 1;
				if (pSaiBus->timer) { //ֹͣ��ʱ��
					VOSTimerStop(pSaiBus->timer);
				}
			}
		}
		else {//��������С��ring tx bufʱ��û�κδ�������
			if (!pSaiBus->is_dma_tx_working && pSaiBus->timer) {//���û����dma���ͣ����趨��ʱ��
				VOSTimerStart(pSaiBus->timer); //������ʱ��, �ظ����ã��᷵�ض�ʱ����������Ϣ
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

//DMA2_Stream3�жϷ�����Sai
void DMA2_Stream3_IRQHandler(void)
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StSaiBus *pSaiBus = SaiBusPtr(SAI1_Block_A);
	VOSIntEnter();
    if(__HAL_DMA_GET_FLAG(&pSaiBus->hdma_tx,DMA_FLAG_TCIF3_7)!=RESET) //DMA�������
    {
        __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_tx,DMA_FLAG_TCIF3_7);     //���DMA��������жϱ�־λ
		if(DMA2_Stream3->CR & (1 << 19)) { //��ǰʹ�õ���buf1, ֤���ж���buf0���
			pbuf = pSaiBus->tx_buf0;
		}
		else { //��ǰʹ�õ���buf0, ֤���ж���buf1���
			pbuf = pSaiBus->tx_buf1;
		}
		if (pSaiBus->mode == MODE_SAI_PLAYER) { //�������������Ч�����ݣ��������0����
			/* �ӷ���ring��������ȡ���ݵ�dma buf */
			ret = VOSRingBufGet(pSaiBus->ring_tx, pbuf,  pSaiBus->tx_len);
			if (ret == 0) { //û���ݣ�ֱ�ӹر�dma, �´η���ʱ��x, pbuf, pI2sB������dma
				__HAL_DMA_DISABLE(&pSaiBus->hdma_tx);
				pSaiBus->is_dma_tx_working = 0;
			}
			else if (ret <= pSaiBus->tx_len) {//����֤��Դͷ����̫��������ֱ�����0�����־���
				memset(pbuf, 0x00, pSaiBus->tx_len-ret);
			}
		}
    }
	VOSIntExit ();
}
//DMA2_Stream5�жϷ����� (���գ�
void DMA2_Stream5_IRQHandler(void)
{
	s32 ret = 0;
	u8 *pbuf = 0;
	struct StSaiBus *pSaiBus = SaiBusPtr(SAI1_Block_B);
	VOSIntEnter();
    if(__HAL_DMA_GET_FLAG(&pSaiBus->hdma_rx,DMA_FLAG_TCIF1_5)!=RESET) //DMA�������
    {
        __HAL_DMA_CLEAR_FLAG(&pSaiBus->hdma_rx,DMA_FLAG_TCIF1_5);     //���DMA��������жϱ�־λ
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
////SAI��ʼ����
//void SAI_Play_Start(void)
//{
//    __HAL_DMA_ENABLE(&SAI1_TXDMA_Handler);//����DMA TX����
//}
////�ر�I2S����
//void SAI_Play_Stop(void)
//{
//    __HAL_DMA_DISABLE(&SAI1_TXDMA_Handler);  //��������
//}
////SAI��ʼ¼��
//void SAI_Rec_Start(void)
//{
//    __HAL_DMA_ENABLE(&SAI1_RXDMA_Handler);//����DMA RX����
//}
////�ر�SAI¼��
//void SAI_Rec_Stop(void)
//{
//    __HAL_DMA_DISABLE(&SAI1_RXDMA_Handler);//����¼��
//}

//I2S��ʼ����
void sai_tx_dma_start(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_ENABLE(&pSaiBus->hdma_tx);//����DMA TX����
}

//�ر�I2S����
void sai_tx_dma_stop(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_DISABLE(&pSaiBus->hdma_tx);//��������;
}

//I2S��ʼ¼��
void sai_rx_dma_start(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_ENABLE(&pSaiBus->hdma_rx); //����DMA RX����
}

//�ر�I2S¼��
void sai_rx_dma_stop(s32 port)
{
	struct StSaiBus *pSaiBus = &gSaiBus[port];
	__HAL_DMA_DISABLE(&pSaiBus->hdma_rx); //����¼��
}

#endif
