/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_DEVICE_FINGER_PRINT_H__
#define __HAL_VOLC_DEVICE_FINGER_PRINT_H__

#include "volc_string.h"

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
 * @brief 获取设备指纹。
 *
 * 该函数用于获取当前设备的指纹信息，并将其存储在提供的 volc_string_t 结构体中。
 *
 * @param finger 指向 volc_string_t 结构体的指针，用于存储设备指纹信息。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_get_device_finger_print(volc_string_t* finger);

/**
 * @brief 生成设备 ID。
 *
 * 该函数用于生成一个唯一的设备 ID，并将其存储在提供的 volc_string_t 结构体中。
 *
 * @param did 指向 volc_string_t 结构体的指针，用于存储生成的设备 ID。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_generate_device_id(volc_string_t* did);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_DEVICE_FINGER_PRINT_H__ */
