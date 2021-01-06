#ifndef __IDWD1016B_H__
#define __IDWD1016B_H__

#include "command.h"
#include "define.h"
 
extern int m_nConnectionMode;
extern BYTE	m_bySrcDeviceID;
extern BYTE	m_byDstDeviceID;

	
int		Run_TestConnection(void);
int		Run_SetParam(int p_nParamIndex, int p_nParamValue);
int		Run_GetParam(int p_nParamIndex, int* p_pnParamValue);
int 	Run_GetDeviceInfo(char* p_pszDevInfo);
int		Run_SetIDNote(int p_nTmplNo, BYTE* pNote);
int		Run_GetIDNote(int p_nTmplNo, BYTE* pNote);
int		Run_SetModuleSN(BYTE* pModuleSN);
int		Run_GetModuleSN(BYTE* pModuleSN);
int		Run_SetDevPass(BYTE* pDevPass);
int		Run_VerfiyDevPass(BYTE* pDevPass);
int		Run_EnterStandbyState(void);
int		Run_UpgradeFirmware(BYTE* p_pData, DWORD p_nSize);

int		Run_GetImage(void);
int		Run_FingerDetect(int* p_pnDetectResult);
int		Run_UpImage(int p_nType, BYTE* p_pData, int* p_pnImgWidth, int* p_pnImgHeight);
int		Run_DownImage(BYTE* p_pData, int p_nWidth, int p_nHeight);
int		Run_SLEDControl(int p_nState);
int		Run_AdjustSensor(void);

int		Run_StoreChar(int p_nTmplNo, int p_nRamBufferID, int* p_pnDupTmplNo);
int		Run_LoadChar(int p_nTmplNo, int p_nRamBufferID);
int		Run_UpChar(int p_nRamBufferID, BYTE* p_pbyTemplate);
int		Run_DownChar(int p_nRamBufferID, BYTE* p_pbyTemplate);

int 	Run_DelChar(int p_nSTmplNo, int p_nETmplNo);
int		Run_GetEmptyID(int p_nSTmplNo, int p_nETmplNo, int* p_pnEmptyID);
int		Run_GetStatus(int p_nTmplNo, int* p_pnStatus);
int		Run_GetBrokenID(int p_nSTmplNo, int p_nETmplNo, int* p_pnCount, int* p_pnFirstID);
int		Run_GetEnrollCount(int p_nSTmplNo, int p_nETmplNo, int* p_pnEnrollCount);
int		Run_GetEnrolledIDList(int* p_pnCount, int* p_pnIDs);

int		Run_Generate(int p_nRamBufferID);
int		Run_Merge(int p_nRamBufferID, int p_nMergeCount);
int		Run_Match(int p_nRamBufferID0, int p_nRamBufferID1, int* p_pnLearnResult);
int		Run_Search(int p_nRamBufferID, int p_nStartID, int p_nSearchCount, int* p_pnTmplNo, int* p_pnLearnResult);
int		Run_Verify(int p_nTmplNo, int p_nRamBufferID, int* p_pnLearnResult);

int 	InitConnection(int ComPort, int p_nBaudRate);

BOOL	EnableCommunicaton(int p_nDevNum, BOOL p_bVerifyDeviceID, BYTE* p_pDevPwd, BOOL p_bMsgOut);
BOOL	OpenSerialPort(int port, int p_nBaudRateIndex);
BOOL	Run_Command_NP( WORD p_wCMD );

void	CloseConnection();
void	SetIPandPort(char *strDestination, DWORD dwPort);
#endif
