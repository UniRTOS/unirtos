/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_DEVICE_H__
#define __HAL_VOLC_DEVICE_H__

#include <stdint.h>

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
 * @brief 获取当前平台的名称。
 *
 * 该函数将当前平台的名称复制到指定的缓冲区中。
 *
 * @param name 指向用于存储平台名称的字符数组的指针。
 * @param len 缓冲区的长度，以字节为单位。
 * @return 如果成功，返回复制到缓冲区的字符数；如果缓冲区长度不足，返回所需的最小缓冲区长度。
 */
__byte_rtc_api__ uint32_t volc_get_platform_name(char* name, uint32_t len);

/**
 * @brief 获取当前操作系统的版本信息。
 *
 * 该函数将当前操作系统的版本信息复制到指定的缓冲区中。
 *
 * @param name 指向用于存储操作系统版本信息的字符数组的指针。
 * @param len 缓冲区的长度，以字节为单位。
 * @return 如果成功，返回复制到缓冲区的字符数；如果缓冲区长度不足，返回所需的最小缓冲区长度。
 */
__byte_rtc_api__ uint32_t volc_get_os_version(char* name, uint32_t len);

/**
 * @brief 获取当前使用的编译器信息。
 *
 * 该函数将当前使用的编译器信息复制到指定的缓冲区中。
 *
 * @param name 指向用于存储编译器信息的字符数组的指针。
 * @param len 缓冲区的长度，以字节为单位。
 * @return 如果成功，返回复制到缓冲区的字符数；如果缓冲区长度不足，返回所需的最小缓冲区长度。
 */
__byte_rtc_api__ uint32_t volc_get_compiler_info(char* name, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_DEVICE_H__ */
