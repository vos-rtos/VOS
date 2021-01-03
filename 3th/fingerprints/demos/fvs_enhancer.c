/*#############################################################################
 * 文件名：fvs_enhancer.c
 * 功能：  指纹图像增强
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/


#include "fvs.h"
#include <stdlib.h>
#define fprintf

int enhancer_main(int argc, char *argv[])
{
    FvsImage_t mask;
    FvsImage_t image;
    FvsFloatField_t direction;
    FvsFloatField_t frequency;
    FvsFloat_t radius = 4.0;

    if (argc<3 || argc>4)
    {
        printf("Usage: fvs input.bmp output.bmp [radius]\n");
        return -1;
    }
    if (argc==4)
       radius = atof(argv[3]);

    mask      = ImageCreate();
    image     = ImageCreate();
    direction = FloatFieldCreate();
    frequency = FloatFieldCreate();

    if (mask!=NULL && image!=NULL && direction!=NULL && frequency!=NULL)
    {
	fprintf(stdout, "Opening file %s...\n", argv[1]);
	(void)FvsImageImport(image, argv[1]);
	(void)ImageSoftenMean(image, 3);

        fprintf(stdout, "1/4 Determining the ridge direction\n");
        (void)FingerprintGetDirection(image, direction, 7, 8);

        fprintf(stdout, "2/4 Determining the ridge frequency\n");
        (void)FingerprintGetFrequency(image, direction, frequency);

        fprintf(stdout, "3/4 Creating the mask\n");
        (void)FingerprintGetMask(image, direction, frequency, mask);

        fprintf(stdout, "4/4 Enhancing the fingerprint image\n");
        (void)ImageEnhanceGabor(image, direction, frequency, NULL, radius);

	fprintf(stdout, "Saving file %s...\n", argv[2]);
        (void)FvsImageExport(image, argv[2]);
    }
    fprintf(stdout, "Cleaning up and exiting...\n");
    FloatFieldDestroy(frequency);
    FloatFieldDestroy(direction);
    ImageDestroy(image);
    ImageDestroy(mask);
    return 0;
}


