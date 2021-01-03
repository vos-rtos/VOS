/*#############################################################################
 * �ļ�����minutia.c
 * ���ܣ�  ϸ�ڵ��һЩ�����ӿ�
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "minutia.h"


typedef struct iFvsMinutiaSet_t
{
    FvsInt_t     nbminutia; 
    FvsInt_t     tablesize;
    FvsMinutia_t ptable[1]; 
} iFvsMinutiaSet_t;


/******************************************************************************
  * ���ܣ�����һ��ϸ�ڵ㼯��
  * ������size  ���ϵĴ�С
  * ���أ���ʧ�ܣ����ؿգ����򷵻��µĶ�����
******************************************************************************/
FvsMinutiaSet_t MinutiaSetCreate(const FvsInt_t size)
{
    iFvsMinutiaSet_t* p = NULL;
    p = (iFvsMinutiaSet_t*)vmalloc(sizeof(iFvsMinutiaSet_t)
    				+size*sizeof(FvsMinutia_t));
    if (p!=NULL)
    {   
    	/* ������ϸ�ڵ� */
        p->nbminutia = 0;
        p->tablesize = size;
    }    
    return (FvsMinutiaSet_t)p;
}


/******************************************************************************
  * ���ܣ�����ϸ�ڵ㼯�ϡ�
  *       һ�����٣��ö����ٿ���Ϊ�κκ������ã�ֱ���������롣
  * ������minutia      ϸ�ڵ㼯��
  * ���أ���
******************************************************************************/
void MinutiaSetDestroy(FvsMinutiaSet_t minutia)
{
    iFvsMinutiaSet_t* p = NULL;
    if (minutia==NULL)
        return;
    p = minutia;
    vfree(p);
}


/******************************************************************************
  * ���ܣ����ϸ�ڵ㼯�ϵĴ�С
  * ������minutia      ϸ�ڵ㼯��
  * ���أ�ϸ�ڵ㼯�ϴ�С
******************************************************************************/
FvsInt_t MinutiaSetGetSize(const FvsMinutiaSet_t min)
{
    const iFvsMinutiaSet_t* minutia = (iFvsMinutiaSet_t*)min;
    FvsInt_t nret = 0;
    
    if (minutia!=NULL)
        nret = minutia->tablesize;

    return nret;
}


/******************************************************************************
  * ���ܣ�ϸ�ڵ㼯�ϵ�ʵ��Ԫ�ظ���
  * ������minutia      ϸ�ڵ㼯��
  * ���أ�Ԫ�ظ���
******************************************************************************/
FvsInt_t MinutiaSetGetCount(const FvsMinutiaSet_t min)
{
    const iFvsMinutiaSet_t* minutia = (iFvsMinutiaSet_t*)min;
    FvsInt_t nret = 0;
    
    if (minutia!=NULL)
        nret = minutia->nbminutia;

    return nret;
}


/******************************************************************************
  * ���ܣ�����ϸ�ڵ㼯�ϵ����ݻ�����ָ��
  * ������minutia      ϸ�ڵ㼯��
  * ���أ�ָ��
******************************************************************************/
FvsMinutia_t* MinutiaSetGetBuffer(FvsMinutiaSet_t min)
{
    iFvsMinutiaSet_t* minutia = (iFvsMinutiaSet_t*)min;
    FvsMinutia_t* pret = NULL;

    if (minutia!=NULL)
        pret = minutia->ptable;

    return pret;
}


/******************************************************************************
  * ���ܣ����ϸ�ڵ㼯��
  * ������minutia      ϸ�ڵ㼯��
  * ���أ�������
******************************************************************************/
FvsError_t MinutiaSetEmpty(FvsMinutiaSet_t min)
{
    iFvsMinutiaSet_t* minutia = (iFvsMinutiaSet_t*)min;
    FvsError_t nRet = FvsOK;

    if (minutia!=NULL)
        minutia->nbminutia = 0;
    else
        nRet = FvsMemory;

    return nRet;
}


/******************************************************************************
  * ���ܣ��ڼ��������һ��ϸ�ڵ㣬������ˣ�����һ������
  * ������minutia      ϸ�ڵ㼯��
  *       x            ϸ�ڵ��X����
  *       y            ϸ�ڵ��Y����
  *       type         ϸ�ڵ�����
  *       angle        �Ƕ�
  * ���أ�������
******************************************************************************/
FvsError_t MinutiaSetAdd(FvsMinutiaSet_t min,
       const FvsFloat_t x, const FvsFloat_t y,
       const FvsMinutiaType_t type, const FvsFloat_t angle)
{
    iFvsMinutiaSet_t* minutia = (iFvsMinutiaSet_t*)min;
    FvsError_t nRet = FvsOK;

    if (minutia->nbminutia < minutia->tablesize)
    {
        minutia->ptable[minutia->nbminutia].x       = x;
        minutia->ptable[minutia->nbminutia].y       = y;
        minutia->ptable[minutia->nbminutia].type    = type;
        minutia->ptable[minutia->nbminutia].angle   = angle;
        minutia->nbminutia++;
    }
    else
        /* �����޿ռ� */
        nRet = FvsMemory;

    return nRet;
}


static FvsError_t MinutiaSetCheckClean(FvsMinutiaSet_t min)
{
    iFvsMinutiaSet_t* minutia = (iFvsMinutiaSet_t*)min;
    FvsError_t    nRet = FvsOK;
    FvsFloat_t    tx, ty, ta;
    FvsInt_t      i, j;
    FvsMinutia_t* mi, *mj;

    tx = 4.0;
    ty = 4.0;
    ta = 0.5;

    if (minutia!=NULL && minutia->nbminutia > 1)
    {
        /* ��������Ƿ����и�ϸ�ڵ� */
        for (j = 0;   j < minutia->nbminutia; j++)
        for (i = j+1; i < minutia->nbminutia; i++)
        {
            mi = minutia->ptable + i;
            mj = minutia->ptable + j;

            /* �Ƚ�ϸ�ڵ�i��j */

            /* ���� 1: ���Ƶ�ϸ�ڵ�˴˿��� -> ɾ��һ�� */
            if ( (fabs(mi->x     - mj->x    ) < tx) &&
                 (fabs(mi->y     - mj->y    ) < ty) &&
                 (fabs(mi->angle - mj->angle) < ta)
               )
            {
                minutia->nbminutia--;
                *mi = minutia->ptable[minutia->nbminutia];
            }

            /* ���� 2: �����෴�����뿿�� -> ͬʱɾ�� */
        }
    }
    else
        /* �����޿ռ� */
        nRet = FvsMemory;

    return nRet;
}


#define P(x,y)      p[(x)+(y)*pitch]


/******************************************************************************
  * ���ܣ���ͼ���л���ϸ�ڵ㣬���ı䱳��
  * ������minutia      ϸ�ڵ㼯��
  *       image        ָ��ͼ��
  * ���أ�������
******************************************************************************/
FvsError_t MinutiaSetDraw(const FvsMinutiaSet_t min, FvsImage_t image)
{
    FvsInt_t w       = ImageGetWidth(image);
    FvsInt_t h       = ImageGetHeight(image);
    FvsInt_t pitch   = ImageGetPitch(image);
    FvsByte_t* p     = ImageGetBuffer(image);

    FvsInt_t n, x, y;
    FvsFloat_t fx, fy;
    FvsMinutia_t* minutia = MinutiaSetGetBuffer(min);
    FvsInt_t nbminutia    = MinutiaSetGetCount(min);

    if (minutia==NULL || p==NULL)
        return FvsMemory;

    /* ����ÿ��ϸ�ڵ� */
    for (n = 0; n < nbminutia; n++)
    {
        x = (FvsInt_t)minutia[n].x;
        y = (FvsInt_t)minutia[n].y;
        if (x<w-5 && x>4)
        {
            if (y<h-5 && y>4)
            {
                switch (minutia[n].type)
                {
                case FvsMinutiaTypeEnding:
                    P(x,y)    = 0xFF;
                    P(x-1, y) = 0xA0;
                    P(x+1, y) = 0xA0;
                    P(x, y-1) = 0xA0;
                    P(x, y+1) = 0xA0;
                    break;
                case FvsMinutiaTypeBranching:
                    P(x,y)    = 0xFF;
                    P(x-1, y-1) = 0xA0;
                    P(x+1, y-1) = 0xA0;
                    P(x-1, y+1) = 0xA0;
                    P(x+1, y+1) = 0xA0;
                    break;
                default:
                    continue;
                }
                fx = sin(minutia[n].angle);
                fy = -cos(minutia[n].angle);
                P(x+(int32_t)(fx)    ,y+(int32_t)(fy)    ) = 0xFF;
                P(x+(int32_t)(fx*2.0),y+(int32_t)(fy*2.0)) = 0xFF;
                P(x+(int32_t)(fx*3.0),y+(int32_t)(fy*3.0)) = 0xFF;
                P(x+(int32_t)(fx*4.0),y+(int32_t)(fy*4.0)) = 0xFF;
                P(x+(int32_t)(fx*5.0),y+(int32_t)(fy*5.0)) = 0xFF;
            }
        }
    }
    return FvsOK;
}


/* �궨�� */
#define P1  P(x  ,y-1)
#define P2  P(x+1,y-1)
#define P3  P(x+1,y  )
#define P4  P(x+1,y+1)
#define P5  P(x  ,y+1)
#define P6  P(x-1,y+1)
#define P7  P(x-1,y  )
#define P8  P(x-1,y-1)


/******************************************************************************
  * ���ܣ���ϸ��ͼ������ȡϸ�ڵ㣬�����浽�����С�
  *       �����ϸ�ڵ㼯�ϱ����㹻�����̫С�ˣ����˺��ֹͣ����ϸ�ڵ㡣
  * ������minutia      ϸ�ڵ㼯�ϣ���������ϸ�ڵ�
  *       image        ϸ�����ͼ��
  *       direction    �������㷽����
  *       mask         ������ʾ��Ч��ָ������
  * ���أ�������
******************************************************************************/
FvsError_t MinutiaSetExtract
    (
    FvsMinutiaSet_t       minutia,
    const FvsImage_t      image,
    const FvsFloatField_t direction,
    const FvsImage_t      mask
    )
{
    FvsInt_t w      = ImageGetWidth(image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsInt_t pitch  = ImageGetPitch(image);
    FvsInt_t pitchm = ImageGetPitch(mask);
    FvsByte_t* p    = ImageGetBuffer(image);
    FvsByte_t* m    = ImageGetBuffer(mask);
    FvsInt_t   x, y;
    FvsFloat_t angle = 0.0;
    FvsInt_t   whitecount;

    if (m==NULL || p==NULL)
        return FvsMemory;

    (void)MinutiaSetEmpty(minutia);

    /* ����ͼ����ȡϸ�ڵ� */
    for (y = 1; y < h-1; y++)
    for (x = 1; x < w-1; x++)
    {
        if (m[x+y*pitchm]==0)
             continue;
        if (P(x,y)==0xFF)
        {
            whitecount = 0;
            if (P1!=0) whitecount++;
            if (P2!=0) whitecount++;
            if (P3!=0) whitecount++;
            if (P4!=0) whitecount++;
            if (P5!=0) whitecount++;
            if (P6!=0) whitecount++;
            if (P7!=0) whitecount++;
            if (P8!=0) whitecount++;

            switch(whitecount)
            {
            case 0:
                /* �����㣬���� */
                break;
            case 1:
                /* ���Ƕ� */
  	            angle = FloatFieldGetValue(direction, x, y);
                (void)MinutiaSetAdd(minutia, (FvsFloat_t)x, (FvsFloat_t)y,
                              FvsMinutiaTypeEnding, (FvsFloat_t)angle);
                break;
            case 2:
                break;
            default:
                {
     	            angle = FloatFieldGetValue(direction, x, y);
                    (void)MinutiaSetAdd(minutia, (FvsFloat_t)x, (FvsFloat_t)y,
                                  FvsMinutiaTypeBranching, (FvsFloat_t)angle);
                }
                break;
            }
        }
    }
    (void)MinutiaSetCheckClean(minutia);
    
    return FvsOK;
}

