/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

 #ifndef __HAL_VOLC_LOG_H__
 #define __HAL_VOLC_LOG_H__
 
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
 * @brief 输出日志
 * 
 * @param str 要输出的字符串
 * @return 预留值，返回0即可
 */
 __byte_rtc_api__ int volc_print(const char* str);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* __HAL_VOLC_LOG_H__ */
 