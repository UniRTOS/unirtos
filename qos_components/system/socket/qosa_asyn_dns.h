/*****************************************************************/ /**
* @file qosa_asyn_dns.h
* @brief Asynchronous DNS resolution interface
* @author larson.li@quectel.com
* @date 2023-07-31
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-07-31 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_ASYN_DNS_H__
#define __QOSA_ASYN_DNS_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/** FILE API function module error base offset code */
#define QOSA_ERRCODE_DNS_BASE    (QOSA_COMPONENT_DNS << 16)

/** DNS maximum domain name length 256 characters */
#define QOSA_DNS_REQUEST_MAX_LEN 256

/**
 * @enum qosa_dns_error_e
 * @brief DNS request result return status
 */
typedef enum
{
    QOSA_DNS_RESULT_OK = 0,                            /*!< DNS request successful */
    QOSA_DNS_RESULT_ERROR = 1 | QOSA_ERRCODE_DNS_BASE, /*!< DNS request failed */
    QOSA_DNS_MEMORY_ERROR = 2,                         /*!< DNS internal memory allocation failed */

    QOSA_DNS_RESULT_MAX
} qosa_dns_error_e;

/**
 * @struct qosa_addrinfo_s
 * @brief DNS resolution address data content
 */
struct qosa_addrinfo_s
{
    int                     ai_family;                             /*!< IP address type */
    char                    ip_addr[CONFIG_QOSA_INET6_ADDRSTRLEN]; /*!< Character type such as 127.0.0.1 2004::1 IPv6 maximum 48 characters */
    struct qosa_addrinfo_s *ai_next;                               /*!< Next node storing address */
};

/**
 * @brief Registered asynchronous notification callback function, it is forbidden to do your own data business in the callback function, which may cause data critical issues
 *
 * @param[in] void *argv
 *          - cb function returns user parameter
 *
 * @param[in] struct qosa_addrinfo_s **info
 *          - DNS resolution result, if failed then info = NULL
 *
 * @param[in] qosa_dns_error_e type
 *          - Corresponding DNS resolution result status, if successful type = QOSA_DNS_RESULT_OK
 *
 * @note Special attention, if DNS request is successful, the caller needs to call qosa_dns_result_free function to release the address pointer
 *
 */
typedef void (*dns_result_callback)(void *argv, struct qosa_addrinfo_s **info, qosa_dns_error_e type);

/**
 * @brief Free DNS return result
 *
 * @param[in] struct qosa_addrinfo_s * info
 *          - struct qosa_addrinfo_s to free
 */
void qosa_dns_result_free(struct qosa_addrinfo_s *info);

/**
 * @brief Asynchronous DNS resolution interface
 *
 * @param[in] qosa_uint8_t simcid
 *          - SIM card ID to use
 *
 * @param[in] qosa_uint8_t pdpcid
 *          - PDP ID to use
 *
 * @param[in] const char * hostname
 *          - Domain name information to be resolved
 *
 * @param[in] struct qosa_addrinfo_s *hints
 *          - Mainly used to specify the address type for resolution
 *
 * @param[in] dns_result_callback cb
 *          - User registered asynchronous notification callback function,
 *          - It is forbidden to do your own data business in the callback function, which may cause data critical issues
 *
 * @param[in] void * user_argv
 *          - User parameter passed in
 *
 * @return qosa_dns_error_e
 *        - Return QOSA_DNS_RESULT_OK on success
 *        - Return other error codes from qosa_dns_error_e on failure
 */
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
qosa_dns_error_e qosa_dns_asyn_getaddrinfo(
    qosa_uint8_t            simcid,
    qosa_uint8_t            pdpcid,
    const char             *hostname,
    struct qosa_addrinfo_s *hints,
    dns_result_callback     cb,
    void                   *user_argv
);

qosa_dns_error_e
qosa_dns_syn_getaddrinfo(qosa_uint8_t simcid, qosa_uint8_t pdpcid, const char *hostname, struct qosa_addrinfo_s *hints, struct qosa_addrinfo_s **res);
#else
qosa_dns_error_e qosa_dns_asyn_getaddrinfo(
    const char             *hostname,
    struct qosa_addrinfo_s *hints,
    dns_result_callback     cb,
    void                   *user_argv
);

qosa_dns_error_e
qosa_dns_syn_getaddrinfo(const char *hostname, struct qosa_addrinfo_s *hints, struct qosa_addrinfo_s **res);
#endif
#endif /* __QOSA_ASYN_DNS_H__ */
