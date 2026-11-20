/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_NETWORK_H__
#define __HAL_VOLC_NETWORK_H__

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
 * @brief 定义最大域名长度。
 * 
 * 该宏定义了域名的最大允许长度，设置为 128 个字符。
 */
#define VOLC_MAX_DOMAIN_LEN 128
/**
 * @brief 定义最大网络接口数量。
 * 
 * 该宏定义了系统支持的最大网络接口数量，设置为 20 个。
 */
#define VOLC_MAX_NETWORK_INTERFACE_COUNT 20
/**
 * @brief 定义最大 IP 地址缓冲区长度。
 * 
 * 该宏定义了存储 IP 地址所需的最大缓冲区长度，设置为 46 个字符，以适应 IPv6 地址。
 */
#define VOLC_MAX_IP_ADDRESS_BUFFER_LEN 46
/**
 * @brief 定义最大 IPv4 地址字符串长度。
 * 
 * 该宏定义了 IPv4 地址字符串的最大长度，设置为 15 个字符（例如 "255.255.255.255"）。
 */
#define VOLC_MAX_IPV4_ADDRESS_STRING_LEN 15
/**
 * @brief 定义 IPv4 地址长度。
 * 
 * 该宏定义了 IPv4 地址的长度，设置为 4 个字节。
 */
#define VOLC_IPV4_ADDRESS_LENGTH 4
/**
 * @brief 定义 IPv6 地址长度。
 * 
 * 该宏定义了 IPv6 地址的长度，设置为 16 个字节。
 */
#define VOLC_IPV6_ADDRESS_LENGTH 16

/**
 * @brief 定义网络回调函数类型。
 * 
 * 该 typedef 定义了一个回调函数类型，用于在网络操作完成时调用。
 * 
 * @param custom_data 用户自定义的数据，类型为 64 位无符号整数。
 * @param message 回调消息，类型为字符指针。
 * @return 布尔值，表示回调操作的结果。
 */
typedef bool (*volc_network_callback_t)(uint64_t, char*);

/**
 * @brief 定义套接字协议类型的枚举。
 * 
 * 该枚举类型定义了不同的套接字协议类型，包括无协议、TCP 协议和 UDP 协议。
 */
typedef enum {
    /**
     * @brief 表示无套接字协议。
     */
    VOLC_SOCKET_PROTOCOL_NONE,
    /**
     * @brief 表示使用 TCP 协议。
     */
    VOLC_SOCKET_PROTOCOL_TCP,
    /**
     * @brief 表示使用 UDP 协议。
     */
    VOLC_SOCKET_PROTOCOL_UDP,
} volc_socket_protocol_e;

/**
 * @brief 定义 IP 地址族类型的枚举。
 * 
 * 该枚举类型定义了不同的 IP 地址族类型，包括 IPv4 和 IPv6。
 */
typedef enum {
    /**
     * @brief 表示 IPv4 地址族。
     */
    VOLC_IP_FAMILY_TYPE_IPV4 = (uint16_t) 0x0001,
    /**
     * @brief 表示 IPv6 地址族。
     */
    VOLC_IP_FAMILY_TYPE_IPV6 = (uint16_t) 0x0002,
} volc_ip_family_type_e;

/**
 * @brief 定义 IP 地址结构体。
 * 
 * 该结构体用于存储 IP 地址的相关信息，包括地址族、端口号、地址和点对点标志。
 */
typedef struct {
    /**
     * @brief IP 地址族，使用 volc_ip_family_type_e 枚举表示。
     */
    uint16_t family;
    /**
     * @brief 端口号。
     */
    uint16_t port;
    /**
     * @brief IP 地址，最大支持 16 字节，可存储 IPv4 和 IPv6 地址。
     */
    uint8_t address[16];
    /**
     * @brief 表示该地址是否为点对点地址。
     */
    bool is_point_to_point;
} volc_ip_addr_t;

/**
 * @brief 获取本地 IP 地址。
 * 
 * 该函数用于获取本地设备的 IP 地址，并将其存储在指定的 volc_ip_addr_t 结构体中。
 * 
 * @param ip 指向 volc_ip_addr_t 结构体的指针，用于存储获取到的 IP 地址。
 * @param dest_ip_list_len 指向存储 IP 地址列表长度的变量的指针。
 * @param callback 网络回调函数，用于在操作完成时通知调用者。
 * @param custom_data 用户自定义的数据，将传递给回调函数。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_get_local_ip(volc_ip_addr_t* ip, uint32_t* dest_ip_list_len, volc_network_callback_t callback, uint64_t custom_data);

/**
 * @brief 根据主机名获取 IP 地址。
 * 
 * 该函数用于根据指定的主机名解析出对应的 IP 地址，并将其存储在指定的 volc_ip_addr_t 结构体中。
 * 
 * @param host_name 要解析的主机名。
 * @param p_ip_address 指向 volc_ip_addr_t 结构体的指针，用于存储解析出的 IP 地址。
 * @return 操作结果的状态码，0 表示成功，非 0 表示失败。
 */
__byte_rtc_api__ uint32_t volc_get_ip_with_host_name(const char* host_name, volc_ip_addr_t* p_ip_address);


__byte_rtc_api__ int volc_inet_pton (int __af, const char *__restrict __cp,void *__restrict __buf) ;

__byte_rtc_api__ uint16_t volc_htons(uint16_t hostshort);
__byte_rtc_api__ uint16_t volc_ntohs(uint16_t netshort);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_NETWORK_H__ */
