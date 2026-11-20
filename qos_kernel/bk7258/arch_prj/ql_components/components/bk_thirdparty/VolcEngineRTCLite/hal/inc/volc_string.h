/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_STRING_H__
#define __HAL_VOLC_STRING_H__

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
 * @brief 定义一个自定义的字符串结构体。
 * 
 * 该结构体用于管理动态分配的字符串，包含字符串的长度、容量和指向字符缓冲区的指针。
 */
typedef struct {
    /**
     * @brief 当前字符串的长度。
     */
    int32_t length;
    /**
     * @brief 字符串缓冲区的容量。
     */
    int32_t capacity;
    /**
     * @brief 指向存储字符串的字符缓冲区的指针。
     */
    char* buffer;
} volc_string_t;

/**
 * @brief 获取 volc_string_t 结构体中的字符串指针。
 * 
 * @param str 指向 volc_string_t 结构体的指针。
 * @return 指向存储在结构体中的字符串的指针。
 */
__byte_rtc_api__ char* volc_string_get(volc_string_t* str);

/**
 * @brief 在 volc_string_t 结构体存储的字符串中查找子字符串。
 * 
 * @param str 指向 volc_string_t 结构体的指针，包含要搜索的字符串。
 * @param src 要查找的子字符串。
 * @return 如果找到，返回子字符串在原字符串中的指针；否则返回 NULL。
 */
__byte_rtc_api__ char* volc_string_strstr(volc_string_t* str,char* src);

/**
 * @brief 初始化 volc_string_t 结构体。
 * 
 * @param string 指向要初始化的 volc_string_t 结构体的指针。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_init(volc_string_t* string);

/**
 * @brief 释放 volc_string_t 结构体占用的资源。
 * 
 * @param string 指向要释放资源的 volc_string_t 结构体的指针。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_deinit(volc_string_t* string);

/**
 * @brief 使用指定长度的源字符串设置 volc_string_t 结构体的内容。
 * 
 * @param str 指向要设置内容的 volc_string_t 结构体的指针。
 * @param src 指向源字符串的指针。
 * @param length 源字符串的长度。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_set_with_length(volc_string_t* str,uint8_t* src,int32_t length);

/**
 * @brief 使用源字符串设置 volc_string_t 结构体的内容。
 * 
 * @param str 指向要设置内容的 volc_string_t 结构体的指针。
 * @param src 指向源字符串的指针。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_set(volc_string_t* str,char* src);

/**
 * @brief 使用另一个 volc_string_t 结构体的内容设置当前结构体的内容。
 * 
 * @param str 指向要设置内容的 volc_string_t 结构体的指针。
 * @param str2 指向提供内容的 volc_string_t 结构体的指针。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string2set(volc_string_t* str,volc_string_t* str2);

/**
 * @brief 将指定长度的源字符串追加到 volc_string_t 结构体的内容后面。
 * 
 * @param str 指向要追加内容的 volc_string_t 结构体的指针。
 * @param src 指向源字符串的指针。
 * @param length 要追加的源字符串的长度。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_append_with_length(volc_string_t* str,char* src,int32_t length);

/**
 * @brief 将源字符串追加到 volc_string_t 结构体的内容后面。
 * 
 * @param str 指向要追加内容的 volc_string_t 结构体的指针。
 * @param src 指向源字符串的指针。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_append(volc_string_t* str,char* src);

/**
 * @brief 使用格式化字符串将内容写入 volc_string_t 结构体。
 * 
 * @param str 指向要写入内容的 volc_string_t 结构体的指针。
 * @param length 要写入的最大长度。
 * @param format 格式化字符串。
 * @param ... 可变参数列表。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_snprintf(volc_string_t* str, int32_t length, const char* format, ...);

/**
 * @brief 重置 volc_string_t 结构体的内容。
 * 
 * @param str 指向要重置的 volc_string_t 结构体的指针。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_reset(volc_string_t* str);

/**
 * @brief 生成一个 JSON 安全的字符串并存储在 volc_string_t 结构体中。
 * 
 * @param str 指向要存储生成字符串的 volc_string_t 结构体的指针。
 * @param len 要生成的字符串的长度。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_string_generate_json_safe_string(volc_string_t* str,int len);

/**
 * @brief 获取 volc_string_t 结构体中字符串的长度。
 * 
 * @param str 指向 volc_string_t 结构体的指针。
 * @return 存储在结构体中的字符串的长度。
 */
__byte_rtc_api__ uint32_t volc_string_get_length(volc_string_t* str);

/**
 * @brief 比较 volc_string_t 结构体中的字符串与另一个字符串。
 * 
 * @param str 指向 volc_string_t 结构体的指针，包含要比较的字符串。
 * @param dst 要比较的另一个字符串。
 * @return 如果相等返回 0，如果 str 小于 dst 返回负数，如果 str 大于 dst 返回正数。
 */
__byte_rtc_api__ int32_t volc_string_cmp(volc_string_t* str,const char* dst);

/**
 * @brief 比较两个 volc_string_t 结构体中的字符串。
 * 
 * @param str 指向第一个 volc_string_t 结构体的指针。
 * @param str2 指向第二个 volc_string_t 结构体的指针。
 * @return 如果相等返回 0，如果 str 小于 str2 返回负数，如果 str 大于 str2 返回正数。
 */
__byte_rtc_api__ int32_t volc_string2cmp(volc_string_t* str,volc_string_t* str2);

/**
 * @brief 比较 volc_string_t 结构体中的字符串与另一个指定长度的字符串。
 * 
 * @param str 指向 volc_string_t 结构体的指针，包含要比较的字符串。
 * @param dst 要比较的另一个字符串。
 * @param len 要比较的长度。
 * @return 如果相等返回 0，如果 str 小于 dst 返回负数，如果 str 大于 dst 返回正数。
 */
__byte_rtc_api__ int32_t volc_string_cmp_with_length(volc_string_t* str,const char* dst,int len);


#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_ERRNO_H__ */
