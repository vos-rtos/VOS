#if 0
/*#############################################################################
 * 文件名：import.c
 * 功能：  一些基本的图像操作
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/


#include "import.h"

#include <stdio.h>
//#include <magick/api.h>

#define printf kprintf
/******************************************************************************
  * 功能：从文件中加载指纹图像
  * 参数：image       指纹图像
  *       filename    文件名
  * 返回：错误编号
******************************************************************************/
#if 0
FvsError_t FvsImageImport(FvsImage_t image, const FvsString_t filename)
{
#if 1
    ExceptionInfo exception;
    Image*        magicimage;
    ImageInfo*    magicinfo;
    
    FvsError_t ret = FvsOK;
    FvsByte_t*    buffer;
    FvsInt_t      pitch;
    FvsInt_t      height;
    FvsInt_t      width;
    FvsInt_t i;
    
    /* 初始化 Magick 环境 */
    InitializeMagick(".");
    GetExceptionInfo(&exception);

    /* 创建一个空的 imageinfo */
    magicinfo = CloneImageInfo((ImageInfo*)NULL);

    /* 设置文件名 */
    (void)strcpy(magicinfo->filename, filename);


    /* 读图像 */
    magicimage = ReadImage(magicinfo, &exception);
    if (exception.severity!=UndefinedException)
      	CatchException(&exception);
    if (magicimage!=(Image*)NULL)
    {
		ret = ImageSetSize(image,
	    	(FvsInt_t)magicimage->columns,
	    	(FvsInt_t)magicimage->rows);
		if (ret==FvsOK)
		{
	    	/* 获得缓冲区 */
	    	buffer = ImageGetBuffer(image);
	    	pitch  = ImageGetPitch(image);
	    	height = ImageGetHeight(image);
	    	width  = ImageGetWidth(image);
	    
	    	/* 归一化 */
	    	NormalizeImage(magicimage);
	
	    	/* 拷贝数据 */
	    	for (i=0; i<height; i++)
	    	{
				ExportImagePixels(magicimage, 0, i, width, 1, "I", CharPixel,
				buffer+i*pitch, &exception);
	    	}
		}
        DestroyImage(magicimage);
    }
    else
        ret = FvsFailure;
    
    /* 清理 */
    DestroyImageInfo(magicinfo);
    DestroyExceptionInfo(&exception);
    DestroyMagick();
    
    return ret;
#else
    return 0;
#endif
}
#endif

typedef int				INT;
typedef unsigned int	UINT;

/* This type MUST be 8-bit */
typedef unsigned char	BYTE;

/* These types MUST be 16-bit */
typedef short			SHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types MUST be 32-bit */
typedef long			LONG;
typedef unsigned long	typedef struct;

/* This type MUST be 64-bit (Remove this for ANSI C (C89) compatibility) */
typedef unsigned long long QWORD;

#define DIB_HEADER_MARKER   ((FvsWord_t) ('M' << 8) | 'B')


typedef struct tagBITMAPINFOHEADER{
	typedef struct biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	typedef struct biCompression;
	typedef struct biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	typedef struct biClrUsed;
	typedef struct biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    BYTE   rgbBlue;
    BYTE   rgbGreen;
    BYTE   rgbRed;
    BYTE   rgbReserved;
} RGBQUAD;
#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4);

FvsByte_t bmfh[14];
BITMAPINFOHEADER bmih0;
FvsByte_t *prgb_map;
FvsInt_t  rgb_map_size;

FvsError_t FvsImageImport(FvsImage_t image, const FvsString_t filename) {

//	BITMAPINFOHEADER *bmih;
//	RGBQUAD *rgbq;
    FvsError_t ret = FvsOK;
    FvsByte_t*    buffer;
    FvsInt_t      pitch;
    FvsInt_t      height;
    FvsInt_t      width;
    FvsInt_t i, x, y;
    FvsFile_t   file;
//  FvsByte_t bmfh0[14];
//    BITMAPINFOHEADER bmih0;
//    RGBQUAD rgbq0[256];
//    bmih = &bmih0;

    //rgbq = prgb_map;
    file      = FileCreate();
    if(FileOpen(file, filename, FvsFileRead) == FvsFailure) {
        ret = FvsFailure;
    }
    if(FileRead(file, bmfh, 14) != 14)
        ret = FvsFailure;
    if(*(FvsWord_t*)bmfh != DIB_HEADER_MARKER)
        ret = FvsFailure;
    if(FileRead(file, &bmih0, sizeof(BITMAPINFOHEADER)) != sizeof(BITMAPINFOHEADER))
        ret = FvsFailure;
    if(bmih0.biBitCount != 8)
        ret = FvsFailure;
    x = *(FvsDword_t*)(bmfh + 10);
    rgb_map_size = x - (sizeof(bmfh) + sizeof(bmih0));
    prgb_map = vmalloc(rgb_map_size);
    if(FileRead(file, prgb_map, rgb_map_size) != rgb_map_size)
        ret = FvsFailure;
    if(ret == FvsFailure) {
        printf("File format or Read file error");
        return ret;
    }
    else {
        ret = ImageSetSize(image, (FvsInt_t)bmih0.biWidth, (FvsInt_t)bmih0.biHeight);
        if (ret == FvsOK) {

            buffer = ImageGetBuffer(image);
            pitch  = ImageGetPitch(image);
            height = ImageGetHeight(image);
            width  = ImageGetWidth(image);
            x = *(FvsDword_t*)(bmfh + 10);
            for (i = 0; i < height; i++) {
                y = (height - 1 - i) * WIDTHBYTES(width * 8);
                FileSeek(file, x + y);
                FileRead(file, buffer + i * pitch, pitch);
            }
        }
    }
    FileDestroy(file);
    return ret;
}
#endif
