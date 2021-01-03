/*#############################################################################
 * 文件名：export.c
 * 功能：  指纹图像输出保存
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include "import.h"

#include <stdio.h>
//#include <magick/api.h>

#if 0
/******************************************************************************
  * 功能：将一个指纹图像输出到一个文件，文件的格式由文件的扩展名决定
  * 参数：filename  将要保存图像的文件名
  *       image     将要导出的图像
  * 返回：错误代码
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

	/* 从输入图像中获取 buffer, size 和 pitch */
	buffer	= ImageGetBuffer(image);
	pitch	= ImageGetPitch(image);
	height	= ImageGetHeight(image);
	width	= ImageGetWidth(image);

	/* 初始化ImageMagick环境 */
	InitializeMagick(".");
	GetExceptionInfo(&exception);

	/* 创建一个空的imageinfo */
	magicinfo = CloneImageInfo((ImageInfo*)NULL);
	magicinfo->depth = 8;

	/* 创建图像 */
	magicimage = ConstituteImage(width, height, "I", CharPixel, 
									buffer, &exception);
	if (exception.severity!=UndefinedException)
		CatchException(&exception);
	if (magicimage!=(Image*)NULL)
	{
		/* 拷贝数据 */
		for (i=0; i<height; i++)
		{
			ImportImagePixels(magicimage, 0, i, width, 1, "I", CharPixel,
			buffer+i*pitch);
		}
		
		/* 保存图像到文件 */
		(void)strcpy(magicimage->filename, filename);
		magicimage->colorspace = GRAYColorspace;
		magicimage->depth      = 8;
		WriteImage(magicinfo, magicimage);

		DestroyImage(magicimage);
	}
	else
		ret = FvsFailure;

	/* 清理 */
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
  * 功能：将一个指纹图像输出到一个文件，文件的格式由文件的扩展名决定
  * 参数：filename  将要保存图像的文件名
  *       image     将要导出的图像
  * 返回：错误代码
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

	/* 初始化Magick */
	wand = NewMagickWand();
	if (wand!=NULL)
	{
		/* 提取参数 */
		buffer = ImageGetBuffer(image);
		width  = ImageGetWidth(image);
		height = ImageGetHeight(image);
		pitch  = ImageGetPitch(image);
	
		/* 为像素申请新的内存 */
		out = (FvsByte_t*)malloc(width*height);
		if (out!=NULL)
		{
			for (i=0; i<height; i++)
			memcpy(out+i*width, buffer+i*pitch, width);

			/* 输出的图像数据现在在连续的缓冲区中 */
			MagickSetSize(wand, width, height);
			MagickSetImagePixels(wand, 0, 0, width, height, "I", CharPixel, out);
	    
			/* 写数据 */    
			MagickWriteImage(wand, filename);
	    
			free(out);
		}
		else
			ret = FvsMemory;

		/* 清理 */	
		DestroyMagickWand(wand);    
	}
	else
		ret = FvsMemory;
    
	return ret;
}

#endif

