/*#############################################################################
 * �ļ�����img_thin.c
 * ���ܣ�  ʵ����ͼ��ϸ������
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "imagemanip.h"

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

#define NOT_BK(pos) (image[pos]!=0)
#define IS_BK(pos)  (image[pos]==0)


bool_t MatchPattern(uint8_t image[], int x, int y, int w, int h)
{
    bool_t nRet = false;
    
    /* ��֤���޳��� */
    int lhe = y * w;        /* ����   */
    int lup = lhe - w;      /* ��һ�� */
    int ldo = lhe + w;      /* ��һ�� */

    int tl = lup + x - 1;   /* ���� */
    int tc = lup + x;       /* ���� */
    int tr = lup + x + 1;   /* ���� */
    int hl = lhe + x - 1;   /* ��   */
    int hr = lhe + x + 1;   /* ��   */
    int bl = ldo + x - 1;   /* ���� */
    int bc = ldo + x;       /* ���� */
    int br = ldo + x + 1;   /* ���� */

    /* ��һģʽ
        ? ? ? one not 0
        0 1 0
        ? ? ? one not 0
    */
    if  ( image[hr]==0  &&  image[hl]==0  &&
        ((image[tl]!=0) || (image[tc]!=0) || (image[tr]!=0))&&
        ((image[bl]!=0) || (image[bc]!=0) || (image[br]!=0))
        )
    {
        nRet = true;
    }
    /* ͬ������ת90��
        ? 0 ?
        ? 1 ?
        ? 0 ?
    */
    else
    if  ( image[tc]==0  &&  image[bc]==0  &&
        ((image[bl]!=0) || (image[hl]!=0) || (image[tl]!=0))&&
        ((image[br]!=0) || (image[hr]!=0) || (image[tr]!=0))
        )
    {
        nRet = true;
    }
    /*
        ? ? ?
        ? 1 0
        ? 0 1
    */
    else
    if
        (image[br]==0xFF     &&  image[hr]==0  &&  image[bc]==0  &&
        (image[tr]!=0   ||  image[tc]!=0   ||
         image[tl]!=0   ||  image[hl]!=0   || image[bl]!=0)
         ) 
    {
        nRet = true;
    }
    /*
        ? ? ?
        0 1 ?
        1 0 ?
    */
    else
    if
        (image[bl]==0xFF     &&  image[hl]==0  &&  image[bc]==0   &&
        (image[br]!=0   ||  image[hr]!=0  ||
         image[tr]!=0   ||  image[tc]!=0  ||  image[tl]!=0))
    {
        nRet = true;
    }
    /*
        1 0 ?
        0 1 ?
        ? ? ?
    */
    else
    if
        (image[tl]==0xFF     &&  image[tc]==0  &&  image[hl]==0   &&
        (image[bl]!=0   ||  image[bc]!=0  ||
         image[br]!=0   ||  image[hr]!=0  ||  image[tr]!=0))
    {
        nRet = true;
    }
    /*
        ? 0 1
        ? 1 0
        ? ? ?
    */
    else
    if
        (image[tr]==0xFF     &&  image[hr]==0  &&  image[tc]==0   &&
        (image[tl]!=0   ||  image[hl]!=0  ||
         image[bl]!=0   ||  image[bc]!=0  ||  image[br]!=0))
    {
        nRet = true;
    }
 
    image[y*w + x] = (nRet==true)?0xFF:0x00;

    return nRet;
}


/* ϸ��ͼ�� */
//FvsError_t ImageThin3(Image_t imgf)
FvsError_t ImageThin3(FvsImage_t imgf)
{
    bool_t Remain;
    int temp;
    uint8_t* image = ImageGetBuffer(imgf);
    register int x, y;
    int w = ImageGetWidth(imgf);  /* ͼ���� */
    int h = ImageGetHeight(imgf); /* ͼ��߶� */
    int tmp;
    int row;

    /* ���ϸ���ٶ� */
    int _lastY;
    int _newY;
    
    /* ��ʼ�� */
    _lastY = _newY = 1;

    /* ��ǣ�ȫ����ɺ��ٴ��� */
    Remain = true;
    while (Remain)
    {
        _lastY = 1;

        _newY = h;

        Remain = false;
        fprintf(stderr, ".");

        temp   = false;
        for (y = _lastY; y < h-1; y++)
            for (x = 1; x < w-1; x++)
            {
                row = y*w;
                tmp = image[row +(x + 1)]; 
                if (image[row + x] == 0xFF && tmp == 0
                    && MatchPattern(image, x, y, w, h) == false)
                    if (temp==false)
                    {
                        _newY  = min(_newY, y);
                        Remain = true;
                        temp   = true;
                    }
            } 

        for (x = w*_lastY; x < w*h; x++)
            if (image[x] == 0x00)
                image[x] = 0;
        
        temp   = false;
        for (y = _lastY; y < h-1; y++)
            for (x = 1; x < w-1; x++)
            {
                row = y*w; 
                tmp = image[(y - 1) * w + x]; 
                if (image[row + x] == 0xFF && tmp == 0
                    && MatchPattern(image, x, y, w, h)==false)
                    if (temp==false)
                    {
                        _newY  = min(_newY, y);
                        Remain = true;
                        temp   = true;
                    }
            } /* end for y */
        
        for (x = w*_lastY; x < w*h; x++)
            if (image[x] == 0x00)
                image[x] = 0;

        temp   = false;
        for (y = _lastY; y < h-1; y++)
            for (x = 1; x < w-1; x++)
            {
                row = y*w;
                tmp = image[row +(x - 1)]; /* -> */
                if (image[row + x] == 0xFF && tmp == 0
                    && MatchPattern(image, x, y, w, h)==false)
                    if (temp==false)
                    {
                        _newY  = min(_newY, y);
                        Remain = true;
                        temp   = true;
                    }
            } /* end for y */
        
        for (x = w*_lastY; x < w*h; x++)
            if (image[x] == 0x00)
                image[x] = 0;

        temp   = false;
        for (y = _lastY; y < h-1; y++)
            for (x = 1; x < w-1; x++)
            {
                row = y*w;                    
                tmp = image[(y + 1) * w + x]; 
                if (image[row + x] == 0xFF && tmp == 0
                    && MatchPattern(image, x, y, w, h)==false)
                    if (temp==false)
                    {
                        _newY  = min(_newY, y);
                        Remain = true;
                        temp   = true;
                    }
            } /* end for y */
        
        for (x = w*_lastY; x < w*h; x++)
            if (image[x] == 0x00)
                image[x] = 0;

    } /* end while */

    return FvsOK;
}


