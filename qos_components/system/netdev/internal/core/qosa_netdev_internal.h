/*****************************************************************/ /**
 * @file qosa_netdev_internal.h
 * @brief Internal helpers and slot state for netdev core modules.
 * @author felix.lin@quectel.com
 * @date 2026-07-03
 *
 * @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description
 * <tr><td>2026-07-03 <td>1.0 <td>felix.lin <td> Add and normalize English Doxygen comments
 * </table>
 **********************************************************************/

#ifndef __QOSA_NETDEV_INTERNAL_H__
#define __QOSA_NETDEV_INTERNAL_H__

#include "qosa_netdev.h"

#ifdef CONFIG_QOSA_NETDEV_CORE_NAT_FUNC
#include "qosa_netdev_nat_internal.h"
#endif

#if defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC)
#include "netif_ex.h"
#elif defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC)
#include "netif.h"
#else
#include "lwip/dhcp.h"
#endif

#if defined(CONFIG_QOSA_EIGEN718_PLATFORM_FUNC) || defined(CONFIG_QOSA_QCX216_PLATFORM_FUNC)
#define CONFIG_QOSA_NETDEV_EIGEN_PLATFORM_FUNC
#endif

#ifdef CONFIG_QOSA_NETDEV_EIGEN_PLATFORM_FUNC
#include "qosa_dev_eigen.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @enum qosa_netdev_ps_type_e
 * @brief Packet source payload representation used by netdev input helpers.
 */
typedef enum
{
    QOSA_NETDEV_PS_TYPE_RAW = 0, /*!< Raw linear packet buffer. */
    QOSA_NETDEV_PS_TYPE_PBUF,    /*!< lwIP pbuf-backed packet buffer. */
    QOSA_NETDEV_PS_TYPE_MAX,     /*!< Boundary check guard. */
} qosa_netdev_ps_type_e;

/**
 * @struct qosa_netdev_slot_t
 * @brief Internal state tracked for each netdev registration slot.
 */
typedef struct
{
    /*!< lwIP netif instance. This field must stay first for pointer casts. */
    struct netif             netif;
    /*!< Copy of the caller-supplied registration context. */
    qosa_netdev_reg_t        reg;
    /*!< Slot index assigned during registration. */
    qosa_uint8_t             idx;
    /*!< Whether the slot is occupied. */
    qosa_uint8_t             valid;
    /*!< Whether netif_set_up() has been applied. */
    qosa_uint8_t             admin_up;
    /*!< Whether the DHCP client is running on this netif. */
    qosa_uint8_t             dhcpc_started;
    /*!< Whether the DHCP server is running on this netif. */
    qosa_uint8_t             dhcpd_started;
    /*!< Active DHCP server mode. */
    qosa_netdev_dhcpd_mode_e dhcpd_mode;
    /*!< Last route_prefer value written through opt_set. */
    qosa_uint8_t             route_prefer;
    /*!< Generation counter used to reject stale RX callbacks. */
    qosa_uint32_t            generation;
    /*!< fix len to lower,avoid to mult-copy. */
    qosa_netdev_noncopy_t    noncopy;
} qosa_netdev_slot_t;

/**
 * @brief Return the lwIP netif for a registered netdev slot.
 *
 * @param[in] qosa_uint8_t idx
 *          - Netdev slot index.
 *
 * @return Returns the lwIP netif pointer, or NULL when the slot is invalid.
 */
struct netif *qosa_netdev_get_netif(qosa_uint8_t idx);

/**
 * @brief Return the internal slot state for a netdev index.
 *
 * @param[in] qosa_uint8_t idx
 *          - Netdev slot index.
 *
 * @return Returns the slot state pointer, or NULL when the slot is invalid.
 */
qosa_netdev_slot_t *qosa_netdev_slot_get(qosa_uint8_t idx);

/**
 * @brief Accept one packet from the PS input path.
 *
 * @param[in] void *data
 *          - Pointer to the incoming packet storage.
 * @param[in] qosa_uint32_t len
 *          - Packet length in bytes.
 * @param[in] qosa_netdev_ps_type_e type
 *          - Payload representation of data.
 *
 * @return Returns QOSA_TRUE when the packet is consumed, otherwise QOSA_FALSE.
 */
qosa_bool_t qosa_netdev_lwip_ps_accept(void *data, qosa_uint32_t len, qosa_netdev_ps_type_e type);

/**
 * @brief Resolve the IPv4 route for a packet handled by netdev.
 *
 * @param[in] const ip4_addr_t *dst
 *          - Destination IPv4 address.
 * @param[in] const ip4_addr_t *src
 *          - Source IPv4 address.
 *
 * @return Returns the selected lwIP netif pointer, or NULL when no route is found.
 */
struct netif *qosa_netdev_lwip_ip4_route(const ip4_addr_t *dst, const ip4_addr_t *src);

/**
 * @brief Resolve the egress Ethernet route for a netif.
 *
 * @param[in] struct netif *netif
 *          - Candidate lwIP netif.
 *
 * @return Returns the routed lwIP netif pointer.
 */
struct netif *qosa_netdev_lwip_eth_route(struct netif *netif);

/**
 * @brief Dump or inspect wrapped netif state for debugging.
 */
void qosa_netdev_netif_wrapper(void);

/**
 * @brief Query the DNS servers associated with a netif.
 *
 * @param[in] struct netif *netif
 *          - Target lwIP netif.
 * @param[out] ip_addr_t *dns1
 *          - Primary DNS server output.
 * @param[out] ip_addr_t *dns2
 *          - Secondary DNS server output.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_lwip_get_dns(struct netif *netif, ip_addr_t *dns1, ip_addr_t *dns2);

#if LWIP_NETIF_API

#if defined(CONFIG_QOSA_ASR160X_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1805_PLATFORM_FUNC) || defined(CONFIG_QOSA_ASR1903_PLATFORM_FUNC)
#include "netifapi.h"
#else
#include "lwip/netifapi.h"
#endif

#define qosa_netdev_netif_add(netif, ip, mask, gw, state, init_fn, input_fn) \
    netifapi_netif_add((netif), (ip), (mask), (gw), (state), (init_fn), (input_fn))

#define qosa_netdev_netif_remove(netif)            netifapi_netif_remove(netif)
#define qosa_netdev_netif_set_up(netif)            netifapi_netif_set_up(netif)
#define qosa_netdev_netif_set_down(netif)          netifapi_netif_set_down(netif)
#define qosa_netdev_netif_set_link_up(netif)       netifapi_netif_set_link_up(netif)
#define qosa_netdev_netif_set_link_down(netif)     netifapi_netif_set_link_down(netif)
#define qosa_netdev_netif_set_default(netif)       netifapi_netif_set_default(netif)
#define qosa_netdev_netif_set_addr(netif, ip, mask, gw) \
    netifapi_netif_set_addr((netif), (ip), (mask), (gw))
#if LWIP_DHCP
#define qosa_netdev_dhcp_start(netif)              netifapi_dhcp_start(netif)
#define qosa_netdev_dhcp_stop(netif)               netifapi_dhcp_stop(netif)
#else /* LWIP_DHCP */
#define qosa_netdev_dhcp_start(netif)
#define qosa_netdev_dhcp_stop(netif)
#endif /* LWIP_DHCP */

#else /* !LWIP_NETIF_API */

#define qosa_netdev_netif_add(netif, ip, mask, gw, state, init_fn, input_fn) \
    (netif_add((netif), (ip), (mask), (gw), (state), (init_fn), (input_fn)) != NULL \
     ? ERR_OK : ERR_IF)

#define qosa_netdev_netif_remove(netif)            netif_remove(netif)
#define qosa_netdev_netif_set_up(netif)            netif_set_up(netif)
#define qosa_netdev_netif_set_down(netif)          netif_set_down(netif)
#define qosa_netdev_netif_set_link_up(netif)       netif_set_link_up(netif)
#define qosa_netdev_netif_set_link_down(netif)     netif_set_link_down(netif)
#define qosa_netdev_netif_set_default(netif)       netif_set_default(netif)
#define qosa_netdev_netif_set_addr(netif, ip, mask, gw) \
    netif_set_addr((netif), (ip), (mask), (gw))
#if LWIP_DHCP
#define qosa_netdev_dhcp_start(netif)              dhcp_start(netif)
#define qosa_netdev_dhcp_stop(netif)               dhcp_stop(netif)
#else /* LWIP_DHCP */
#define qosa_netdev_dhcp_start(netif)
#define qosa_netdev_dhcp_stop(netif)
#endif /* LWIP_DHCP */

#endif /* LWIP_NETIF_API */

#ifndef LWIP_NETIF_STATUS_CALLBACK
#define netif_set_status_callback(netif, status_callback)  /* no-op */
#endif /* LWIP_NETIF_STATUS_CALLBACK */

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_NETDEV_INTERNAL_H__ */
