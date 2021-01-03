/*#############################################################################
 * �ļ�����histogram.c
 * ���ܣ�  ʵ����ָ��ֱ��ͼ�Ĳ���
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "histogram.h"

/* ֱ��ͼ���Կ��ټ���λͼ��һЩ��Ϣ�������ֵ������� */
typedef struct iFvsHistogram_t
{
    FvsUint_t       ptable[256];    /* 8λͼ���ֱ��ͼ */
    FvsInt_t        ncount;         /* ֱ��ͼ�еĵ��� */
    FvsInt_t        nmean;          /* -1 = ��û�м��� */
    FvsInt_t        nvariance;      /* -1 = ��û�м��� */
} iFvsHistogram_t;


/******************************************************************************
  * ���ܣ�����һ���µ�ֱ��ͼ����
  * ��������
  * ���أ�ʧ�ܷ��ؿգ����򷵻�ֱ��ͼ����
******************************************************************************/
FvsHistogram_t HistogramCreate()
{
    iFvsHistogram_t* p = NULL;
    p = (FvsHistogram_t)malloc(sizeof(iFvsHistogram_t));

    if (p!=NULL)
    {
        /* ���ñ� */
        HistogramReset(p);
    }
    return (FvsHistogram_t)p;
}


/******************************************************************************
  * ���ܣ��ƻ�һ�����ڵ�ֱ��ͼ����
  * ������histogram ֱ��ͼ����ָ��
  * ���أ�������
******************************************************************************/
void HistogramDestroy(FvsHistogram_t histogram)
{
    iFvsHistogram_t* p = NULL;
    if (histogram==NULL)
        return;
    p = histogram;
    free(p);
}


/******************************************************************************
  * ���ܣ�����һ�����ڵ�ֱ��ͼ����Ϊ0
  * ������histogram ֱ��ͼ����ָ��
  * ���أ�������
******************************************************************************/
FvsError_t HistogramReset(FvsHistogram_t hist)
{
    iFvsHistogram_t* histogram = (iFvsHistogram_t*)hist;
    int i;
    for (i = 0; i < 256; i++)
        histogram->ptable[i] = 0;
    histogram->ncount    = 0;
    histogram->nmean     = -1;
    histogram->nvariance = -1;
    return FvsOK;
}


/******************************************************************************
  * ���ܣ�����һ��8-bitͼ���ֱ��ͼ
  * ������histogram ֱ��ͼ����ָ��
  *       image     ͼ��ָ��
  * ���أ�������
******************************************************************************/
FvsError_t HistogramCompute(FvsHistogram_t hist, const FvsImage_t image)
{
    iFvsHistogram_t* histogram = (iFvsHistogram_t*)hist;
    FvsError_t nRet = FvsOK;
    FvsInt_t w      = ImageGetWidth(image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsInt_t pitch  = ImageGetPitch(image);
    uint8_t* p      = ImageGetBuffer(image);
    FvsInt_t x, y;

    if (histogram==NULL || p==NULL)
        return FvsMemory;

    /* ��������ֱ��ͼ */
    nRet = HistogramReset(hist);
    /* ���� */
    if (nRet==FvsOK)
    {
        FvsInt_t pos;
        for (y=0; y<h; y++)
        {
            pos = pitch*y;
            for (x=0; x<w; x++)
            {
                histogram->ptable[p[pos++]]++;
            }
        }
        histogram->ncount = w*h;
    }

    return nRet;
}


/******************************************************************************
  * ���ܣ�����һ��ֱ��ͼ����ľ�ֵ
  * ������histogram ֱ��ͼ����ָ��
  * ���أ���ֵ
******************************************************************************/
FvsByte_t HistogramGetMean(const FvsHistogram_t hist)
{
    iFvsHistogram_t* histogram = (iFvsHistogram_t*)hist;
    FvsInt_t val, i;

    val = histogram->nmean;
    if (val==-1)
    {
        val = 0;
        for (i = 1; i < 255; i++)
            val += i*histogram->ptable[i];

        i = histogram->ncount;
        if (i>0)
            val = val/i;
        else
            val = 0;

        histogram->nmean = val;
    }
    return (uint8_t)val;
}


/******************************************************************************
  * ���ܣ�����һ��ֱ��ͼ����ķ���
  * ������histogram ֱ��ͼ����ָ��
  * ���أ�����
******************************************************************************/
FvsUint_t HistogramGetVariance(const FvsHistogram_t hist)
{
    iFvsHistogram_t* histogram = (iFvsHistogram_t*)hist;
    FvsInt_t val;
    FvsInt_t i;
    uint8_t mean;

    val = histogram->nvariance;
    if (val==-1)
    {
        /* �����ֵ */
        mean = HistogramGetMean(hist);
        val  = 0;
        for (i = 0; i < 255; i++)
            val += histogram->ptable[i]*(i - mean)*(i - mean);

        i = histogram->ncount;
        if (i>0)
            val = val/i;
        else
            val = 0;

        histogram->nvariance = val;
    }
    return (FvsUint_t)val;
}



