/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_ATOMIC_H__
#define __HAL_VOLC_ATOMIC_H__

#include <stdbool.h>
#include <stddef.h>

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
 * @brief 将一个值原子地存储到指定的原子变量中。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param val 要存储的值。
 */
__byte_rtc_api__ void volc_atomic_store(volatile size_t* p_atomic, size_t val);

/**
 * @brief 原子地比较并交换指定原子变量的值。
 *
 * 如果原子变量的值等于预期值，则将其替换为期望的值。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param p_expected 指向预期值的指针。如果比较成功，该指针指向的值将被更新为原子变量的旧值。
 * @param desired 期望的值。
 * @return 如果比较并交换操作成功，则返回 true；否则返回 false。
 */
__byte_rtc_api__ bool volc_atomic_compare_exchange(volatile size_t* p_atomic, size_t* p_expected, size_t desired);

/**
 * @brief 原子地加载指定原子变量的值。
 *
 * @param p_atomic 指向原子变量的指针。
 * @return 原子变量的当前值。
 */
__byte_rtc_api__ size_t volc_atomic_load(volatile size_t* p_atomic);

/**
 * @brief 原子地交换指定原子变量的值。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param val 要交换的值。
 * @return 原子变量的旧值。
 */
__byte_rtc_api__ size_t volc_atomic_exchange(volatile size_t* p_atomic, size_t val);

/**
 * @brief 原子地将指定原子变量的值加 1。
 *
 * @param p_atomic 指向原子变量的指针。
 * @return 原子变量的新值。
 */
__byte_rtc_api__ size_t volc_atomic_increment(volatile size_t* p_atomic);

/**
 * @brief 原子地将指定原子变量的值减 1。
 *
 * @param p_atomic 指向原子变量的指针。
 * @return 原子变量的新值。
 */
__byte_rtc_api__ size_t volc_atomic_decrement(volatile size_t* p_atomic);

/**
 * @brief 原子地将指定原子变量的值加上一个给定的值。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param val 要添加的值。
 * @return 原子变量的新值。
 */
__byte_rtc_api__ size_t volc_atomic_add(volatile size_t* p_atomic, size_t val);

/**
 * @brief 原子地将指定原子变量的值减去一个给定的值。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param val 要减去的值。
 * @return 原子变量的新值。
 */
__byte_rtc_api__ size_t volc_atomic_sub(volatile size_t* p_atomic, size_t val);

/**
 * @brief 原子地对指定原子变量的值和一个给定的值进行按位与操作。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param val 要进行按位与操作的值。
 * @return 原子变量的新值。
 */
__byte_rtc_api__ size_t volc_atomic_and(volatile size_t* p_atomic, size_t val);

/**
 * @brief 原子地对指定原子变量的值和一个给定的值进行按位或操作。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param val 要进行按位或操作的值。
 * @return 原子变量的新值。
 */
__byte_rtc_api__ size_t volc_atomic_or(volatile size_t* p_atomic, size_t val);

/**
 * @brief 原子地对指定原子变量的值和一个给定的值进行按位异或操作。
 *
 * @param p_atomic 指向原子变量的指针。
 * @param val 要进行按位异或操作的值。
 * @return 原子变量的新值。
 */
__byte_rtc_api__ size_t volc_atomic_xor(volatile size_t* p_atomic, size_t val);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_ATOMIC_H__ */
