/*****************************************************************/ /**
* @file qosa_ip_addr.h
* @brief IP address operation interface
* @author Joe.tu@quectel.com
* @date 2024-05-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-05-16 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_IP_ADDR_H__
#define __QOSA_IP_ADDR_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_sockets.h"

#if (defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))   
typedef enum
{
    QOSA_IP_INVALID = 0, /*!< Invalid IP type */
    QOSA_IP_V4 = 1,    /*!< IPv4 type */
    QOSA_IP_V6 = 2,    /*!< IPv6 type */
    QOSA_IP_V4V6 = 3   /*!< IPv4v6 dual stack type */
} qosa_ip_type_e;
#endif
/**
 * @struct qosa_ip_addr_t
 * @brief Used to save IP address status information
 */
struct qosa_ip_addr
{
    qosa_uint8_t ip_vsn; /*!< Save whether the IP address type is IPv4 (QOSA_PDP_IPV4) or IPv6 (QOSA_PDP_IPV6) */
    union
    {
        struct in_addr  ipv4_addr;
        struct in6_addr ipv6_addr;
    } addr;
#define apptcpip_ipv4_addr addr.ipv4_addr
#define apptcpip_ipv6_addr addr.ipv6_addr
};
// Temporary solution to cross-reference header files, to be optimized later
typedef struct qosa_ip_addr qosa_ip_addr_t;

#define qosa_ip_addr_cmp(addr1, addr2)                                                                                                                         \
    (((addr1)->ip_vsn == (addr2)->ip_vsn)                                                                                                                      \
     && (((addr1)->ip_vsn == QOSA_PDP_IPV4) ? ((addr1)->apptcpip_ipv4_addr.s_addr == (addr2)->apptcpip_ipv4_addr.s_addr)                                       \
                                            : (qosa_memcmp(&(addr1)->apptcpip_ipv6_addr, &(addr2)->apptcpip_ipv6_addr, sizeof(struct in6_addr)) == 0)))

#define qosa_ip_addr_copy(dest, src) (qosa_memcpy(dest, src, sizeof(qosa_ip_addr_t)))

#ifdef CONFIG_QOSA_MTK_6813_PLATFORM_FUNC
#define QOSA_IP_ADDR_AF_INET  AF_INET
#define QOSA_IP_ADDR_AF_INET6 AF_INET6
#else
#define QOSA_IP_ADDR_AF_INET  2
#define QOSA_IP_ADDR_AF_INET6 10
#endif

#define qosa_ip_addr_inet_ntop(ip_vsn, src, dest, size) inet_ntop(ip_vsn, src, dest, size)
// #define qosa_ip_addr_ntoa(addr) ((addr)->ip_vsn == QOSA_PDP_IPV4) ? inet_ntoa((addr)->apptcpip_ipv4_addr) : inet6_ntoa((addr)->apptcpip_ipv6_addr)

#endif /* __QOSA_IP_ADDR_H__ */
