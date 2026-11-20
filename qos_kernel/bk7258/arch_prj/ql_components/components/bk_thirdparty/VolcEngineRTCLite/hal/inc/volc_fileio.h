/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_FILEIO_H__
#define __HAL_VOLC_FILEIO_H__

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
 * @brief 从文件中读取内容。
 *
 * 该函数用于从指定路径的文件中读取内容到缓冲区。
 *
 * @param path 文件的路径。
 * @param bin_mode 是否以二进制模式读取文件。
 * @param buffer 用于存储读取内容的缓冲区。
 * @param size 指向存储读取字节数的变量的指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_read(const char* path, bool bin_mode, uint8_t* buffer, uint64_t* size);

/**
 * @brief 从文件的指定偏移位置读取指定长度的内容。
 *
 * 该函数用于从指定路径的文件的指定偏移位置开始读取指定长度的内容到缓冲区。
 *
 * @param path 文件的路径。
 * @param bin_mode 是否以二进制模式读取文件。
 * @param buffer 用于存储读取内容的缓冲区。
 * @param offset 读取的起始偏移位置。
 * @param size 要读取的字节数。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_read_segment(const char* path, bool bin_mode, uint8_t* buffer, uint64_t offset, uint64_t size);

/**
 * @brief 向文件中写入内容。
 *
 * 该函数用于向指定路径的文件中写入内容。可以选择以追加模式写入。
 *
 * @param path 文件的路径。
 * @param bin_mode 是否以二进制模式写入文件。
 * @param append 是否以追加模式写入。
 * @param buffer 要写入的内容的缓冲区。
 * @param size 要写入的字节数。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_write(const char* path, bool bin_mode, bool append, uint8_t* buffer, uint64_t size);

/**
 * @brief 更新文件中指定偏移位置的内容。
 *
 * 该函数用于更新指定路径的文件中指定偏移位置的内容。
 *
 * @param path 文件的路径。
 * @param bin_mode 是否以二进制模式更新文件。
 * @param buffer 要更新的内容的缓冲区。
 * @param offset 更新的起始偏移位置。
 * @param size 要更新的字节数。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_update(const char* path, bool bin_mode, uint8_t* buffer, uint64_t offset, uint64_t size);

/**
 * @brief 获取文件的长度。
 *
 * 该函数用于获取指定路径的文件的长度。
 *
 * @param path 文件的路径。
 * @param size 指向存储文件长度的变量的指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_get_length(const char* path, uint64_t* size);

/**
 * @brief 设置文件的长度。
 *
 * 该函数用于设置指定路径的文件的长度。
 *
 * @param path 文件的路径。
 * @param size 要设置的文件长度。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_set_length(const char* path, uint64_t size);

/**
 * @brief 检查文件是否存在。
 *
 * 该函数用于检查指定路径的文件是否存在。
 *
 * @param path 文件的路径。
 * @param p_exists 指向存储检查结果的布尔变量的指针。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_exists(const char* path, bool* p_exists);

/**
 * @brief 创建一个指定长度的文件。
 *
 * 该函数用于创建一个指定路径和长度的文件。
 *
 * @param path 文件的路径。
 * @param size 要创建的文件的长度。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_create(const char* path, uint64_t size);

/**
 * @brief 删除指定的文件。
 *
 * 该函数用于删除指定路径的文件。
 *
 * @param path 文件的路径。
 * @return 操作结果的状态码。
 */
__byte_rtc_api__ uint32_t volc_file_delete(const char* path);


#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_FILEIO_H__ */
