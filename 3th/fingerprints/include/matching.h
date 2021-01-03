/*#############################################################################
 * �ļ�����matching.h
 * ���ܣ�  ʵ����ָ��ƥ���㷨
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#if !defined FVS__MATCHING_HEADER__INCLUDED__
#define FVS__MATCHING_HEADER__INCLUDED__

#include "image.h"
#include "minutia.h"


/******************************************************************************
  * ���ܣ�ƥ������ָ��
  * ������image1      ָ��ͼ��1
  *       image2      ָ��ͼ��2
  *       pgoodness   ƥ��ȣ�Խ��Խ��
  * ���أ�������
******************************************************************************/
FvsError_t MatchingCompareImages(const FvsImage_t image1,
                                 const FvsImage_t image2,
                                 FvsInt_t* pgoodness);


/******************************************************************************
  * ���ܣ�ƥ��ָ��ϸ�ڵ�
  * ������minutia1      ϸ�ڵ㼯��1
  *       minutia2      ϸ�ڵ㼯��2
  *       pgoodness   ƥ��ȣ�Խ��Խ��
  * ���أ�������
******************************************************************************/
FvsError_t MatchingCompareMinutiaSets(const FvsMinutiaSet_t minutia1,
                                      const FvsMinutiaSet_t minutia2,
                                      FvsInt_t* pgoodness);


#endif /* __MATCHING_HEADER__INCLUDED__ */

