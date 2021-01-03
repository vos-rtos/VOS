/*#############################################################################
 * 文件名：fvs_createtestimages.c
 * 功能：  创建测试图像集
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/



#include "fvs.h"

#define fprintf
#define printf kprintf

#define IMG_SIZE 256


static void GenerateLineImage(FvsImage_t img, FvsInt_t spacing, FvsInt_t angle)
{
    FvsInt_t x, y, w, h;
    char name[1024];
    FvsFloat_t dir   = angle*M_PI/180.0;
    FvsFloat_t piom, s, c;
    FvsByte_t* p     = ImageGetBuffer(img);
    FvsInt_t pitch   = ImageGetPitch(img);

    if (p==NULL)
    {
        printf("Error, NULL pointer in function GenerateTestImage\n");
        return;
    }

    w = ImageGetWidth(img);
    h = ImageGetHeight(img);
    s = sin(dir);
    c = cos(dir);

    printf("Generating a test image with spacing %3i and angle %3i\n", spacing, angle);
    (void)ImageClear(img);
    piom = 2*M_PI / (FvsFloat_t)(spacing);

    for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
    {
        p[x+y*pitch] = (uint8_t)(127.5*(1+cos(piom*(-x*s + y*c))));
    }

    (void)snprintf(name, (size_t)1000, "testimg%03i%03i.bmp", spacing, angle);
    (void)FvsImageExport(img, name);
}

static void GenerateCircleImage(FvsImage_t img, FvsInt_t spacing)
{
    FvsInt_t x, y, w, h;
    char name[1024];
    FvsFloat_t piom, d;
    FvsByte_t* p     = ImageGetBuffer(img);
    FvsInt_t pitch   = ImageGetPitch(img);

    if (p==NULL)
    {
        printf("Error, NULL pointer in function GenerateCircleImage\n");
        return;
    }

    w = ImageGetWidth(img);
    h = ImageGetHeight(img);

    printf("Generating a circle test image with spacing %3i\n", spacing);
    (void)ImageClear(img);
    piom = 2*M_PI / (FvsFloat_t)(spacing);

    for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
    {
        d = sqrt((h/2-y)*(h/2-y) + (w/2-x)*(w/2-x));
        p[x+y*pitch] = (uint8_t)(127.5*(1+cos(piom*d)));
    }

    (void)snprintf(name, (size_t)1000, "testimgcircle%03i.bmp", spacing);
    (void)FvsImageExport(img, name);
}

int create_main(int argc, char *argv[])
{
    FvsImage_t img = ImageCreate();
    FvsInt_t   spacing, angle;
    FvsError_t nRet;

    printf("Creating a new image\n");    
    if (img==NULL) return -1;

    printf("Setting the size\n");
    nRet = ImageSetSize(img, IMG_SIZE, IMG_SIZE);
    if (nRet==FvsOK)
    {
        for (spacing=4; spacing <= 16; spacing+=4)
	{
            for (angle=0; angle < 180; angle+=15)
                GenerateLineImage(img, spacing, angle);
	    GenerateCircleImage(img, spacing);
	}
    }

    printf("Cleaning up\n");
    ImageDestroy(img);
    return 0;
}


