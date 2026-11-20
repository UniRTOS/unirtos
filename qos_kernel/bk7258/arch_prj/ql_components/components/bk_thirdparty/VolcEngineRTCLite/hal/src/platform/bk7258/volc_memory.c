#include <common/bk_include.h>
#include <components/log.h>
#include <os/mem.h>
#include "FreeRTOS_POSIX.h"

#include "volc_memory.h"

#include <stdlib.h>

#define TAG "volc_m"

void* volc_malloc(size_t size) {
#if CONFIG_PSRAM_AS_SYS_MEMORY
    return psram_malloc(size);
#else
    return os_malloc(size);
#endif
}

void* volc_align_alloc(size_t size, size_t alignment) {
    if (alignment > portBYTE_ALIGNMENT) {
        BK_LOGW(TAG, "Not support alloc alignment(%d).\n", alignment);
        return NULL;
    }

    return volc_malloc(size);
}

void* volc_calloc(size_t num, size_t size) {
	if (size && num > (~(size_t) 0) / size)
		return NULL;

#if CONFIG_PSRAM_AS_SYS_MEMORY
    return psram_zalloc(num * size);
#else
    return os_zalloc(num * size);
#endif
}

void* volc_realloc(void* ptr, size_t new_size) {
#if CONFIG_PSRAM_AS_SYS_MEMORY
    return bk_psram_realloc(ptr, new_size);
#else
    return os_realloc(ptr, new_size);
#endif
}

void volc_free(void* ptr) {
#if CONFIG_PSRAM_AS_SYS_MEMORY
    psram_free(ptr);
#else
    os_free(ptr);
#endif
}

bool volc_memory_check(void* ptr, uint8_t val, size_t size) {
    uint8_t* p_buf = (uint8_t *)ptr;
    
    if (NULL == p_buf) {
        return false;
    }

    for (int i = 0; i < size; p_buf++, i++) {
        if (*p_buf != val) {
            return false;
        }
    }

    return true;
}