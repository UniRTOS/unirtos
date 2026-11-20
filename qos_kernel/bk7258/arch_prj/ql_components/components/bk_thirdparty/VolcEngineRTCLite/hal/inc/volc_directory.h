/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_DIRECTORY_H__
#define __HAL_VOLC_DIRECTORY_H__

#include <stdbool.h>
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
 * @brief 定义最大路径长度。
 * 
 * 该宏定义了路径字符串的最大长度，设置为 256 个字符。
 */
#define VOLC_MAX_PATH_LEN 256

/**
 * @brief 定义文件路径分隔符字符。
 * 
 * 该宏定义了文件路径中使用的分隔符字符，这里使用正斜杠 '/'。
 */
#define VOLC_FPATHSEPARATOR     '/'

/**
 * @brief 定义文件路径分隔符字符串。
 * 
 * 该宏定义了文件路径中使用的分隔符字符串，这里使用正斜杠字符串 "/"。
 */
#define VOLC_FPATHSEPARATOR_STR "/"

/**
 * @brief 定义目录项类型的枚举。
 * 
 * 该枚举类型定义了不同类型的目录项，包括文件、符号链接、目录和未知类型。
 */
typedef enum {
    /**
     * @brief 表示目录项为文件。
     */
    VOLC_DIR_ENTRY_TYPE_FILE,
    /**
     * @brief 表示目录项为符号链接。
     */
    VOLC_DIR_ENTRY_TYPE_LINK,
    /**
     * @brief 表示目录项为目录。
     */
    VOLC_DIR_ENTRY_TYPE_DIRECTORY,
    /**
     * @brief 表示目录项类型未知。
     */
    VOLC_DIR_ENTRY_TYPE_UNKNOWN
} volc_dir_entry_type_e;

/**
 * @brief 定义目录项回调函数类型。
 * 
 * 该 typedef 定义了一个回调函数类型，用于在遍历目录时处理每个目录项。
 * 
 * @param param 一个 64 位无符号整数参数，可用于传递额外信息。
 * @param type 目录项的类型，使用 volc_dir_entry_type_e 枚举表示。
 * @param path 目录项的完整路径。
 * @param name 目录项的名称。
 * @return 一个 32 位无符号整数，表示操作结果。
 */
typedef uint32_t (*volc_directory_entry_callback)(uint64_t, volc_dir_entry_type_e, char*, char*);

/**
 * @brief 遍历指定目录及其子目录。
 * 
 * 该函数用于递归地遍历指定目录及其子目录，并对每个目录项调用回调函数。
 * 
 * @param path 要遍历的目录的路径。
 * @param param 一个 64 位无符号整数参数，将传递给回调函数。
 * @param recursive 是否递归遍历子目录。
 * @param callback 处理每个目录项的回调函数。
 * @return 一个 32 位无符号整数，表示操作结果。
 */
__byte_rtc_api__ uint32_t volc_traverse_directory(const char*, uint64_t, bool, volc_directory_entry_callback);

/**
 * @brief 删除指定目录。
 * 
 * 该函数用于删除指定的目录及其所有内容。
 * 
 * @param path 要删除的目录的路径。
 * @return 一个 32 位无符号整数，表示操作结果。
 */
__byte_rtc_api__ uint32_t volc_remove_directory(const char*);

/**
 * @brief 创建指定目录。
 * 
 * 该函数用于创建指定的目录。
 * 
 * @param path 要创建的目录的路径。
 * @return 一个 32 位无符号整数，表示操作结果。
 */
__byte_rtc_api__ uint32_t volc_create_directory(const char*);


#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_DIRECTORY_H__ */
