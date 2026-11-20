/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_UUID_H__
#define __HAL_VOLC_UUID_H__

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
 * @brief 生成唯一ID
 * @param uuid 获取到的uuid
 */
__byte_rtc_api__ void volc_uuid_generate(char* uuid);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_UUID_H__ */
