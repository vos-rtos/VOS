#include "w25qxx.h" 
#include "spi.h"
#include "vtype.h"

#define printf kprintf

#include "stm32f4xx_hal.h"

u16 W25QXX_TYPE=W25Q256;	//Ĭ����W25Q256

//4KbytesΪһ��Sector
//16������Ϊ1��Block
//W25Q256
//����Ϊ32M�ֽ�,����512��Block,8192��Sector

//��ʼ��SPI FLASH��IO��
void W25QXX_Init(void)
{
    u8 temp;
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_GPIOB_CLK_ENABLE();           //ʹ��GPIOFʱ��

    //PF6
    GPIO_Initure.Pin=GPIO_PIN_14;            //PF6
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //�������
    GPIO_Initure.Pull=GPIO_PULLUP;          //����
    GPIO_Initure.Speed=GPIO_SPEED_FAST;     //����
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);     //��ʼ��

	W25QXX_CS=1;			                //SPI FLASH��ѡ��
	//SPI1_Init();		   			        //��ʼ��SPI
	//SPI1_SetSpeed(SPI_BAUDRATEPRESCALER_2); //����Ϊ45Mʱ��,����ģʽ
	spi_open(0, SPI_BAUDRATEPRESCALER_2); //����Ϊ45Mʱ��,����ģʽ
	W25QXX_TYPE=W25QXX_ReadID();	        //��ȡFLASH ID.
    if(W25QXX_TYPE==W25Q256)                //SPI FLASHΪW25Q256
    {
        temp=W25QXX_ReadSR(3);              //��ȡ״̬�Ĵ���3���жϵ�ַģʽ
        if((temp&0X01)==0)			        //�������4�ֽڵ�ַģʽ,�����4�ֽڵ�ַģʽ
		{
			W25QXX_CS=0; 			        //ѡ��
			spi_read_write(0, W25X_Enable4ByteAddr, 1000);//���ͽ���4�ֽڵ�ַģʽָ��
			W25QXX_CS=1;       		        //ȡ��Ƭѡ
		}
    }
}

//��ȡW25QXX��״̬�Ĵ�����W25QXXһ����3��״̬�Ĵ���
//״̬�Ĵ���1��
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:Ĭ��0,״̬�Ĵ�������λ,���WPʹ��
//TB,BP2,BP1,BP0:FLASH����д��������
//WEL:дʹ������
//BUSY:æ���λ(1,æ;0,����)
//Ĭ��:0x00
//״̬�Ĵ���2��
//BIT7  6   5   4   3   2   1   0
//SUS   CMP LB3 LB2 LB1 (R) QE  SRP1
//״̬�Ĵ���3��
//BIT7      6    5    4   3   2   1   0
//HOLD/RST  DRV1 DRV0 (R) (R) WPS ADP ADS
//regno:״̬�Ĵ����ţ���:1~3
//����ֵ:״̬�Ĵ���ֵ
u8 W25QXX_ReadSR(u8 regno)
{
	u8 byte=0,command=0;
    switch(regno)
    {
        case 1:
            command=W25X_ReadStatusReg1;    //��״̬�Ĵ���1ָ��
            break;
        case 2:
            command=W25X_ReadStatusReg2;    //��״̬�Ĵ���2ָ��
            break;
        case 3:
            command=W25X_ReadStatusReg3;    //��״̬�Ĵ���3ָ��
            break;
        default:
            command=W25X_ReadStatusReg1;
            break;
    }
	W25QXX_CS=0;                            //ʹ������
	spi_read_write(0, command, 1000);            //���Ͷ�ȡ״̬�Ĵ�������
	byte=spi_read_write(0, 0Xff, 1000);          //��ȡһ���ֽ�
	W25QXX_CS=1;                            //ȡ��Ƭѡ
	return byte;
}
//дW25QXX״̬�Ĵ���
void W25QXX_Write_SR(u8 regno,u8 sr)
{
    u8 command=0;
    switch(regno)
    {
        case 1:
            command=W25X_WriteStatusReg1;    //д״̬�Ĵ���1ָ��
            break;
        case 2:
            command=W25X_WriteStatusReg2;    //д״̬�Ĵ���2ָ��
            break;
        case 3:
            command=W25X_WriteStatusReg3;    //д״̬�Ĵ���3ָ��
            break;
        default:
            command=W25X_WriteStatusReg1;
            break;
    }
	W25QXX_CS=0;                            //ʹ������
	spi_read_write(0, command, 1000);            //����дȡ״̬�Ĵ�������
	spi_read_write(0, sr, 1000);                 //д��һ���ֽ�
	W25QXX_CS=1;                            //ȡ��Ƭѡ
}
//W25QXXдʹ��
//��WEL��λ
void W25QXX_Write_Enable(void)
{
	W25QXX_CS=0;                            //ʹ������
    spi_read_write(0, W25X_WriteEnable, 1000);   //����дʹ��
	W25QXX_CS=1;                            //ȡ��Ƭѡ
}
//W25QXXд��ֹ
//��WEL����
void W25QXX_Write_Disable(void)
{
	W25QXX_CS=0;                            //ʹ������
    spi_read_write(0, W25X_WriteDisable, 1000);  //����д��ָֹ��
	W25QXX_CS=1;                            //ȡ��Ƭѡ
}

//��ȡоƬID
//����ֵ����:
//0XEF13,��ʾоƬ�ͺ�ΪW25Q80
//0XEF14,��ʾоƬ�ͺ�ΪW25Q16
//0XEF15,��ʾоƬ�ͺ�ΪW25Q32
//0XEF16,��ʾоƬ�ͺ�ΪW25Q64
//0XEF17,��ʾоƬ�ͺ�ΪW25Q128
//0XEF18,��ʾоƬ�ͺ�ΪW25Q256
u16 W25QXX_ReadID(void)
{
	u16 Temp = 0;
	W25QXX_CS=0;
	spi_read_write(0, 0x90, 1000);//���Ͷ�ȡID����
	spi_read_write(0, 0x00, 1000);
	spi_read_write(0, 0x00, 1000);
	spi_read_write(0, 0x00, 1000);
	Temp|=spi_read_write(0, 0xFF, 1000)<<8;
	Temp|=spi_read_write(0, 0xFF, 1000);
	W25QXX_CS=1;
	return Temp;
}
//��ȡSPI FLASH
//��ָ����ַ��ʼ��ȡָ�����ȵ�����
//pBuffer:���ݴ洢��
//ReadAddr:��ʼ��ȡ�ĵ�ַ(24bit)
//NumByteToRead:Ҫ��ȡ���ֽ���(���65535)
void W25QXX_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead)
{
 	u16 i;
	W25QXX_CS=0;                            //ʹ������
    spi_read_write(0, W25X_ReadData, 1000);      //���Ͷ�ȡ����
    if(W25QXX_TYPE==W25Q256)                //�����W25Q256�Ļ���ַΪ4�ֽڵģ�Ҫ�������8λ
    {
        spi_read_write(0, (u8)((ReadAddr)>>24), 1000);
    }
    spi_read_write(0, (u8)((ReadAddr)>>16), 1000);   //����24bit��ַ
    spi_read_write(0, (u8)((ReadAddr)>>8), 1000);
    spi_read_write(0, (u8)ReadAddr, 1000);
    for(i=0;i<NumByteToRead;i++)
	{
        pBuffer[i]=spi_read_write(0, 0XFF, 1000);    //ѭ������
    }
	W25QXX_CS=1;
}
//SPI��һҳ(0~65535)��д������256���ֽڵ�����
//��ָ����ַ��ʼд�����256�ֽڵ�����
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)
//NumByteToWrite:Ҫд����ֽ���(���256),������Ӧ�ó�����ҳ��ʣ���ֽ���!!!
void W25QXX_Write_Page(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
{
 	u16 i;
    W25QXX_Write_Enable();                  //SET WEL
	W25QXX_CS=0;                            //ʹ������
    spi_read_write(0, W25X_PageProgram, 1000);   //����дҳ����
    if(W25QXX_TYPE==W25Q256)                //�����W25Q256�Ļ���ַΪ4�ֽڵģ�Ҫ�������8λ
    {
        spi_read_write(0, (u8)((WriteAddr)>>24), 1000);
    }
    spi_read_write(0, (u8)((WriteAddr)>>16), 1000); //����24bit��ַ
    spi_read_write(0, (u8)((WriteAddr)>>8), 1000);
    spi_read_write(0, (u8)WriteAddr, 1000);
    for(i=0;i<NumByteToWrite;i++)spi_read_write(0, pBuffer[i], 1000);//ѭ��д��
	W25QXX_CS=1;                            //ȡ��Ƭѡ
	W25QXX_Wait_Busy();					   //�ȴ�д�����
}
//�޼���дSPI FLASH
//����ȷ����д�ĵ�ַ��Χ�ڵ�����ȫ��Ϊ0XFF,�����ڷ�0XFF��д������ݽ�ʧ��!
//�����Զ���ҳ����
//��ָ����ַ��ʼд��ָ�����ȵ�����,����Ҫȷ����ַ��Խ��!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)
//NumByteToWrite:Ҫд����ֽ���(���65535)
//CHECK OK
void W25QXX_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
{
	u16 pageremain;
	pageremain=256-WriteAddr%256; //��ҳʣ����ֽ���
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//������256���ֽ�
	while(1)
	{
		W25QXX_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//д�������
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;

			NumByteToWrite-=pageremain;			  //��ȥ�Ѿ�д���˵��ֽ���
			if(NumByteToWrite>256)pageremain=256; //һ�ο���д��256���ֽ�
			else pageremain=NumByteToWrite; 	  //����256���ֽ���
		}
	};
}
//дSPI FLASH
//��ָ����ַ��ʼд��ָ�����ȵ�����
//�ú�������������!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)
//NumByteToWrite:Ҫд����ֽ���(���65535)
u8 W25QXX_BUFFER[4096];
void W25QXX_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
{
	u32 secpos;
	u16 secoff;
	u16 secremain;
 	u16 i;
	u8 * W25QXX_BUF;
   	W25QXX_BUF=W25QXX_BUFFER;
 	secpos=WriteAddr/4096;//������ַ
	secoff=WriteAddr%4096;//�������ڵ�ƫ��
	secremain=4096-secoff;//����ʣ��ռ��С
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//������
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//������4096���ֽ�
	while(1)
	{
		W25QXX_Read(W25QXX_BUF,secpos*4096,4096);//������������������
		for(i=0;i<secremain;i++)//У������
		{
			if(W25QXX_BUF[secoff+i]!=0XFF)break;//��Ҫ����
		}
		if(i<secremain)//��Ҫ����
		{
			W25QXX_Erase_Sector(secpos);//�����������
			for(i=0;i<secremain;i++)	   //����
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];
			}
			W25QXX_Write_NoCheck(W25QXX_BUF,secpos*4096,4096);//д����������

		}else W25QXX_Write_NoCheck(pBuffer,WriteAddr,secremain);//д�Ѿ������˵�,ֱ��д������ʣ������.
		if(NumByteToWrite==secremain)break;//д�������
		else//д��δ����
		{
			secpos++;//������ַ��1
			secoff=0;//ƫ��λ��Ϊ0

		   	pBuffer+=secremain;  //ָ��ƫ��
			WriteAddr+=secremain;//д��ַƫ��
		   	NumByteToWrite-=secremain;				//�ֽ����ݼ�
			if(NumByteToWrite>4096)secremain=4096;	//��һ����������д����
			else secremain=NumByteToWrite;			//��һ����������д����
		}
	};
}
//��������оƬ
//�ȴ�ʱ�䳬��...
void W25QXX_Erase_Chip(void)
{
    W25QXX_Write_Enable();                  //SET WEL
    W25QXX_Wait_Busy();
  	W25QXX_CS=0;                            //ʹ������
    spi_read_write(0, W25X_ChipErase, 1000);        //����Ƭ��������
	W25QXX_CS=1;                            //ȡ��Ƭѡ
	W25QXX_Wait_Busy();   				   //�ȴ�оƬ��������
}
//����һ������
//Dst_Addr:������ַ ����ʵ����������
//����һ������������ʱ��:150ms
void W25QXX_Erase_Sector(u32 Dst_Addr)
{
	//����falsh�������,������
 	//printf("fe:%x\r\n",Dst_Addr);
 	Dst_Addr*=4096;
    W25QXX_Write_Enable();                  //SET WEL
    W25QXX_Wait_Busy();
  	W25QXX_CS=0;                            //ʹ������
    spi_read_write(0, W25X_SectorErase, 1000);   //������������ָ��
    if(W25QXX_TYPE==W25Q256)                //�����W25Q256�Ļ���ַΪ4�ֽڵģ�Ҫ�������8λ
    {
        spi_read_write(0, (u8)((Dst_Addr)>>24), 1000);
    }
    spi_read_write(0, (u8)((Dst_Addr)>>16), 1000);  //����24bit��ַ
    spi_read_write(0, (u8)((Dst_Addr)>>8), 1000);
    spi_read_write(0, (u8)Dst_Addr, 1000);
	W25QXX_CS=1;                            //ȡ��Ƭѡ
    W25QXX_Wait_Busy();   				    //�ȴ��������
}
//�ȴ�����
void W25QXX_Wait_Busy(void)
{
	while((W25QXX_ReadSR(1)&0x01)==0x01);   // �ȴ�BUSYλ���
}
//�������ģʽ
void W25QXX_PowerDown(void)
{
  	W25QXX_CS=0;                            //ʹ������
    spi_read_write(0, W25X_PowerDown, 1000);     //���͵�������
	W25QXX_CS=1;                            //ȡ��Ƭѡ
    delay_us(3);                            //�ȴ�TPD
}
//����
void W25QXX_WAKEUP(void)
{
  	W25QXX_CS=0;                                //ʹ������
    spi_read_write(0, W25X_ReleasePowerDown, 1000);  //  send W25X_PowerDown command 0xAB
	W25QXX_CS=1;                                //ȡ��Ƭѡ
    delay_us(3);                                //�ȴ�TRES1
}
























