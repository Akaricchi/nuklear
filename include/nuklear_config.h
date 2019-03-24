
#ifndef NK_CONFIG_H
#define NK_CONFIG_H

#include <math.h>
#include <string.h>

#include "nuklear_config_auto.h"

#ifndef NK_POOL_DEFAULT_CAPACITY
#define NK_POOL_DEFAULT_CAPACITY 16
#endif

#ifndef NK_DEFAULT_COMMAND_BUFFER_SIZE
#define NK_DEFAULT_COMMAND_BUFFER_SIZE (4*1024)
#endif

#ifndef NK_BUFFER_DEFAULT_INITIAL_SIZE
#define NK_BUFFER_DEFAULT_INITIAL_SIZE (4*1024)
#endif

/* standard library headers */
#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
#include <stdlib.h> /* malloc, free */
#endif
#ifdef NK_INCLUDE_STANDARD_IO
#include <stdio.h> /* fopen, fclose,... */
#endif
#ifdef NK_INCLUDE_STANDARD_VARARGS
#include <stdarg.h> /* valist, va_start, va_end, ... */
#endif
#ifndef NK_ASSERT
#include <assert.h>
#define NK_ASSERT(expr) assert(expr)
#endif

#ifndef NK_MEMSET
#define NK_MEMSET nk_memset
#endif
#ifndef NK_MEMCPY
#define NK_MEMCPY nk_memcopy
#endif
#ifndef NK_SQRT
#define NK_SQRT nk_sqrt
#endif
#ifndef NK_SIN
#define NK_SIN nk_sin
#endif
#ifndef NK_COS
#define NK_COS nk_cos
#endif
#ifndef NK_STRTOD
#define NK_STRTOD nk_strtod
#endif
#ifndef NK_DTOA
#define NK_DTOA nk_dtoa
#endif

#endif // NK_CONFIG_H
