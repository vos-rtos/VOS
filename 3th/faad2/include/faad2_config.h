#include "vos.h"

#define FIXED_POINT 1

#define HAVE_MEMCPY 1

#define PACKAGE_VERSION "2.10.0"


#define malloc vmalloc
#define free vfree
#define realloc vrealloc
#define calloc vcalloc
