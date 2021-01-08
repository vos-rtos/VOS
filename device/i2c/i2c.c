#include "i2c.h"
#include "stm32f4xx_hal.h"

#define printf kprintf
#define delay_ms(x) VOSDelayUs((x)*1000)
#define delay_us(x) VOSDelayUs(x)


typedef struct StI2cBusSoft {
	u32 pin_scl;
	u32 pin_sda;
	u32 odr_addr;
	u32 idr_addr;
	GPIO_TypeDef *GPIOx;
}StI2cBusSoft;

struct StI2cBusSoft gI2cBusSoft[2];


void i2c_init(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];

    GPIO_InitTypeDef GPIO_Initure;

    if (port == 0) {//for wm8987

    	pI2cBus->pin_scl = GPIO_PIN_8;
    	pI2cBus->pin_sda = GPIO_PIN_9;
    	pI2cBus->GPIOx = GPIOB;
    	pI2cBus->odr_addr = GPIOB_ODR_Addr;
    	pI2cBus->idr_addr = GPIOB_IDR_Addr;

		__HAL_RCC_GPIOB_CLK_ENABLE();   //ʹ��GPIOBʱ��
		//PH4,5��ʼ������
		GPIO_Initure.Pin	= pI2cBus->pin_scl|pI2cBus->pin_sda;
		GPIO_Initure.Mode	= GPIO_MODE_OUTPUT_PP;  //�������
		GPIO_Initure.Pull	= GPIO_PULLUP;          //����
		GPIO_Initure.Speed	= GPIO_SPEED_FAST;     //����
		HAL_GPIO_Init(pI2cBus->GPIOx, &GPIO_Initure);
		IIC_SDA = 1;
		IIC_SCL = 1;
    }
    if (port == 1) {//for touch screen

    	pI2cBus->pin_scl = GPIO_PIN_8;
    	pI2cBus->pin_sda = GPIO_PIN_9;
    	pI2cBus->GPIOx = GPIOB;
    	pI2cBus->odr_addr = GPIOB_ODR_Addr;
    	pI2cBus->idr_addr = GPIOB_IDR_Addr;

		__HAL_RCC_GPIOB_CLK_ENABLE();   //ʹ��GPIOBʱ��
		//PH4,5��ʼ������
		GPIO_Initure.Pin	= pI2cBus->pin_scl|pI2cBus->pin_sda;
		GPIO_Initure.Mode	= GPIO_MODE_OUTPUT_PP;  //�������
		GPIO_Initure.Pull	= GPIO_PULLUP;          //����
		GPIO_Initure.Speed	= GPIO_SPEED_FAST;     //����
		HAL_GPIO_Init(pI2cBus->GPIOx, &GPIO_Initure);
		IIC_SDA = 1;
		IIC_SCL = 1;
    }
}

//IIC����һ���ֽ�
//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��
void i2c_send(s32 port, u8 ch)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
    u8 i = 0;
	SDA_OUT();
    IIC_SCL = 0;
    for(i=0; i<8; i++)
    {
        IIC_SDA = (ch&0x80)>>7;
        ch <<= 1;
        VOSDelayUs(2);
		IIC_SCL = 1;
		VOSDelayUs(2);
		IIC_SCL = 0;
		VOSDelayUs(2);
    }
}
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK
u8 i2c_read(s32 port, u8 ack)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	u8 i = 0;
	u8 recv = 0;
	SDA_IN();
    for (i=0; i<8; i++) {
        IIC_SCL=0;
        VOSDelayUs(2);
		IIC_SCL = 1;
		recv <<= 1;
        if (READ_SDA) recv++;
        VOSDelayUs(1);
    }
    if (!ack)
        IIC_NAck();//����nACK
    else
        IIC_Ack(); //����ACK
    return recv;
}


//����IIC��ʼ�ź�
void i2c_start(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	SDA_OUT();
	IIC_SDA = 1;
	IIC_SCL = 1;
	VOSDelayUs(4);
 	IIC_SDA = 0;//START:when CLK is high,DATA change form high to low
 	VOSDelayUs(4);
	IIC_SCL = 0;//ǯסI2C���ߣ�׼�����ͻ��������
}
//����IICֹͣ�ź�
void i2c_stop(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	SDA_OUT();//sda�����
	IIC_SCL = 0;
	IIC_SDA = 0;//STOP:when CLK is high DATA change form low to high
	VOSDelayUs(4);
	IIC_SCL = 1;
	IIC_SDA = 1;//����I2C���߽����ź�
	VOSDelayUs(4);
}
//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
u8 i2c_wait_ack(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	u8 ucErrTime=0;
	SDA_IN();      //SDA����Ϊ����
	IIC_SDA = 1;
	VOSDelayUs(1);
	IIC_SCL = 1;
	VOSDelayUs(1);
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			i2c_stop(0);
			return 1;
		}
	}
	IIC_SCL = 0;//ʱ�����0
	return 0;
}
//����ACKӦ��
void i2c_ack(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 0;
	VOSDelayUs(2);
	IIC_SCL = 1;
	VOSDelayUs(2);
	IIC_SCL = 0;
}
//������ACKӦ��
void i2c_nack(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 1;
	VOSDelayUs(2);
	IIC_SCL = 1;
	VOSDelayUs(2);
	IIC_SCL = 0;
}


