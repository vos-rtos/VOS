/*#############################################################################
 * 文件名：import.h
 * 功能：  加载图像
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#if !defined FVS__IMPORT_HEADER__INCLUDED__
#define FVS__IMPORT_HEADER__INCLUDED__

#include "file.h"
#include "image.h"


/******************************************************************************
  * 功能：从文件中加载指纹图像
  * 参数：image       指纹图像
  *       filename    文件名
  * 返回：错误编号
******************************************************************************/
FvsError_t FvsImageImport(FvsImage_t image, const FvsString_t filename);


#endif /* FVS__IMPORT_HEADER__INCLUDED__ */

