#ifndef __SNPRINTF_H__
#define __SNPRINTF_H__

#include "stddef.h"
#include "float.h"
#define HAVE_STDARG_H 1

#if HAVE_STDARG_H
#include <stdarg.h>
#if !HAVE_VSNPRINTF
int rpl_vsnprintf(char *, size_t, const char *, va_list);
#endif	/* !HAVE_VSNPRINTF */
#if !HAVE_SNPRINTF
int rpl_snprintf(char *, size_t, const char *, ...);
#endif	/* !HAVE_SNPRINTF */
#if !HAVE_VASPRINTF
int rpl_vasprintf(char **, const char *, va_list);
#endif	/* !HAVE_VASPRINTF */
#if !HAVE_ASPRINTF
int rpl_asprintf(char **, const char *, ...);
#endif	/* !HAVE_ASPRINTF */
#endif	/* HAVE_STDARG_H */
#endif	/* SYSTEM_H */
