/*#############################################################################
 * �ļ�����export.c
 * ���ܣ�  ָ��ͼ���������
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include "import.h"

#include <stdio.h>
//#include <magick/api.h>

#if 0
/******************************************************************************
  * ���ܣ���һ��ָ��ͼ�������һ���ļ����ļ��ĸ�ʽ���ļ�����չ������
  * ������filename  ��Ҫ����ͼ����ļ���
  *       image     ��Ҫ������ͼ��
  * ���أ��������
******************************************************************************/
FvsError_t FvsImageExport(const FvsImage_t image, const FvsString_t filename)
{

	ExceptionInfo	exception;
	Image*			magicimage;
	ImageInfo*		magicinfo;

	FvsError_t ret = FvsOK;
	FvsByte_t*		buffer;
	FvsInt_t		pitch;
	FvsInt_t		height;
	FvsInt_t		width;
	FvsInt_t		i;

	/* ������ͼ���л�ȡ buffer, size �� pitch */
	buffer	= ImageGetBuffer(image);
	pitch	= ImageGetPitch(image);
	height	= ImageGetHeight(image);
	width	= ImageGetWidth(image);

	/* ��ʼ��ImageMagick���� */
	InitializeMagick(".");
	GetExceptionInfo(&exception);

	/* ����һ���յ�imageinfo */
	magicinfo = CloneImageInfo((ImageInfo*)NULL);
	magicinfo->depth = 8;

	/* ����ͼ�� */
	magicimage = ConstituteImage(width, height, "I", CharPixel, 
									buffer, &exception);
	if (exception.severity!=UndefinedException)
		CatchException(&exception);
	if (magicimage!=(Image*)NULL)
	{
		/* �������� */
		for (i=0; i<height; i++)
		{
			ImportImagePixels(magicimage, 0, i, width, 1, "I", CharPixel,
			buffer+i*pitch);
		}
		
		/* ����ͼ���ļ� */
		(void)strcpy(magicimage->filename, filename);
		magicimage->colorspace = GRAYColorspace;
		magicimage->depth      = 8;
		WriteImage(magicinfo, magicimage);

		DestroyImage(magicimage);
	}
	else
		ret = FvsFailure;

	/* ���� */
	DestroyImageInfo(magicinfo);
	DestroyExceptionInfo(&exception);
	DestroyMagick();

	return ret;
}
#endif

//typedef struct tagBITMAPINFOHEADER{
//	typedef struct biSize;
//	LONG biWidth;
//	LONG biHeight;
//	WORD biPlanes;
//	WORD biBitCount;
//	typedef struct biCompression;
//	typedef struct biSizeImage;
//	LONG biXPelsPerMeter;
//	LONG biYPelsPerMeter;
//	typedef struct biClrUsed;
//	typedef struct biClrImportant;
//} BITMAPINFOHEADER;
//
//typedef struct tagRGBQUAD {
//    BYTE   rgbBlue;
//    BYTE   rgbGreen;
//    BYTE   rgbRed;
//    BYTE   rgbReserved;
//} RGBQUAD;
FvsError_t  FvsImageExport(const FvsImage_t image, const FvsString_t filename)
{
	return 0;
#if 0
	//FvsByte_t bmfh[14], BITMAPINFOHEADER *bmih, RGBQUAD *rgbq
	FvsByte_t bmfh[14];
	BITMAPINFOHEADER *bmih;
	RGBQUAD *rgbq;
    FvsError_t ret = FvsOK;
    FvsByte_t*      buffer;
    FvsInt_t        pitch;
    FvsInt_t        height;
    FvsInt_t        width;
    FvsInt_t        i;
    FvsFile_t   file;
    file      = FileCreate();
    if(FileOpen(file, filename, (FvsFileOptions_t)(FvsFileWrite | FvsFileCreate)) == FvsFailure) {
        ret = FvsFailure;
    }
    if(FileWrite(file, bmfh, 14) != 14)
        ret = FvsFailure;
    if(FileWrite(file, bmih, sizeof(BITMAPINFOHEADER)) != sizeof(BITMAPINFOHEADER))
        ret = FvsFailure;
    if(FileWrite(file, rgbq, sizeof(RGBQUAD) * 256) != sizeof(RGBQUAD) * 256)
        ret = FvsFailure;
    if(ret == FvsFailure) {
        printf("Write file error");
        return ret;
    }
    else {
        buffer = ImageGetBuffer(image);
        pitch  = ImageGetPitch(image);
        height = ImageGetHeight(image);
        width  = ImageGetWidth(image);
        for (i = height - 1; i >= 0; i--) {
            FileWrite(file, buffer + i * pitch, WIDTHBYTES(pitch * 8));
        }
    }
    FileDestroy(file);
    return ret;
#endif
}

#if 0

/* The WAND interface is pretty buggy... use the old API */
/******************************************************************************
  * ���ܣ���һ��ָ��ͼ�������һ���ļ����ļ��ĸ�ʽ���ļ�����չ������
  * ������filename  ��Ҫ����ͼ����ļ���
  *       image     ��Ҫ������ͼ��
  * ���أ��������
******************************************************************************/
FvsError_t FvsImageExport(const FvsImage_t image, const FvsString_t filename)
{
	MagickWand*		wand;
	FvsByte_t*		buffer;
	FvsByte_t*    	out;
	FvsInt_t		pitch;
	FvsInt_t		height;
	FvsInt_t		width;
	FvsError_t		ret = FvsOK;
	FvsInt_t		i;

	/* ��ʼ��Magick */
	wand = NewMagickWand();
	if (wand!=NULL)
	{
		/* ��ȡ���� */
		buffer = ImageGetBuffer(image);
		width  = ImageGetWidth(image);
		height = ImageGetHeight(image);
		pitch  = ImageGetPitch(image);
	
		/* Ϊ���������µ��ڴ� */
		out = (FvsByte_t*)malloc(width*height);
		if (out!=NULL)
		{
			for (i=0; i<height; i++)
			memcpy(out+i*width, buffer+i*pitch, width);

			/* �����ͼ�����������������Ļ������� */
			MagickSetSize(wand, width, height);
			MagickSetImagePixels(wand, 0, 0, width, height, "I", CharPixel, out);
	    
			/* д���� */    
			MagickWriteImage(wand, filename);
	    
			free(out);
		}
		else
			ret = FvsMemory;

		/* ���� */	
		DestroyMagickWand(wand);    
	}
	else
		ret = FvsMemory;
    
	return ret;
}

#endif

