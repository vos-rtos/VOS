#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "define.h"
#include "usart.h"

#define __packed __attribute__((packed))

#define		COMM_TIMEOUT					5000

extern struct StFpCmdMgr gFpCmdMgr;

typedef struct StFpCmdMgr {
	int port;
	int baudrate;
	int pack_size;
	unsigned char pack[1024*6/*1024*64*/];
}StFpCmdMgr;

typedef struct _ST_CMD_PACKET_
{
	WORD	m_wPrefix;
	BYTE	m_bySrcDeviceID;
	BYTE	m_byDstDeviceID;
	WORD	m_wCMDCode;
	WORD	m_wDataLen;
	BYTE	m_abyData[16];
	WORD	m_wCheckSum;
}__packed ST_CMD_PACKET, *PST_CMD_PACKET;

typedef struct _ST_RCM_PACKET_
{
	WORD	m_wPrefix;
	BYTE	m_bySrcDeviceID;
	BYTE	m_byDstDeviceID;
	WORD	m_wCMDCode;
	WORD	m_wDataLen;
	WORD	m_wRetCode;
	BYTE	m_abyData[14];
	WORD	m_wCheckSum;
} ST_RCM_PACKET, *PST_RCM_PACKET;

typedef struct _ST_COMMAND_
{
	char	szCommandName[64];
	WORD	wCode;
}__packed ST_COMMAND, *PST_COMMAND;


#define	RESPONSE_RET				GetResponse()
#define	CMD_PACKET_LEN				(sizeof(ST_CMD_PACKET))
#define	DATA_PACKET_LEN				(sizeof(ST_RCM_PACKET))

#define	MAX_DATA_LEN				512
/////////////////////////////	Value	/////////////////////////////////////////////

//extern BYTE				g_Packet[1024*64];
//extern DWORD			g_dwPacketSize;
//extern PST_CMD_PACKET	g_pCmdPacket;
//extern PST_RCM_PACKET	g_pRcmPacket;
//extern ST_COMMAND		g_Commands[];

/////////////////////////////	Function	/////////////////////////////////////////////
#define SEND_COMMAND(cmd, ret, nSrcDeviceID, nDstDeviceID)									\
		ret = SendCommand(cmd, nSrcDeviceID, nDstDeviceID);									\

#define SEND_DATAPACKET(cmd, ret, nSrcDeviceID, nDstDeviceID)								\
		ret = SendDataPacket(cmd, nSrcDeviceID, nDstDeviceID);								\

#define RECEIVE_DATAPACKET(cmd, ret, nSrcDeviceID, nDstDeviceID)						\
		ret = ReceiveDataPacket(cmd, 0, nSrcDeviceID, nDstDeviceID);						\

WORD	GetCheckSum(BOOL bCmdData);
BOOL	CheckReceive(BYTE* p_pbyPacket, DWORD p_dwPacketLen, WORD p_wPrefix, WORD p_wCMDCode);
void	InitCmdPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen);
void	InitCmdDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen);
BOOL	SendCommand(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);
BOOL	ReceiveAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);

BOOL	SendDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);
BOOL	ReceiveDataAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);
BOOL	ReceiveDataPacket(WORD p_wCMDCode, BYTE p_byDataLen, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID);
BOOL	ReadDataN(BYTE* p_pData, int p_nLen, DWORD p_dwTimeOut);

BYTE *GetErrorMsg(DWORD p_dwErrorCode);

PST_RCM_PACKET	RcmPacketPtr();

#endif
