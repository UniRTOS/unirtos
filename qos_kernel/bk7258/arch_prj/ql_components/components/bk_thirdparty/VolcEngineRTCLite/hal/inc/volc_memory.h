/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_MEMORY_H__
#define __HAL_VOLC_MEMORY_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#if defined(__BUILDING_BYTE_RTC_SDK__)
#define __byte_rtc_api__ __declspec(dllexport)
#else
#define __byte_rtc_api__ __declspec(dllimport)
#endif
#else
#define __byte_rtc_api__ __attribute__((visibility("default")))
#endif

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 申请内存
 * @param size 申请内存大小，单位: 字节
 * @return 方法调用结果：<br>
 *         - 成功: 内存首地址 <br>
 *         - 失败: NULL
 */
__byte_rtc_api__ void* volc_malloc(size_t size);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 申请指定对齐方式的内存块
 * @param size 申请内存大小，单位: 字节
 * @param alignment 所需对齐的字节数
 * @return 方法调用结果：<br>
 *         - 成功: 内存首地址 <br>
 *         - 失败: NULL
 */
__byte_rtc_api__ void* volc_align_alloc(size_t size, size_t alignment);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 申请内存，并初始化分配的内存块为零
 * @param num 要分配的元素数量
 * @param size 申请内存大小，单位: 字节
 * @return 方法调用结果：<br>
 *         - 成功: 内存首地址 <br>
 *         - 失败: NULL
 */
__byte_rtc_api__ void* volc_calloc(size_t num, size_t size);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 申请内存，重新分配先前分配的内存块的大小
 * @param ptr 指向之前分配的内存块的指针，如果ptr为NULL，则行为与volc_malloc一致
 * @param new_size 申请内存大小，单位: 字节
 * @return 方法调用结果：<br>
 *         - 成功: 内存首地址 <br>
 *         - 失败: NULL
 */
__byte_rtc_api__ void* volc_realloc(void* ptr, size_t new_size);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 释放内存
 * @param ptr 待释放内存首地址
 */
__byte_rtc_api__ void volc_free(void* ptr);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 检查内存中的值
 * @param ptr 内存首地址
 * @param val 待比较的值
 * @param size 内存的带下，单位: 字节
 * @return 方法调用结果: <br>
 *         - 成功: true
 *         - 失败: false
 */
__byte_rtc_api__ bool volc_memory_check(void* ptr, uint8_t val, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_MEMORY_H__ */
