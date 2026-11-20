/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_TLS_H__
#define __HAL_VOLC_TLS_H__

#include <stdbool.h>
#include <stddef.h>
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

/* Note: these error code must be the same as mbedtls */
#define VOLC_MBEDTLS_ERR_SSL_WANT_WRITE -0x6880
#define VOLC_MBEDTLS_ERR_SSL_WANT_READ -0x6900
#define VOLC_MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY -0x7880

/**
 * @brief TLS句柄
 * 
 * 用于https的连接
 */
typedef void* volc_tls_t;

/**
 * @brief 定义 TLS 发送回调函数类型。
 * 
 * 该回调函数用于在 TLS 会话中发送数据。
 * 
 * @param custom_data 用户自定义的数据指针，可用于传递额外信息。
 * @param buf 要发送的数据缓冲区。
 * @param len 要发送的数据长度。
 * @return 发送的数据长度，如果发生错误则返回负数。
 */
typedef int32_t (*volc_tls_send_callback)(void* custom_data, const unsigned char* buf, unsigned long len);

/**
 * @brief 定义 TLS 接收回调函数类型。
 * 
 * 该回调函数用于在 TLS 会话中接收数据。
 * 
 * @param custom_data 用户自定义的数据指针，可用于传递额外信息。
 * @param buf 用于存储接收数据的缓冲区。
 * @param len 缓冲区的最大长度。
 * @return 实际接收的数据长度，如果发生错误则返回负数。
 */
typedef int32_t (*volc_tls_recv_callback)(void* custom_data, unsigned char* buf, unsigned long len);

/**
 * @brief 创建一个新的 TLS 上下文。
 * 
 * 该函数用于初始化一个新的 TLS 会话上下文。
 * 
 * @param tls 指向 volc_tls_t 类型的指针，用于存储新创建的 TLS 上下文。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_tls_create(volc_tls_t* tls);

/**
 * @brief 销毁一个 TLS 上下文。
 * 
 * 该函数用于释放 TLS 会话上下文占用的资源。
 * 
 * @param tls 要销毁的 TLS 上下文。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_tls_destroy(volc_tls_t tls);

/**
 * @brief 启动一个 TLS 会话。
 * 
 * 该函数用于开始一个新的 TLS 会话，可以作为服务器或客户端。
 * 
 * @param tls 要启动的 TLS 上下文。
 * @param is_server 是否作为服务器启动。
 * @param host 服务器的主机名，如果作为客户端启动则需要指定。
 * @param send_callback 用于发送数据的回调函数。
 * @param recv_callback 用于接收数据的回调函数。
 * @param custom_data 用户自定义的数据指针，将传递给回调函数。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ int volc_tls_start(volc_tls_t tls, bool is_server, const char* host, volc_tls_send_callback send_callback, volc_tls_recv_callback recv_callback, void* custom_data);

/**
 * @brief 从 TLS 会话中读取数据。
 * 
 * 该函数用于从 TLS 会话中读取数据到指定的缓冲区。
 * 
 * @param tls 要读取数据的 TLS 上下文。
 * @param buf 用于存储读取数据的缓冲区。
 * @param len 缓冲区的最大长度。
 * @return 实际读取的数据长度，如果发生错误则返回负数。
 */
__byte_rtc_api__ int volc_tls_read(volc_tls_t tls, unsigned char* buf, size_t len);

/**
 * @brief 向 TLS 会话中写入数据。
 * 
 * 该函数用于将数据写入到 TLS 会话中。
 * 
 * @param tls 要写入数据的 TLS 上下文。
 * @param buf 要写入的数据缓冲区。
 * @param len 要写入的数据长度。
 * @return 实际写入的数据长度，如果发生错误则返回负数。
 */
__byte_rtc_api__ int volc_tls_write(volc_tls_t tls, const unsigned char* buf, size_t len);

/**
 * @brief 检查 TLS 握手是否完成。
 * 
 * 该函数用于检查指定的 TLS 会话是否已经完成握手过程。
 * 
 * @param tls 要检查的 TLS 上下文。
 * @return 如果握手完成返回 true，否则返回 false。
 */
__byte_rtc_api__ bool volc_tls_is_handshake_over(volc_tls_t tls);

/**
 * @brief 获取 TLS 会话中可用的字节数。
 * 
 * 该函数用于获取指定的 TLS 会话中当前可用的字节数。
 * 
 * @param tls 要检查的 TLS 上下文。
 * @return 可用的字节数。
 */
__byte_rtc_api__ size_t volc_tls_get_bytes_avail(volc_tls_t tls);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_TLS_H__ */
