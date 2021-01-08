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

		__HAL_RCC_GPIOB_CLK_ENABLE();   //使能GPIOB时钟
		//PH4,5初始化设置
		GPIO_Initure.Pin	= pI2cBus->pin_scl|pI2cBus->pin_sda;
		GPIO_Initure.Mode	= GPIO_MODE_OUTPUT_PP;  //推挽输出
		GPIO_Initure.Pull	= GPIO_PULLUP;          //上拉
		GPIO_Initure.Speed	= GPIO_SPEED_FAST;     //快速
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

		__HAL_RCC_GPIOB_CLK_ENABLE();   //使能GPIOB时钟
		//PH4,5初始化设置
		GPIO_Initure.Pin	= pI2cBus->pin_scl|pI2cBus->pin_sda;
		GPIO_Initure.Mode	= GPIO_MODE_OUTPUT_PP;  //推挽输出
		GPIO_Initure.Pull	= GPIO_PULLUP;          //上拉
		GPIO_Initure.Speed	= GPIO_SPEED_FAST;     //快速
		HAL_GPIO_Init(pI2cBus->GPIOx, &GPIO_Initure);
		IIC_SDA = 1;
		IIC_SCL = 1;
    }
}

//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答
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
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK
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
        IIC_NAck();//发送nACK
    else
        IIC_Ack(); //发送ACK
    return recv;
}


//产生IIC起始信号
void i2c_start(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	SDA_OUT();
	IIC_SDA = 1;
	IIC_SCL = 1;
	VOSDelayUs(4);
 	IIC_SDA = 0;//START:when CLK is high,DATA change form high to low
 	VOSDelayUs(4);
	IIC_SCL = 0;//钳住I2C总线，准备发送或接收数据
}
//产生IIC停止信号
void i2c_stop(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	SDA_OUT();//sda线输出
	IIC_SCL = 0;
	IIC_SDA = 0;//STOP:when CLK is high DATA change form low to high
	VOSDelayUs(4);
	IIC_SCL = 1;
	IIC_SDA = 1;//发送I2C总线结束信号
	VOSDelayUs(4);
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 i2c_wait_ack(s32 port)
{
	struct StI2cBusSoft *pI2cBus = &gI2cBusSoft[port];
	u8 ucErrTime=0;
	SDA_IN();      //SDA设置为输入
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
	IIC_SCL = 0;//时钟输出0
	return 0;
}
//产生ACK应答
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
//不产生ACK应答
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


