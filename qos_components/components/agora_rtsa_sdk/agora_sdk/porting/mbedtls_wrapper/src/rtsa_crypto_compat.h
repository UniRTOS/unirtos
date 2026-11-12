#ifndef __RTSA_CRYPTO_COMPAT_H__
#define __RTSA_CRYPTO_COMPAT_H__

#include <stdlib.h>

/*
 * RTSA internal builds define RTSA_CRYPTO_WRAPPER_USE_RTSA_MEM so wrapper
 * allocations follow the same allocator domain as the rest of RTSA. Packaged
 * external wrapper builds do not define it and fall back to the standard C
 * allocator by default.
 */
#if defined(RTSA_CRYPTO_WRAPPER_USE_RTSA_MEM)
#include "api/aosl_mm.h"
#define RTSA_CRYPTO_WRAPPER_MALLOC aosl_malloc_impl
#define RTSA_CRYPTO_WRAPPER_FREE aosl_free_impl
#else
#define RTSA_CRYPTO_WRAPPER_MALLOC malloc
#define RTSA_CRYPTO_WRAPPER_FREE free
#endif

#endif
