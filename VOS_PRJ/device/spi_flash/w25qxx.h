#ifndef __W25QXX_H__
#define __W25QXX_H__
#include "stm32f4xx.h"

#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)  *((volatile unsigned long*)(addr))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))

#define GPIOA_ODR_Addr    (GPIOA_BASE+20) //0x40020014
#define GPIOB_ODR_Addr    (GPIOB_BASE+20) //0x40020414
#define GPIOC_ODR_Addr    (GPIOC_BASE+20) //0x40020814
#define GPIOD_ODR_Addr    (GPIOD_BASE+20) //0x40020C14
#define GPIOE_ODR_Addr    (GPIOE_BASE+20) //0x40021014
#define GPIOF_ODR_Addr    (GPIOF_BASE+20) //0x40021414
#define GPIOG_ODR_Addr    (GPIOG_BASE+20) //0x40021814
#define GPIOH_ODR_Addr    (GPIOH_BASE+20) //0x40021C14
#define GPIOI_ODR_Addr    (GPIOI_BASE+20) //0x40022014

#define GPIOA_IDR_Addr    (GPIOA_BASE+16) //0x40020010
#define GPIOB_IDR_Addr    (GPIOB_BASE+16) //0x40020410
#define GPIOC_IDR_Addr    (GPIOC_BASE+16) //0x40020810
#define GPIOD_IDR_Addr    (GPIOD_BASE+16) //0x40020C10
#define GPIOE_IDR_Addr    (GPIOE_BASE+16) //0x40021010
#define GPIOF_IDR_Addr    (GPIOF_BASE+16) //0x40021410
#define GPIOG_IDR_Addr    (GPIOG_BASE+16) //0x40021810
#define GPIOH_IDR_Addr    (GPIOH_BASE+16) //0x40021C10
#define GPIOI_IDR_Addr    (GPIOI_BASE+16) //0x40022010

#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)

#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)

#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)

#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)
#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)

#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)
#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)

#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)

#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)
#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)

#define PHout(n)   BIT_ADDR(GPIOH_ODR_Addr,n)
#define PHin(n)    BIT_ADDR(GPIOH_IDR_Addr,n)

#define PIout(n)   BIT_ADDR(GPIOI_ODR_Addr,n)
#define PIin(n)    BIT_ADDR(GPIOI_IDR_Addr,n)

//W25Xϵ��/Qϵ��оƬ�б�	   
//W25Q80  ID  0XEF13
//W25Q16  ID  0XEF14
//W25Q32  ID  0XEF15
//W25Q64  ID  0XEF16	
//W25Q128 ID  0XEF17	
#define W25Q80 	0XEF13 	
#define W25Q16 	0XEF14
#define W25Q32 	0XEF15
#define W25Q64 	0XEF16
#define W25Q128	0XEF17

extern u16 W25QXX_TYPE;					//����W25QXXоƬ�ͺ�		   

#define	W25QXX_CS 		PBout(14)  		//W25QXX��Ƭѡ�ź�,�͵�ƽʹ�ܣ���������0ʹ��

//ָ���
#define W25X_WriteEnable		0x06 
#define W25X_WriteDisable		0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData			0x03 
#define W25X_FastReadData		0x0B 
#define W25X_FastReadDual		0x3B 
#define W25X_PageProgram		0x02 
#define W25X_BlockErase			0xD8 
#define W25X_SectorErase		0x20 
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 

void W25QXX_Init();
u16  W25QXX_ReadID();
u8	 W25QXX_ReadSR();
void W25QXX_Write_SR(u8 sr);
void W25QXX_Write_Enable();
void W25QXX_Write_Disable();
void W25QXX_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);
void W25QXX_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead);
void W25QXX_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);
void W25QXX_Erase_Chip();
void W25QXX_Erase_Sector(u32 Dst_Addr);
void W25QXX_Wait_Busy();
void W25QXX_PowerDown();
void W25QXX_WAKEUP();

#endif
















