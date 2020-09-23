#ifndef __CC_H__
#define __CC_H__

#include "cpu.h"
#include "stdio.h"

#define printf kprintf


typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;
//typedef u32_t mem_ptr_t;
typedef int sys_prot_t;



#define SYS_ARCH_DECL_PROTECT(lev)	u32_t irq_save
#define SYS_ARCH_PROTECT(lev)		irq_save = __vos_irq_save()
#define SYS_ARCH_UNPROTECT(lev)		__vos_irq_restore(irq_save)

//根据不同的编译器定义一些符号
#if defined (__ICCARM__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES

#elif defined (__CC_ARM)

#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__GNUC__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__TASKING__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif

//LWIP用printf调试时使用到的一些类型
#define U16_F "4d"
#define S16_F "4d"
#define X16_F "4x"
#define U32_F "8ld"
#define S32_F "8ld"
#define X32_F "8lx"


#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) \
    do \
    {   kprintf("Assertion \"%s\" failed at line %d in %s\r\n", x, __LINE__, __FILE__); \
    	while(1);\
    } while(0)
#endif

#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x) do {kprintf x;} while(0)
#endif

#endif /* __CC_H__ */
