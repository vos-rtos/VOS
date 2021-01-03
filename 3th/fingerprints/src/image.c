/*#############################################################################
 * �ļ�����image.c
 * ���ܣ�  ʵ����ָ��ͼ��Ļ�������
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/image.h"

/* ָ��ͼ��ṹ��256���Ҷ�ͼ */
typedef struct iFvsImage_t
{
    FvsByte_t       *pimg;         /* 8-bitͼ������ */    
    FvsInt_t        w;             /* ���          */
    FvsInt_t        h;             /* �߶�          */
    FvsInt_t        pitch;         /* ��б��        */
    FvsImageFlag_t  flags;         /* ���          */
} iFvsImage_t;


/******************************************************************************
  * ���ܣ�����һ���µ�ͼ�����
  * ��������
  * ���أ�ʧ�ܷ��ؿգ����򷵻��µ�ͼ�����
******************************************************************************/
FvsImage_t ImageCreate()
{
    iFvsImage_t* p = NULL;

    p = (FvsImage_t)vmalloc(sizeof(iFvsImage_t));
    if (p!=NULL)
    {
        p->h        = 0;
        p->w        = 0;
        p->pitch    = 0;
        p->pimg     = NULL;
        p->flags    = FvsImageGray; /* ȱʡ�ı�� */    
    }
    return (FvsImage_t)p;
}


/******************************************************************************
  * ���ܣ�����һ��ͼ�����
  * ������image  ָ��ͼ������ָ��
  * ���أ���
******************************************************************************/
void ImageDestroy(FvsImage_t image)
{
    iFvsImage_t* p = NULL;
    if (image==NULL)
        return;
    (void)ImageSetSize(image, 0, 0);
    p = image;
    vfree(p);
}


/******************************************************************************
  * ���ܣ�����ͼ���ǣ��ò����󲿷��ɿ⺯���Զ����
  * ������image  ָ��ͼ������ָ��
  *       flag   ���
  * ���أ�������
******************************************************************************/
FvsError_t ImageSetFlag(FvsImage_t img, const FvsImageFlag_t flag)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    image->flags = flag; 
    return FvsOK;
}


/******************************************************************************
  * ���ܣ����ͼ����
  * ������image  ָ��ͼ������ָ��
  * ���أ�ͼ����
******************************************************************************/
FvsImageFlag_t ImageGetFlag(const FvsImage_t img)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    return image->flags; 
}


/******************************************************************************
  * ���ܣ�����һ��ͼ�����Ĵ�С
  * ������image   ָ��ͼ������ָ��
  *       width   ͼ����
  *       height  ͼ��߶�
  * ���أ�������
******************************************************************************/
FvsError_t ImageSetSize(FvsImage_t img, const FvsInt_t width, 
						const FvsInt_t height)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    FvsError_t nRet = FvsOK;
    FvsInt_t newsize = width*height;

    /* sizeΪ0����� */
    if (newsize==0)
    {
        if (image->pimg!=NULL)
        {
            vfree(image->pimg);
            image->pimg = NULL;
            image->w = 0;
            image->h = 0;
            image->pitch = 0;
        }
        return FvsOK;
    }

    if (image->h*image->w != newsize)
    {
        vfree(image->pimg);
        image->w = 0;
        image->h = 0;
        image->pitch = 0;
        /* �����ڴ� */
        image->pimg = (uint8_t*)vmalloc((size_t)newsize);
    }

    if (image->pimg == NULL)
        nRet = FvsMemory;
    else
    {
        image->h = height;
        image->w = width;
        image->pitch = width;
    }
    return nRet;
}


/******************************************************************************
  * ���ܣ�����ͼ��
  * ������destination  ָ��Ŀ��ͼ������ָ��
  *       source       ָ��Դͼ������ָ��
  * ���أ�������
******************************************************************************/
FvsError_t ImageCopy(FvsImage_t destination, const FvsImage_t source)
{
    iFvsImage_t* dest = (iFvsImage_t*)destination;
    iFvsImage_t* src  = (iFvsImage_t*)source;
    FvsError_t nRet = FvsOK;

    nRet = ImageSetSize(dest, src->w, src->h);
    
    if (nRet==FvsOK)
        memcpy(dest->pimg, src->pimg, (size_t)src->h*src->w);

    /* ������� */
    dest->flags = src->flags;

    return nRet;
}


/******************************************************************************
  * ���ܣ����ͼ��
  * ������image  ָ��ͼ������ָ��
  * ���أ�������
******************************************************************************/
FvsError_t ImageClear(FvsImage_t img)
{
    return ImageFlood(img, 0);
}


/******************************************************************************
  * ���ܣ�����ͼ������������Ϊ�ض�ֵ
  * ������image  ָ��ͼ������ָ��
  *       value  Ҫ�趨��ֵ
  * ���أ�������
******************************************************************************/
FvsError_t ImageFlood(FvsImage_t img, const FvsByte_t value)
{
    FvsError_t nRet = FvsOK;
    iFvsImage_t* image = (iFvsImage_t*)img;
    if (image==NULL) return FvsMemory;
    if (image->pimg!=NULL)
        memset(image->pimg, (int)value, (size_t)(image->h*image->w));
    return nRet;
}


/******************************************************************************
  * ���ܣ�����ͼ����ĳ�����ص�ֵ
  * ������image  ָ��ͼ������ָ��
  *       x      X������
  *       y      Y������
  *       val    Ҫ�趨��ֵ
  * ���أ���
******************************************************************************/
void ImageSetPixel(FvsImage_t img, const FvsInt_t x, const FvsInt_t y, 
						const FvsByte_t val)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    int address = y * image->w + x;
    image->pimg[address] = val;
}


/******************************************************************************
  * ���ܣ����ͼ����ĳ�����ص�ֵ
  * ������image  ָ��ͼ������ָ��
  *       x      X������
  *       y      Y������
  * ���أ����ص�ֵ
******************************************************************************/
FvsByte_t ImageGetPixel(const FvsImage_t img, const FvsInt_t x, 
						const FvsInt_t y)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    /* �����е�λ�� */
    int address = y * image->pitch + x;
    return image->pimg[address];
}


/******************************************************************************
  * ���ܣ����ͼ�񻺳���ָ��
  * ������image  ָ��ͼ������ָ��
  * ���أ�ָ��ͼ���ڴ滺������ָ��
******************************************************************************/
FvsByte_t* ImageGetBuffer(FvsImage_t img)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    if (image==NULL) 
    	return NULL;
    return image->pimg;
}


/******************************************************************************
  * ���ܣ����ͼ����
  * ������image  ָ��ͼ������ָ��
  * ���أ�ͼ����
******************************************************************************/
FvsInt_t ImageGetWidth(const FvsImage_t img)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    if (image==NULL) 
    	return -1;
    return image->w;
}


/******************************************************************************
  * ���ܣ����ͼ��߶�
  * ������image  ָ��ͼ������ָ��
  * ���أ�ͼ��߶�
******************************************************************************/
FvsInt_t ImageGetHeight(const FvsImage_t img)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    if (image==NULL) 
    	return -1;
    return image->h;
}


/******************************************************************************
  * ���ܣ����ͼ�񻺳����Ĵ�С
  * ������image  ָ��ͼ������ָ��
  * ���أ���������С
******************************************************************************/
FvsInt_t ImageGetSize(const FvsImage_t img)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    if (image==NULL) 
    	return 0;
    return image->h * image->w;
}


/******************************************************************************
  * ���ܣ����ͼ����б��
  * ������image  ָ��ͼ������ָ��
  * ���أ���б��
******************************************************************************/
FvsInt_t ImageGetPitch(const FvsImage_t img)
{
    iFvsImage_t* image = (iFvsImage_t*)img;
    if (image==NULL) 
    	return -1;
    return image->pitch;
}


/******************************************************************************
  * ���ܣ��Ƚ�����ͼ���С
  * ������image1  ָ��ͼ�����1��ָ��
  *       image2  ָ��ͼ�����2��ָ��
  * ���أ�������ͼ���С��ȣ�����true�����򷵻�false
******************************************************************************/
FvsBool_t ImageCompareSize(const FvsImage_t image1, const FvsImage_t image2)
{
    if (ImageGetWidth(image1)!=ImageGetWidth(image2))
        return FvsFalse;
    if (ImageGetHeight(image1)!=ImageGetHeight(image2))
        return FvsFalse;
    return FvsTrue;
}

