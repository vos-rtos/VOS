#ifndef __LAN8720_H
#define __LAN8720_H
#if 0
#include "stm32f4xx.h"
#include "stm32f4x7_eth.h"

#define LAN8720_PHY_ADDRESS  	0x00				//LAN8720 PHYоƬ��ַ.

extern ETH_DMADESCTypeDef  *DMATxDescToSet;			//DMA����������׷��ָ��
extern ETH_DMADESCTypeDef  *DMARxDescToGet; 		//DMA����������׷��ָ�� 
extern ETH_DMA_Rx_Frame_infos *DMA_RX_FRAME_infos;	//DMA�����յ���֡��Ϣָ��
 
u8 LAN8720_Init();
u8 LAN8720_Get_Speed();
u8 ETH_MACDMA_Config();
FrameTypeDef ETH_Rx_Packet();
u8 ETH_Tx_Packet(u16 FrameLength);
u32 ETH_GetCurrentTxBuffer();
#endif

#define ETH_MAC "\x08\x00\x00\x22\x33\x44"

#endif 

