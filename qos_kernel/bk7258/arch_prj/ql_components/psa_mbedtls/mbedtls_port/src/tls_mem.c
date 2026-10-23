#include "mbedtls/build_info.h"

#ifdef MBEDTLS_PLATFORM_MEMORY

#include <os/mem.h>

void *tls_mbedtls_mem_calloc(size_t n, size_t size)
{
	unsigned int len = n * size;
	if(len == 0){
		return 0;
	}
#if CONFIG_MBEDTLS_USE_PSRAM
    return psram_zalloc( len );
#else
    return os_zalloc( len );
#endif
}

void tls_mbedtls_mem_free(void *ptr)
{
#if CONFIG_MBEDTLS_USE_PSRAM
    psram_free(ptr);
#else
    os_free(ptr);
#endif
}

#endif /* !MBEDTLS_PLATFORM_MEMORY */
