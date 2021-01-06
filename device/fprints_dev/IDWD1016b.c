#include "IDWD1016b.h"
#include "command.h"

BYTE	m_bySrcDeviceID;
BYTE	m_byDstDeviceID;

DWORD	m_dwPort;
char *m_strDestIp;

extern int g_nMaxFpCount;

int BAUDRATES[] = {9600,19200,38400,57600,115200,230400,460800,921600};


/************************************************************************/
/************************************************************************/
int	Run_TestConnection(void)
{
	BOOL	w_bRet;

	InitCmdPacket(CMD_TEST_CONNECTION, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);
	
	SEND_COMMAND(CMD_TEST_CONNECTION, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}

/************************************************************************/
/************************************************************************/
int	Run_SetParam(int p_nParamIndex, int p_nParamValue)
{
	BOOL	w_bRet;
	BYTE	w_abyData[5];
	
	w_abyData[0] = p_nParamIndex;
	memcpy(&w_abyData[1], &p_nParamValue, 4);
	
	InitCmdPacket(CMD_SET_PARAM, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 5);
 	 	
	SEND_COMMAND(CMD_SET_PARAM, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}


/************************************************************************/
/************************************************************************/
int	Run_GetParam(int p_nParamIndex, int* p_pnParamValue)
{
	BOOL	w_bRet;
	BYTE	w_byData;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_byData = p_nParamIndex;
	
	InitCmdPacket(CMD_GET_PARAM, m_bySrcDeviceID, m_byDstDeviceID, &w_byData, 1);
 	 	
	SEND_COMMAND(CMD_GET_PARAM, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(p_pnParamValue, pRcmPack->m_abyData, 4);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int Run_GetDeviceInfo(char* p_szDevInfo)
{
	BOOL	w_bRet;
	WORD	w_wDevInfoLen;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_bRet = Run_Command_NP(CMD_GET_DEVICE_INFO);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;	
	
	w_wDevInfoLen = MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);
	
	RECEIVE_DATAPACKET(CMD_GET_DEVICE_INFO, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if ( RESPONSE_RET != ERR_SUCCESS )	
		return RESPONSE_RET;
	
	memcpy(p_szDevInfo, pRcmPack->m_abyData, w_wDevInfoLen);
	
	return ERR_SUCCESS;
}


/***************************************************************************/
/***************************************************************************/
int	Run_SetIDNote(int p_nTmplNo, BYTE* pNote)
{
	BOOL	w_bRet = FALSE;
	BYTE	w_abyData[ID_NOTE_SIZE+2];
	WORD	w_wData;
	
	//. Assemble command packet
	w_wData = ID_NOTE_SIZE + 2;
	InitCmdPacket(CMD_SET_ID_NOTE, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_wData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_SET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if( RESPONSE_RET != ERR_SUCCESS)	
		return RESPONSE_RET;
	
	Sleep(10);
	
	//. Assemble data packet
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	memcpy(&w_abyData[2], pNote, ID_NOTE_SIZE);
	
	InitCmdDataPacket(CMD_SET_ID_NOTE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, ID_NOTE_SIZE+2);
	
	//. Send data packet to target
	SEND_DATAPACKET(CMD_SET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == FALSE)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	Run_GetIDNote(int p_nTmplNo, BYTE* pNote)
{
	BOOL		w_bRet = FALSE;
	BYTE		w_abyData[2];
	int			w_nTemplateNo = 0;
	WORD		w_nCmdCks = 0, w_nSize = 0;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	//. Assemble command packet
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	InitCmdPacket(CMD_GET_ID_NOTE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_GET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	RECEIVE_DATAPACKET(CMD_GET_ID_NOTE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(pNote, &pRcmPack->m_abyData[0], ID_NOTE_SIZE);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	Run_SetModuleSN(BYTE* pModuleSN)
{
	BOOL	w_bRet = FALSE;
	BYTE	w_abyData[MODULE_SN_LEN];
	WORD	w_wData;
	
	//. Assemble command packet
	w_wData = MODULE_SN_LEN;
	InitCmdPacket(CMD_SET_MODULE_SN, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_wData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_SET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if( RESPONSE_RET != ERR_SUCCESS)	
		return RESPONSE_RET;
	
	Sleep(10);
	
	//. Assemble data packet
	memcpy(&w_abyData[0], pModuleSN, MODULE_SN_LEN);
	
	InitCmdDataPacket(CMD_SET_MODULE_SN, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, MODULE_SN_LEN);
	
	//. Send data packet to target
	SEND_DATAPACKET(CMD_SET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == FALSE)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	Run_GetModuleSN(BYTE* pModuleSN)
{
	BOOL		w_bRet = FALSE;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	//. Assemble command packet
	InitCmdPacket(CMD_GET_MODULE_SN, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_GET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	RECEIVE_DATAPACKET(CMD_GET_MODULE_SN, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(pModuleSN, &pRcmPack->m_abyData[0], MODULE_SN_LEN);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	Run_SetDevPass(BYTE* pDevPass)
{
	BOOL	w_bRet;

	InitCmdPacket(CMD_SET_DEVPASS, m_bySrcDeviceID, m_byDstDeviceID, pDevPass, MAX_DEVPASS_LEN);

	SEND_COMMAND(CMD_SET_DEVPASS, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	if (!w_bRet)
		return ERR_CONNECTION;

	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	Run_VerfiyDevPass(BYTE* pDevPass)
{
	BOOL	w_bRet;

	InitCmdPacket(CMD_VERIFY_DEVPASS, m_bySrcDeviceID, m_byDstDeviceID, pDevPass, MAX_DEVPASS_LEN);

	SEND_COMMAND(CMD_VERIFY_DEVPASS, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	if (!w_bRet)
		return ERR_CONNECTION;

	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	Run_EnterStandbyState(void)
{
	BOOL	w_bRet;

	InitCmdPacket(CMD_ENTER_STANDY_STATE, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);

	SEND_COMMAND(CMD_ENTER_STANDY_STATE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	if (!w_bRet)
		return ERR_CONNECTION;

	return RESPONSE_RET;
}

/************************************************************************/
/************************************************************************/
#define DOWN_FW_DATA_UINT	512
int	Run_UpgradeFirmware(BYTE* p_pData, DWORD p_nSize)
{
	int		i, n, r;
	BOOL	w_bRet;
	BYTE	w_abyData[840];

	w_abyData[0] = LOBYTE(LOWORD(p_nSize));
	w_abyData[1] = HIBYTE(LOWORD(p_nSize));
	w_abyData[2] = LOBYTE(HIWORD(p_nSize));
	w_abyData[3] = HIBYTE(HIWORD(p_nSize));

	//. Assemble command packet
	InitCmdPacket(CMD_UPGRADE_FIRMWARE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);

	SEND_COMMAND(CMD_UPGRADE_FIRMWARE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	if(!w_bRet)
		return ERR_CONNECTION;

	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;

	n = p_nSize/DOWN_FW_DATA_UINT;
	r = p_nSize%DOWN_FW_DATA_UINT;


	for (i=0; i<n; i++)
	{
		memcpy(&w_abyData[0], &p_pData[i*DOWN_FW_DATA_UINT], DOWN_FW_DATA_UINT);

		InitCmdDataPacket(CMD_UPGRADE_FIRMWARE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, DOWN_FW_DATA_UINT);

		SEND_DATAPACKET(CMD_UPGRADE_FIRMWARE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

		if (w_bRet == FALSE)
			return ERR_CONNECTION;

		if (RESPONSE_RET != ERR_SUCCESS)
			return RESPONSE_RET;

		Sleep(6);
	}

	if (r > 0)
	{
		memcpy(&w_abyData[0], &p_pData[i*DOWN_FW_DATA_UINT], r);

		InitCmdDataPacket(CMD_UPGRADE_FIRMWARE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, r);

		SEND_DATAPACKET(CMD_UPGRADE_FIRMWARE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

		if (w_bRet == FALSE)
			return ERR_CONNECTION;

		if (RESPONSE_RET != ERR_SUCCESS)
			return RESPONSE_RET;

	}

	return ERR_SUCCESS;	
}

/************************************************************************/
/************************************************************************/
int	Run_GetImage(void)
{
	BOOL	w_bRet;
	
	w_bRet = Run_Command_NP(CMD_GET_IMAGE);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	Run_FingerDetect(int* p_pnDetectResult)
{
	BOOL	w_bRet;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_bRet = Run_Command_NP(CMD_FINGER_DETECT);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnDetectResult = pRcmPack->m_abyData[0];
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
#define IMAGE_DATA_UINT	496
int	Run_UpImage(int p_nType, BYTE* p_pFpData, int* p_pnImgWidth, int* p_pnImgHeight)
{
	int		i, n, r, w, h, size;
	BOOL	w_bRet;
	BYTE	w_byData;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_byData = p_nType;

	InitCmdPacket(CMD_UP_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, &w_byData, 1);
	
	SEND_COMMAND(CMD_UP_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	w = MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);
	h = MAKEWORD(pRcmPack->m_abyData[2], pRcmPack->m_abyData[3]);
	
	size = w*h;

	n = (size)/IMAGE_DATA_UINT;
	r = (size)%IMAGE_DATA_UINT;


	for (i=0; i<n; i++)
	{
		RECEIVE_DATAPACKET(CMD_UP_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

		if(w_bRet == FALSE)
			return ERR_CONNECTION;

		if (RESPONSE_RET != ERR_SUCCESS)
			return RESPONSE_RET;

		memcpy(&p_pFpData[i*IMAGE_DATA_UINT], &pRcmPack->m_abyData[2], IMAGE_DATA_UINT);

	}

	if (r > 0)
	{
		RECEIVE_DATAPACKET(CMD_UP_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

		if(w_bRet == FALSE)
			return ERR_CONNECTION;

		if (RESPONSE_RET != ERR_SUCCESS)
			return RESPONSE_RET;

		memcpy(&p_pFpData[i*IMAGE_DATA_UINT], &pRcmPack->m_abyData[2], r);
		
	}

	*p_pnImgWidth = w;
	*p_pnImgHeight = h;
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
#define DOWN_IMAGE_DATA_UINT	496
int	Run_DownImage(BYTE* p_pData, int p_nWidth, int p_nHeight)
{
	int		i, n, r, w, h;
	BOOL	w_bRet;
	BYTE	w_abyData[840];
	
	w = p_nWidth;
	h = p_nHeight;

	w_abyData[0] = LOBYTE(p_nWidth);
	w_abyData[1] = HIBYTE(p_nWidth);
	w_abyData[2] = LOBYTE(p_nHeight);
	w_abyData[3] = HIBYTE(p_nHeight);

	//. Assemble command packet
	InitCmdPacket(CMD_DOWN_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_DOWN_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	n = (w*h)/DOWN_IMAGE_DATA_UINT;
	r = (w*h)%DOWN_IMAGE_DATA_UINT;

	for (i=0; i<n; i++)
	{
		w_abyData[0] = LOBYTE(i);
		w_abyData[1] = HIBYTE(i);
		memcpy(&w_abyData[2], &p_pData[i*DOWN_IMAGE_DATA_UINT], DOWN_IMAGE_DATA_UINT);

		InitCmdDataPacket(CMD_DOWN_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2+DOWN_IMAGE_DATA_UINT);

		SEND_DATAPACKET(CMD_DOWN_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

		if (w_bRet == FALSE)
			return ERR_CONNECTION;

		if (RESPONSE_RET != ERR_SUCCESS)
			return RESPONSE_RET;

		Sleep(6);
	}

	if (r > 0)
	{
		w_abyData[0] = LOBYTE(i);
		w_abyData[1] = HIBYTE(i);
		memcpy(&w_abyData[2], &p_pData[i*DOWN_IMAGE_DATA_UINT], r);

		InitCmdDataPacket(CMD_DOWN_IMAGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2+r);

		SEND_DATAPACKET(CMD_DOWN_IMAGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

		if (w_bRet == FALSE)
			return ERR_CONNECTION;

		if (RESPONSE_RET != ERR_SUCCESS)
			return RESPONSE_RET;
		
	}

	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	Run_SLEDControl(int p_nState)
{
	BOOL	w_bRet;
	BYTE	w_abyData[2];
	
	w_abyData[0] = LOBYTE(p_nState);
	w_abyData[1] = HIBYTE(p_nState);
	
	InitCmdPacket(CMD_SLED_CTRL, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_abyData, 2);
	
	SEND_COMMAND(CMD_SLED_CTRL, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	Run_AdjustSensor(void)
{
	BOOL	w_bRet;

	InitCmdPacket(CMD_ADJUST_SENSOR, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);

	SEND_COMMAND(CMD_ADJUST_SENSOR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	if(!w_bRet)
		return ERR_CONNECTION;

	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	Run_StoreChar(int p_nTmplNo, int p_nRamBufferID, int* p_pnDupTmplNo)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	w_abyData[2] = LOBYTE(p_nRamBufferID);
	w_abyData[3] = HIBYTE(p_nRamBufferID);
	
	InitCmdPacket(CMD_STORE_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_STORE_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
	{
		if (RESPONSE_RET == ERR_DUPLICATION_ID)
			*p_pnDupTmplNo = MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);

		return RESPONSE_RET;
	}
	
	return RESPONSE_RET;
}

/************************************************************************/
/************************************************************************/
int	Run_LoadChar(int p_nTmplNo, int p_nRamBufferID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	w_abyData[2] = LOBYTE(p_nRamBufferID);
	w_abyData[3] = HIBYTE(p_nRamBufferID);
	
	InitCmdPacket(CMD_LOAD_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_LOAD_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	Run_UpChar(int p_nRamBufferID, BYTE* p_pbyTemplate)
{	
	BOOL		w_bRet = FALSE;
	BYTE		w_abyData[2];
	int			w_nTemplateNo = 0;
	WORD		w_nCmdCks = 0, w_nSize = 0;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	//. Assemble command packet
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	InitCmdPacket(CMD_UP_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_UP_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	RECEIVE_DATAPACKET(CMD_UP_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	memcpy(p_pbyTemplate, &pRcmPack->m_abyData[0], GD_RECORD_SIZE);
	
	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	Run_DownChar(int p_nRamBufferID, BYTE* p_pbyTemplate)
{
	BOOL	w_bRet = FALSE;
	BYTE	w_abyData[GD_RECORD_SIZE+2];
	WORD	w_wData;
	
	//. Assemble command packet
	w_wData = GD_RECORD_SIZE + 2;
	InitCmdPacket(CMD_DOWN_CHAR, m_bySrcDeviceID, m_byDstDeviceID, (BYTE*)&w_wData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_DOWN_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(w_bRet == FALSE)
		return ERR_CONNECTION;
	
	if( RESPONSE_RET != ERR_SUCCESS)	
		return RESPONSE_RET;
	
	Sleep(10);
	
	//. Assemble data packet
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	memcpy(&w_abyData[2], p_pbyTemplate, GD_RECORD_SIZE);
		
	InitCmdDataPacket(CMD_DOWN_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, GD_RECORD_SIZE+2);
	
	//. Send data packet to target
	SEND_DATAPACKET(CMD_DOWN_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (w_bRet == FALSE)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int Run_DelChar(int p_nSTmplNo, int p_nETmplNo)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	
	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);
	
	//. Assemble command packet
	InitCmdPacket(CMD_DEL_CHAR, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_DEL_CHAR, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	Run_GetEmptyID(int p_nSTmplNo, int p_nETmplNo, int* p_pnEmptyID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);

	//. Assemble command packet
	InitCmdPacket(CMD_GET_EMPTY_ID, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_GET_EMPTY_ID, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if ( RESPONSE_RET != ERR_SUCCESS )	
		return RESPONSE_RET;
	
	*p_pnEmptyID = MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	Run_GetStatus(int p_nTmplNo, int* p_pnStatus)
{
	BOOL	w_bRet;
	BYTE	w_abyData[2];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	
	InitCmdPacket(CMD_GET_STATUS, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	SEND_COMMAND(CMD_GET_STATUS, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if ( RESPONSE_RET != ERR_SUCCESS )	
		return RESPONSE_RET;
	
	*p_pnStatus = pRcmPack->m_abyData[0];
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	Run_GetBrokenID(int p_nSTmplNo, int p_nETmplNo, int* p_pnCount, int* p_pnFirstID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);

	//. Assemble command packet
	InitCmdPacket(CMD_GET_BROKEN_ID, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_GET_BROKEN_ID, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet)
		return ERR_CONNECTION;
	
	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnCount		= MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);
	*p_pnFirstID	= MAKEWORD(pRcmPack->m_abyData[2], pRcmPack->m_abyData[3]);
	
	return ERR_SUCCESS;	
}
/************************************************************************/
/************************************************************************/
int	Run_GetEnrollCount(int p_nSTmplNo, int p_nETmplNo, int* p_pnEnrollCount)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nSTmplNo);
	w_abyData[1] = HIBYTE(p_nSTmplNo);
	w_abyData[2] = LOBYTE(p_nETmplNo);
	w_abyData[3] = HIBYTE(p_nETmplNo);
	
	//. Assemble command packet
	InitCmdPacket(CMD_GET_ENROLL_COUNT, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_GET_ENROLL_COUNT, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if(RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnEnrollCount = MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);
	
	return ERR_SUCCESS;
}

/************************************************************************/
/************************************************************************/
#define 	VALID_FLAG_BIT_CHECK(A, V)		(A[0 + V/8] & (0x01 << (V & 0x07)))
int	Run_GetEnrolledIDList(int* p_pnCount, int* p_pnIDs)
{
	int			i, w_nValidBufSize;
	BOOL		w_bRet = FALSE;
	BYTE*		w_pValidBuf;
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	InitCmdPacket(CMD_GET_ENROLLED_ID_LIST, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);

	//. Send command packet to target
	SEND_COMMAND(CMD_GET_ENROLLED_ID_LIST, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	if(w_bRet == FALSE)
		return ERR_CONNECTION;

	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;

	w_nValidBufSize = MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);

	RECEIVE_DATAPACKET(CMD_GET_ENROLLED_ID_LIST, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	if (w_bRet == FALSE)
		return ERR_CONNECTION;

	if (RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	w_pValidBuf = vmalloc(w_nValidBufSize);

	memcpy(w_pValidBuf, &pRcmPack->m_abyData[0], w_nValidBufSize);

	*p_pnCount = 0;
	for (i=1; i<(w_nValidBufSize*8); i++)
	{
		if (VALID_FLAG_BIT_CHECK(w_pValidBuf, i) != 0)
		{
			p_pnIDs[*p_pnCount] = i;
			*p_pnCount = *p_pnCount + 1;
		}
		if (i >= g_nMaxFpCount)
			break;
	}

	vfree(w_pValidBuf);

	return ERR_SUCCESS;
}
/************************************************************************/
/************************************************************************/
int	Run_Generate(int p_nRamBufferID)
{
	BOOL	w_bRet;
	BYTE	w_abyData[2];
	
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	
	InitCmdPacket(CMD_GENERATE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 2);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_GENERATE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if(!w_bRet) 
		return ERR_CONNECTION;
	
	return RESPONSE_RET;		
}
/************************************************************************/
/************************************************************************/
int	Run_Merge(int p_nRamBufferID, int p_nMergeCount)
{
	BOOL	w_bRet;
	BYTE	w_abyData[3];
	
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	w_abyData[2] = p_nMergeCount;
	
	InitCmdPacket(CMD_MERGE, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 3);
	
	SEND_COMMAND(CMD_MERGE, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet) 
		return ERR_CONNECTION;
	
	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	Run_Match(int p_nRamBufferID0, int p_nRamBufferID1, int* p_pnLearnResult)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nRamBufferID0);
	w_abyData[1] = HIBYTE(p_nRamBufferID0);
	w_abyData[2] = LOBYTE(p_nRamBufferID1);
	w_abyData[3] = HIBYTE(p_nRamBufferID1);
	
	InitCmdPacket(CMD_MATCH, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_MATCH, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet) 
		return ERR_CONNECTION;

	if(RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;

	*p_pnLearnResult	= pRcmPack->m_abyData[0];

	return RESPONSE_RET;	
}
/************************************************************************/
/************************************************************************/
int	Run_Search(int p_nRamBufferID, int p_nStartID, int p_nSearchCount, int* p_pnTmplNo, int* p_pnLearnResult)
{
	BOOL	w_bRet;
	BYTE	w_abyData[6];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nRamBufferID);
	w_abyData[1] = HIBYTE(p_nRamBufferID);
	w_abyData[2] = LOBYTE(p_nStartID);
	w_abyData[3] = HIBYTE(p_nStartID);
	w_abyData[4] = LOBYTE(p_nSearchCount);
	w_abyData[5] = HIBYTE(p_nSearchCount);
	
	InitCmdPacket(CMD_SEARCH, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 6);
	
	//. Send command packet to target
	SEND_COMMAND(CMD_SEARCH, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	if(RESPONSE_RET != ERR_SUCCESS)
		return RESPONSE_RET;
	
	*p_pnTmplNo			= MAKEWORD(pRcmPack->m_abyData[0], pRcmPack->m_abyData[1]);
	*p_pnLearnResult	= pRcmPack->m_abyData[2];
	
	return RESPONSE_RET;
}
/************************************************************************/
/************************************************************************/
int	Run_Verify(int p_nTmplNo, int p_nRamBufferID, int* p_pnLearnResult)
{
	BOOL	w_bRet;
	BYTE	w_abyData[4];
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	PST_RCM_PACKET	pRcmPack = (PST_RCM_PACKET)(pFpCmdMgr->pack);
	w_abyData[0] = LOBYTE(p_nTmplNo);
	w_abyData[1] = HIBYTE(p_nTmplNo);
	w_abyData[2] = LOBYTE(p_nRamBufferID);
	w_abyData[3] = HIBYTE(p_nRamBufferID);
	
	//. Assemble command packet
	InitCmdPacket(CMD_VERIFY, m_bySrcDeviceID, m_byDstDeviceID, w_abyData, 4);
	
	SEND_COMMAND(CMD_VERIFY, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);
	
	if (!w_bRet)
		return ERR_CONNECTION;
	
	*p_pnLearnResult = pRcmPack->m_abyData[2];
	
	return RESPONSE_RET;
}


/***************************************************************************/
/***************************************************************************/
BOOL Run_Command_NP( WORD p_wCMD )
{
	BOOL w_bRet;

	//. Assemble command packet
	InitCmdPacket(p_wCMD, m_bySrcDeviceID, m_byDstDeviceID, NULL, 0);

 	SEND_COMMAND(p_wCMD, w_bRet, m_bySrcDeviceID, m_byDstDeviceID);

	return w_bRet;
}


/***************************************************************************/
/***************************************************************************/

BOOL OpenSerialPort(int port, int BaudRate)
{
	LONG    w_lRetCode = ERROR_SUCCESS;
	w_lRetCode = uart_open(port, BaudRate, 8, "none", 1);

	return TRUE;
}
/************************************************************************
*      Test Connection with Target
************************************************************************/
BOOL EnableCommunicaton(int p_nDeviceID, BOOL p_bVerifyDeviceID, BYTE* p_pDevPwd, BOOL p_bMsgOut)
{
	return TRUE;
}

int InitConnection(int ComPort, int BaudRate)
{
 	BOOL	w_blRet = FALSE;

	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;

	pFpCmdMgr->port = ComPort;

	w_blRet = OpenSerialPort(ComPort, BaudRate);

	if( w_blRet != TRUE )
	{
		CloseConnection();
		return ERR_COM_OPEN_FAIL;
	}

	return CONNECTION_SUCCESS;
}

/***************************************************************************/
/***************************************************************************/

void CloseConnection()
{
	struct StFpCmdMgr *pFpCmdMgr = &gFpCmdMgr;
	uart_close(pFpCmdMgr->port);
}
/***************************************************************************/
/***************************************************************************/
void SetIPandPort(char *strDestination, DWORD dwPort)
{
	m_strDestIp = strDestination;
	m_dwPort	= dwPort;		 
}

