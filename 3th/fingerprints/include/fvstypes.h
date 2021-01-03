/*#############################################################################
 * �ļ�����fvstype.h
 * ���ܣ�  �������͵Ķ���
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#if !defined FVS__FVSTYPES_HEADER__INCLUDED__
#define FVS__FVSTYPES_HEADER__INCLUDED__

/******************************************************************************
  * ��Щ���Ϳ����Ѿ���ϵͳ�ж����ˣ������Լ�ϵͳ������޸�
******************************************************************************/
#define HAVE_STDINT_H
#if defined(HAVE_STDINT_H) || defined(HAVE_INTTYPES_H)
	
	#if defined(HAVE_STDINT_H)
		#include <stdint.h>
	#endif

	#if defined(HAVE_INTTYPES_H)
		#include <inttypes.h>
	#endif

#else

    /* windows�û�ʹ�� */
    typedef unsigned char	uint8_t;
    typedef unsigned short	uint16_t;
    typedef unsigned int	uint32_t;

    typedef signed char		int8_t;
    typedef signed short	int16_t;
    typedef signed int		int32_t;

#endif


/* ����ָ������ */
typedef int				FvsInt_t;


/* ����ָ���޷������� */
typedef unsigned int	FvsUint_t;


/* �ֽڣ��֣�˫�� */
typedef int8_t			FvsInt8_t;
typedef int16_t			FvsInt16_t;
typedef int32_t			FvsInt32_t;


/* �޷����ֽڣ��֣�˫�� */
typedef uint8_t			FvsUint8_t;
typedef uint16_t		FvsUint16_t;
typedef uint32_t		FvsUint32_t;

typedef uint8_t			FvsByte_t;
typedef uint16_t		FvsWord_t;
typedef uint32_t		FvsDword_t;

#define bool_t char
#define false 0
#define true 1

/* �������� */
typedef double			FvsFloat_t;


/* ָ������ */
typedef void*			FvsPointer_t;


/* ������ͣ�����������͸����ָ�� */
typedef void*			FvsHandle_t;


/* �ַ������� */
typedef char*			FvsString_t;


/* �������� */
typedef enum FvsBool_t
{
	/* false��� */
	FvsFalse = 0,
    /* true��� */
	FvsTrue  = 1
} FvsBool_t;


/* ����PIֵ */
#ifndef M_PI
#define M_PI			3.1415926535897932384626433832795
#endif


/* ����������� */
typedef enum FvsError_t
{
	/* Ϊ������� */
	FvsFailure			= -1,
	/* û�д��� */
	FvsOK				= 0,
	/* �ڴ治�� */
	FvsMemory,
	/* ������Ч */
	FvsBadParameter,
	/* �ļ���ʽ���� */
	FvsBadFormat,
	/* ����������� */
	FvsIoError,
} FvsError_t;


#endif /* FVS__FVSTYPES_HEADER__INCLUDED__ */

