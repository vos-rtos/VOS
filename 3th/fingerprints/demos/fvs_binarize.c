/*#############################################################################
 * 文件名：fvs_binarize.c
 * 功能：  指纹图像二值化
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/


#include "fvs.h"

#define fprintf

int binary_main(int argc, char *argv[])
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

        fprintf(stdout, "1/5 Determining the ridge direction\n");
        (void)FingerprintGetDirection(image, direction, 5, 8);

        fprintf(stdout, "2/5 Determining the ridge frequency\n");
        (void)FingerprintGetFrequency(image, direction, frequency);

        fprintf(stdout, "3/5 Creating the mask\n");
        (void)FingerprintGetMask(image, direction, frequency, mask);

        fprintf(stdout, "4/5 Enhancing the fingerprint image\n");
        (void)ImageEnhanceGabor(image, direction, frequency, mask, 4.0);

        fprintf(stdout, "5/5 Binarize\n");
        (void)ImageBinarize(image, (FvsByte_t)0x80);

        (void)FvsImageExport(image, argv[2]);
    }
    fprintf(stdout, "Cleaning up and exiting...\n");
    ImageDestroy(image);
    ImageDestroy(mask);
    FloatFieldDestroy(direction);
    FloatFieldDestroy(frequency);

    return 0;
}


