/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_TIME_H__
#define __HAL_VOLC_TIME_H__

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

// NOTE: Timer precision is in 100ns intervals. This is used in heuristics and in time functionality
#define VOLC_DEFAULT_TIME_UNIT_IN_NANOS         100
#define VOLC_HUNDREDS_OF_NANOS_IN_A_MICROSECOND 10LL
#define VOLC_HUNDREDS_OF_NANOS_IN_A_MILLISECOND (VOLC_HUNDREDS_OF_NANOS_IN_A_MICROSECOND * 1000LL)
#define VOLC_HUNDREDS_OF_NANOS_IN_A_SECOND      (VOLC_HUNDREDS_OF_NANOS_IN_A_MILLISECOND * 1000LL)
#define VOLC_HUNDREDS_OF_NANOS_IN_A_MINUTE      (VOLC_HUNDREDS_OF_NANOS_IN_A_SECOND * 60LL)
#define VOLC_HUNDREDS_OF_NANOS_IN_AN_HOUR       (VOLC_HUNDREDS_OF_NANOS_IN_A_MINUTE * 60LL)
#define VOLC_MILLISECONDS_IN_A_SECOND           1000
#define VOLC_MICROSECONDS_IN_A_MILLISECONDS_SECOND           1000
#define VOLC_NANOS_IN_A_MILLISECONDS_SECOND     1000000LL
#define VOLC_SECONDS_IN_A_DAY (24 * 60 * 60LL)
#define VOLC_HUNDREDS_OF_NANOS_IN_A_DAY (VOLC_HUNDREDS_OF_NANOS_IN_AN_HOUR * 24LL)

#define VOLC_INFINITE_TIME_VALUE ((uint64_t) 0xffffffffffffffff)

// yyyy-mm-dd HH:MM:SS.xxx
#define VOLC_MAX_TIMESTAMP_FORMAT_STR_LEN 23

// Max timestamp string length including null terminator
#define VOLC_MAX_TIMESTAMP_STR_LEN 17

/**
 * @brief 获取系统时间, ms
 */
__byte_rtc_api__ uint64_t volc_get_time_ms(void);

__byte_rtc_api__ uint64_t volc_get_montionic_time_ms(void);

/**
 * @locale zh
 * @type api
 * @list 方法
 * @brief 获取格式化时间
 * @return 方法调用结果：<br>
 *         - 成功: VOLC_SUCCESS <br>
 *         - 失败: VOLC_FAILED
 */
__byte_rtc_api__ uint32_t volc_generate_timestamp_str(uint64_t timestamp, char* format_str, char* p_dest_buffer, uint32_t dest_buffer_len, uint32_t* p_formatted_str_len);

/**
 * @brief 获取当前系统时间的字符串表示。
 * 
 * 该函数将当前系统时间转换为字符串格式，并存储在提供的缓冲区中。
 * 
 * @param time_str 指向用于存储时间字符串的字符数组的指针。
 * @param maxsize 缓冲区的最大长度，以字节为单位。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_get_time_str(char* time_str, uint32_t maxsize);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_TIME_H__ */
