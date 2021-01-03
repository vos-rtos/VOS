/*#############################################################################
 * �ļ�����export.h
 * ���ܣ�  ָ��ͼ���������
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#if !defined FVS__EXPORT_HEADER__INCLUDED__
#define FVS__EXPORT_HEADER__INCLUDED__

#include "file.h"
#include "image.h"

/******************************************************************************
  * ���ܣ���һ��ָ��ͼ�������һ���ļ����ļ��ĸ�ʽ���ļ�����չ������
  * ������filename  ��Ҫ����ͼ����ļ���
  *       image     ��Ҫ������ͼ��
  * ���أ��������
******************************************************************************/
FvsError_t FvsImageExport(const FvsImage_t image, const FvsFile_t filename);

#endif /* FVS__EXPORT_HEADER__INCLUDED__ */

