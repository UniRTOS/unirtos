/*************************************************************
 * Author:	zhangguanxian@agora.io
 * Date	 :	2025/12/16
 * Module:	memory hal definitions.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#ifndef __AOSL_HAL_MEMORY_H__
#define __AOSL_HAL_MEMORY_H__
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief allocate memory
 * @param [in] size size of memory to allocate
 * @return pointer to allocated memory, or NULL on error
 */
void *aosl_hal_malloc(size_t size);

/**
 * @brief   free memory
 * @param [in] ptr pointer to memory to free
 */
void aosl_hal_free(void *ptr);

/**
 * @brief allocate memory and initialize it to zero
 * @param [in] nmemb number of elements
 * @param [in] size size of each element
 * @return pointer to allocated memory, or NULL on error
 */
void *aosl_hal_calloc(size_t nmemb, size_t size);

/**
 * @brief reallocate memory
 * @param [in] ptr pointer to existing memory block
 * @param [in] size new size of memory block
 * @return pointer to reallocated memory, or NULL on error
 */
void *aosl_hal_realloc(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif