/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_ERRNO_H__
#define __HAL_VOLC_ERRNO_H__

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

#define VOLC_SUCCESS    0
#define VOLC_FAILED     -1

/**
 * @brief 获取当前fd的errno
 * 
 * @param fd 文件描述符
 * @return 返回errno
 */
__byte_rtc_api__ int volc_errno(int fd);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_ERRNO_H__ */
