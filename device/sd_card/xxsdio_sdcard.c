#if 0
#include "sdio_sdcard.h"
#include "string.h"	 

#define INTX_DISABLE() 	int irq_save = __vos_irq_save();
#define INTX_ENABLE() __vos_irq_restore(irq_save)


#if 0
//GPIOͨ������
//GPIOx:GPIOA~GPIOI.
//BITx:0X0000~0XFFFF,λ����,ÿ��λ����һ��IO,��0λ����Px0,��1λ����Px1,��������.����0X0101,����ͬʱ����Px0��Px8.
//MODE:0~3;ģʽѡ��,0,����(ϵͳ��λĬ��״̬);1,��ͨ���;2,���ù���;3,ģ������.
//OTYPE:0/1;�������ѡ��,0,�������;1,��©���.
//OSPEED:0~3;����ٶ�����,0,2Mhz;1,25Mhz;2,50Mhz;3,100Mh.
//PUPD:0~3:����������,0,����������;1,����;2,����;3,����.
//ע��:������ģʽ(��ͨ����/ģ������)��,OTYPE��OSPEED������Ч!!
void GPIO_Set(GPIO_TypeDef* GPIOx,u32 BITx,u32 MODE,u32 OTYPE,u32 OSPEED,u32 PUPD)
{
	u32 pinpos=0,pos=0,curpin=0;
	for(pinpos=0;pinpos<16;pinpos++)
	{
		pos=1<<pinpos;	//һ����λ���
		curpin=BITx&pos;//��������Ƿ�Ҫ����
		if(curpin==pos)	//��Ҫ����
		{
			GPIOx->MODER&=~(3<<(pinpos*2));	//�����ԭ��������
			GPIOx->MODER|=MODE<<(pinpos*2);	//�����µ�ģʽ
			if((MODE==0X01)||(MODE==0X02))	//��������ģʽ/���ù���ģʽ
			{
				GPIOx->OSPEEDR&=~(3<<(pinpos*2));	//���ԭ��������
				GPIOx->OSPEEDR|=(OSPEED<<(pinpos*2));//�����µ��ٶ�ֵ
				GPIOx->OTYPER&=~(1<<pinpos) ;		//���ԭ��������
				GPIOx->OTYPER|=OTYPE<<pinpos;		//�����µ����ģʽ
			}
			GPIOx->PUPDR&=~(3<<(pinpos*2));	//�����ԭ��������
			GPIOx->PUPDR|=PUPD<<(pinpos*2);	//�����µ�������
		}
	}
}
//����NVIC����
//NVIC_Group:NVIC���� 0~4 �ܹ�5��
void MY_NVIC_PriorityGroupConfig(u8 NVIC_Group)
{
	u32 temp,temp1;
	temp1=(~NVIC_Group)&0x07;//ȡ����λ
	temp1<<=8;
	temp=SCB->AIRCR;  //��ȡ��ǰ������
	temp&=0X0000F8FF; //�����ǰ����
	temp|=0X05FA0000; //д��Կ��
	temp|=temp1;
	SCB->AIRCR=temp;  //���÷���
}
//����NVIC
//NVIC_PreemptionPriority:��ռ���ȼ�
//NVIC_SubPriority       :��Ӧ���ȼ�
//NVIC_Channel           :�жϱ��
//NVIC_Group             :�жϷ��� 0~4
//ע�����ȼ����ܳ����趨����ķ�Χ!����������벻���Ĵ���
//�黮��:
//��0:0λ��ռ���ȼ�,4λ��Ӧ���ȼ�
//��1:1λ��ռ���ȼ�,3λ��Ӧ���ȼ�
//��2:2λ��ռ���ȼ�,2λ��Ӧ���ȼ�
//��3:3λ��ռ���ȼ�,1λ��Ӧ���ȼ�
//��4:4λ��ռ���ȼ�,0λ��Ӧ���ȼ�
//NVIC_SubPriority��NVIC_PreemptionPriority��ԭ����,��ֵԽС,Խ����
void MY_NVIC_Init(u8 NVIC_PreemptionPriority,u8 NVIC_SubPriority,u8 NVIC_Channel,u8 NVIC_Group)
{
	u32 temp;
	MY_NVIC_PriorityGroupConfig(NVIC_Group);//���÷���
	temp=NVIC_PreemptionPriority<<(4-NVIC_Group);
	temp|=NVIC_SubPriority&(0x0f>>NVIC_Group);
	temp&=0xf;								//ȡ����λ
	NVIC->ISER[NVIC_Channel/32]|=1<<NVIC_Channel%32;//ʹ���ж�λ(Ҫ����Ļ�,����ICER��ӦλΪ1����)
	NVIC->IP[NVIC_Channel]|=temp<<4;				//������Ӧ���ȼ����������ȼ�
}

//GPIO��������
//GPIOx:GPIOA~GPIOI.
//BITx:0~15,����IO���ű��.
//AFx:0~15,����AF0~AF15.
//AF0~15�������(��������г����õ�,��ϸ�����429/746�����ֲ�,Table 12):
//AF0:MCO/SWD/SWCLK/RTC   		AF1:TIM1/TIM2;            		AF2:TIM3~5;               		AF3:TIM8~11
//AF4:I2C1~I2C4;          		AF5:SPI1~SPI6;            		AF6:SPI3/SAI1;            	 	AF7:SPI2/3/USART1~3/UART5/SPDIFRX;
//AF8:USART4~8/SPDIFRX/SAI2; 	AF9;CAN1~2/TIM12~14/LCD/QSPI; 	AF10:USB_OTG/USB_HS/SAI2/QSPI  	AF11:ETH
//AF12:FMC/SDIO/OTG/HS   		AF13:DCIM                 		AF14:LCD;                  		AF15:EVENTOUT
void GPIO_AF_Set(GPIO_TypeDef* GPIOx,u8 BITx,u8 AFx)
{
	GPIOx->AFR[BITx>>3]&=~(0X0F<<((BITx&0X07)*4));
	GPIOx->AFR[BITx>>3]|=(u32)AFx<<((BITx&0X07)*4);
}


/*����sdio��ʼ���Ľṹ��*/
SDIO_InitTypeDef SDIO_InitStructure;
SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
SDIO_DataInitTypeDef SDIO_DataInitStructure;   

SD_Error CmdError(void);  
SD_Error CmdResp7Error(void);
SD_Error CmdResp1Error(u8 cmd);
SD_Error CmdResp3Error(void);
SD_Error CmdResp2Error(void);
SD_Error CmdResp6Error(u8 cmd,u16*prca);  
SD_Error SDEnWideBus(u8 enx);	  
SD_Error IsCardProgramming(u8 *pstatus); 
SD_Error FindSCR(u16 rca,u32 *pscr);
u8 convert_from_bytes_to_power_of_two(u16 NumberOfBytes); 


static u8 CardType=SDIO_STD_CAPACITY_SD_CARD_V1_1;		//SD�����ͣ�Ĭ��Ϊ1.x����
static u32 CSD_Tab[4],CID_Tab[4],RCA=0;					//SD��CSD,CID�Լ���Ե�ַ(RCA)����
static u8 DeviceMode=SD_DMA_MODE;		   				//����ģʽ,ע��,����ģʽ����ͨ��SD_SetDeviceMode,�������.����ֻ�Ƕ���һ��Ĭ�ϵ�ģʽ(SD_DMA_MODE)
static u8 StopCondition=0; 								//�Ƿ���ֹͣ�����־λ,DMA����д��ʱ���õ�  
volatile SD_Error TransferError=HAL_OK;					//���ݴ�������־,DMA��дʱʹ��
volatile u8 TransferEnd=0;								//���������־,DMA��дʱʹ��
SD_CardInfo SDCardInfo;									//SD����Ϣ

//SD_ReadDisk/SD_WriteDisk����ר��buf,�����������������ݻ�������ַ����4�ֽڶ����ʱ��,
//��Ҫ�õ�������,ȷ�����ݻ�������ַ��4�ֽڶ����.
//__align(4) u8 SDIO_DATA_BUFFER[512];
u8 SDIO_DATA_BUFFER[512] __attribute__ ((aligned (4)));
 
 
void SDIO_Register_Deinit()
{
	SDIO->POWER=0x00000000;
	SDIO->CLKCR=0x00000000;
	SDIO->ARG=0x00000000;
	SDIO->CMD=0x00000000;
	SDIO->DTIMER=0x00000000;
	SDIO->DLEN=0x00000000;
	SDIO->DCTRL=0x00000000;
	SDIO->ICR=0x00C007FF;
	SDIO->MASK=0x00000000;	 
}

int kprintf(char* format, ...);
#define PRT_REG(x) kprintf(#x"=0x%08x\r\n", x)

void SDIO_SET_REGS(unsigned int v) {
	SDIO->CLKCR = v;
}

void SDIO_SHOW_REGS()
{

  PRT_REG(SDIO->POWER);
  PRT_REG(SDIO->CLKCR);
  PRT_REG(SDIO->ARG);
  PRT_REG(SDIO->CMD);
  PRT_REG(SDIO->RESPCMD);
  PRT_REG(SDIO->RESP1);
  PRT_REG(SDIO->RESP2);
  PRT_REG(SDIO->RESP3);
  PRT_REG(SDIO->RESP4);
  PRT_REG(SDIO->DTIMER);
	PRT_REG(SDIO->DLEN);
  PRT_REG(SDIO->DCTRL);
  PRT_REG(SDIO->DCOUNT);
  PRT_REG(SDIO->STA);
	PRT_REG(SDIO->ICR);
  PRT_REG(SDIO->MASK);
  //PRT_REG(SDIO->RESERVED0[2]);
  PRT_REG(SDIO->FIFOCNT);
  //PRT_REG(SDIO->RESERVED1[13]);
  PRT_REG(SDIO->FIFO);

}


//��ʼ��SD��
//����ֵ:�������;(0,�޴���)
SD_Error SD_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	SD_Error errorstatus=HAL_OK;
  u8 clkdiv=0;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_DMA2, ENABLE);//ʹ��GPIOC,GPIOD DMA2ʱ��
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, ENABLE);//SDIOʱ��ʹ��
	
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, ENABLE);//SDIO��λ
	
	
  GPIO_InitStructure.GPIO_Pin =GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12; 	//PC8,9,10,11,12���ù������	
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100M
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOC, &GPIO_InitStructure);// PC8,9,10,11,12���ù������

	
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_2;
  GPIO_Init(GPIOD, &GPIO_InitStructure);//PD2���ù������
	
	 //���Ÿ���ӳ������
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_SDIO); //PC8,AF12
  GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_SDIO);	
  GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_SDIO);	
	
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, DISABLE);//SDIO������λ
		
 	//SDIO����Ĵ�������ΪĬ��ֵ 			   
	SDIO_Register_Deinit();
	
  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����
	
   	errorstatus=SD_PowerON();			//SD���ϵ�
 	if(errorstatus==HAL_OK)errorstatus=SD_InitializeCards();			//��ʼ��SD��
  	if(errorstatus==HAL_OK)errorstatus=SD_GetCardInfo(&SDCardInfo);	//��ȡ����Ϣ
 	if(errorstatus==HAL_OK)errorstatus=SD_SelectDeselect((u32)(SDCardInfo.RCA<<16));//ѡ��SD��
   	if(errorstatus==HAL_OK)errorstatus=SD_EnableWideBusOperation(SDIO_BusWide_4b);	//4λ���,�����MMC��,������4λģʽ
  	if((errorstatus==HAL_OK)||(SDIO_MULTIMEDIA_CARD==CardType))
	{  		    
		if(SDCardInfo.CardType==SDIO_STD_CAPACITY_SD_CARD_V1_1||SDCardInfo.CardType==SDIO_STD_CAPACITY_SD_CARD_V2_0)
		{
			clkdiv=SDIO_TRANSFER_CLK_DIV+2;	//V1.1/V2.0�����������48/4=12Mhz
		}else clkdiv=SDIO_TRANSFER_CLK_DIV;	//SDHC�����������������48/2=24Mhz
		SDIO_Clock_Set(clkdiv);	//����ʱ��Ƶ��,SDIOʱ�Ӽ��㹫ʽ:SDIO_CKʱ��=SDIOCLK/[clkdiv+2];����,SDIOCLK�̶�Ϊ48Mhz 
		errorstatus=SD_SetDeviceMode(SD_DMA_MODE);	//����ΪDMAģʽ
		//errorstatus=SD_SetDeviceMode(SD_POLLING_MODE);//����Ϊ��ѯģʽ
 	}
	return errorstatus;		 
}
//SDIOʱ�ӳ�ʼ������
//clkdiv:ʱ�ӷ�Ƶϵ��
//CKʱ��=SDIOCLK/[clkdiv+2];(SDIOCLKʱ�ӹ̶�Ϊ48Mhz)
void SDIO_Clock_Set(u8 clkdiv)
{
	u32 tmpreg=SDIO->CLKCR; 
  	tmpreg&=0XFFFFFF00; 
 	//tmpreg|=clkdiv;
  	tmpreg = tmpreg/clkdiv;
	SDIO->CLKCR=tmpreg;
} 


//���ϵ�
//��ѯ����SDIO�ӿ��ϵĿ��豸,����ѯ���ѹ������ʱ��
//����ֵ:�������;(0,�޴���)
SD_Error SD_PowerON(void)
{
 	u8 i=0;
	SD_Error errorstatus=HAL_OK;
	u32 response=0,count=0,validvoltage=0;
	u32 SDType=SD_STD_CAPACITY;
	
	 /*��ʼ��ʱ��ʱ�Ӳ��ܴ���400KHz*/ 
  SDIO_InitStructure.SDIO_ClockDiv = SDIO_INIT_CLK_DIV;	/* HCLK = 72MHz, SDIOCLK = 72MHz, SDIO_CK = HCLK/(178 + 2) = 400 KHz */
  SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;  //��ʹ��bypassģʽ��ֱ����HCLK���з�Ƶ�õ�SDIO_CK
  SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;	// ����ʱ���ر�ʱ�ӵ�Դ
  SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;	 				//1λ������
  SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;//Ӳ����
  SDIO_Init(&SDIO_InitStructure);

	SDIO_SetPowerState(SDIO_PowerState_ON);	//�ϵ�״̬,������ʱ��   
  SDIO->CLKCR|=1<<8;			//SDIOCKʹ��  
 
 	for(i=0;i<74;i++)
	{
 
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;//����CMD0����IDLE STAGEģʽ����.
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE; //cmd0
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;  //����Ӧ
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;  //��CPSM�ڿ�ʼ��������֮ǰ�ȴ����ݴ�������� 
    SDIO_SendCommand(&SDIO_CmdInitStructure);	  		//д���������Ĵ���
		
		errorstatus=CmdError();
		
		if(errorstatus==HAL_OK)break;
 	}
 	if(errorstatus)return errorstatus;//���ش���״̬
	
  SDIO_CmdInitStructure.SDIO_Argument = SD_CHECK_PATTERN;	//����CMD8,����Ӧ,���SD���ӿ�����
  SDIO_CmdInitStructure.SDIO_CmdIndex = SDIO_SEND_IF_COND;	//cmd8
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;	 //r7
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;			 //�رյȴ��ж�
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
	
  errorstatus=CmdResp7Error();						//�ȴ�R7��Ӧ
	
 	if(errorstatus==HAL_OK) 								//R7��Ӧ����
	{
		CardType=SDIO_STD_CAPACITY_SD_CARD_V2_0;		//SD 2.0��
		SDType=SD_HIGH_CAPACITY;			   			//��������
	}
	  
	  SDIO_CmdInitStructure.SDIO_Argument = 0x00;//����CMD55,����Ӧ	
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);		//����CMD55,����Ӧ	 
	
	 errorstatus=CmdResp1Error(SD_CMD_APP_CMD); 		 	//�ȴ�R1��Ӧ   
	
	if(errorstatus==HAL_OK)//SD2.0/SD 1.1,����ΪMMC��
	{																  
		//SD��,����ACMD41 SD_APP_OP_COND,����Ϊ:0x80100000 
		while((!validvoltage)&&(count<SD_MAX_VOLT_TRIAL))
		{	   										   
		  SDIO_CmdInitStructure.SDIO_Argument = 0x00;//����CMD55,����Ӧ
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;	  //CMD55
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);			//����CMD55,����Ӧ	 
			
			errorstatus=CmdResp1Error(SD_CMD_APP_CMD); 	 	//�ȴ�R1��Ӧ  
			
 			if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����

      //acmd41�����������֧�ֵĵ�ѹ��Χ��HCSλ��ɣ�HCSλ��һ�����ֿ���SDSc����sdhc
      SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_SD | SDType;	//����ACMD41,����Ӧ	
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_OP_COND;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r3
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
			
			errorstatus=CmdResp3Error(); 					//�ȴ�R3��Ӧ   
			
 			if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
			response=SDIO->RESP1;;			   				//�õ���Ӧ
			validvoltage=(((response>>31)==1)?1:0);			//�ж�SD���ϵ��Ƿ����
			count++;
		}
		if(count>=SD_MAX_VOLT_TRIAL)
		{
			errorstatus=SD_INVALID_VOLTRANGE;
			return errorstatus;
		}	 
		if(response&=SD_HIGH_CAPACITY)
		{
			CardType=SDIO_HIGH_CAPACITY_SD_CARD;
		}
 	}else//MMC��
	{
		//MMC��,����CMD1 SDIO_SEND_OP_COND,����Ϊ:0x80FF8000 
		while((!validvoltage)&&(count<SD_MAX_VOLT_TRIAL))
		{	   										   				   
			SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_MMC;//����CMD1,����Ӧ	   
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_OP_COND;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r3
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
			
			errorstatus=CmdResp3Error(); 					//�ȴ�R3��Ӧ   
			
 			if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
			response=SDIO->RESP1;;			   				//�õ���Ӧ
			validvoltage=(((response>>31)==1)?1:0);
			count++;
		}
		if(count>=SD_MAX_VOLT_TRIAL)
		{
			errorstatus=SD_INVALID_VOLTRANGE;
			return errorstatus;
		}	 			    
		CardType=SDIO_MULTIMEDIA_CARD;	  
  	}  
  	return(errorstatus);		
}
//SD�� Power OFF
//����ֵ:�������;(0,�޴���)
SD_Error SD_PowerOFF(void)
{
 
  SDIO_SetPowerState(SDIO_PowerState_OFF);//SDIO��Դ�ر�,ʱ��ֹͣ	

  return HAL_OK;
}   
//��ʼ�����еĿ�,���ÿ��������״̬
//����ֵ:�������
SD_Error SD_InitializeCards(void)
{
 	SD_Error errorstatus=HAL_OK;
	u16 rca = 0x01;
	
  if (SDIO_GetPowerState() == SDIO_PowerState_OFF)	//����Դ״̬,ȷ��Ϊ�ϵ�״̬
  {
    errorstatus = SD_REQUEST_NOT_APPLICABLE;
    return(errorstatus);
  }

 	if(SDIO_SECURE_DIGITAL_IO_CARD!=CardType)			//��SECURE_DIGITAL_IO_CARD
	{
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;//����CMD2,ȡ��CID,����Ӧ
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);//����CMD2,ȡ��CID,����Ӧ	
		
		errorstatus=CmdResp2Error(); 					//�ȴ�R2��Ӧ 
		
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
		
 		CID_Tab[0]=SDIO->RESP1;
		CID_Tab[1]=SDIO->RESP2;
		CID_Tab[2]=SDIO->RESP3;
		CID_Tab[3]=SDIO->RESP4;
	}
	if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_SECURE_DIGITAL_IO_COMBO_CARD==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))//�жϿ�����
	{
		SDIO_CmdInitStructure.SDIO_Argument = 0x00;//����CMD3,����Ӧ 
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;	//cmd3
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short; //r6
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);	//����CMD3,����Ӧ 
		
		errorstatus=CmdResp6Error(SD_CMD_SET_REL_ADDR,&rca);//�ȴ�R6��Ӧ 
		
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
	}   
    if (SDIO_MULTIMEDIA_CARD==CardType)
    {

		  SDIO_CmdInitStructure.SDIO_Argument = (u32)(rca<<16);//����CMD3,����Ӧ 
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;	//cmd3
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short; //r6
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);	//����CMD3,����Ӧ 	
			
      errorstatus=CmdResp2Error(); 					//�ȴ�R2��Ӧ   
			
		  if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
    }
	if (SDIO_SECURE_DIGITAL_IO_CARD!=CardType)			//��SECURE_DIGITAL_IO_CARD
	{
		RCA = rca;
		
    SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)(rca << 16);//����CMD9+��RCA,ȡ��CSD,����Ӧ 
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_CSD;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);
		
		errorstatus=CmdResp2Error(); 					//�ȴ�R2��Ӧ   
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
  		
		CSD_Tab[0]=SDIO->RESP1;
	  CSD_Tab[1]=SDIO->RESP2;
		CSD_Tab[2]=SDIO->RESP3;						
		CSD_Tab[3]=SDIO->RESP4;					    
	}
	return HAL_OK;//����ʼ���ɹ�
} 
//�õ�����Ϣ
//cardinfo:����Ϣ�洢��
//����ֵ:����״̬
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo)
{
 	SD_Error errorstatus=HAL_OK;
	u8 tmp=0;	   
	cardinfo->CardType=(u8)CardType; 				//������
	cardinfo->RCA=(u16)RCA;							//��RCAֵ
	tmp=(u8)((CSD_Tab[0]&0xFF000000)>>24);
	cardinfo->SD_csd.CSDStruct=(tmp&0xC0)>>6;		//CSD�ṹ
	cardinfo->SD_csd.SysSpecVersion=(tmp&0x3C)>>2;	//2.0Э�黹û�����ⲿ��(Ϊ����),Ӧ���Ǻ���Э�鶨���
	cardinfo->SD_csd.Reserved1=tmp&0x03;			//2������λ  
	tmp=(u8)((CSD_Tab[0]&0x00FF0000)>>16);			//��1���ֽ�
	cardinfo->SD_csd.TAAC=tmp;				   		//���ݶ�ʱ��1
	tmp=(u8)((CSD_Tab[0]&0x0000FF00)>>8);	  		//��2���ֽ�
	cardinfo->SD_csd.NSAC=tmp;		  				//���ݶ�ʱ��2
	tmp=(u8)(CSD_Tab[0]&0x000000FF);				//��3���ֽ�
	cardinfo->SD_csd.MaxBusClkFrec=tmp;		  		//�����ٶ�	   
	tmp=(u8)((CSD_Tab[1]&0xFF000000)>>24);			//��4���ֽ�
	cardinfo->SD_csd.CardComdClasses=tmp<<4;    	//��ָ�������λ
	tmp=(u8)((CSD_Tab[1]&0x00FF0000)>>16);	 		//��5���ֽ�
	cardinfo->SD_csd.CardComdClasses|=(tmp&0xF0)>>4;//��ָ�������λ
	cardinfo->SD_csd.RdBlockLen=tmp&0x0F;	    	//����ȡ���ݳ���
	tmp=(u8)((CSD_Tab[1]&0x0000FF00)>>8);			//��6���ֽ�
	cardinfo->SD_csd.PartBlockRead=(tmp&0x80)>>7;	//����ֿ��
	cardinfo->SD_csd.WrBlockMisalign=(tmp&0x40)>>6;	//д���λ
	cardinfo->SD_csd.RdBlockMisalign=(tmp&0x20)>>5;	//�����λ
	cardinfo->SD_csd.DSRImpl=(tmp&0x10)>>4;
	cardinfo->SD_csd.Reserved2=0; 					//����
 	if((CardType==SDIO_STD_CAPACITY_SD_CARD_V1_1)||(CardType==SDIO_STD_CAPACITY_SD_CARD_V2_0)||(SDIO_MULTIMEDIA_CARD==CardType))//��׼1.1/2.0��/MMC��
	{
		cardinfo->SD_csd.DeviceSize=(tmp&0x03)<<10;	//C_SIZE(12λ)
	 	tmp=(u8)(CSD_Tab[1]&0x000000FF); 			//��7���ֽ�	
		cardinfo->SD_csd.DeviceSize|=(tmp)<<2;
 		tmp=(u8)((CSD_Tab[2]&0xFF000000)>>24);		//��8���ֽ�	
		cardinfo->SD_csd.DeviceSize|=(tmp&0xC0)>>6;
 		cardinfo->SD_csd.MaxRdCurrentVDDMin=(tmp&0x38)>>3;
		cardinfo->SD_csd.MaxRdCurrentVDDMax=(tmp&0x07);
 		tmp=(u8)((CSD_Tab[2]&0x00FF0000)>>16);		//��9���ֽ�	
		cardinfo->SD_csd.MaxWrCurrentVDDMin=(tmp&0xE0)>>5;
		cardinfo->SD_csd.MaxWrCurrentVDDMax=(tmp&0x1C)>>2;
		cardinfo->SD_csd.DeviceSizeMul=(tmp&0x03)<<1;//C_SIZE_MULT
 		tmp=(u8)((CSD_Tab[2]&0x0000FF00)>>8);	  	//��10���ֽ�	
		cardinfo->SD_csd.DeviceSizeMul|=(tmp&0x80)>>7;
 		cardinfo->CardCapacity=(cardinfo->SD_csd.DeviceSize+1);//���㿨����
		cardinfo->CardCapacity*=(1<<(cardinfo->SD_csd.DeviceSizeMul+2));
		cardinfo->CardBlockSize=1<<(cardinfo->SD_csd.RdBlockLen);//���С
		cardinfo->CardCapacity*=cardinfo->CardBlockSize;
	}else if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)	//��������
	{
 		tmp=(u8)(CSD_Tab[1]&0x000000FF); 		//��7���ֽ�	
		cardinfo->SD_csd.DeviceSize=(tmp&0x3F)<<16;//C_SIZE
 		tmp=(u8)((CSD_Tab[2]&0xFF000000)>>24); 	//��8���ֽ�	
 		cardinfo->SD_csd.DeviceSize|=(tmp<<8);
 		tmp=(u8)((CSD_Tab[2]&0x00FF0000)>>16);	//��9���ֽ�	
 		cardinfo->SD_csd.DeviceSize|=(tmp);
 		tmp=(u8)((CSD_Tab[2]&0x0000FF00)>>8); 	//��10���ֽ�	
 		cardinfo->CardCapacity=(long long)(cardinfo->SD_csd.DeviceSize+1)*512*1024;//���㿨����
		cardinfo->CardBlockSize=512; 			//���С�̶�Ϊ512�ֽ�
	}	  
	cardinfo->SD_csd.EraseGrSize=(tmp&0x40)>>6;
	cardinfo->SD_csd.EraseGrMul=(tmp&0x3F)<<1;	   
	tmp=(u8)(CSD_Tab[2]&0x000000FF);			//��11���ֽ�	
	cardinfo->SD_csd.EraseGrMul|=(tmp&0x80)>>7;
	cardinfo->SD_csd.WrProtectGrSize=(tmp&0x7F);
 	tmp=(u8)((CSD_Tab[3]&0xFF000000)>>24);		//��12���ֽ�	
	cardinfo->SD_csd.WrProtectGrEnable=(tmp&0x80)>>7;
	cardinfo->SD_csd.ManDeflECC=(tmp&0x60)>>5;
	cardinfo->SD_csd.WrSpeedFact=(tmp&0x1C)>>2;
	cardinfo->SD_csd.MaxWrBlockLen=(tmp&0x03)<<2;	 
	tmp=(u8)((CSD_Tab[3]&0x00FF0000)>>16);		//��13���ֽ�
	cardinfo->SD_csd.MaxWrBlockLen|=(tmp&0xC0)>>6;
	cardinfo->SD_csd.WriteBlockPaPartial=(tmp&0x20)>>5;
	cardinfo->SD_csd.Reserved3=0;
	cardinfo->SD_csd.ContentProtectAppli=(tmp&0x01);  
	tmp=(u8)((CSD_Tab[3]&0x0000FF00)>>8);		//��14���ֽ�
	cardinfo->SD_csd.FileFormatGrouop=(tmp&0x80)>>7;
	cardinfo->SD_csd.CopyFlag=(tmp&0x40)>>6;
	cardinfo->SD_csd.PermWrProtect=(tmp&0x20)>>5;
	cardinfo->SD_csd.TempWrProtect=(tmp&0x10)>>4;
	cardinfo->SD_csd.FileFormat=(tmp&0x0C)>>2;
	cardinfo->SD_csd.ECC=(tmp&0x03);  
	tmp=(u8)(CSD_Tab[3]&0x000000FF);			//��15���ֽ�
	cardinfo->SD_csd.CSD_CRC=(tmp&0xFE)>>1;
	cardinfo->SD_csd.Reserved4=1;		 
	tmp=(u8)((CID_Tab[0]&0xFF000000)>>24);		//��0���ֽ�
	cardinfo->SD_cid.ManufacturerID=tmp;		    
	tmp=(u8)((CID_Tab[0]&0x00FF0000)>>16);		//��1���ֽ�
	cardinfo->SD_cid.OEM_AppliID=tmp<<8;	  
	tmp=(u8)((CID_Tab[0]&0x000000FF00)>>8);		//��2���ֽ�
	cardinfo->SD_cid.OEM_AppliID|=tmp;	    
	tmp=(u8)(CID_Tab[0]&0x000000FF);			//��3���ֽ�	
	cardinfo->SD_cid.ProdName1=tmp<<24;				  
	tmp=(u8)((CID_Tab[1]&0xFF000000)>>24); 		//��4���ֽ�
	cardinfo->SD_cid.ProdName1|=tmp<<16;	  
	tmp=(u8)((CID_Tab[1]&0x00FF0000)>>16);	   	//��5���ֽ�
	cardinfo->SD_cid.ProdName1|=tmp<<8;		 
	tmp=(u8)((CID_Tab[1]&0x0000FF00)>>8);		//��6���ֽ�
	cardinfo->SD_cid.ProdName1|=tmp;		   
	tmp=(u8)(CID_Tab[1]&0x000000FF);	  		//��7���ֽ�
	cardinfo->SD_cid.ProdName2=tmp;			  
	tmp=(u8)((CID_Tab[2]&0xFF000000)>>24); 		//��8���ֽ�
	cardinfo->SD_cid.ProdRev=tmp;		 
	tmp=(u8)((CID_Tab[2]&0x00FF0000)>>16);		//��9���ֽ�
	cardinfo->SD_cid.ProdSN=tmp<<24;	   
	tmp=(u8)((CID_Tab[2]&0x0000FF00)>>8); 		//��10���ֽ�
	cardinfo->SD_cid.ProdSN|=tmp<<16;	   
	tmp=(u8)(CID_Tab[2]&0x000000FF);   			//��11���ֽ�
	cardinfo->SD_cid.ProdSN|=tmp<<8;		   
	tmp=(u8)((CID_Tab[3]&0xFF000000)>>24); 		//��12���ֽ�
	cardinfo->SD_cid.ProdSN|=tmp;			     
	tmp=(u8)((CID_Tab[3]&0x00FF0000)>>16);	 	//��13���ֽ�
	cardinfo->SD_cid.Reserved1|=(tmp&0xF0)>>4;
	cardinfo->SD_cid.ManufactDate=(tmp&0x0F)<<8;    
	tmp=(u8)((CID_Tab[3]&0x0000FF00)>>8);		//��14���ֽ�
	cardinfo->SD_cid.ManufactDate|=tmp;		 	  
	tmp=(u8)(CID_Tab[3]&0x000000FF);			//��15���ֽ�
	cardinfo->SD_cid.CID_CRC=(tmp&0xFE)>>1;
	cardinfo->SD_cid.Reserved2=1;	 
	return errorstatus;
}
//����SDIO���߿��(MMC����֧��4bitģʽ)
//wmode:λ��ģʽ.0,1λ���ݿ��;1,4λ���ݿ��;2,8λ���ݿ��
//����ֵ:SD������״̬

//����SDIO���߿��(MMC����֧��4bitģʽ)
//   @arg SDIO_BusWide_8b: 8-bit data transfer (Only for MMC)
//   @arg SDIO_BusWide_4b: 4-bit data transfer
//   @arg SDIO_BusWide_1b: 1-bit data transfer (Ĭ��)
//����ֵ:SD������״̬


SD_Error SD_EnableWideBusOperation(u32 WideMode)
{
  	SD_Error errorstatus=HAL_OK;
  if (SDIO_MULTIMEDIA_CARD == CardType)
  {
    errorstatus = SD_UNSUPPORTED_FEATURE;
    return(errorstatus);
  }
	
 	else if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
	{
		 if (SDIO_BusWide_8b == WideMode)   //2.0 sd��֧��8bits
    {
      errorstatus = SD_UNSUPPORTED_FEATURE;
      return(errorstatus);
    }
 		else   
		{
			errorstatus=SDEnWideBus(WideMode);
 			if(HAL_OK==errorstatus)
			{
				SDIO->CLKCR&=~(3<<11);		//���֮ǰ��λ������    
				SDIO->CLKCR|=WideMode;//1λ/4λ���߿�� 
				SDIO->CLKCR|=0<<14;			//������Ӳ��������
			}
		}  
	}
	return errorstatus; 
}
//����SD������ģʽ
//Mode:
//����ֵ:����״̬
SD_Error SD_SetDeviceMode(u32 Mode)
{
	SD_Error errorstatus = HAL_OK;
 	if((Mode==SD_DMA_MODE)||(Mode==SD_POLLING_MODE))DeviceMode=Mode;
	else errorstatus=SD_INVALID_PARAMETER;
	return errorstatus;	    
}
//ѡ��
//����CMD7,ѡ����Ե�ַ(rca)Ϊaddr�Ŀ�,ȡ��������.���Ϊ0,�򶼲�ѡ��.
//addr:����RCA��ַ
SD_Error SD_SelectDeselect(u32 addr)
{

  SDIO_CmdInitStructure.SDIO_Argument =  addr;//����CMD7,ѡ��,����Ӧ	
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);//����CMD7,ѡ��,����Ӧ
	
 	return CmdResp1Error(SD_CMD_SEL_DESEL_CARD);	  
}
//SD����ȡһ���� 
//buf:�����ݻ�����(����4�ֽڶ���!!)
//addr:��ȡ��ַ
//blksize:���С
SD_Error SD_ReadBlock(u8 *buf,long long addr,u16 blksize)
{	  
	SD_Error errorstatus=HAL_OK;
	u8 power;
  u32 count=0,*tempbuff=(u32*)buf;//ת��Ϊu32ָ�� 
	u32 timeout=SDIO_DATATIMEOUT;   
  if(NULL==buf)
		return SD_INVALID_PARAMETER; 
  SDIO->DCTRL=0x0;	//���ݿ��ƼĴ�������(��DMA) 
  
	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)//��������
	{
		blksize=512;
		addr>>=9;
	}   
  	SDIO_DataInitStructure.SDIO_DataBlockSize= SDIO_DataBlockSize_1b ;//���DPSM״̬������
	  SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	  SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	  SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	  SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	  SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
    SDIO_DataConfig(&SDIO_DataInitStructure);
	
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//������
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);	
		
   
		SDIO_CmdInitStructure.SDIO_Argument =  blksize;
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);//����CMD16+�������ݳ���Ϊblksize,����Ӧ
		
		
		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//�ȴ�R1��Ӧ 
		
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
		
	}else return SD_INVALID_PARAMETER;	  	 
	
	  SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4 ;//���DPSM״̬������
	  SDIO_DataInitStructure.SDIO_DataLength= blksize ;
	  SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	  SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	  SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToSDIO;
	  SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
    SDIO_DataConfig(&SDIO_DataInitStructure);
	
	  SDIO_CmdInitStructure.SDIO_Argument =  addr;
    SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand(&SDIO_CmdInitStructure);//����CMD17+��addr��ַ����ȡ����,����Ӧ 
	
	errorstatus=CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);//�ȴ�R1��Ӧ   
	if(errorstatus!=HAL_OK)return errorstatus;   		//��Ӧ����
 	if(DeviceMode==SD_POLLING_MODE)						//��ѯģʽ,��ѯ����	 
	{
 		INTX_DISABLE();//�ر����ж�(POLLINGģʽ,�Ͻ��жϴ��SDIO��д����!!!)
		while(!(SDIO->STA&((1<<5)|(1<<1)|(1<<3)|(1<<10)|(1<<9))))//������/CRC/��ʱ/���(��־)/��ʼλ����
		{
			if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)						//����������,��ʾ���ٴ���8����
			{
				for(count=0;count<8;count++)			//ѭ����ȡ����
				{
					*(tempbuff+count)=SDIO->FIFO;
				}
				tempbuff+=8;	 
				timeout=0X7FFFFF; 	//���������ʱ��
			}else 	//����ʱ
			{
				if(timeout==0) {
					INTX_ENABLE();//�������ж�
					return SD_DATA_TIMEOUT;
				}
				timeout--;
			}
		} 
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//���ݳ�ʱ����
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//������־
	 		INTX_ENABLE();//�������ж�
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//���ݿ�CRC����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//������־
	 		INTX_ENABLE();//�������ж�
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) 	//����fifo�������
		{
	 		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);		//������־
	 		INTX_ENABLE();//�������ж�
			return SD_RX_OVERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//������ʼλ����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//������־
	 		INTX_ENABLE();//�������ж�
			return SD_START_BIT_ERR;		 
		}   
		while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)	//FIFO����,�����ڿ�������
		{
			*tempbuff=SDIO->FIFO;	//ѭ����ȡ����
			tempbuff++;
		}
		INTX_ENABLE();//�������ж�
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	 
	}else if(DeviceMode==SD_DMA_MODE)
	{
 		TransferError=HAL_OK;
		StopCondition=0;			//�����,����Ҫ����ֹͣ����ָ��
		TransferEnd=0;				//�����������λ�����жϷ�����1
		SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9);	//������Ҫ���ж� 
	 	SDIO->DCTRL|=1<<3;		 	//SDIO DMAʹ�� 
 	    SD_DMA_Config((u32*)buf,blksize,DMA_DIR_PeripheralToMemory); 
 		while(((DMA2->LISR&(1<<27))==RESET)&&(TransferEnd==0)&&(TransferError==HAL_OK)&&timeout)timeout--;//�ȴ��������
		if(timeout==0)return SD_DATA_TIMEOUT;//��ʱ
		if(TransferError!=HAL_OK)errorstatus=TransferError;
    }   
 	return errorstatus; 
}
//SD����ȡ����� 
//buf:�����ݻ�����
//addr:��ȡ��ַ
//blksize:���С
//nblks:Ҫ��ȡ�Ŀ���
//����ֵ:����״̬
u32 *tempbuff __attribute__ ((aligned (4)));;
SD_Error SD_ReadMultiBlocks(u8 *buf,long long addr,u16 blksize,u32 nblks)
{
  SD_Error errorstatus=HAL_OK;
	u8 power;
  u32 count=0;
	u32 timeout=SDIO_DATATIMEOUT;  
	tempbuff=(u32*)buf;//ת��Ϊu32ָ��
	
  SDIO->DCTRL=0x0;		//���ݿ��ƼĴ�������(��DMA)   
	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)//��������
	{
		blksize=512;
		addr>>=9;
	}  
	
	  SDIO_DataInitStructure.SDIO_DataBlockSize= 0; ;//���DPSM״̬������
	  SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	  SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	  SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	  SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	  SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
    SDIO_DataConfig(&SDIO_DataInitStructure);
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//������
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);	    
		
	  SDIO_CmdInitStructure.SDIO_Argument =  blksize;//����CMD16+�������ݳ���Ϊblksize,����Ӧ 
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		
		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//�ȴ�R1��Ӧ  
		
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
		
	}else return SD_INVALID_PARAMETER;	  
	
	if(nblks>1)											//����  
	{									    
 	  	if(nblks*blksize>SD_MAX_DATA_LENGTH)return SD_INVALID_PARAMETER;//�ж��Ƿ񳬹������ճ��� 
		
		   SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4; ;//nblks*blksize,512���С,����������
			 SDIO_DataInitStructure.SDIO_DataLength= nblks*blksize ;
			 SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
			 SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
			 SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToSDIO;
			 SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
			 SDIO_DataConfig(&SDIO_DataInitStructure);

       SDIO_CmdInitStructure.SDIO_Argument =  addr;//����CMD18+��addr��ַ����ȡ����,����Ӧ 
	     SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_MULT_BLOCK;
		   SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		   SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		   SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		   SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_READ_MULT_BLOCK);//�ȴ�R1��Ӧ 
		
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
		
 		if(DeviceMode==SD_POLLING_MODE)
		{
			INTX_DISABLE();//�ر����ж�(POLLINGģʽ,�Ͻ��жϴ��SDIO��д����!!!)
			while(!(SDIO->STA&((1<<5)|(1<<1)|(1<<3)|(1<<8)|(1<<9))))//������/CRC/��ʱ/���(��־)/��ʼλ����
			{
				if(SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)						//����������,��ʾ���ٴ���8����
				{
					for(count=0;count<8;count++)			//ѭ����ȡ����
					{
						*(tempbuff+count)=SDIO->FIFO;
					}
					tempbuff+=8;	 
					timeout=0X7FFFFF; 	//���������ʱ��
				}else 	//����ʱ
				{
					if(timeout==0) {
						INTX_ENABLE();//�������ж�
						return SD_DATA_TIMEOUT;
					}
					timeout--;
				}
			}  
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//���ݳ�ʱ����
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//������־
	 		INTX_ENABLE();//�������ж�
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//���ݿ�CRC����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//������־
	 		INTX_ENABLE();//�������ж�
	 		return SD_DATA_CRC_FAIL;
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) 	//����fifo�������
		{
	 		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);		//������־
	 		INTX_ENABLE();//�������ж�
	 		return SD_RX_OVERRUN;
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//������ʼλ����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//������־
	 		INTX_ENABLE();//�������ж�
	 		return SD_START_BIT_ERR;
		}   
	    
		while(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)	//FIFO����,�����ڿ�������
		{
			*tempbuff=SDIO->FIFO;	//ѭ����ȡ����
			tempbuff++;
		}
	 		if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)		//���ս���
			{
				if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
				{				
					SDIO_CmdInitStructure.SDIO_Argument =  0;//����CMD12+��������
				  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
					SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
					SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
					SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
					SDIO_SendCommand(&SDIO_CmdInitStructure);	
					
					errorstatus=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);//�ȴ�R1��Ӧ   
					
					if(errorstatus!=HAL_OK) {
						INTX_ENABLE();//�������ж�
						return errorstatus;
					}
				}
 			}
			INTX_ENABLE();//�������ж�
	 		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
 		}else if(DeviceMode==SD_DMA_MODE)
		{
	   		TransferError=HAL_OK;
			StopCondition=1;			//����,��Ҫ����ֹͣ����ָ�� 
			TransferEnd=0;				//�����������λ�����жϷ�����1
			SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9);	//������Ҫ���ж� 
		 	SDIO->DCTRL|=1<<3;		 						//SDIO DMAʹ�� 
	 	    SD_DMA_Config((u32*)buf,nblks*blksize,DMA_DIR_PeripheralToMemory); 
	 		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//�ȴ�������� 
			if(timeout==0)return SD_DATA_TIMEOUT;//��ʱ
			while((TransferEnd==0)&&(TransferError==HAL_OK));
			if(TransferError!=HAL_OK)errorstatus=TransferError;
		}		 
  	}
	return errorstatus;
}			    																  
//SD��д1���� 
//buf:���ݻ�����
//addr:д��ַ
//blksize:���С	  
//����ֵ:����״̬
SD_Error SD_WriteBlock(u8 *buf,long long addr,  u16 blksize)
{
	SD_Error errorstatus = HAL_OK;
	
	u8  power=0,cardstate=0;
	
	u32 timeout=0,bytestransferred=0;
	
	u32 cardstatus=0,count=0,restwords=0;
	
	u32	tlen=blksize;						//�ܳ���(�ֽ�)
	
	u32*tempbuff=(u32*)buf;					
	
 	if(buf==NULL)return SD_INVALID_PARAMETER;//��������  
	
  SDIO->DCTRL=0x0;							//���ݿ��ƼĴ�������(��DMA)
	
	SDIO_DataInitStructure.SDIO_DataBlockSize= 0; ;//���DPSM״̬������
	SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
  SDIO_DataConfig(&SDIO_DataInitStructure);
	
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//������
 	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)	//��������
	{
		blksize=512;
		addr>>=9;
	}    
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);	
		
		SDIO_CmdInitStructure.SDIO_Argument = blksize;//����CMD16+�������ݳ���Ϊblksize,����Ӧ 	
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//�ȴ�R1��Ӧ  
		
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
		
	}else return SD_INVALID_PARAMETER;	
	
			SDIO_CmdInitStructure.SDIO_Argument = (u32)RCA<<16;//����CMD13,��ѯ����״̬,����Ӧ 	
		  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);	

	  errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);		//�ȴ�R1��Ӧ  
	
	if(errorstatus!=HAL_OK)return errorstatus;
	cardstatus=SDIO->RESP1;													  
	timeout=SD_DATATIMEOUT;
   	while(((cardstatus&0x00000100)==0)&&(timeout>0)) 	//���READY_FOR_DATAλ�Ƿ���λ
	{
		timeout--;  
		
		SDIO_CmdInitStructure.SDIO_Argument = (u32)RCA<<16;//����CMD13,��ѯ����״̬,����Ӧ
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);	//�ȴ�R1��Ӧ   
		
		if(errorstatus!=HAL_OK)return errorstatus;
		
		cardstatus=SDIO->RESP1;													  
	}
	if(timeout==0)return SD_ERROR;

			SDIO_CmdInitStructure.SDIO_Argument = addr;//����CMD24,д����ָ��,����Ӧ 	
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);	
	
	errorstatus=CmdResp1Error(SD_CMD_WRITE_SINGLE_BLOCK);//�ȴ�R1��Ӧ  
	
	if(errorstatus!=HAL_OK)return errorstatus;
	
	StopCondition=0;									//����д,����Ҫ����ֹͣ����ָ�� 

	SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4; ;	//blksize, ����������	
	SDIO_DataInitStructure.SDIO_DataLength= blksize ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
  SDIO_DataConfig(&SDIO_DataInitStructure);
	
	
	timeout=SDIO_DATATIMEOUT;
	
	if (DeviceMode == SD_POLLING_MODE)
	{
		INTX_DISABLE();//�ر����ж�(POLLINGģʽ,�Ͻ��жϴ��SDIO��д����!!!)
		while(!(SDIO->STA&((1<<10)|(1<<4)|(1<<1)|(1<<3)|(1<<9))))//���ݿ鷢�ͳɹ�/����/CRC/��ʱ/��ʼλ����
		{
			if(SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)							//���������,��ʾ���ٴ���8����
			{
				if((tlen-bytestransferred)<SD_HALFFIFOBYTES)//����32�ֽ���
				{
					restwords=((tlen-bytestransferred)%4==0)?((tlen-bytestransferred)/4):((tlen-bytestransferred)/4+1);
					
					for(count=0;count<restwords;count++,tempbuff++,bytestransferred+=4)
					{
						SDIO->FIFO=*tempbuff;
					}
				}else
				{
					for(count=0;count<8;count++)
					{
						SDIO->FIFO=*(tempbuff+count);
					}
					tempbuff+=8;
					bytestransferred+=32;
				}
				timeout=0X3FFFFFFF;	//д�������ʱ��
			}else
			{
				if(timeout==0) {
					INTX_ENABLE();//�������ж�
					return SD_DATA_TIMEOUT;
				}
				timeout--;
			}
		} 
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//���ݳ�ʱ����
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//������־
	 		INTX_ENABLE();//�������ж�
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//���ݿ�CRC����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//������־
	 		INTX_ENABLE();//�������ж�
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET) 	//����fifo�������
		{
	 		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);		//������־
	 		INTX_ENABLE();//�������ж�
			return SD_TX_UNDERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//������ʼλ����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//������־
	 		INTX_ENABLE();//�������ж�
			return SD_START_BIT_ERR;		 
		}   
	      
		INTX_ENABLE();//�������ж�
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��  
	}else if(DeviceMode==SD_DMA_MODE)
	{
   		TransferError=HAL_OK;
		StopCondition=0;			//����д,����Ҫ����ֹͣ����ָ�� 
		TransferEnd=0;				//�����������λ�����жϷ�����1
		SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);	//���ò������ݽ�������ж�
		SD_DMA_Config((u32*)buf,blksize,DMA_DIR_MemoryToPeripheral);				//SDIO DMA����
 	 	SDIO->DCTRL|=1<<3;								//SDIO DMAʹ��.  
 		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//�ȴ�������� 
		if(timeout==0)
		{
  			SD_Init();	 					//���³�ʼ��SD��,���Խ��д������������
			return SD_DATA_TIMEOUT;			//��ʱ	 
 		}
		timeout=SDIO_DATATIMEOUT;
		while((TransferEnd==0)&&(TransferError==HAL_OK)&&timeout)timeout--;
 		if(timeout==0)return SD_DATA_TIMEOUT;			//��ʱ	 
  		if(TransferError!=HAL_OK)return TransferError;
 	}  
 	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
 	errorstatus=IsCardProgramming(&cardstate);
 	while((errorstatus==HAL_OK)&&((cardstate==SD_CARD_PROGRAMMING)||(cardstate==SD_CARD_RECEIVING)))
	{
		errorstatus=IsCardProgramming(&cardstate);
	}   
	return errorstatus;
}
//SD��д����� 
//buf:���ݻ�����
//addr:д��ַ
//blksize:���С
//nblks:Ҫд��Ŀ���
//����ֵ:����״̬												   
SD_Error SD_WriteMultiBlocks(u8 *buf,long long addr,u16 blksize,u32 nblks)
{
	SD_Error errorstatus = HAL_OK;
	u8  power = 0, cardstate = 0;
	u32 timeout=0,bytestransferred=0;
	u32 count = 0, restwords = 0;
	u32 tlen=nblks*blksize;				//�ܳ���(�ֽ�)
	u32 *tempbuff = (u32*)buf;  
  if(buf==NULL)return SD_INVALID_PARAMETER; //��������  
  SDIO->DCTRL=0x0;							//���ݿ��ƼĴ�������(��DMA)   
	
	SDIO_DataInitStructure.SDIO_DataBlockSize= 0; ;	//���DPSM״̬������	
	SDIO_DataInitStructure.SDIO_DataLength= 0 ;
	SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
	SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
	SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
  SDIO_DataConfig(&SDIO_DataInitStructure);
	
	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//������
 	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)//��������
	{
		blksize=512;
		addr>>=9;
	}    
	if((blksize>0)&&(blksize<=2048)&&((blksize&(blksize-1))==0))
	{
		power=convert_from_bytes_to_power_of_two(blksize);
		
		SDIO_CmdInitStructure.SDIO_Argument = blksize;	//����CMD16+�������ݳ���Ϊblksize,����Ӧ
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	
		
		errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);	//�ȴ�R1��Ӧ  
		
		if(errorstatus!=HAL_OK)return errorstatus;   	//��Ӧ����
		
	}else return SD_INVALID_PARAMETER;	 
	if(nblks>1)
	{					  
		if(nblks*blksize>SD_MAX_DATA_LENGTH)return SD_INVALID_PARAMETER;   
     	if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
    	{
			//�������
				SDIO_CmdInitStructure.SDIO_Argument = (u32)RCA<<16;		//����ACMD55,����Ӧ 	
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);	
				
			errorstatus=CmdResp1Error(SD_CMD_APP_CMD);		//�ȴ�R1��Ӧ 
				
			if(errorstatus!=HAL_OK)return errorstatus;
				
				SDIO_CmdInitStructure.SDIO_Argument =nblks;		//����CMD23,���ÿ�����,����Ӧ 	 
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCK_COUNT;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);
			  
				errorstatus=CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);//�ȴ�R1��Ӧ 
				
			if(errorstatus!=HAL_OK)return errorstatus;
		    
		} 

				SDIO_CmdInitStructure.SDIO_Argument =addr;	//����CMD25,���дָ��,����Ӧ 	  
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_MULT_BLOCK;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);	

 		errorstatus=CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);	//�ȴ�R1��Ӧ   		   
	
		if(errorstatus!=HAL_OK) return errorstatus;

        SDIO_DataInitStructure.SDIO_DataBlockSize= power<<4; ;	//blksize, ����������	
				SDIO_DataInitStructure.SDIO_DataLength= nblks*blksize ;
				SDIO_DataInitStructure.SDIO_DataTimeOut=SD_DATATIMEOUT ;
				SDIO_DataInitStructure.SDIO_DPSM=SDIO_DPSM_Enable;
				SDIO_DataInitStructure.SDIO_TransferDir=SDIO_TransferDir_ToCard;
				SDIO_DataInitStructure.SDIO_TransferMode=SDIO_TransferMode_Block;
				SDIO_DataConfig(&SDIO_DataInitStructure);
				
		if(DeviceMode==SD_POLLING_MODE)
	    {
			timeout=SDIO_DATATIMEOUT;
			INTX_DISABLE();//�ر����ж�(POLLINGģʽ,�Ͻ��жϴ��SDIO��д����!!!)
			while(!(SDIO->STA&((1<<4)|(1<<1)|(1<<8)|(1<<3)|(1<<9))))//����/CRC/���ݽ���/��ʱ/��ʼλ����
			{
				if(SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)							//���������,��ʾ���ٴ���8��(32�ֽ�)
				{	  
					if((tlen-bytestransferred)<SD_HALFFIFOBYTES)//����32�ֽ���
					{
						restwords=((tlen-bytestransferred)%4==0)?((tlen-bytestransferred)/4):((tlen-bytestransferred)/4+1);
						for(count=0;count<restwords;count++,tempbuff++,bytestransferred+=4)
						{
							SDIO->FIFO=*tempbuff;
						}
					}else 										//���������,���Է�������8��(32�ֽ�)����
					{
						for(count=0;count<SD_HALFFIFO;count++)
						{
							SDIO->FIFO=*(tempbuff+count);
						}
						tempbuff+=SD_HALFFIFO;
						bytestransferred+=SD_HALFFIFOBYTES;
					}
					timeout=0X3FFFFFFF;	//д�������ʱ��
				}else
				{
					if(timeout==0) {
						INTX_ENABLE();//�������ж�
						return SD_DATA_TIMEOUT;
					}
					timeout--;
				}
			} 
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//���ݳ�ʱ����
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//������־
	 		INTX_ENABLE();//�������ж�
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//���ݿ�CRC����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//������־
	 		INTX_ENABLE();//�������ж�
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET) 	//����fifo�������
		{
	 		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);		//������־
	 		INTX_ENABLE();//�������ж�
			return SD_TX_UNDERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//������ʼλ����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//������־
	 		INTX_ENABLE();//�������ж�
			return SD_START_BIT_ERR;		 
		}   
	      										   
			if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)		//���ͽ���
			{															 
				if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
				{   
					SDIO_CmdInitStructure.SDIO_Argument =0;//����CMD12+�������� 	  
					SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
					SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
					SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
					SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
					SDIO_SendCommand(&SDIO_CmdInitStructure);	
					
					errorstatus=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);//�ȴ�R1��Ӧ   
					if(errorstatus!=HAL_OK) {
						INTX_ENABLE();//�������ж�
						return errorstatus;
					}
				}
			}
			INTX_ENABLE();//�������ж�
	 		SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	    }else if(DeviceMode==SD_DMA_MODE)
		{
	   	TransferError=HAL_OK;
			StopCondition=1;			//���д,��Ҫ����ֹͣ����ָ�� 
			TransferEnd=0;				//�����������λ�����жϷ�����1
			SDIO->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);	//���ò������ݽ�������ж�
			SD_DMA_Config((u32*)buf,nblks*blksize,DMA_DIR_MemoryToPeripheral);		//SDIO DMA����
	 	 	SDIO->DCTRL|=1<<3;								//SDIO DMAʹ��. 
			timeout=SDIO_DATATIMEOUT;
	 		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//�ȴ�������� 
			if(timeout==0)	 								//��ʱ
			{									  
  				SD_Init();	 					//���³�ʼ��SD��,���Խ��д������������
	 			return SD_DATA_TIMEOUT;			//��ʱ	 
	 		}
			timeout=SDIO_DATATIMEOUT;
			while((TransferEnd==0)&&(TransferError==HAL_OK)&&timeout)timeout--;
	 		if(timeout==0)return SD_DATA_TIMEOUT;			//��ʱ	 
	 		if(TransferError!=HAL_OK)return TransferError;
		}
  	}
 	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
 	errorstatus=IsCardProgramming(&cardstate);
 	while((errorstatus==HAL_OK)&&((cardstate==SD_CARD_PROGRAMMING)||(cardstate==SD_CARD_RECEIVING)))
	{
		errorstatus=IsCardProgramming(&cardstate);
	}   
	return errorstatus;	   
}
//SDIO�жϷ�����		  
void SDIO_IRQHandler(void) 
{											
 	SD_ProcessIRQSrc();//��������SDIO����ж�
}	 																    
//SDIO�жϴ�����
//����SDIO��������еĸ����ж�����
//����ֵ:�������
SD_Error SD_ProcessIRQSrc(void)
{
	if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)//��������ж�
	{	 
		if (StopCondition==1)
		{  
				SDIO_CmdInitStructure.SDIO_Argument =0;//����CMD12+�������� 	  
				SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
				SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
				SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
				SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
				SDIO_SendCommand(&SDIO_CmdInitStructure);	
					
			TransferError=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
		}else TransferError = HAL_OK;
 		SDIO->ICR|=1<<8;//�������жϱ��
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//�ر�����ж�
 		TransferEnd = 1;
		return(TransferError);
	}
 	if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)//����CRC����
	{
		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//������־
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//�ر�����ж�
	    TransferError = SD_DATA_CRC_FAIL;
	    return(SD_DATA_CRC_FAIL);
	}
 	if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)//���ݳ�ʱ����
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);  			//���жϱ�־
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//�ر�����ж�
	    TransferError = SD_DATA_TIMEOUT;
	    return(SD_DATA_TIMEOUT);
	}
  	if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)//FIFO�������
	{
		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);  			//���жϱ�־
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//�ر�����ж�
	    TransferError = SD_RX_OVERRUN;
	    return(SD_RX_OVERRUN);
	}
   	if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)//FIFO�������
	{
		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);  			//���жϱ�־
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//�ر�����ж�
	    TransferError = SD_TX_UNDERRUN;
	    return(SD_TX_UNDERRUN);
	}
	if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)//��ʼλ����
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);  		//���жϱ�־
		SDIO->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//�ر�����ж�
	    TransferError = SD_START_BIT_ERR;
	    return(SD_START_BIT_ERR);
	}
	return(HAL_OK);
}
  
//���CMD0��ִ��״̬
//����ֵ:sd��������
SD_Error CmdError(void)
{
	SD_Error errorstatus = HAL_OK;
	u32 timeout=SDIO_CMD0TIMEOUT;	   
	while(timeout--)
	{
		if(SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) != RESET)break;	//�����ѷ���(������Ӧ)	 
	}	    
	if(timeout==0)return SD_CMD_RSP_TIMEOUT;  
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	return errorstatus;
}	 
//���R7��Ӧ�Ĵ���״̬
//����ֵ:sd��������
SD_Error CmdResp7Error(void)
{
	SD_Error errorstatus=HAL_OK;
	u32 status;
	u32 timeout=SDIO_CMD0TIMEOUT;
 	while(timeout--)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�)	
	}
 	if((timeout==0)||(status&(1<<2)))	//��Ӧ��ʱ
	{																				    
		errorstatus=SD_CMD_RSP_TIMEOUT;	//��ǰ������2.0���ݿ�,���߲�֧���趨�ĵ�ѹ��Χ
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT); 			//���������Ӧ��ʱ��־
		return errorstatus;
	}	 
	if(status&1<<6)						//�ɹ����յ���Ӧ
	{								   
		errorstatus=HAL_OK;
		SDIO_ClearFlag(SDIO_FLAG_CMDREND); 				//�����Ӧ��־
 	}
	return errorstatus;
}	   
//���R1��Ӧ�Ĵ���״̬
//cmd:��ǰ����
//����ֵ:sd��������
SD_Error CmdResp1Error(u8 cmd)
{	  
   	u32 status; 
	while(1)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�)
	} 
	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)					//��Ӧ��ʱ
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT); 				//���������Ӧ��ʱ��־
		return SD_CMD_RSP_TIMEOUT;
	}	
 	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)					//CRC����
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL); 				//�����־
		return SD_CMD_CRC_FAIL;
	}		
	if(SDIO->RESPCMD!=cmd)return SD_ILLEGAL_CMD;//���ƥ�� 
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	return (SD_Error)(SDIO->RESP1&SD_OCR_ERRORBITS);//���ؿ���Ӧ
}
//���R3��Ӧ�Ĵ���״̬
//����ֵ:����״̬
SD_Error CmdResp3Error(void)
{
	u32 status;						 
 	while(1)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�)	
	}
 	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)					//��Ӧ��ʱ
	{											 
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);			//���������Ӧ��ʱ��־
		return SD_CMD_RSP_TIMEOUT;
	}	 
   SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
 	return HAL_OK;
}
//���R2��Ӧ�Ĵ���״̬
//����ֵ:����״̬
SD_Error CmdResp2Error(void)
{
	SD_Error errorstatus=HAL_OK;
	u32 status;
	u32 timeout=SDIO_CMD0TIMEOUT;
 	while(timeout--)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�)	
	}
  	if((timeout==0)||(status&(1<<2)))	//��Ӧ��ʱ
	{																				    
		errorstatus=SD_CMD_RSP_TIMEOUT; 
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT); 		//���������Ӧ��ʱ��־
		return errorstatus;
	}	 
	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)						//CRC����
	{								   
		errorstatus=SD_CMD_CRC_FAIL;
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);		//�����Ӧ��־
 	}
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
 	return errorstatus;								    		 
} 
//���R6��Ӧ�Ĵ���״̬
//cmd:֮ǰ���͵�����
//prca:�����ص�RCA��ַ
//����ֵ:����״̬
SD_Error CmdResp6Error(u8 cmd,u16*prca)
{
	SD_Error errorstatus=HAL_OK;
	u32 status;					    
	u32 rspr1;
 	while(1)
	{
		status=SDIO->STA;
		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC����/������Ӧ��ʱ/�Ѿ��յ���Ӧ(CRCУ��ɹ�)	
	}
	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)					//��Ӧ��ʱ
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);			//���������Ӧ��ʱ��־
		return SD_CMD_RSP_TIMEOUT;
	}	 	 
	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)						//CRC����
	{								   
		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);					//�����Ӧ��־
 		return SD_CMD_CRC_FAIL;
	}
	if(SDIO->RESPCMD!=cmd)				//�ж��Ƿ���Ӧcmd����
	{
 		return SD_ILLEGAL_CMD; 		
	}	    
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	rspr1=SDIO->RESP1;					//�õ���Ӧ 	 
	if(SD_ALLZERO==(rspr1&(SD_R6_GENERAL_UNKNOWN_ERROR|SD_R6_ILLEGAL_CMD|SD_R6_COM_CRC_FAILED)))
	{
		*prca=(u16)(rspr1>>16);			//����16λ�õ�,rca
		return errorstatus;
	}
   	if(rspr1&SD_R6_GENERAL_UNKNOWN_ERROR)return SD_GENERAL_UNKNOWN_ERROR;
   	if(rspr1&SD_R6_ILLEGAL_CMD)return SD_ILLEGAL_CMD;
   	if(rspr1&SD_R6_COM_CRC_FAILED)return SD_COM_CRC_FAILED;
	return errorstatus;
}

//SDIOʹ�ܿ�����ģʽ
//enx:0,��ʹ��;1,ʹ��;
//����ֵ:����״̬
SD_Error SDEnWideBus(u8 enx)
{
	SD_Error errorstatus = HAL_OK;
 	u32 scr[2]={0,0};
	u8 arg=0X00;
	if(enx)arg=0X02;
	else arg=0X00;
 	if(SDIO->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//SD������LOCKED״̬		    
 	errorstatus=FindSCR(RCA,scr);						//�õ�SCR�Ĵ�������
 	if(errorstatus!=HAL_OK)return errorstatus;
	if((scr[1]&SD_WIDE_BUS_SUPPORT)!=SD_ALLZERO)		//֧�ֿ�����
	{
		  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16;//����CMD55+RCA,����Ӧ	
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
		
	 	errorstatus=CmdResp1Error(SD_CMD_APP_CMD);
		
	 	if(errorstatus!=HAL_OK)return errorstatus;
		
		  SDIO_CmdInitStructure.SDIO_Argument = arg;//����ACMD6,����Ӧ,����:10,4λ;00,1λ.	
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
			
     errorstatus=CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
		
		return errorstatus;
	}else return SD_REQUEST_NOT_APPLICABLE;				//��֧�ֿ��������� 	 
}												   
//��鿨�Ƿ�����ִ��д����
//pstatus:��ǰ״̬.
//����ֵ:�������
SD_Error IsCardProgramming(u8 *pstatus)
{
 	vu32 respR1 = 0, status = 0;  
  
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16; //����Ե�ַ����
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;//����CMD13 	
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);	
 	
	status=SDIO->STA;
	
	while(!(status&((1<<0)|(1<<6)|(1<<2))))status=SDIO->STA;//�ȴ��������
   	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)			//CRC���ʧ��
	{  
	  SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);	//���������
		return SD_CMD_CRC_FAIL;
	}
   	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)			//���ʱ 
	{
		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);			//���������
		return SD_CMD_RSP_TIMEOUT;
	}
 	if(SDIO->RESPCMD!=SD_CMD_SEND_STATUS)return SD_ILLEGAL_CMD;
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	respR1=SDIO->RESP1;
	*pstatus=(u8)((respR1>>9)&0x0000000F);
	return HAL_OK;
}
//��ȡ��ǰ��״̬
//pcardstatus:��״̬
//����ֵ:�������
SD_Error SD_SendStatus(uint32_t *pcardstatus)
{
	SD_Error errorstatus = HAL_OK;
	if(pcardstatus==NULL)
	{
		errorstatus=SD_INVALID_PARAMETER;
		return errorstatus;
	}
	
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16;//����CMD13,����Ӧ		 
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);	
	
	errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);	//��ѯ��Ӧ״̬ 
	if(errorstatus!=HAL_OK)return errorstatus;
	*pcardstatus=SDIO->RESP1;//��ȡ��Ӧֵ
	return errorstatus;
} 
//����SD����״̬
//����ֵ:SD��״̬
SDCardState SD_GetState(void)
{
	u32 resp1=0;
	if(SD_SendStatus(&resp1)!=HAL_OK)return SD_CARD_ERROR;
	else return (SDCardState)((resp1>>9) & 0x0F);
}
//����SD����SCR�Ĵ���ֵ
//rca:����Ե�ַ
//pscr:���ݻ�����(�洢SCR����)
//����ֵ:����״̬		   
SD_Error FindSCR(u16 rca,u32 *pscr)
{ 
	u32 index = 0; 
	SD_Error errorstatus = HAL_OK;
	u32 tempscr[2]={0,0};  
	
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)8;	 //����CMD16,����Ӧ,����Block SizeΪ8�ֽ�	
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN; //	 cmd16
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r1
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
	
 	errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	
 	if(errorstatus!=HAL_OK)return errorstatus;
	
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) RCA << 16; 
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;//����CMD55,����Ӧ 	
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
	
 	errorstatus=CmdResp1Error(SD_CMD_APP_CMD);
 	if(errorstatus!=HAL_OK)return errorstatus;
	
  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = 8;  //8���ֽڳ���,blockΪ8�ֽ�,SD����SDIO.
  SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_8b  ;  //���С8byte 
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);		

  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_SEND_SCR;	//����ACMD51,����Ӧ,����Ϊ0	
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;  //r1
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
	
 	errorstatus=CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);
 	if(errorstatus!=HAL_OK)return errorstatus;
 	while(!(SDIO->STA&(SDIO_FLAG_RXOVERR|SDIO_FLAG_DCRCFAIL|SDIO_FLAG_DTIMEOUT|SDIO_FLAG_DBCKEND|SDIO_FLAG_STBITERR)))
	{ 
		if(SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)//����FIFO���ݿ���
		{
			*(tempscr+index)=SDIO->FIFO;	//��ȡFIFO����
			index++;
			if(index>=2)break;
		}
	}
		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)		//���ݳ�ʱ����
		{										   
	 		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT); 	//������־
			return SD_DATA_TIMEOUT;
	 	}else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)	//���ݿ�CRC����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);  		//������־
			return SD_DATA_CRC_FAIL;		   
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET) 	//����fifo�������
		{
	 		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);		//������־
			return SD_RX_OVERRUN;		 
		}else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET) 	//������ʼλ����
		{
	 		SDIO_ClearFlag(SDIO_FLAG_STBITERR);//������־
			return SD_START_BIT_ERR;		 
		}  
   SDIO_ClearFlag(SDIO_STATIC_FLAGS);//������б��
	//������˳��8λΪ��λ������.   	
	*(pscr+1)=((tempscr[0]&SD_0TO7BITS)<<24)|((tempscr[0]&SD_8TO15BITS)<<8)|((tempscr[0]&SD_16TO23BITS)>>8)|((tempscr[0]&SD_24TO31BITS)>>24);
	*(pscr)=((tempscr[1]&SD_0TO7BITS)<<24)|((tempscr[1]&SD_8TO15BITS)<<8)|((tempscr[1]&SD_16TO23BITS)>>8)|((tempscr[1]&SD_24TO31BITS)>>24);
 	return errorstatus;
}
//�õ�NumberOfBytes��2Ϊ�׵�ָ��.
//NumberOfBytes:�ֽ���.
//����ֵ:��2Ϊ�׵�ָ��ֵ
u8 convert_from_bytes_to_power_of_two(u16 NumberOfBytes)
{
	u8 count=0;
	while(NumberOfBytes!=1)
	{
		NumberOfBytes>>=1;
		count++;
	}
	return count;
} 	 

//����SDIO DMA  
//mbuf:�洢����ַ
//bufsize:����������
//dir:����;DMA_DIR_MemoryToPeripheral  �洢��-->SDIO(д����);DMA_DIR_PeripheralToMemory SDIO-->�洢��(������);
void SD_DMA_Config(u32*mbuf,u32 bufsize,u32 dir)
{		 

  DMA_InitTypeDef  DMA_InitStructure;
	
	while (DMA_GetCmdStatus(DMA2_Stream3) != DISABLE){}//�ȴ�DMA������ 
		
  DMA_DeInit(DMA2_Stream3);//���֮ǰ��stream3�ϵ������жϱ�־
	
 
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;  //ͨ��ѡ��
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SDIO->FIFO;//DMA�����ַ
  DMA_InitStructure.DMA_Memory0BaseAddr = (u32)mbuf;//DMA �洢��0��ַ
  DMA_InitStructure.DMA_DIR = dir;//�洢��������ģʽ
  DMA_InitStructure.DMA_BufferSize = 0;//���ݴ����� 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//���������ģʽ
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//�洢������ģʽ
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;//�������ݳ���:32λ
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;//�洢�����ݳ���:32λ
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;// ʹ����ͨģʽ 
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;//������ȼ�
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;   //FIFOʹ��      
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;//ȫFIFO
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;//����ͻ��4�δ���
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;//�洢��ͻ��4�δ���
  DMA_Init(DMA2_Stream3, &DMA_InitStructure);//��ʼ��DMA Stream

	DMA_FlowControllerConfig(DMA2_Stream3,DMA_FlowCtrl_Peripheral);//���������� 
	 
  DMA_Cmd(DMA2_Stream3 ,ENABLE);//����DMA����	 

}   


//��SD��
//buf:�����ݻ�����
//sector:������ַ
//cnt:��������	
//����ֵ:����״̬;0,����;����,�������;				  				 
u8 SD_ReadDisk(u8*buf,u32 sector,u8 cnt)
{
	u8 sta=HAL_OK;
	long long lsector=sector;
	u8 n;
	lsector<<=9;
	if((u32)buf%4!=0)
	{
	 	for(n=0;n<cnt;n++)
		{
		 	sta=SD_ReadBlock(SDIO_DATA_BUFFER,lsector+512*n,512);//����sector�Ķ�����
			memcpy(buf,SDIO_DATA_BUFFER,512);
			buf+=512;
		} 
	}else
	{
		if(cnt==1)sta=SD_ReadBlock(buf,lsector,512);    	//����sector�Ķ�����
		else sta=SD_ReadMultiBlocks(buf,lsector,512,cnt);//���sector  
	}
	return sta;
}
//дSD��
//buf:д���ݻ�����
//sector:������ַ
//cnt:��������	
//����ֵ:����״̬;0,����;����,�������;	
u8 SD_WriteDisk(u8*buf,u32 sector,u8 cnt)
{
	u8 sta=HAL_OK;
	u8 n;
	long long lsector=sector;
	lsector<<=9;
	if((u32)buf%4!=0)
	{
	 	for(n=0;n<cnt;n++)
		{
			memcpy(SDIO_DATA_BUFFER,buf,512);
		 	sta=SD_WriteBlock(SDIO_DATA_BUFFER,lsector+512*n,512);//����sector��д����
			buf+=512;
		} 
	}else
	{
		if(cnt==1)sta=SD_WriteBlock(buf,lsector,512);    	//����sector��д����
		else sta=SD_WriteMultiBlocks(buf,lsector,512,cnt);	//���sector  
	}
	return sta;
}
#else
#include "vos.h"
#include "stm32f4xx_hal.h"
#include "sdio_sdcard.h"

s32 flag_read_done = 0;
s32 flag_write_done = 0;
SD_HandleTypeDef        SDCARD_Handler;     //SD�����
HAL_SD_CardInfoTypeDef  SDCardInfo;         //SD����Ϣ�ṹ��
DMA_HandleTypeDef SDTxDMAHandler,SDRxDMAHandler;    //SD��DMA���ͺͽ��վ��

//SD_ReadDisk/SD_WriteDisk����ר��buf,�����������������ݻ�������ַ����4�ֽڶ����ʱ��,
//��Ҫ�õ�������,ȷ�����ݻ�������ַ��4�ֽڶ����.
u8 SDIO_DATA_BUFFER[512] __attribute__ ((aligned (4)));
//SD����ʼ��
//����ֵ:0 ��ʼ����ȷ������ֵ����ʼ������
u8 SD_Init(void)
{
    u8 SD_Error;

    //��ʼ��ʱ��ʱ�Ӳ��ܴ���400KHZ
    SDCARD_Handler.Instance=SDIO;
    SDCARD_Handler.Init.ClockEdge=SDIO_CLOCK_EDGE_RISING;          //������
    SDCARD_Handler.Init.ClockBypass=SDIO_CLOCK_BYPASS_DISABLE;     //��ʹ��bypassģʽ��ֱ����HCLK���з�Ƶ�õ�SDIO_CK
    SDCARD_Handler.Init.ClockPowerSave=SDIO_CLOCK_POWER_SAVE_DISABLE;    //����ʱ���ر�ʱ�ӵ�Դ
    SDCARD_Handler.Init.BusWide=SDIO_BUS_WIDE_1B;                        //1λ������
    SDCARD_Handler.Init.HardwareFlowControl=SDIO_HARDWARE_FLOW_CONTROL_DISABLE;//�ر�Ӳ������
    SDCARD_Handler.Init.ClockDiv=SDIO_TRANSFER_CLK_DIV;            //SD����ʱ��Ƶ�����25MHZ

    SD_Error=HAL_SD_Init(&SDCARD_Handler/*,&SDCardInfo*/);
    if(SD_Error!=HAL_OK) return 1;

    if (HAL_OK != HAL_SD_GetCardInfo(&SDCARD_Handler, &SDCardInfo)) {
    	return 2;
    }

    SD_Error=HAL_SD_ConfigWideBusOperation(&SDCARD_Handler,SDIO_BUS_WIDE_4B);//ʹ�ܿ�����ģʽ
    if(SD_Error!=HAL_OK) return 3;
    return 0;
}

//SDMMC�ײ�������ʱ��ʹ�ܣ��������ã�DMA����
//�˺����ᱻHAL_SD_Init()����
//hsd:SD�����
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    DMA_HandleTypeDef TxDMAHandler,RxDMAHandler;
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_SDIO_CLK_ENABLE();    //ʹ��SDIOʱ��
    __HAL_RCC_DMA2_CLK_ENABLE();    //ʹ��DMA2ʱ��
    __HAL_RCC_GPIOC_CLK_ENABLE();   //ʹ��GPIOCʱ��
    __HAL_RCC_GPIOD_CLK_ENABLE();   //ʹ��GPIODʱ��

    //PC8,9,10,11,12
    GPIO_Initure.Pin=GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;      //���츴��
    GPIO_Initure.Pull=GPIO_PULLUP;          //����
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //����
    GPIO_Initure.Alternate=GPIO_AF12_SDIO;  //����ΪSDIO
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);     //��ʼ��

    //PD2
    GPIO_Initure.Pin=GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);     //��ʼ��

#if (SD_DMA_MODE==1)                        //ʹ��DMAģʽ
    HAL_NVIC_SetPriority(SDMMC1_IRQn,2,0);  //����SDMMC1�жϣ���ռ���ȼ�2�������ȼ�0
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);        //ʹ��SDMMC1�ж�

    //���÷���DMA
    SDRxDMAHandler.Instance=DMA2_Stream3;
    SDRxDMAHandler.Init.Channel=DMA_CHANNEL_4;
    SDRxDMAHandler.Init.Direction=DMA_PERIPH_TO_MEMORY;
    SDRxDMAHandler.Init.PeriphInc=DMA_PINC_DISABLE;
    SDRxDMAHandler.Init.MemInc=DMA_MINC_ENABLE;
    SDRxDMAHandler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;
    SDRxDMAHandler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;
    SDRxDMAHandler.Init.Mode=DMA_PFCTRL;
    SDRxDMAHandler.Init.Priority=DMA_PRIORITY_VERY_HIGH;
    SDRxDMAHandler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;
    SDRxDMAHandler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL;
    SDRxDMAHandler.Init.MemBurst=DMA_MBURST_INC4;
    SDRxDMAHandler.Init.PeriphBurst=DMA_PBURST_INC4;

    __HAL_LINKDMA(hsd, hdmarx, SDRxDMAHandler); //������DMA��SD���ķ���DMA��������
    HAL_DMA_DeInit(&SDRxDMAHandler);
    HAL_DMA_Init(&SDRxDMAHandler);              //��ʼ������DMA

    //���ý���DMA
    SDTxDMAHandler.Instance=DMA2_Stream6;
    SDTxDMAHandler.Init.Channel=DMA_CHANNEL_4;
    SDTxDMAHandler.Init.Direction=DMA_MEMORY_TO_PERIPH;
    SDTxDMAHandler.Init.PeriphInc=DMA_PINC_DISABLE;
    SDTxDMAHandler.Init.MemInc=DMA_MINC_ENABLE;
    SDTxDMAHandler.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;
    SDTxDMAHandler.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;
    SDTxDMAHandler.Init.Mode=DMA_PFCTRL;
    SDTxDMAHandler.Init.Priority=DMA_PRIORITY_VERY_HIGH;
    SDTxDMAHandler.Init.FIFOMode=DMA_FIFOMODE_ENABLE;
    SDTxDMAHandler.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_FULL;
    SDTxDMAHandler.Init.MemBurst=DMA_MBURST_INC4;
    SDTxDMAHandler.Init.PeriphBurst=DMA_PBURST_INC4;

    __HAL_LINKDMA(hsd, hdmatx, SDTxDMAHandler);//������DMA��SD���ķ���DMA��������
    HAL_DMA_DeInit(&SDTxDMAHandler);
    HAL_DMA_Init(&SDTxDMAHandler);              //��ʼ������DMA


    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 3, 0);  //����DMA�ж����ȼ�
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 3, 0);  //����DMA�ж����ȼ�
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
#endif
}

//�õ�����Ϣ
//cardinfo:����Ϣ�洢��
//����ֵ:����״̬

u8 SD_GetCardInfo(HAL_SD_CardInfoTypeDef *cardinfo)
{
    u8 sta;
    sta=HAL_SD_Get_CardInfo(&SDCARD_Handler,cardinfo);
    return sta;
}
#if (SD_DMA_MODE==1)        //DMAģʽ

//ͨ��DMA��ȡSD��һ������
//buf:�����ݻ�����
//sector:������ַ
//blocksize:������С(һ�㶼��512�ֽ�)
//cnt:��������
//����ֵ:����״̬;0,����;����,�������;
u8 SD_ReadBlocks_DMA(uint32_t *buf,uint64_t sector,uint32_t blocksize,uint32_t cnt)
{
    u8 err=0;
    flag_read_done = 0;
    err=HAL_SD_ReadBlocks_DMA(&SDCARD_Handler,buf,/*sector,*/blocksize,cnt);//ͨ��DMA��ȡSD��һ������
    if(err==0)//��ȡ�ɹ�
    {
        //�ȴ���ȡ���
        //err=HAL_SD_CheckReadOperation(&SDCARD_Handler,(uint32_t)SD_TIMEOUT);
    }

    return err;
}

//дSD��
//buf:д���ݻ�����
//sector:������ַ
//blocksize:������С(һ�㶼��512�ֽ�)
//cnt:��������
//����ֵ:����״̬;0,����;����,�������;
u8 SD_WriteBlocks_DMA(uint32_t *buf,uint64_t sector,uint32_t blocksize,uint32_t cnt)
{
    u8 err=0;
    flag_write_done = 0;
    err=HAL_SD_WriteBlocks_DMA(&SDCARD_Handler,buf,/*sector,*/blocksize,cnt);//ͨ��DMAдSD��һ������
    if(err==0)//д�ɹ�
    {
    	//return MSD_ERROR
       //err=HAL_SD_CheckWriteOperation(&SDCARD_Handler,(uint32_t)SD_TIMEOUT);//�ȴ���ȡ���/
    }
    return err;
}

//��SD��
//buf:�����ݻ�����
//sector:������ַ
//cnt:��������
//����ֵ:����״̬;0,����;����,�������;

u8 SD_ReadDisk(u8* buf,u32 sector,u32 cnt)
{
    u8 sta=HAL_OK;
    long long lsector=sector;
    u8 n;
    if(SDCardInfo.CardVersion!=CARD_V1_X)lsector<<=9;
    if((u32)buf%4!=0)
    {
        for(n=0;n<cnt;n++)
        {
            sta=SD_ReadBlocks_DMA((uint32_t*)SDIO_DATA_BUFFER,lsector+512*n,512,1);
            memcpy(buf,SDIO_DATA_BUFFER,512);
            buf+=512;
        }
    }else
    {
        sta=SD_ReadBlocks_DMA((uint32_t*)buf,lsector, 512,cnt);
    }
    return sta;
}

//дSD��
//buf:д���ݻ�����
//sector:������ַ
//cnt:��������
//����ֵ:����״̬;0,����;����,�������;
u8 SD_WriteDisk(u8 *buf,u32 sector,u32 cnt)
{
    u8 sta=HAL_OK;
    long long lsector=sector;
    u8 n;
    if(SDCardInfo.CardVersion!=CARD_V1_X)lsector<<=9;
    if((u32)buf%4!=0)
    {
        for(n=0;n<cnt;n++)
        {
            memcpy(SDIO_DATA_BUFFER,buf,512);
            sta=SD_WriteBlocks_DMA((uint32_t*)SDIO_DATA_BUFFER,lsector+512*n,512,1);//����sector��д����
            buf+=512;
        }
    }else
    {
        sta=SD_WriteBlocks_DMA((uint32_t*)buf,lsector,512,cnt);//���sector��д����
    }
    return sta;
}

//SDMMC1�жϷ�����
void SDMMC1_IRQHandler(void)
{
    HAL_SD_IRQHandler(&SDCARD_Handler);
}

//DMA2������6�жϷ�����
void DMA2_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(SDCARD_Handler.hdmatx);
}

//DMA2������3�жϷ�����
void DMA2_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(SDCARD_Handler.hdmarx);
}
#else                                   //��ѵģʽ

//��SD��
//buf:�����ݻ�����
//sector:������ַ
//cnt:��������
//����ֵ:����״̬;0,����;����,�������;
u8 SD_ReadDisk(u8* buf,u32 sector,u32 cnt)
{
    u8 sta=HAL_OK;
    long long lsector=sector;
    u8 n;
    lsector<<=9;
    INTX_DISABLE();//�ر����ж�(POLLINGģʽ,�Ͻ��жϴ��SDIO��д����!!!)
    if((u32)buf%4!=0)
    {
        for(n=0;n<cnt;n++)
        {
            sta=HAL_SD_ReadBlocks(&SDCARD_Handler,(uint32_t*)SDIO_DATA_BUFFER,lsector+512*n,512,1);//����sector�Ķ�����
            memcpy(buf,SDIO_DATA_BUFFER,512);
            buf+=512;
        }
    }else
    {
        sta=HAL_SD_ReadBlocks(&SDCARD_Handler,(uint32_t*)buf,lsector,512,cnt);//����sector�Ķ�����
    }
    INTX_ENABLE();//�������ж�
    return sta;
}

//дSD��
//buf:д���ݻ�����
//sector:������ַ
//cnt:��������
//����ֵ:����״̬;0,����;����,�������;
u8 SD_WriteDisk(u8 *buf,u32 sector,u32 cnt)
{
    u8 sta=HAL_OK;
    long long lsector=sector;
    u8 n;
    lsector<<=9;
    INTX_DISABLE();//�ر����ж�(POLLINGģʽ,�Ͻ��жϴ��SDIO��д����!!!)
    if((u32)buf%4!=0)
    {
        for(n=0;n<cnt;n++)
        {
            memcpy(SDIO_DATA_BUFFER,buf,512);
            sta=HAL_SD_WriteBlocks(&SDCARD_Handler,(uint32_t*)SDIO_DATA_BUFFER,lsector+512*n,512,1);//����sector��д����
            buf+=512;
        }
    }else
    {
        sta=HAL_SD_WriteBlocks(&SDCARD_Handler,(uint32_t*)buf,lsector,512,cnt);//���sector��д����
    }
	INTX_ENABLE();//�������ж�
    return sta;
}
#endif

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  /* Prevent unused argument(s) compilation warning */
	flag_write_done = 1;

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SD_TxCpltCallback can be implemented in the user file
   */
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsd Pointer SD handle
  * @retval None
  */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  /* Prevent unused argument(s) compilation warning */
	flag_read_done = 1;

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SD_RxCpltCallback can be implemented in the user file
   */
}

/**
  * @brief SD error callbacks
  * @param hsd Pointer SD handle
  * @retval None
  */
void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsd);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SD_ErrorCallback can be implemented in the user file
   */
}

/**
  * @brief SD Abort callbacks
  * @param hsd Pointer SD handle
  * @retval None
  */
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hsd);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_SD_ErrorCallback can be implemented in the user file
   */
}


#endif





#endif
