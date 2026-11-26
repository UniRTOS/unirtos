/*****************************************************************/ /**
* @file mpc_clat.h
* @brief mpc clat interface definitions
* @author John.ou@quectel.com
* @date 2026-03-24
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-03-24 <td>1.0 <td>John.ou <td> Init
* </table>
**********************************************************************/

#ifndef __MPC_CLAT__H__
#define __MPC_CLAT__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_clat.h"

/** Feature disabled flag */
#define MPC_DISABLE_FEATURE    0
/** Feature enabled flag */
#define MPC_ENABLE_FEATURE     1
/** Zero value flag */
#define MPC_ZERO_VALUE         0
/** Non-zero value flag */
#define MPC_NONZERO_VALUE      1
/** Convert CLAT option to bit mask for flag operations */
#define QOSA_CLAT_OPT_BIT(opt) (1 << (opt))
/** Check if source address matches the given prefix (compares first 32-bit address part) */
#define QOSA_ADDR_MATCH_PREFIX_SRC(a, b)  (a.addr[0] == b.addr[0])
/** Check if destination address matches the given IPv6 prefix (compares first 32-bit address part) */
#define QOSA_ADDR_MATCH_PREFIX_DST(a, b)  (a.addr[0] == b.addr.ipv6_addr.un.u32_addr[0])
/** Check if header address matches the given IPv6 prefix (compares first two 32-bit address parts) */
#define QOSA_HEAD_ADDR_MATCH_PREFIX(a, b) (a.addr[0] == b.addr.ipv6_addr.un.u32_addr[0] && a.addr[1] == b.addr.ipv6_addr.un.u32_addr[1])
/** Convert QOSA IPv6 address format to lwIP ip6_addr_t format */
#define QOSA_IN6_ADDR_TO_IP6_ADDR(qip)    (*(ip6_addr_t *)&(qip).addr.ipv6_addr)
/** Default gateway IP address in RFC 7335 defined shared address space range */
#define QOSA_DEFAULT_GW                   "192.0.0.1"
/** Default local IP address in RFC 7335 defined shared address space range */
#define QOSA_DEFAULT_IP                   "192.0.0.2"
/** Default IPv6 prefix for CLAT gateway */
#define QOSA_DEFAULT_IP6_PREFIX           "fd64:64:64::"
/** Default network interface name for CLAT */
#define QOSA_DEFAULT_IFNAME               "roku"
/** Default MTU size for CLAT interface */
#define QOSA_DEFAULT_MTU                  1500
/** Routing metric priority for CLAT routes */
#define QOSA_ROUTE_METRIC                 2000
/** Route MTU adjusted by IPv4/IPv6 header size difference (MTU - QOSA_MTU_DIFF) */
#define QOSA_ROUTE_MTU                    (roku_cfg.mtu - 20)
/** GCC compiler attribute to suppress unused variable/parameter warnings */
#define QOSA_POSSIBLY_UNUSED              __attribute__((unused))

typedef struct qosa_xlat_clat_config
{
    qosa_uint8_t       pdptype;
    qosa_uint8_t       src_prefix_len;
    qosa_uint8_t       icmp_check;
    qosa_int32_t       mtu;
    qosa_int32_t       tunfd;
    qosa_bool_t        add_route;
    in_addr_t          gateway;
    ip6_addr_t         src_prefix;
    ip6_addr_t         gateway6;
    ip4_addr_t         ip, dest;
    ip4_addr_t         dns_server[2];
    ip4_addr_t         host_dns_server[2];
    qosa_clat_config_t clat_config;
} qosa_xlat_clat_config_t;

typedef struct qosa_xlat_client
{
    qosa_int32_t simcid;
    qosa_int32_t profileid;
    qosa_uint8_t pdptype;
    qosa_uint8_t xlat_enable;
    qosa_uint8_t plat_support;
    ip4_addr_t   ip4_dns;
} qosa_xlat_client_t;

typedef struct qosa_clat_config qosa_clat_config_t;

/**
 * @brief Set CLAT configuration to platform storage
 *
 * Persist the provided CLAT configuration for the specified profile into
 * non-volatile storage.
 *
 * @param[in] profile_id
 *          - Profile identifier to associate with the configuration
 * @param[in] cfg
 *          - Pointer to the CLAT configuration data to save
 * @return qosa_nvm_error_e
 *          - NVM error code: 0 indicates success, non-zero indicates failure
 */
qosa_nvm_error_e mpc_clat_set_config(int profile_id, qosa_clat_config_t *cfg);

/**
 * @brief Get CLAT configuration from platform storage
 *
 * Read the CLAT configuration for the specified profile into the caller-provided
 * buffer.
 *
 * @param[in] profile_id
 *          - Profile identifier to read
 * @param[out] cfg
 *          - Output buffer to receive the qosa_clat_config_t data
 * @return qosa_common_error_e
 *          - Common error code: 0 indicates success, non-zero indicates failure
 */
qosa_common_error_e mpc_clat_get_config(int profile_id, qosa_clat_config_t *cfg);

/**
 * @brief Initialize the XLAT/CLAT subsystem
 *
 * Perform any initialization required by the XLAT/CLAT module. Should be called
 * after network stack or system initialization to prepare internal data structures,
 * timers and worker threads.
 */
void qosa_xlat_init(void);

/**
 * @brief Translate an IPv6 packet to IPv4 according to CLAT rules (6to4)
 *
 * Translate an incoming IPv6 packet that encapsulates an IPv4 payload into a
 * corresponding IPv4 packet. The translated packet is written to `newp`.
 *
 * @param[in] p
 *          - Input IPv6 packet (lwIP pbuf)
 * @param[out] newp
 *          - Output IPv4 packet buffer filled by the function
 * @param[in] inp
 *          - Network interface that received the packet
 * @return qosa_bool_t
 *          - QOSA_TRUE if translation succeeded and `newp` is valid, QOSA_FALSE otherwise
 */
qosa_bool_t qosa_clat_6to4(struct pbuf *p, struct pbuf *newp, struct netif *inp);

/**
 * @brief Get an option value from the XLAT module
 *
 * Query the XLAT/CLAT module for an option identified by `opt_tag` under the
 * specified profile and copy the result into `value`.
 *
 * @param[in] profile_id
 *          - Profile identifier
 * @param[in] opt_tag
 *          - Option identifier (module-specific)
 * @param[out] value
 *          - Output buffer to receive the option value
 * @param[in] len
 *          - Length of the `value` buffer in bytes
 * @return int
 *          - On success returns number of bytes written to `value`, on failure
 *            returns 0 or a negative value depending on implementation
 */
int qosa_xlat_getopt(int profile_id, int opt_tag, void *value, int len);

/**
 * @brief Translate an IPv4 packet to IPv6 and return a new pbuf (4to6)
 *
 * Create an IPv6-encapsulated packet from the provided IPv4 pbuf for forwarding
 * into an IPv6 network via CLAT.
 *
 * @param[in] p
 *          - Input IPv4 packet (lwIP pbuf)
 * @param[in] inp
 *          - Network interface used for translation
 * @return struct pbuf*
 *          - Newly allocated IPv6 pbuf on success, NULL on failure
 */
struct pbuf *qosa_clat_4to6(struct pbuf *p, struct netif *inp);

/**
 * @brief Get a pointer to the runtime CLAT configuration for a profile
 *
 * Return a pointer to the internal configuration object for the given profile.
 * The returned pointer is owned by the module and must not be freed by the caller.
 *
 * @param[in] profile_id
 *          - Profile identifier
 * @return void*
 *          - Read-only pointer to the profile's configuration, or NULL on error
 */
void *qosa_xlat_get_cfg(int profile_id);

/**
 * @brief Persist an in-memory CLAT configuration to NVM
 *
 * Persist the configuration pointed to by `cfg_ptr` into non-volatile storage so
 * that it remains after reboot.
 *
 * @param[in] cfg_ptr
 *          - Pointer to the configuration object to persist
 * @return int
 *          - 0 on success, non-zero on failure
 */
int qosa_clat_nv_save(void *cfg_ptr);

#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif
