#ifndef __I2C_H__
#define __I2C_H__

#endif

#ifndef _MYIIC_H
#define _MYIIC_H

#include "common.h"
#include "vos.h"


//IO��������
#define SDA_IN()  GPIOx_IN(pI2cBus->GPIOx, pI2cBus->pin_sda) //PB9����ģʽ
#define SDA_OUT() GPIOx_IN(pI2cBus->GPIOx, pI2cBus->pin_sda) //PB9���ģʽ

//IO����
#define IIC_SCL   BIT_ADDR(pI2cBus->odr_addr, pI2cBus->pin_scl) //SCL
#define IIC_SDA   BIT_ADDR(pI2cBus->odr_addr, pI2cBus->pin_sda) //SDA
#define READ_SDA  BIT_ADDR(pI2cBus->idr_addr, pI2cBus->pin_sda) //����SDA


void i2c_nack(s32 port);
void i2c_ack(s32 port);
u8 i2c_wait_ack(s32 port);
void i2c_stop(s32 port);
void i2c_start(s32 port);
u8 i2c_read(s32 port, u8 ack);
void i2c_send(s32 port, u8 ch);
void i2c_init(s32 port);

#endif

