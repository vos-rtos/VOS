/*#############################################################################
 * �ļ�����import.h
 * ���ܣ�  ����ͼ��
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#if !defined FVS__IMPORT_HEADER__INCLUDED__
#define FVS__IMPORT_HEADER__INCLUDED__

#include "file.h"
#include "image.h"


/******************************************************************************
  * ���ܣ����ļ��м���ָ��ͼ��
  * ������image       ָ��ͼ��
  *       filename    �ļ���
  * ���أ�������
******************************************************************************/
FvsError_t FvsImageImport(FvsImage_t image, const FvsString_t filename);


#endif /* FVS__IMPORT_HEADER__INCLUDED__ */

