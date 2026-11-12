#include "qosa_def.h"
#include "aosl_hal_memory.h"

void *aosl_hal_malloc(size_t size)
{
    return qosa_malloc(size);
}

void aosl_hal_free(void *ptr)
{
    return qosa_free(ptr);
}

void *aosl_hal_calloc(size_t nmemb, size_t size)
{
    return qosa_calloc(nmemb,size);
}

void *aosl_hal_realloc(void *ptr, size_t size)
{
    return qosa_realloc(ptr,size);
}
