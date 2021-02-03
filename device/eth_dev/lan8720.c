#include "lan8720.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_conf.h"
#include "usart.h"  

void VOSDelayUs(u32 us);

ETH_HandleTypeDef ETH_Handler;

u32  ETH_GetRxPktSize(ETH_DMADescTypeDef *DMARxDesc);

#ifdef STM32F429xx
#include "io_exp/pcf8574.h"
#endif

void Lan8720ResetPinInit()
{
#ifdef STM32F407xx
   GPIO_InitTypeDef GPIO_Initure;
   __HAL_RCC_GPIOD_CLK_ENABLE();

   GPIO_Initure.Pin=GPIO_PIN_3;
   GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;
   GPIO_Initure.Pull=GPIO_PULLUP;
   GPIO_Initure.Speed=GPIO_SPEED_HIGH;
   HAL_GPIO_Init(GPIOD,&GPIO_Initure);
   HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_SET);
#elif defined(STM32F429xx)
   PCF8574_Init();
#endif
}

void Lan8720Reset()
{
#ifdef STM32F407xx
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_RESET);
	VOSDelayUs(50*1000);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_3,GPIO_PIN_SET);
#elif defined(STM32F429xx)
	PCF8574_WriteBit(ETH_RESET_IO,1);
	VOSDelayUs(100*1000);
	PCF8574_WriteBit(ETH_RESET_IO,0);
	VOSDelayUs(100*1000);
#endif
}


u8 LAN8720_Init(void)
{
    u8 macaddress[6];
    u32 irq_save;
	irq_save = __vos_irq_save();
	Lan8720ResetPinInit();
	Lan8720Reset();
	__vos_irq_restore(irq_save);

	memcpy(macaddress, ETH_MAC, 6);

	ETH_Handler.Instance=ETH;
    ETH_Handler.Init.AutoNegotiation=ETH_AUTONEGOTIATION_ENABLE;
    ETH_Handler.Init.Speed=ETH_SPEED_100M;
    ETH_Handler.Init.DuplexMode=ETH_MODE_FULLDUPLEX;
    ETH_Handler.Init.PhyAddress=LAN8720_PHY_ADDRESS;
    ETH_Handler.Init.MACAddr=macaddress;
    ETH_Handler.Init.RxMode=ETH_RXINTERRUPT_MODE;
    ETH_Handler.Init.ChecksumMode=ETH_CHECKSUM_BY_HARDWARE;
    ETH_Handler.Init.MediaInterface=ETH_MEDIA_INTERFACE_RMII;
    if(HAL_ETH_Init(&ETH_Handler)==HAL_OK)
    {
        return 0;
    }
    else return 1;
}


void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_ETH_CLK_ENABLE();

#ifdef STM32F407xx

    /*
    ETH_MDIO -------------------------> PA2
    ETH_MDC --------------------------> PC1
    ETH_RMII_REF_CLK------------------> PA1
    ETH_RMII_CRS_DV ------------------> PA7
    ETH_RMII_RXD0 --------------------> PC4
    ETH_RMII_RXD1 --------------------> PC5
    ETH_RMII_TX_EN -------------------> PG11
    ETH_RMII_TXD0 --------------------> PG13
    ETH_RMII_TXD1 --------------------> PG14
    ETH_RESET------------------------->
    */


    __HAL_RCC_GPIOA_CLK_ENABLE();
//	__HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();


    //PA1,2,7
    GPIO_Initure.Pin=GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7;
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;
    GPIO_Initure.Pull=GPIO_NOPULL;
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;
    GPIO_Initure.Alternate=GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);


    //PC1,4,5
    GPIO_Initure.Pin=GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);

    //PG11,13,14
    GPIO_Initure.Pin=GPIO_PIN_11|GPIO_PIN_13|GPIO_PIN_14;
    HAL_GPIO_Init(GPIOG,&GPIO_Initure);
#elif defined(STM32F429xx)
    /*
    ETH_MDIO -------------------------> PA2
    ETH_MDC --------------------------> PC1
    ETH_RMII_REF_CLK------------------> PA1
    ETH_RMII_CRS_DV ------------------> PA7
    ETH_RMII_RXD0 --------------------> PC4
    ETH_RMII_RXD1 --------------------> PC5
    ETH_RMII_TX_EN -------------------> PB11
    ETH_RMII_TXD0 --------------------> PG13
    ETH_RMII_TXD1 --------------------> PG14
    ETH_RESET-------------------------> PCF8574À©Õ¹IO
    */
    __HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();


    //PA1,2,7
    GPIO_Initure.Pin=GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7;
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;
    GPIO_Initure.Pull=GPIO_NOPULL;
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;
    GPIO_Initure.Alternate=GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);

    //PC1,4,5
    GPIO_Initure.Pin=GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);

    //PB11
    GPIO_Initure.Pin=GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);

    //PG13,14
    GPIO_Initure.Pin=GPIO_PIN_13|GPIO_PIN_14;
    HAL_GPIO_Init(GPIOG,&GPIO_Initure);
#endif
    HAL_NVIC_SetPriority(ETH_IRQn,1,0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}


u32 LAN8720_ReadPHY(u16 reg)
{
    u32 regval;
    HAL_ETH_ReadPHYRegister(&ETH_Handler,reg,&regval);
    return regval;
}

void LAN8720_WritePHY(u16 reg,u16 value)
{
    u32 temp=value;
    HAL_ETH_ReadPHYRegister(&ETH_Handler,reg,&temp);
}

u8 LAN8720_Get_Speed(void)
{
	u8 speed;
	speed=((LAN8720_ReadPHY(31)&0x1C)>>2);
	return speed;
}
int dumphex(const unsigned char *buf, int size);

extern void lwip_packet_handler();
void ETH_IRQHandler(void)
{
	u32 ret = 0;
	s32 cnts = 50;
    while(ret = ETH_GetRxPktSize(ETH_Handler.RxDesc))
    {
//    	kprintf("ETH RECV:\r\n");
//		dumphex((unsigned char*)(ETH_Handler.RxDesc->Buffer1Addr), ret);
    	lwip_packet_handler();
    	if (cnts-- <= 0) break;
    }
    __HAL_ETH_DMA_CLEAR_IT(&ETH_Handler,ETH_DMA_IT_NIS);
    __HAL_ETH_DMA_CLEAR_IT(&ETH_Handler,ETH_DMA_IT_R);
}


u32  ETH_GetRxPktSize(ETH_DMADescTypeDef *DMARxDesc)
{
    u32 frameLength = 0;
    if(((DMARxDesc->Status&ETH_DMARXDESC_OWN)==(uint32_t)RESET) &&
     ((DMARxDesc->Status&ETH_DMARXDESC_ES)==(uint32_t)RESET) &&
     ((DMARxDesc->Status&ETH_DMARXDESC_LS)!=(uint32_t)RESET))
    {
        frameLength=((DMARxDesc->Status&ETH_DMARXDESC_FL)>>ETH_DMARXDESC_FRAME_LENGTHSHIFT);
    }
    return frameLength;
}
