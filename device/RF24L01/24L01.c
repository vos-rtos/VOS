#include "24l01.h"
#include "spi.h"
#include "stm32f4xx_hal.h"

const u8 TX_ADDRESS[TX_ADR_WIDTH]={0x34,0x43,0x10,0x10,0x01}; //���͵�ַ
const u8 RX_ADDRESS[RX_ADR_WIDTH]={0x34,0x43,0x10,0x10,0x01}; //���͵�ַ

//��ʼ��24L01��IO��
void NRF24L01_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOB_CLK_ENABLE();			//����GPIOBʱ��
    __HAL_RCC_GPIOG_CLK_ENABLE();			//����GPIOGʱ��

	//GPIOB14��ʼ������:�������
    GPIO_Initure.Pin = GPIO_PIN_14; 			//PB14
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;  //�������
    GPIO_Initure.Pull = GPIO_PULLUP;          //����
    GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //����
    HAL_GPIO_Init(GPIOB, &GPIO_Initure);     //��ʼ��

	//GPIOG6,7�������
    GPIO_Initure.Pin = GPIO_PIN_6|GPIO_PIN_7;	//PG6,7
    HAL_GPIO_Init(GPIOG, &GPIO_Initure);     //��ʼ��

	//GPIOG.8��������
	GPIO_Initure.Pin = GPIO_PIN_8;			//PG8
	GPIO_Initure.Mode = GPIO_MODE_INPUT;      //����
	HAL_GPIO_Init(GPIOG, &GPIO_Initure);     //��ʼ��

	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14, GPIO_PIN_SET);//PB14���1,��ֹSPI FLASH����NRF��ͨ��

	spi_open(0, SPI_BAUDRATEPRESCALER_2);

	u32 value = SPI_POLARITY_LOW;
	spi_ctrl(0, PARA_SPI_CLK_POLARITY, &value, 4);
	value = SPI_PHASE_1EDGE;
	spi_ctrl(0, PARA_SPI_CLK_PHASE, &value, 4);

	NRF24L01_CE = 0; 			                //ʹ��24L01
	NRF24L01_CSN= 1;			                //SPIƬѡȡ��
}
//���24L01�Ƿ����
//����ֵ:0���ɹ�;1��ʧ��
u8 NRF24L01_Check(void)
{
	u8 buf[5]={0XA5,0XA5,0XA5,0XA5,0XA5};
	u8 i;
	//SPI1_SetSpeed(SPI_BAUDRATEPRESCALER_8); //spi�ٶ�Ϊ10.5Mhz����24L01�����SPIʱ��Ϊ10Mhz,�����һ��û��ϵ��
	u32 value = SPI_BAUDRATEPRESCALER_8;
	spi_ctrl(0, PARA_SPI_BAUDRATE_PRESCALER, &value, 4);
	NRF24L01_Write_Buf(NRF_WRITE_REG+TX_ADDR,buf,5);//д��5���ֽڵĵ�ַ.
	NRF24L01_Read_Buf(TX_ADDR,buf,5); //����д��ĵ�ַ
	for(i=0;i<5;i++)if(buf[i]!=0XA5)break;
	if(i!=5)return 1;//���24L01����
	return 0;		 //��⵽24L01
}
//SPIд�Ĵ���
//reg:ָ���Ĵ�����ַ
//value:д���ֵ
u8 NRF24L01_Write_Reg(u8 reg,u8 value)
{
	  u8 status;
   	NRF24L01_CSN=0;                 //ʹ��SPI����
  	//status =SPI1_ReadWriteByte(reg);//���ͼĴ�����
   	status = spi_read_write(0, reg, 1000);
  	//SPI1_ReadWriteByte(value);      //д��Ĵ�����ֵ
   	spi_read_write(0, value, 1000);
  	NRF24L01_CSN=1;                 //��ֹSPI����
  	return(status);       		    //����״ֵ̬
}
//��ȡSPI�Ĵ���ֵ
//reg:Ҫ���ļĴ���
u8 NRF24L01_Read_Reg(u8 reg)
{
	  u8 reg_val;
   	NRF24L01_CSN=0;             //ʹ��SPI����
  	//SPI1_ReadWriteByte(reg);    //���ͼĴ�����
  	spi_read_write(0, reg, 1000);
  	//reg_val=SPI1_ReadWriteByte(0XFF);//��ȡ�Ĵ�������
  	reg_val=spi_read_write(0, 0XFF, 1000);//��ȡ�Ĵ�������
  	NRF24L01_CSN=1;             //��ֹSPI����
  	return(reg_val);            //����״ֵ̬
}
//��ָ��λ�ö���ָ�����ȵ�����
//reg:�Ĵ���(λ��)
//*pBuf:����ָ��
//len:���ݳ���
//����ֵ,�˴ζ�����״̬�Ĵ���ֵ
u8 NRF24L01_Read_Buf(u8 reg,u8 *pBuf,u8 len)
{
	u8 status,u8_ctr;
  	NRF24L01_CSN=0;            //ʹ��SPI����
  	//status=SPI1_ReadWriteByte(reg);//���ͼĴ���ֵ(λ��),����ȡ״ֵ̬
  	status = spi_read_write(0, reg, 1000);
	for(u8_ctr=0;u8_ctr<len;u8_ctr++) {
		//pBuf[u8_ctr]=SPI1_ReadWriteByte(0XFF);//��������
		pBuf[u8_ctr] = spi_read_write(0, 0XFF, 1000);
	}
  	NRF24L01_CSN=1;            //�ر�SPI����
  	return status;             //���ض�����״ֵ̬
}
//��ָ��λ��дָ�����ȵ�����
//reg:�Ĵ���(λ��)
//*pBuf:����ָ��
//len:���ݳ���
//����ֵ,�˴ζ�����״̬�Ĵ���ֵ
u8 NRF24L01_Write_Buf(u8 reg, u8 *pBuf, u8 len)
{
	u8 status,u8_ctr;
	NRF24L01_CSN=0;             //ʹ��SPI����
  	//status = SPI1_ReadWriteByte(reg);//���ͼĴ���ֵ(λ��),����ȡ״ֵ̬
  	status = spi_read_write(0, reg, 1000);
  	for(u8_ctr=0; u8_ctr<len; u8_ctr++) {
  		//SPI1_ReadWriteByte(*pBuf++); //д������
  		spi_read_write(0, *pBuf++, 1000);
  	}
  	NRF24L01_CSN=1;             //�ر�SPI����
  	return status;              //���ض�����״ֵ̬
}
//����NRF24L01����һ������
//txbuf:�����������׵�ַ
//����ֵ:�������״��
u8 NRF24L01_TxPacket(u8 *txbuf)
{
	u8 sta;
 	//SPI1_SetSpeed(SPI_BAUDRATEPRESCALER_8); //spi�ٶ�Ϊ6.75Mhz��24L01�����SPIʱ��Ϊ10Mhz��
#if 0
	u32 value = SPI_BAUDRATEPRESCALER_8;
	spi_ctrl(0, PARA_SPI_BAUDRATE_PRESCALER, &value, 4);
#endif
	NRF24L01_CE=0;
  	NRF24L01_Write_Buf(WR_TX_PLOAD,txbuf,TX_PLOAD_WIDTH);//д���ݵ�TX BUF  32���ֽ�
 	NRF24L01_CE=1;                         //��������
	while(NRF24L01_IRQ!=0);                 //�ȴ��������
	sta=NRF24L01_Read_Reg(STATUS);          //��ȡ״̬�Ĵ�����ֵ
	NRF24L01_Write_Reg(NRF_WRITE_REG+STATUS,sta); //���TX_DS��MAX_RT�жϱ�־
	if(sta&MAX_TX)                          //�ﵽ����ط�����
	{
		NRF24L01_Write_Reg(FLUSH_TX,0xff);  //���TX FIFO�Ĵ���
		return MAX_TX;
	}
	if(sta&TX_OK)                           //�������
	{
		return TX_OK;
	}
	return 0xff;//����ԭ����ʧ��
}
//����NRF24L01����һ������
//txbuf:�����������׵�ַ
//����ֵ:0��������ɣ��������������
u8 NRF24L01_RxPacket(u8 *rxbuf)
{
	u8 sta;
	//SPI1_SetSpeed(SPI_BAUDRATEPRESCALER_8); //spi�ٶ�Ϊ6.75Mhz��24L01�����SPIʱ��Ϊ10Mhz��
#if 0
	u32 value = SPI_BAUDRATEPRESCALER_8;
	spi_ctrl(0, PARA_SPI_BAUDRATE_PRESCALER, &value, 4);
#endif
	sta=NRF24L01_Read_Reg(STATUS);          //��ȡ״̬�Ĵ�����ֵ
	NRF24L01_Write_Reg(NRF_WRITE_REG+STATUS,sta); //���TX_DS��MAX_RT�жϱ�־
	if(sta&RX_OK)//���յ�����
	{
		NRF24L01_Read_Buf(RD_RX_PLOAD,rxbuf,RX_PLOAD_WIDTH);//��ȡ����
		NRF24L01_Write_Reg(FLUSH_RX,0xff);  //���RX FIFO�Ĵ���
		return 0;
	}
	return 1;//û�յ��κ�����
}
//�ú�����ʼ��NRF24L01��RXģʽ
//����RX��ַ,дRX���ݿ��,ѡ��RFƵ��,�����ʺ�LNA HCURR
//��CE��ߺ�,������RXģʽ,�����Խ���������
void NRF24L01_RX_Mode(void)
{
	NRF24L01_CE=0;
  	NRF24L01_Write_Buf(NRF_WRITE_REG+RX_ADDR_P0,(u8*)RX_ADDRESS,RX_ADR_WIDTH);//дRX�ڵ��ַ

  	NRF24L01_Write_Reg(NRF_WRITE_REG+EN_AA,0x01);       //ʹ��ͨ��0���Զ�Ӧ��
  	NRF24L01_Write_Reg(NRF_WRITE_REG+EN_RXADDR,0x01);   //ʹ��ͨ��0�Ľ��յ�ַ
  	NRF24L01_Write_Reg(NRF_WRITE_REG+RF_CH,40);	        //����RFͨ��Ƶ��
  	NRF24L01_Write_Reg(NRF_WRITE_REG+RX_PW_P0,RX_PLOAD_WIDTH);//ѡ��ͨ��0����Ч���ݿ��
  	NRF24L01_Write_Reg(NRF_WRITE_REG+RF_SETUP,0x0f);    //����TX�������,0db����,2Mbps,���������濪��
  	NRF24L01_Write_Reg(NRF_WRITE_REG+CONFIG, 0x0f);     //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ
  	NRF24L01_CE=1; //CEΪ��,�������ģʽ
}
//�ú�����ʼ��NRF24L01��TXģʽ
//����TX��ַ,дTX���ݿ��,����RX�Զ�Ӧ��ĵ�ַ,���TX��������,ѡ��RFƵ��,�����ʺ�LNA HCURR
//PWR_UP,CRCʹ��
//��CE��ߺ�,������RXģʽ,�����Խ���������
//CEΪ�ߴ���10us,����������.
void NRF24L01_TX_Mode(void)
{
	NRF24L01_CE=0;
	NRF24L01_Write_Buf(NRF_WRITE_REG+TX_ADDR,(u8*)TX_ADDRESS,TX_ADR_WIDTH);//дTX�ڵ��ַ
	NRF24L01_Write_Buf(NRF_WRITE_REG+RX_ADDR_P0,(u8*)RX_ADDRESS,RX_ADR_WIDTH); //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK

	NRF24L01_Write_Reg(NRF_WRITE_REG+EN_AA,0x01);     //ʹ��ͨ��0���Զ�Ӧ��
	NRF24L01_Write_Reg(NRF_WRITE_REG+EN_RXADDR,0x01); //ʹ��ͨ��0�Ľ��յ�ַ
	NRF24L01_Write_Reg(NRF_WRITE_REG+SETUP_RETR,0x1a);//�����Զ��ط����ʱ��:500us + 86us;����Զ��ط�����:10��
	NRF24L01_Write_Reg(NRF_WRITE_REG+RF_CH,40);       //����RFͨ��Ϊ40
	NRF24L01_Write_Reg(NRF_WRITE_REG+RF_SETUP,0x0f);  //����TX�������,0db����,2Mbps,���������濪��
	NRF24L01_Write_Reg(NRF_WRITE_REG+CONFIG,0x0e);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�
	NRF24L01_CE=1;//CEΪ��,10us����������
}

void NRF24L01_TEST()
{
	u32 mode = 1;
	u32 value = 0;

	s32 i = 0;
	u32 totals = 0;
	u32 bytes_1s = 0;

	u32 time_span = 0;
	u8 tmp_buf[RX_PLOAD_WIDTH+1];
	u32 timemark = VOSGetTimeMs();
	NRF24L01_Init();
	while(NRF24L01_Check())
	{
		VOSTaskDelay(10);
	}
	if(mode==0)
	{
		kprintf("working at rx mode!\r\n");
		NRF24L01_RX_Mode();
		value = SPI_BAUDRATEPRESCALER_16;//SPI_BAUDRATEPRESCALER_8;
		spi_ctrl(0, PARA_SPI_BAUDRATE_PRESCALER, &value, 4);
		while(1)
		{
			if(NRF24L01_RxPacket(tmp_buf)==0)
			{
				tmp_buf[RX_PLOAD_WIDTH]=0;
				//kprintf("rx: %s!\r\n", tmp_buf);
		  		time_span = VOSGetTimeMs()-timemark;
		  		totals += RX_PLOAD_WIDTH;
		  		bytes_1s += RX_PLOAD_WIDTH;
		  		if (VOSGetTimeMs() - timemark > 1000) {
		  			kprintf("=====%d(KBps), totals=%d(KB) =====!\r\n", bytes_1s/1000, totals/1000);
		  			timemark = VOSGetTimeMs();
		  			bytes_1s = 0;
		  		}
			}
			//VOSTaskDelay(10);
		};
	}
	else
	{
		kprintf("working at tx mode!\r\n");

		for (i = 0; i<sizeof(tmp_buf); i++) {
			tmp_buf[i] = 'a' + i%26;
		}

		u32 cnts = 0;
		NRF24L01_TX_Mode();
		value = SPI_BAUDRATEPRESCALER_16;//SPI_BAUDRATEPRESCALER_8;
		spi_ctrl(0, PARA_SPI_BAUDRATE_PRESCALER, &value, 4);
		while(1)
		{
			if(NRF24L01_TxPacket(tmp_buf)==TX_OK)
			{
				//kprintf("send ok!\r\n");
				//kprintf(".");
				tmp_buf[RX_PLOAD_WIDTH] = 0;
			}
			//VOSTaskDelay(10);
		};
	}
}
