/*#############################################################################
 * �ļ�����imagemanip.c
 * ���ܣ�  ʵ������Ҫ��ͼ�������
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


/* �궨�� */
#define PIJKL p[i+k + (j+l)*nSizeX]


/******************************************************************************
  * ���ܣ�ͼ�����Ų���
  * ������image       ָ��ͼ��
  *       size        ���ŵ�ͼ����С
  *       tolerance   ��ȥֱ��ͼ�ı߽�
  * ���أ�������
******************************************************************************/
FvsError_t ImageLocalStretch(FvsImage_t image, const FvsInt_t size, 
						const FvsInt_t tolerance)
{
    /* ����һЩ���� */
    int nSizeX = ImageGetWidth(image)  - size + 1;
    int nSizeY = ImageGetHeight(image) - size + 1;
    FvsInt_t i, j, t, l;
    FvsInt_t sum, denom;
    FvsByte_t a = 0;
    FvsInt_t k = 0;
    FvsByte_t b = 255;
    int hist[256];
    FvsByte_t* p = ImageGetBuffer(image);
    if (p==NULL)
    	return FvsMemory;
    for (j=0; j < nSizeY; j+=size)
    {
        for (i=0; i < nSizeX; i+=size)
        {
            /* ����ֱ��ͼ */
            memset(hist, 0, 256*sizeof(int));
            for (l = 0; l<size; l++)
                for (k = 0; k<size; k++)
                    hist[PIJKL]++;
            
            /* ���� */
            for (k=0,   sum=0;   k <256; k++)
            {
                sum+=hist[k];
                a = (FvsByte_t)k;
                if (sum>tolerance) break;
            }
            
            for (k=255, sum=0; k >= 0; k--)
            {
                sum+=hist[k];
                b = (FvsByte_t)k;
                if (sum>tolerance) break;
            }

            denom = (FvsInt_t)(b-a);
            if (denom!=0)
            {
                for (l = 0; l<size; l++)
                {
                    for (k = 0; k<size; k++)
                    {
                        if (PIJKL<a) PIJKL = a;
                        if (PIJKL>b) PIJKL = b;
                        t = (FvsInt_t)((((PIJKL)-a)*255)/denom);
                        PIJKL = (FvsByte_t)(t);
                    }
                }
            }
        }
    }

    return FvsOK;
}


#define P(x,y)      ((int32_t)p[(x)+(y)*pitch])

/******************************************************************************
** ���㼹�ߵķ���
** ����һ����һ����ָ��ͼ���㷨����Ҫ�������£�
**
** 1 - ��G�ֳɴ�СΪ w x w - (15 x 15) �Ŀ飻
**
** 2 - ����ÿ������ (i,j)���ݶ� dx(i,j) �� dy(i,j) ,
**     ���ݼ���������ݶ����ӿ��ԴӼ򵥵�Sobel���ӵ����ӵ�Marr-Hildreth ���ӡ�
**
** 3 - �������Ʒ���(i,j), ʹ�����µĲ�����
**
**               i+w/2 j+w/2
**               ---   --- 
**               \     \
**     Nx(i,j) =  --    -- 2 dx(u,v) dy(u,v)
**               /     /
**               ---   ---
**            u=i-w/2 v=j-w/2
**
**               i+w/2 j+w/2
**               ---   --- 
**               \     \
**     Ny(i,j) =  --    -- dx(u,v) - dy(u,v)
**               /     /
**               ---   ---
**            u=i-w/2 v=j-w/2
**
**                  1    -1  / Nx(i,j) \
**     Theta(i,j) = - tan   |  -------  |
**                  2        \ Ny(i,j) /
**
**     ���Theta(i,j)�Ǿֲ����߷������С������ƣ������� (i,j) Ϊ���ġ�
**     ����ѧ�ĽǶȿ�����������ҶƵ����ֱ��ռ��ʱ�ķ���
**
** 4 - ���������������ߵ��жϣ�ϸ�ڵ�ȵȵĴ��ڣ�������ͼ���У��Ծֲ�����
**     ����Ĺ��Ʋ���������ȷ�ġ����ھֲ����߷���仯���������Կ����õ�ͨ
**     �˲�������������ȷ�ļ��߷���Ϊ�����õ�ͨ�˲���������ͼ����ת����
**     ������ʸ���򣬶������£�
**       Phi_x(i,j) = cos( 2 x theta(i,j) )
**       Phi_y(i,j) = sin( 2 x theta(i,j) )
**     ��ʸ���򣬿��������µľ����ͨ�˲���
**       Phi2_x(i,j) = (W @ Phi_x) (i,j)
**       Phi2_y(i,j) = (W @ Phi_y) (i,j)
**     W��һ����ά�ĵ�ͨ�˲�����
**
** 5 - �����¹�ʽ���� (i,j) ���ķ���
**
**              1    -1  / Phi2_y(i,j) \
**     O(i,j) = - tan   |  -----------  |
**              2        \ Phi2_x(i,j) /
**
** ������㷨���Եõ��൱ƽ���ķ���ͼ
**
*/

static FvsError_t FingerprintDirectionLowPass(FvsFloat_t* theta, 
					FvsFloat_t* out, FvsInt_t nFilterSize, 
					FvsInt_t w, FvsInt_t h)
{
    FvsError_t nRet = FvsOK;
    FvsFloat_t* filter = NULL;
    FvsFloat_t* phix   = NULL;
    FvsFloat_t* phiy   = NULL;
    FvsFloat_t* phi2x  = NULL;
    FvsFloat_t* phi2y  = NULL;
    FvsInt_t fsize  = nFilterSize*2+1;
    size_t nbytes = (size_t)(w*h*sizeof(FvsFloat_t));
    FvsFloat_t nx, ny;
    FvsInt_t val;
    FvsInt_t i, j, x, y;

    filter= (FvsFloat_t*)malloc((size_t)fsize*fsize*sizeof(FvsFloat_t));
    phix  = (FvsFloat_t*)malloc(nbytes);
    phiy  = (FvsFloat_t*)malloc(nbytes);
    phi2x = (FvsFloat_t*)malloc(nbytes);
    phi2y = (FvsFloat_t*)malloc(nbytes);

    if (filter==NULL || phi2x==NULL || phi2y==NULL || phix==NULL || phiy==NULL)
        nRet = FvsMemory;
    else
    {
        /* �� 0 */
        memset(filter, 0, (size_t)fsize*fsize*sizeof(FvsFloat_t));
        memset(phix,   0, nbytes);
        memset(phiy,   0, nbytes);
        memset(phi2x,  0, nbytes);
        memset(phi2y,  0, nbytes);

        /* ����4 */
        for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
        {
            val = x+y*w;
            phix[val] = cos(theta[val]);
            phiy[val] = sin(theta[val]);
        }
        /* �����ͨ�˲��� */
        nx = 0.0;
        for (j = 0; j < fsize; j++)
        for (i = 0; i < fsize; i++)
        {
            filter[j*fsize+i] = 1.0;

            nx += filter[j*fsize+i]; /* ϵ���� */
        }
        if (nx>1.0)
        {
            for (j = 0; j < fsize; j++)
            for (i = 0; i < fsize; i++)
                /* ��һ����� */
                filter[j*fsize+i] /= nx;
        }
        /* ��ͨ�˲� */
        for (y = 0; y < h-fsize; y++)
        for (x = 0; x < w-fsize; x++)
        {
            nx = 0.0;
            ny = 0.0;
            for (j = 0; j < fsize; j++)
            for (i = 0; i < fsize; i++)
            {
                val = (x+i)+(j+y)*w;
                nx += filter[j*fsize+i]*phix[val];
                ny += filter[j*fsize+i]*phiy[val];
            }
            val = x+y*w;
            phi2x[val] = nx;
            phi2y[val] = ny;
        }
        /* ���� phix, phiy */
        if (phix!=NULL) 
        { 
        	vfree(phix);
        	phix=NULL; 
        }
        if (phiy!=NULL) 
        { 
        	vfree(phiy);
        	phiy=NULL; 
        }

        /* ����5 */
        for (y = 0; y < h-fsize; y++)
        for (x = 0; x < w-fsize; x++)
        {
            val = x+y*w;
            out[val] = atan2(phi2y[val], phi2x[val])*0.5;
        }
    }

    if (phix!=NULL)  vfree(phix);
    if (phiy!=NULL)  vfree(phiy);
    if (phi2x!=NULL) vfree(phi2x);
    if (phi2y!=NULL) vfree(phi2y);
    if (filter!=NULL) vfree(filter);

    return nRet;
}


/******************************************************************************
  * ���ܣ�����ָ��ͼ���ߵķ���
          ���㷨����������ж������������ͼ�����˹�һ�������ҶԱȶȽϸߣ�
          �����Ĵ���Ч��Ҳ�Ϻá�
          �����ֵ��-PI/2��PI/2֮�䣬���Ⱥͼ�������ͬ��
          ѡȡ�Ŀ�Խ�󣬷�����Ч��ҲԽ�ã�������Ĵ������ʱ��ҲԽ����
          ����ָ��ͼ���м��߷���ı仯�Ƚϻ��������Ե�ͨ�˲������ԽϺõ�
          ���ǵ������е������ʹ���
  * ������image          ָ��ͼ������ָ��
  *       field          ָ�򸡵�������ָ�룬������
  *       nBlockSize     ���С
  *       nFilterSize    �˲�����С
  * ���أ�������
******************************************************************************/
FvsError_t FingerprintGetDirection(const FvsImage_t image, 
						FvsFloatField_t field, const FvsInt_t nBlockSize, 
						const FvsInt_t nFilterSize)
{
    /* ����ͼ��Ŀ�Ⱥ͸߶� */
    FvsInt_t w       = ImageGetWidth (image);
    FvsInt_t h       = ImageGetHeight(image);
    FvsInt_t pitch   = ImageGetPitch (image);
    FvsByte_t* p     = ImageGetBuffer(image);
    FvsInt_t i, j, u, v, x, y;
    FvsFloat_t dx[(nBlockSize*2+1)][(nBlockSize*2+1)];
    FvsFloat_t dy[(nBlockSize*2+1)][(nBlockSize*2+1)];
    FvsFloat_t nx, ny;
    FvsFloat_t* out;
    FvsFloat_t* theta  = NULL;
    FvsError_t nRet = FvsOK;

    /* ���ͼ�� */
    nRet = FloatFieldSetSize(field, w, h);
    if (nRet!=FvsOK) return nRet;
    nRet = FloatFieldClear(field);
    if (nRet!=FvsOK) return nRet;
    out = FloatFieldGetBuffer(field);

    /* Ϊ�������������ڴ� */
    if (nFilterSize>0)
    {
        theta = (FvsFloat_t*)vmalloc(w * h * sizeof(FvsFloat_t));
        if (theta!=NULL)
            memset(theta, 0, (w * h * sizeof(FvsFloat_t)));
    }

    /* �ڴ���󣬷��� */
    if (out==NULL || (nFilterSize>0 && theta==NULL))
        nRet = FvsMemory;
    else
    {
        /* 1 - ͼ��ֿ� */
        for (y = nBlockSize+1; y < h-nBlockSize-1; y++)
        for (x = nBlockSize+1; x < w-nBlockSize-1; x++)
        {
            /* 2 - �����ݶ� */
            for (j = 0; j < (nBlockSize*2+1); j++)
            for (i = 0; i < (nBlockSize*2+1); i++)
            {
                dx[i][j] = (FvsFloat_t)
                           (P(x+i-nBlockSize,   y+j-nBlockSize) -
                            P(x+i-nBlockSize-1, y+j-nBlockSize));
                dy[i][j] = (FvsFloat_t)
                           (P(x+i-nBlockSize,   y+j-nBlockSize) -
                            P(x+i-nBlockSize,   y+j-nBlockSize-1));
            }

            /* 3 - ���㷽�� */
            nx = 0.0;
            ny = 0.0;
            for (v = 0; v < (nBlockSize*2+1); v++)
            for (u = 0; u < (nBlockSize*2+1); u++)
            {
                nx += 2 * dx[u][v] * dy[u][v];
                ny += dx[u][v]*dx[u][v] - dy[u][v]*dy[u][v];
            }
            /* ����Ƕ� (-pi/2 .. pi/2) */
            if (nFilterSize>0)
                theta[x+y*w] = atan2(nx, ny);
            else
                out[x+y*w] = atan2(nx, ny)*0.5;
        }
        if (nFilterSize>0)
            nRet = FingerprintDirectionLowPass(theta, out, nFilterSize, w, h);
    }

    if (theta!=NULL) vfree(theta);
    return nRet;
}


/* ָ��Ƶ���� */

/******************************************************************************
** �����������ǹ���ָ�Ƽ��ߵ�Ƶ�ʡ��ھֲ������û��͹�ֵ�ϸ�ڵ���߹µ㣬
** ���ż��ߺ͹ȵף�������һ���������߲�����Ϊģ�ͣ���ˣ��ֲ�����Ƶ����ָ��ͼ
** �����һ�����ʵ���������ָ��ͼ��G���й�һ����O���䷽��ͼ������ֲ�����Ƶ��
** �Ĳ������£�
**
** 1 - ͼ��ֿ� w x w - (16 x 16)
**
** 2 - ��ÿ�飬�����СΪl x w (32 x 16)�ķ���ͼ����
**
** 3 - �������� (i,j) ��ÿ��, ���㼹�ߺ͹ȵ׵� x-signature
**     X[0], X[1], ... X[l-1] �������¹�ʽ��
**
**              --- w-1
**            1 \
**     X[k] = -  --  G (u, v), k = 0, 1, ..., l-1
**            w /
**              --- d=0
**
**     u = i + (d - w/2).cos O(i,j) + (k - l/2).sin O(i,j)
**
**     v = j + (d - w/2).sin O(i,j) - (k - l/2).cos O(i,j)
**
**     �������ͼ������û��ϸ�ڵ�͹����ĵ㣬��x-signature�γ���һ����ɢ
**     ���������߲����뷽��ͼ�м��ߺ͹ȵ׵�Ƶ��һ������ˣ����ߺ͹ȵ׵�
**     Ƶ�ʿ�����x-signature�����ơ���T(i,j)�������嶥��ƽ�����룬��Ƶ��
**     OHM(i,j)�����������㣺OHM(i,j) = 1 / T(i,j)��
**
**     ���û�����������ķ嶥����Ƶ����Ϊ-1��˵������Ч��
**
** 4 - ����һ��ָ��ͼ����ԣ�����Ƶ�ʵ�ֵ��һ����Χ֮�ڱ䶯������˵����500
**     dpi��ͼ�񣬱䶯��ΧΪ[1/3, 1/25]����ˣ�������Ƴ���Ƶ�ʲ��������
**     Χ�ڣ�˵��Ƶ�ʹ�����Ч��ͬ����Ϊ-1��
**
** 5 - ���ĳ���жϵ����ϸ�ڵ㣬�򲻻����������ߣ���Ƶ�ʿ������ڿ��Ƶ��
**     ��ֵ���ƣ�����˵��˹��������ֵΪ0������Ϊ9�����Ϊ7����
**
** 6 - �����ڲ�����仯�����������õ�ͨ�˲���
**
*/

/* ��� */
#define BLOCK_W     16
#define BLOCK_W2     8

/* ���� */
#define BLOCK_L     32
#define BLOCK_L2    16

#define EPSILON     0.0001
#define LPSIZE      3

#define LPFACTOR    (1.0/((LPSIZE*2+1)*(LPSIZE*2+1)))


FvsError_t FingerprintGetFrequency(const FvsImage_t image, const FvsFloatField_t direction,
           FvsFloatField_t frequency)
{
    /* ����ͼ��Ŀ�Ⱥ͸߶� */
    FvsError_t nRet = FvsOK;
    FvsInt_t w      = ImageGetWidth (image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsInt_t pitchi = ImageGetPitch (image);
    FvsByte_t* p    = ImageGetBuffer(image);
    FvsFloat_t* out;
    FvsFloat_t* freq;
    FvsFloat_t* orientation = FloatFieldGetBuffer(direction);

    FvsInt_t x, y, u, v, d, k;
    size_t size;

    if (p==NULL)
        return FvsMemory;

    /* ���ͼ����ڴ����� */
    nRet = FloatFieldSetSize(frequency, w, h);
    if (nRet!=FvsOK) return nRet;
    (void)FloatFieldClear(frequency);
    freq = FloatFieldGetBuffer(frequency);
    if (freq==NULL)
        return FvsMemory;

    /* ������ڴ����� */
    size = w*h*sizeof(FvsFloat_t);
    out  = (FvsFloat_t*)vmalloc(size);
    if (out!=NULL)
    {
        FvsFloat_t dir = 0.0;
        FvsFloat_t cosdir = 0.0;
        FvsFloat_t sindir = 0.0;

        FvsInt_t peak_pos[BLOCK_L];		/* ����			*/
        FvsInt_t peak_cnt;				/* ������Ŀ		*/
        FvsFloat_t peak_freq;			/* ����Ƶ��		*/
        FvsFloat_t Xsig[BLOCK_L];		/* x signature	*/
        FvsFloat_t pmin, pmax;

        memset(out,  0, size);
        memset(freq, 0, size);

        /* 1 - ͼ��ֿ�  BLOCK_W x BLOCK_W - (16 x 16) */
        for (y = BLOCK_L2; y < h-BLOCK_L2; y++)
        for (x = BLOCK_L2; x < w-BLOCK_L2; x++)
        {
            /* 2 - ���߷���Ĵ��� l x w (32 x 16) */
            dir = orientation[(x+BLOCK_W2) + (y+BLOCK_W2)*w];
            cosdir = -sin(dir); 
            sindir = cos(dir);

            /* 3 - ���� x-signature X[0], X[1], ... X[l-1] */
            for (k = 0; k < BLOCK_L; k++)
            {
                Xsig[k] = 0.0;
                for (d = 0; d < BLOCK_W; d++)
                {
                    u = (FvsInt_t)(x + (d-BLOCK_W2)*cosdir + (k-BLOCK_L2)*sindir);
                    v = (FvsInt_t)(y + (d-BLOCK_W2)*sindir - (k-BLOCK_L2)*cosdir);
                    /* clipping */
                    if (u<0) u = 0; else if (u>w-1) u = w-1;
                    if (v<0) v = 0; else if (v>h-1) v = h-1;
                    Xsig[k] += p[u + (v*pitchi)];
                }
                Xsig[k] /= BLOCK_W;
            }

            /* ���� T(i,j) */
            /* Ѱ�� x signature �еĶ��� */
            peak_cnt = 0;
  
            pmax = pmin = Xsig[0];
            for (k = 1; k < BLOCK_L; k++)
            {
                if (pmin>Xsig[k]) pmin = Xsig[k];
                if (pmax<Xsig[k]) pmax = Xsig[k];
            }
            if ((pmax - pmin)>64.0)
            {
                for (k = 1; k < BLOCK_L-1; k++)
                if ((Xsig[k-1] < Xsig[k]) && (Xsig[k] >= Xsig[k+1]))
                {
                    peak_pos[peak_cnt++] = k;
                }
            }
            /* �����ֵ */
            peak_freq = 0.0;
            if (peak_cnt>=2)
            {
                for (k = 0; k < peak_cnt-1; k++)
                    peak_freq += peak_pos[k+1]-peak_pos[k];
                peak_freq /= peak_cnt-1;
            }
            /* 4 - ��֤Ƶ�ʷ�Χ [1/25-1/3] */
            /*     �������� [1/30-1/2]   */
            if (peak_freq > 30.0)
                out[x+y*w] = 0.0;
            else if (peak_freq < 2.0)
                out[x+y*w] = 0.0;
            else
                out[x+y*w] = 1.0/peak_freq;
        }
        /* 5 - δ֪�� */
        for (y = BLOCK_L2; y < h-BLOCK_L2; y++)
        for (x = BLOCK_L2; x < w-BLOCK_L2; x++)
        {
            if (out[x+y*w]<EPSILON)
            {
                if (out[x+(y-1)*w]>EPSILON)
                {
                    out[x+(y*w)] = out[x+(y-1)*w];
                }
                else
                {
                    if (out[x-1+(y*w)]>EPSILON)
                        out[x+(y*w)] = out[x-1+(y*w)];
                }
            }
        }
        /* 6 - Ƶ�ʲ�ֵ */
        for (y = BLOCK_L2; y < h-BLOCK_L2; y++)
        for (x = BLOCK_L2; x < w-BLOCK_L2; x++)
        {
            k = x + y*w;
            peak_freq = 0.0;
            for ( v = -LPSIZE; v <= LPSIZE; v++)
            for ( u = -LPSIZE; u <= LPSIZE; u++)
                peak_freq += out[(x+u)+(y+v)*w];
            freq[k] = peak_freq*LPFACTOR;
        }
        vfree(out);
    }
    return nRet;
}


/******************************************************************************
  * ���ܣ���ȡָ��ͼ�����Ч�����Խ��н�һ���Ĵ���
  *       ���ĳ�����򲻿����ã���������Ϊ0��������������
  *       �߽磬�����㣬ͼ�������ܲ������
  *       ��Ч�����������Ϊ255��
  * ������image        ָ��ͼ��
  *       direction    ���߷���
  *       frequency    ����Ƶ��
  *       mask         ���������
  * ���أ�������
******************************************************************************/
FvsError_t FingerprintGetMask(const FvsImage_t image, 
					const FvsFloatField_t direction,
					const FvsFloatField_t frequency, FvsImage_t mask)
{
    FvsError_t nRet = FvsOK;
    FvsFloat_t freqmin = 1.0 / 25;
    FvsFloat_t freqmax = 1.0 / 3;

    /* ����ͼ��Ŀ�ȸ߶� */
    FvsInt_t w      = ImageGetWidth (image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsByte_t* out;
    FvsInt_t pitchout;
    FvsInt_t pos, posout, x, y;
    FvsFloat_t* freq = FloatFieldGetBuffer(frequency);

    if (freq==NULL)
        return FvsMemory;

    /* ��Ҫ���Ľ������ */
    nRet = ImageSetSize(mask, w, h);
    if (nRet==FvsOK)
        nRet = ImageClear(mask);
    out = ImageGetBuffer(mask);
    if (out==NULL)
        return FvsMemory;
    if (nRet==FvsOK)
    {
    pitchout = ImageGetPitch(mask);

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
        {
            pos    = x + y * w;
            posout = x + y * pitchout;
            out[posout] = 0;
            if (freq[pos] >= freqmin && freq[pos] <= freqmax)
            {
                out[posout] = 255;
            }
        }
    /* ���� */
    for (y = 0; y < 4; y++)
        (void)ImageDilate(mask);
    /* ȥ���߽� */
    for (y = 0; y < 12; y++)
        (void)ImageErode(mask);
    }
    return nRet;
}


/* ϸ���㷨 */
#undef P
#define P(x,y)      ((x)+(y)*pitch)
#define REMOVE_P    { p[P(x,y)]=0x80; changed = FvsTrue; }


/******************************************************************************
** ����㶨������:
**     9 2 3
**     8 1 4
**     7 5 6
******************************************************************************/
/* �궨�� */
#define P1  p[P(x  ,y  )]
#define P2  p[P(x  ,y-1)]
#define P3  p[P(x+1,y-1)]
#define P4  p[P(x+1,y  )]
#define P5  p[P(x+1,y+1)]
#define P6  p[P(x  ,y+1)]
#define P7  p[P(x-1,y+1)]
#define P8  p[P(x-1,y  )]
#define P9  p[P(x-1,y-1)]


FvsError_t ImageRemoveSpurs(FvsImage_t image)
{
    FvsInt_t w       = ImageGetWidth(image);
    FvsInt_t h       = ImageGetHeight(image);
    FvsInt_t pitch   = ImageGetPitch(image);
    FvsByte_t* p     = ImageGetBuffer(image);
    FvsInt_t x, y, n, t, c;

    c = 0;
    
    do 
    {
	n = 0;    
	for (y=1; y<h-1; y++)
        for (x=1; x<w-1; x++)
        {
            if( p[P(x,y)]==0xFF)
            {
                t=0;
                if (P3==0 && P2!=0 && P4==0) t++;
                if (P5==0 && P4!=0 && P6==0) t++;
                if (P7==0 && P6!=0 && P8==0) t++;
                if (P9==0 && P8!=0 && P2==0) t++;
                if (P3!=0 && P4==0) t++;
                if (P5!=0 && P6==0) t++;
                if (P7!=0 && P8==0) t++;
                if (P9!=0 && P2==0) t++;
                if (t==1)
				{
                    p[P(x,y)] = 0x80;
		    		n++;	    
				}
            }
        }
		for (y=1; y<h-1; y++)
        	for (x=1; x<w-1; x++)
        	{
            	if( p[P(x,y)]==0x80)
	        	p[P(x,y)] = 0;
			}   
    } while (n>0 && ++c < 5);
        
    return FvsOK;
}


/* a) ��֤����2-6���ڵ� */
#define STEP_A  n = 0; /* �ڵ���� */ \
                if (P2!=0) n++; if (P3!=0) n++; if (P4!=0) n++; if (P5!=0) n++; \
                if (P6!=0) n++; if (P7!=0) n++; if (P8!=0) n++; if (P9!=0) n++; \
                if (n>=2 && n<=6)

/* b) ͳ����0��1�ĸ��� */
#define STEP_B  t = 0; /* �仯����Ŀ */ \
                if (P9==0 && P2!=0) t++; if (P2==0 && P3!=0) t++; \
                if (P3==0 && P4!=0) t++; if (P4==0 && P5!=0) t++; \
                if (P5==0 && P6!=0) t++; if (P6==0 && P7!=0) t++; \
                if (P7==0 && P8!=0) t++; if (P8==0 && P9!=0) t++; \
                if (t==1)

/******************************************************************************
  * ���ܣ�ϸ��ָ��ͼ��
  *       ͼ������Ƕ�ֵ�����ģ�ֻ����0x00��oxFF��
  *       ���㷨����������жϣ�����ĳ�����ظ���ȥ���Ǳ���
  * ������image   ָ��ͼ��
  * ���أ�������
******************************************************************************/ 
FvsError_t ImageThinConnectivity(FvsImage_t image)
{
    FvsInt_t w       = ImageGetWidth(image);
    FvsInt_t h       = ImageGetHeight(image);
    FvsInt_t pitch   = ImageGetPitch(image);
    FvsByte_t* p     = ImageGetBuffer(image);
    FvsInt_t x, y, n, t;
    FvsBool_t changed = FvsTrue;

    if (p==NULL)
        return FvsMemory;
    if (ImageGetFlag(image)!=FvsImageBinarized)
        return FvsBadParameter;

    while (changed==FvsTrue)
    {
        changed = FvsFalse;
        for (y=1; y<h-1; y++)
        for (x=1; x<w-1; x++)
        {
            if (p[P(x,y)]==0xFF)
            {
                STEP_A
                {
                    STEP_B
                    {
                        /*
                        c) 2*4*6=0  (2,4 ,or 6 Ϊ0)
                        d) 4*6*8=0
                        */
                        if (P2*P4*P6==0 && P4*P6*P8==0)
                            REMOVE_P;

                    }
                }
            }
        }

        for (y=1; y<h-1; y++)
        for (x=1; x<w-1; x++)
            if (p[P(x,y)]==0x80)
                p[P(x,y)] = 0;

        for (y=1; y<h-1; y++)
        for (x=1; x<w-1; x++)
        {
            if (p[P(x,y)]==0xFF)
            {
                STEP_A
                {
                    STEP_B
                    {
                        /*
                        c) 2*6*8=0
                        d) 2*4*8=0
                        */
                        if (P2*P6*P8==0 && P2*P4*P8==0)
                            REMOVE_P;

                    }
                }
            }
        }

        for (y=1; y<h-1; y++)
        for (x=1; x<w-1; x++)
            if (p[P(x,y)]==0x80)
                p[P(x,y)] = 0;
    }

    ImageRemoveSpurs(image);
    
    return ImageSetFlag(image, FvsImageThinned);
}

 
	
/* ���¶��� REMOVE_P */
#undef REMOVE_P


#define REMOVE_P    { p[P(x,y)]=0x00; changed = FvsTrue; }


/******************************************************************************
  * ���ܣ�ϸ��ָ��ͼ��ʹ�á�Hit and Miss���ṹԪ�ء�
  *       ͼ������Ƕ�ֵ�����ģ�ֻ����0x00��oxFF��
  *       ���㷨��ȱ���ǲ����ܶ�α���������α��������
  *       ������������㷨������������ǳ���Ҫ��
  * ������image   ָ��ͼ��
  * ���أ�������
******************************************************************************/
FvsError_t ImageThinHitMiss(FvsImage_t image)
{
    FvsInt_t w      = ImageGetWidth(image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsInt_t pitch  = ImageGetPitch(image);
    FvsByte_t* p    = ImageGetBuffer(image);

    /* 
    // 
    // 0 0 0      0 0 
    //   1      1 1 0
    // 1 1 1      1
    //
    */
    FvsInt_t x,y, t;
    FvsBool_t changed = FvsTrue;

    if (p==NULL)
        return FvsMemory;

    if (ImageGetFlag(image)!=FvsImageBinarized)
        return FvsBadParameter;

    while (changed==FvsTrue)
    {
        changed = FvsFalse;
        for (y=1; y<h-1; y++)
        for (x=1; x<w-1; x++)
        {
            if (p[P(x,y)]==0xFF)
            {
                /*
                // 0 0 0  0   1  1 1 1  1   0
                //   1    0 1 1    1    1 1 0
                // 1 1 1  0   1  0 0 0  1   0
                */
                if (p[P(x-1,y-1)]==0 && p[P(x,y-1)]==0 && p[P(x+1,y-1)]==0 &&
                    p[P(x-1,y+1)]!=0 && p[P(x,y+1)]!=0 && p[P(x+1,y+1)]!=0)
                    REMOVE_P;

                if (p[P(x-1,y-1)]!=0 && p[P(x,y-1)]!=0 && p[P(x+1,y-1)]!=0 &&
                    p[P(x-1,y+1)]==0 && p[P(x,y+1)]==0 && p[P(x+1,y+1)]==0)
                    REMOVE_P;

                if (p[P(x-1,y-1)]==0 && p[P(x-1,y)]==0 && p[P(x-1,y+1)]==0 &&
                    p[P(x+1,y-1)]!=0 && p[P(x+1,y)]!=0 && p[P(x+1,y+1)]!=0)
                    REMOVE_P;

                if (p[P(x-1,y-1)]!=0 && p[P(x-1,y)]!=0 && p[P(x-1,y+1)]!=0 &&
                    p[P(x+1,y-1)]==0 && p[P(x+1,y)]==0 && p[P(x+1,y+1)]==0)
                    REMOVE_P;

                /*
                //   0 0  0 0      1      1  
                // 1 1 0  0 1 1  0 1 1  1 1 0
                //   1      1    0 0      0 0
                */                
                if (p[P(x,y-1)]==0 && p[P(x+1,y-1)]==0 && p[P(x+1,y)]==0 &&
                    p[P(x-1,y)]!=0 && p[P(x,y+1)]!=0)
                    REMOVE_P;

                if (p[P(x-1,y-1)]==0 && p[P(x,y-1)]==0 && p[P(x-1,y)]==0 &&
                    p[P(x+1,y)]!=0 && p[P(x,y+1)]!=0)
                    REMOVE_P;

                if (p[P(x-1,y+1)]==0 && p[P(x-1,y)]==0 && p[P(x,y+1)]==0 &&
                    p[P(x+1,y)]!=0 && p[P(x,y-1)]!=0)
                    REMOVE_P;

                if (p[P(x+1,y+1)]==0 && p[P(x+1,y)]==0 && p[P(x,y+1)]==0 &&
                    p[P(x-1,y)]!=0 && p[P(x,y-1)]!=0)
                    REMOVE_P;                
            }
        }
    }
    
    ImageRemoveSpurs(image);

    return ImageSetFlag(image, FvsImageThinned);
}





