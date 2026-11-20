/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_MUTEX_H__
#define __HAL_VOLC_MUTEX_H__

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
 * @type keytype
 * @brief mutex句柄
 */
typedef void* volc_mutex_t;

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 互斥锁创建
 * @param reentrant 是否可重入
 * @return 方法调用结果：<br>
 *         - 成功: 互斥锁句柄 <br>
 *         - 失败: NULL
 */
volc_mutex_t volc_mutex_create(bool reentrant);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 互斥锁 Lock
 * @param mutex 互斥锁句柄
 */
void volc_mutex_lock(volc_mutex_t mutex);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 互斥锁 Trylock
 * @param mutex 互斥锁句柄
 * @return 方法调用结果：<br>
 *         - 成功: true <br>
 *         - 失败: false
 */
bool volc_mutex_trylock(volc_mutex_t mutex);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 互斥锁 Unlock
 * @param mutex 互斥锁句柄
 */
 void volc_mutex_unlock(volc_mutex_t mutex);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 互斥锁销毁
 * @param mutex 互斥锁句柄
 */
void volc_mutex_destroy(volc_mutex_t mutex);


#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_MEMORY_H__ */
