/********************************************************************
	created:	2012/01/26
	file base:	Command
	file ext:	cpp
	author:		SYFP Team
	
	purpose:
*********************************************************************/

#include "define.h"
#include "command.h"

struct StFpCmdMgr gFpCmdMgr;

WORD GetResponse()
{
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	return pRcmPack->m_wRetCode;
}

PST_RCM_PACKET	RcmPacketPtr()
{
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	return pRcmPack;
}

/***************************************************************************/
/***************************************************************************/
WORD GetCheckSum(BOOL bCmdData)
{
	WORD	w_Ret = 0;
	
	//w_Ret = g_pCmdPacket->;

	return w_Ret;
}

/***************************************************************************/
/***************************************************************************/
BOOL CheckReceive( BYTE* p_pbyPacket, DWORD p_dwPacketLen, WORD p_wPrefix, WORD p_wCMDCode )
{
	int				i;
	WORD			w_wCalcCheckSum, w_wCheckSum;
	ST_RCM_PACKET*	w_pstRcmPacket;

	w_pstRcmPacket = (ST_RCM_PACKET*)p_pbyPacket;

	//. Check prefix code
 	if (p_wPrefix != w_pstRcmPacket->m_wPrefix)
 		return FALSE;
 	
	//. Check checksum
	w_wCheckSum = MAKEWORD(p_pbyPacket[p_dwPacketLen-2], p_pbyPacket[p_dwPacketLen-1]);
	w_wCalcCheckSum = 0;
	for (i=0; i<p_dwPacketLen-2; i++)
	{
		w_wCalcCheckSum = w_wCalcCheckSum + p_pbyPacket[i];
	}
	
	if (w_wCheckSum != w_wCalcCheckSum)
		return FALSE;
	
	if (p_wCMDCode != w_pstRcmPacket->m_wCMDCode)
	{
		return FALSE;
	}

	return TRUE;
}


/***************************************************************************/
/***************************************************************************/
void InitCmdPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen)
{
	int		i;
	WORD	w_wCheckSum;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_CMD_PACKET	pCmdPack = (PST_CMD_PACKET)(pFpCmdMgr->pack);

	memset( pFpCmdMgr->pack, 0, sizeof( pFpCmdMgr->pack ));
	pCmdPack->m_wPrefix = CMD_PREFIX_CODE;
	pCmdPack->m_bySrcDeviceID = p_bySrcDeviceID;
	pCmdPack->m_byDstDeviceID = p_byDstDeviceID;
	pCmdPack->m_wCMDCode = p_wCMDCode;
	pCmdPack->m_wDataLen = p_wDataLen;

	if (p_wDataLen)
		memcpy(pCmdPack->m_abyData, p_pbyData, p_wDataLen);

	w_wCheckSum = 0;

	for (i=0; i<sizeof(ST_CMD_PACKET)-2; i++)
	{
		w_wCheckSum = w_wCheckSum + pFpCmdMgr->pack[i];
	}

	pCmdPack->m_wCheckSum = w_wCheckSum;
	
	pFpCmdMgr->pack_size = sizeof(ST_CMD_PACKET);
}


/***************************************************************************/
/***************************************************************************/
void	InitCmdDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID, BYTE* p_pbyData, WORD p_wDataLen)
{
	int		i;
	WORD	w_wCheckSum;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_CMD_PACKET	pCmdPack = (PST_CMD_PACKET)(pFpCmdMgr->pack);
	pCmdPack->m_wPrefix = CMD_DATA_PREFIX_CODE;
	pCmdPack->m_bySrcDeviceID = p_bySrcDeviceID;
	pCmdPack->m_byDstDeviceID = p_byDstDeviceID;
	pCmdPack->m_wCMDCode = p_wCMDCode;
	pCmdPack->m_wDataLen = p_wDataLen;

	memcpy(&pCmdPack->m_abyData[0], p_pbyData, p_wDataLen);

	//. Set checksum
	w_wCheckSum = 0;
	
	for (i=0; i<(p_wDataLen + 8); i++)
	{
		w_wCheckSum = w_wCheckSum + pFpCmdMgr->pack[i];
	}

	pFpCmdMgr->pack[p_wDataLen+8] = LOBYTE(w_wCheckSum);
	pFpCmdMgr->pack[p_wDataLen+9] = HIBYTE(w_wCheckSum);

	pFpCmdMgr->pack_size = p_wDataLen + 10;
}



/***************************************************************************/
/***************************************************************************/
BOOL SendCommand(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nSendCnt = 0;
	LONG	w_nResult = 0;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;

	kprintf("send:\r\n");
	dumphex((s8*)(pFpCmdMgr->pack), pFpCmdMgr->pack_size);

	w_nResult = uart_sends(pFpCmdMgr->port, pFpCmdMgr->pack, pFpCmdMgr->pack_size, COMM_TIMEOUT);

	if (w_nResult <= 0) {
		return FALSE;
	}

	return ReceiveAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID);
}


/***************************************************************************/
/***************************************************************************/
BOOL ReceiveAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nAckCnt = 0;
	LONG	w_nResult = 0;
	DWORD	w_dwTimeOut = COMM_TIMEOUT;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_CMD_PACKET	pCmdPack = (PST_CMD_PACKET)(pFpCmdMgr->pack);
	if (p_wCMDCode == CMD_TEST_CONNECTION)
		w_dwTimeOut = 2000;
	else if (p_wCMDCode == CMD_ADJUST_SENSOR)
		w_dwTimeOut = 30000;
	
l_read_packet:

	//w_nResult = g_Serial.Read(pFpCmdMgr->pack, sizeof(ST_RCM_PACKET), &w_nAckCnt, NULL, w_dwTimeOut);
	if (!ReadDataN(pFpCmdMgr->pack, sizeof(ST_RCM_PACKET), w_dwTimeOut))
	{
		return FALSE;
	}

	pFpCmdMgr->pack_size = sizeof(ST_RCM_PACKET);


	if (!CheckReceive(pFpCmdMgr->pack, sizeof(ST_RCM_PACKET), RCM_PREFIX_CODE, p_wCMDCode))
		return FALSE;
	
	if (pCmdPack->m_byDstDeviceID != p_bySrcDeviceID)
	{
		goto l_read_packet;
	}

	return TRUE;
}


/***************************************************************************/
/***************************************************************************/
BOOL ReceiveDataAck(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nAckCnt = 0;
	LONG	w_nResult = 0;
	DWORD	w_dwTimeOut = COMM_TIMEOUT;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	//w_nResult = g_Serial.Read(pFpCmdMgr->pack, 8, &w_nAckCnt, NULL, w_dwTimeOut);
	
	if (!ReadDataN(pFpCmdMgr->pack, 8, w_dwTimeOut))
	{
		return FALSE;
	}
	
	//w_nResult = g_Serial.Read(pFpCmdMgr->pack+8, pRmdPack->m_wDataLen+2, &w_nAckCnt, NULL, w_dwTimeOut);
	if (!ReadDataN(pFpCmdMgr->pack+8, pRcmPack->m_wDataLen+2, w_dwTimeOut))
	{
		return FALSE;
	}

	pFpCmdMgr->pack_size = pRcmPack->m_wDataLen + 10;
	
	return CheckReceive(pFpCmdMgr->pack, 10 + pRcmPack->m_wDataLen, RCM_DATA_PREFIX_CODE, p_wCMDCode);
}


/***************************************************************************/
/***************************************************************************/
BOOL SendDataPacket(WORD p_wCMDCode, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	DWORD	w_nSendCnt = 0;
	LONG	w_nResult = 0;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;

	w_nResult = uart_sends(pFpCmdMgr->port, pFpCmdMgr->pack, pFpCmdMgr->pack_size, COMM_TIMEOUT);

	if (w_nResult <= 0) {
		return FALSE;
	}
	return ReceiveDataAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID);
}


/***************************************************************************/
/***************************************************************************/
BOOL ReceiveDataPacket(WORD	p_wCMDCode, BYTE p_byDataLen, BYTE p_bySrcDeviceID, BYTE p_byDstDeviceID)
{
	return ReceiveDataAck(p_wCMDCode, p_bySrcDeviceID, p_byDstDeviceID);
}
/***************************************************************************/
/***************************************************************************/


BOOL ReadDataN(BYTE* p_pData, int p_nLen, DWORD p_dwTimeOut)
{
	DWORD	w_nAckCnt = 0;
	LONG	w_nResult = 0;
	int		w_nRecvLen, w_nTotalRecvLen;

	w_nRecvLen = p_nLen;
	w_nTotalRecvLen = 0;

	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;

	u32 mark_time=0;
	mark_time = VOSGetTimeMs();
	while(w_nTotalRecvLen < p_nLen )
	{
		w_nResult = uart_recvs(pFpCmdMgr->port, &p_pData[w_nTotalRecvLen], w_nRecvLen, 100);
		if (w_nResult > 0) {
//			kprintf("recv:\r\n");
//			dumphex(&p_pData[w_nTotalRecvLen], w_nResult);
			w_nRecvLen = w_nRecvLen - w_nResult;
			w_nTotalRecvLen = w_nTotalRecvLen + w_nResult;
		}
		if (w_nRecvLen == 0) {//³É¹¦
			break;
		}
		if (VOSGetTimeMs()-mark_time >(u32)p_dwTimeOut) {
			return FALSE;
		}
	}

	return TRUE;
}


/***************************************************************************/
/***************************************************************************/
BYTE *GetErrorMsg(DWORD p_dwErrorCode)
{
	BYTE *w_ErrMsg = 0;

	switch(LOBYTE(p_dwErrorCode))
	{
	case ERROR_SUCCESS:
		w_ErrMsg = "Succcess";
		break;
	case ERR_VERIFY:
		w_ErrMsg = "Verify NG";
		break;
	case ERR_IDENTIFY:
		w_ErrMsg = "Identify NG";
		break;
	case ERR_EMPTY_ID_NOEXIST:
		w_ErrMsg = "Empty Template no Exist";
		break;
	case ERR_BROKEN_ID_NOEXIST:
		w_ErrMsg = "Broken Template no Exist";
		break;
	case ERR_TMPL_NOT_EMPTY:
		w_ErrMsg = "Template of this ID Already Exist";
		break;
	case ERR_TMPL_EMPTY:
		w_ErrMsg = "This Template is Already Empty";
		break;
	case ERR_INVALID_TMPL_NO:
		w_ErrMsg = "Invalid Template No";
		break;
	case ERR_ALL_TMPL_EMPTY:
		w_ErrMsg = "All Templates are Empty";
		break;
	case ERR_INVALID_TMPL_DATA:
		w_ErrMsg = "Invalid Template Data";
		break;
	case ERR_DUPLICATION_ID:
		w_ErrMsg = "Duplicated ID : ";
		break;
	case ERR_BAD_QUALITY:
		w_ErrMsg = "Bad Quality Image";
		break;
	case ERR_MERGE_FAIL:
		w_ErrMsg = "Merge failed";
		break;
	case ERR_NOT_AUTHORIZED:
		w_ErrMsg = "Device not authorized.";
		break;
	case ERR_MEMORY:
		w_ErrMsg = "Memory Error ";
		break;
	case ERR_INVALID_PARAM:
		w_ErrMsg = "Invalid Parameter";
		break;
	case ERR_GEN_COUNT:
		w_ErrMsg = "Generation Count is invalid";
		break;
	case ERR_INVALID_BUFFER_ID:
		w_ErrMsg = "Ram Buffer ID is invalid.";
		break;
	case ERR_INVALID_OPERATION_MODE:
		w_ErrMsg = "Invalid Operation Mode!";
		break;
	case ERR_FP_NOT_DETECTED:
		w_ErrMsg = "Finger is not detected.";
		break;
	default:
		//w_ErrMsg.Format("Fail, error code=%d"), p_dwErrorCode);
		w_ErrMsg = "Fail, error code=default!\r\n";
		break;
	}

	return w_ErrMsg;
}

