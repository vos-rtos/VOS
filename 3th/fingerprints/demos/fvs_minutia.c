/*#############################################################################
 * 文件名：fvs_minutia.c
 * 功能：  指纹图像细节点提取
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/



#include "fvs.h"
#define fprintf

int minutia_main(int argc, char *argv[])
{
    FvsImage_t mask;
    FvsImage_t image;
    FvsFloatField_t direction;
    FvsFloatField_t frequency;
    FvsMinutiaSet_t minutia;

    if (argc!=3)
    {
        printf("Usage: fvs input.bmp output.bmp\n");
        return -1;
    }

    direction = FloatFieldCreate();
    frequency = FloatFieldCreate();
    mask      = ImageCreate();
    image     = ImageCreate();
    minutia   = MinutiaSetCreate(128);//5000

    if (direction!=NULL && frequency!=NULL &&
        mask!=NULL && image!=NULL && minutia!=NULL)
    {
        (void)FvsImageImport(image, argv[1]);
	(void)ImageSoftenMean(image, 3);

        fprintf(stdout, "1/8 Determining the ridge direction\n");
        (void)FingerprintGetDirection(image, direction, 5, 8);

        fprintf(stdout, "2/8 Determining the ridge frequency\n");
        (void)FingerprintGetFrequency(image, direction, frequency);

        fprintf(stdout, "3/8 Creating the mask\n");
        (void)FingerprintGetMask(image, direction, frequency, mask);

        fprintf(stdout, "4/8 Enhancing the fingerprint image\n");
        (void)ImageEnhanceGabor(image, direction, frequency, mask, 4.0);

        fprintf(stdout, "5/8 Binarize\n");
        (void)ImageBinarize(image, (FvsByte_t)0x80);

        fprintf(stdout, "6/8 Thinning\n");
        (void)ImageThinHitMiss(image);

        fprintf(stdout, "7/8 Detecting minutia\n");
        (void)MinutiaSetExtract(minutia, image, direction, mask);

        fprintf(stdout, "8/8 Drawing minutia\n");
        (void)ImageClear(image);
        (void)MinutiaSetDraw(minutia, image);

        (void)FvsImageExport(image, argv[2]);
    }
    fprintf(stdout, "Cleaning up and exiting...\n");
    MinutiaSetDestroy(minutia);
    ImageDestroy(image);
    ImageDestroy(mask);
    FloatFieldDestroy(frequency);
    FloatFieldDestroy(direction);

    return 0;
}


