/*#############################################################################
 * �ļ�����fvs.h
 * ���ܣ�  �ṩ��һ��ָ��ʶ��Ŀ��
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#if !defined FVS__GLOBAL_HEADER__INCLUDED__
#define FVS__GLOBAL_HEADER__INCLUDED__


/**********          Fingerprint Verification System - FVS         ************
  * �ṩ���û��ӿڣ����Դ�ָ���ļ�������ָ��ͼ�񲢷�����
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* �����Զ������ļ� */
#include "config.h"

/* �������Ͷ����ļ� */
#include "fvstypes.h"

/* �ļ�����/��� */
#include "import.h"		/* �Զ���������ļ������� */
#include "export.h"		/* ��ָ��ͼ��������ļ�   */

/* ͼ����� */
#include "image.h"

/* ��������� */
#include "floatfield.h"

/* �ļ����� */
#include "file.h"

/* ����ϸ�� */
#include "minutia.h"

/* ֱ��ͼ���� */
#include "histogram.h"

/* ͼ������� */
#include "imagemanip.h"

/* ƥ���㷨 */
#include "matching.h"

/* �汾 */
const FvsString_t FvsGetVersion(void);


#endif /* FVS__GLOBAL_HEADER__INCLUDED__*/


