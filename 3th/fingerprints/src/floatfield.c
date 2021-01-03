/*#############################################################################
 * �ļ�����floatfield.c
 * ���ܣ�  ʵ����ָ�Ƹ�����Ĳ���
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "floatfield.h"

/* ָ�Ƹ�����ṹ */
typedef struct iFvsFloatField_t
{
	FvsFloat_t		*pimg;		/* ������ָ������ */
	FvsInt_t		w;			/* ��� */
	FvsInt_t		h;			/* �߶� */
	FvsInt_t		pitch;		/* ��б�� */
} iFvsFloatField_t;


/******************************************************************************
  * ���ܣ�����һ���ĵĸ��������
  * ��������
  * ���أ�����ʧ�ܣ����ؿգ����򷵻��µĶ�����
******************************************************************************/
FvsFloatField_t FloatFieldCreate()
{
    iFvsFloatField_t* p = NULL;
    p = (FvsFloatField_t)vmalloc(sizeof(iFvsFloatField_t));

    if (p!=NULL)
    {
        p->h        = 0;
        p->w        = 0;
        p->pitch    = 0;
        p->pimg     = NULL;
    }

    return (FvsFloatField_t)p;
}


/******************************************************************************
  * ���ܣ��ƻ��Ѿ����ڵĸ��������
  * ������field   ָ�򸡵�������ָ��
  * ���أ���
******************************************************************************/
void FloatFieldDestroy(FvsFloatField_t field)
{
    iFvsFloatField_t* p = NULL;

    if (field==NULL)
        return;

    p = field;
    (void)FloatFieldSetSize(field, 0, 0);
    vfree(p);
}


/******************************************************************************
  * ���ܣ����ø��������Ĵ�С��
          �ڴ�����Զ���ɣ����ʧ�ܣ�����һ��������
  * ������field   ָ�򸡵�������ָ��
  *       width   ��
  *       height  ��
  * ���أ�������
******************************************************************************/
FvsError_t FloatFieldSetSize(FvsFloatField_t img, const FvsInt_t width, 
						const FvsInt_t height)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    FvsError_t nRet = FvsOK;
    FvsInt_t newsize = (FvsInt_t)(width*height*sizeof(FvsFloat_t));

    /* ��СΪ0����� */
    if (newsize==0)
    {
        if (field->pimg!=NULL)
        {
            vfree(field->pimg);
            field->pimg = NULL;
            field->w = 0;
            field->h = 0;
            field->pitch = 0;
        }
        return FvsOK;
    }

    if ((FvsInt_t)(field->h*field->w*sizeof(FvsFloat_t)) != newsize)
    {
        vfree(field->pimg);
        field->w = 0;
        field->h = 0;
        field->pitch = 0;
        /* �����ڴ� */
        field->pimg = (FvsFloat_t*)vmalloc((size_t)newsize);
    }

    if (field->pimg == NULL)
        nRet = FvsMemory;
    else
    {
        field->h = height;
        field->w = width;
        field->pitch = width;
    }
    return nRet;
}


/******************************************************************************
  * ���ܣ�����һ��Դͼ��Ŀ��ͼ���ڴ�����ʹ�С���ò����Զ����
  * ������destination ָ��Ŀ�긡��������ָ��
  *       source      ָ��Դ����������ָ��
  * ���أ�������
******************************************************************************/
FvsError_t FloatFieldCopy(FvsFloatField_t destination, 
						const FvsFloatField_t source)
{
    iFvsFloatField_t* dest = (iFvsFloatField_t*)destination;
    iFvsFloatField_t* src  = (iFvsFloatField_t*)source;
    FvsError_t nRet = FvsOK;

    nRet = FloatFieldSetSize(dest, src->w, src->h);
    
    if (nRet==FvsOK)
        memcpy(dest->pimg, src->pimg, src->h*src->w*sizeof(FvsFloat_t));

    return nRet;
}


/******************************************************************************
  * ���ܣ����ͼ�����ø��������ָ��Ϊ��
  * ������field ָ�򸡵�������ָ��
  * ���أ�������
******************************************************************************/
FvsError_t FloatFieldClear(FvsFloatField_t img)
{
    return FloatFieldFlood(img, 0.0);
}


/******************************************************************************
  * ���ܣ�������������������ֵ�����ض�ֵ
  * ������field  ָ�򸡵�������ָ��
  *       value  Ҫ���õ�ֵ
  * ���أ�������
******************************************************************************/
FvsError_t FloatFieldFlood(FvsFloatField_t img, const FvsFloat_t value)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    FvsError_t nRet = FvsOK;
    FvsInt_t i;
    if (field->pimg!=NULL)
    {
        for (i=0; i<field->h*field->w; i++)
            field->pimg[i] = value;
    }
    return nRet;
}


/******************************************************************************
  * ���ܣ�Ϊ�������е��ض����������ض�ֵ
  * ������field  ָ�򸡵�������ָ��
  *       x      X������
  *       y      Y������
  *       val    Ҫ�趨��ֵ
  * ���أ���
******************************************************************************/
void FloatFieldSetValue(FvsFloatField_t img, const FvsInt_t x, 
					const FvsInt_t y, const FvsFloat_t val)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    int address = y * field->w + x;
    field->pimg[address] = val;
}


/******************************************************************************
  * ���ܣ��õ��ض�λ�õ�ֵ
  * ������field  ָ�򸡵�������ָ��
  *       x      X������
  *       y      Y������
  * ���أ�����ֵ
******************************************************************************/
FvsFloat_t FloatFieldGetValue(FvsFloatField_t img, const FvsInt_t x, 
					const FvsInt_t y)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    /* �����е�λ�� */
    int address = y * field->pitch + x;
    return field->pimg[address];
}


/******************************************************************************
  * ���ܣ��õ������򻺳���ָ��
  * ������field  ָ�򸡵�������ָ��
  * ���أ��ڴ滺����ָ��
******************************************************************************/
FvsFloat_t* FloatFieldGetBuffer(FvsFloatField_t img)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    return field->pimg;
}


/******************************************************************************
  * ���ܣ���ÿ��
  * ������field  ָ�򸡵�������ָ��
  * ���أ����
******************************************************************************/
FvsInt_t FloatFieldGetWidth(const FvsFloatField_t img)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    return field->w;
}


/******************************************************************************
  * ���ܣ���ø߶�
  * ������field  ָ�򸡵�������ָ��
  * ���أ��߶�
******************************************************************************/
FvsInt_t FloatFieldGetHeight(const FvsFloatField_t img)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    return field->h;
}


/******************************************************************************
  * ���ܣ������б�̶�
  * ������field  ָ�򸡵�������ָ��
  * ���أ���б�̶�
******************************************************************************/
FvsInt_t FloatFieldGetPitch(const FvsFloatField_t img)
{
    iFvsFloatField_t* field = (iFvsFloatField_t*)img;
    return field->pitch;
}


