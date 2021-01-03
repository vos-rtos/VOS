/*#############################################################################
 * 文件名：fvs_direction.c
 * 功能：  指纹图像方向图计算
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/



#include "fvs.h"
#define fprintf

static FvsError_t OverlayDirection(FvsImage_t image, const FvsFloatField_t field)
{
    FvsError_t nRet = FvsOK;
    FvsInt_t w      = ImageGetWidth (image);
    FvsInt_t h      = ImageGetHeight(image);
    FvsInt_t pitch, dirp;
    FvsFloat_t theta, c, s;
    FvsByte_t* p;
    FvsFloat_t* orientation;
    FvsInt_t x, y, size, i, j, l;

    size = 8; 

    (void)ImageLuminosity(image, 168);

    pitch  = ImageGetPitch (image);
    p      = ImageGetBuffer(image);

    orientation = FloatFieldGetBuffer(field);
    dirp        = FloatFieldGetPitch(field);

    if (p==NULL || orientation==NULL)
        return FvsMemory;

    for (y = size; y < h-size; y+=size-2)
    for (x = size; x < w-size; x+=size-2)
    {
        theta = orientation[x+y*dirp];
        c = cos(theta);
        s = sin(theta);
        
        for (l = 0; l < size; l++)
        {
            i = (FvsInt_t)(x + size/2 - l*s);
            j = (FvsInt_t)(y + size/2 + l*c);
            p[i+j*pitch] = 0;
        }
    }
    return nRet;
}

int direction_main(int argc, char *argv[])
{
    FvsImage_t      image = ImageCreate();
    FvsFloatField_t field = FloatFieldCreate();

    if (argc!=3)
    {
        printf("Usage: fvs input.bmp output.bmp\n");
        FloatFieldDestroy(field);
        ImageDestroy(image);
        return -1;
    }

    if ( (image!=NULL) && (field!=NULL) )
    {
        fprintf(stdout, "Opening file %s...\n", argv[1]);
        (void)FvsImageImport(image, argv[1]);
	(void)ImageSoftenMean(image, 3);

        fprintf(stdout, "1/2 Determining the ridge direction\n");
        (void)FingerprintGetDirection(image, field, 7, 8);

        fprintf(stdout, "2/2 Creating a new image with the direction\n");
        (void)OverlayDirection(image, field);

        (void)FvsImageExport(image, argv[2]);
        fprintf(stdout, "Cleaning up and exiting...\n");
    }
    FloatFieldDestroy(field);
    ImageDestroy(image);
    return 0;
}


