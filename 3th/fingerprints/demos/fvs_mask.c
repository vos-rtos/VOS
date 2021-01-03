/*#############################################################################
 * 文件名：fvs_mask.c
 * 功能：  指纹图像有效区域
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/


#include "fvs.h"
#define fprintf

int mask_main(int argc, char *argv[])
{
    FvsImage_t image;
    FvsImage_t mask;
    FvsFloatField_t direction;
    FvsFloatField_t frequency;

    if (argc!=3)
    {
        printf("Usage: fvs input.bmp output.bmp\n");
        return -1;
    }

    mask      = ImageCreate();
    image     = ImageCreate();
    direction = FloatFieldCreate();
    frequency = FloatFieldCreate();

    if (mask!=NULL && image!=NULL && direction!=NULL && frequency!=NULL)
    {
        (void)FvsImageImport(image, argv[1]);
	(void)ImageSoftenMean(image, 3);

        fprintf(stdout, "1/3 Determining the ridge direction\n");
        (void)FingerprintGetDirection(image, direction, 7, 8);

        fprintf(stdout, "2/3 Determining the ridge frequency\n");
        (void)FingerprintGetFrequency(image, direction, frequency);

        fprintf(stdout, "3/3 Creating the mask\n");
        (void)FingerprintGetMask(image, direction, frequency, mask);

        (void)FvsImageExport(mask, argv[2]);
    }
    fprintf(stdout, "Cleaning up and exiting...\n");
    FloatFieldDestroy(frequency);
    FloatFieldDestroy(direction);
    ImageDestroy(image);
    ImageDestroy(mask);
    return 0;
}


