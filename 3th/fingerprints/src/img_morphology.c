/*#############################################################################
 * �ļ�����img_morphology.c
 * ���ܣ�  ʵ������Ҫ��ͼ����̬ѧ����
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/


#include "img_base.h"

#include <string.h>


#define P(x,y)      p[(x)+(y)*pitch]

/******************************************************************************
  * ���ܣ�ͼ�������㷨
  * ������image   ָ��ͼ��
  * ���أ�������
******************************************************************************/
FvsError_t ImageDilate(FvsImage_t image)
{
    FvsInt_t w      = ImageGetWidth (image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsInt_t pitch  = ImageGetPitch (image);
    FvsInt_t size   = ImageGetSize  (image);
    FvsByte_t* p    = ImageGetBuffer(image);
    FvsInt_t x,y;

    if (p==NULL)
        return FvsMemory;
    
    for (y=1; y<h-1; y++)
    for (x=1; x<w-1; x++)
    {
        if (P(x,y)==0xFF)
        {
            P(x-1, y) |= 0x80;
            P(x+1, y) |= 0x80;
            P(x, y-1) |= 0x80;
            P(x, y+1) |= 0x80;
        }
    }

    for (y=0; y<size; y++)
        if (p[y])
            p[y] = 0xFF;

    return FvsOK;
}


/******************************************************************************
  * ���ܣ�ͼ��ʴ�㷨
  * ������image   ָ��ͼ��
  * ���أ�������
******************************************************************************/
FvsError_t ImageErode(FvsImage_t image)
{
    FvsInt_t w      = ImageGetWidth (image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsInt_t pitch  = ImageGetPitch (image);
    FvsInt_t size   = ImageGetSize  (image);
    FvsByte_t* p    = ImageGetBuffer(image);
    FvsInt_t x,y;
    
    if (p==NULL)
        return FvsMemory;
    
    for (y=1; y<h-1; y++)
    for (x=1; x<w-1; x++)
    {
        if (P(x,y)==0x0)
        {
            P(x-1, y) &= 0x80;
            P(x+1, y) &= 0x80;
            P(x, y-1) &= 0x80;
            P(x, y+1) &= 0x80;
        }
    }

    for (y=0; y<size; y++)
        if (p[y]!=0xFF)
            p[y] = 0x0;

    return FvsOK;
}


