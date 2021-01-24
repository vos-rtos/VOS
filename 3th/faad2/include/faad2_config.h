#include "vos.h"

#define FIXED_POINT 1

#define VOS_CLOSE_SBR 1 //关闭SBR_DEC, PS_DEC, 提高解码速度，减少输出数据一半大小，减少malloc空间，音质减低

#define HAVE_MEMCPY 1

#define PACKAGE_VERSION "2.10.0"


#define malloc vmalloc
#define free vfree
#define realloc vrealloc
#define calloc vcalloc
