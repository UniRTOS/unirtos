/*****************************************************************/ /**
 * @file qosa_netdev_nat_internal.h
 * @brief Internal NAT declarations for netdev integration.
 * @author felix.lin@quectel.com
 * @date 2026-07-03
 *
 * @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description
 * <tr><td>2026-07-03 <td>1.0 <td>felix.lin <td> Add English Doxygen comments
 * </table>
 **********************************************************************/

#ifndef __QOSA_NETDEV_NAT_INTERNAL_H__
#define __QOSA_NETDEV_NAT_INTERNAL_H__

#include "qosa_netdev.h"
#include "lwip/netif.h"

/** Enable lwIP IPv4 NAT support for this internal module. */
#define IP_NAT 1
/* #define NAT_DEBUG LWIP_DBG_ON */

/**
 * @brief Try NAT egress processing for an outgoing packet.
 *
 * @param[in] struct netif *netif
 *          - Outgoing lwIP netif.
 * @param[in] struct pbuf *p
 *          - Packet buffer to translate.
 *
 * @return Returns QOSA_TRUE when NAT consumes the packet, otherwise QOSA_FALSE.
 */
qosa_bool_t qosa_netdev_nat_try_egress(struct netif *netif, struct pbuf *p);

/**
 * @brief Accept one WAN packet provided as a pbuf-backed frame.
 *
 * @param[in] qosa_uint8_t *data
 *          - Pointer to the packet data.
 * @param[in] qosa_uint32_t len
 *          - Packet length in bytes.
 *
 * @return Returns QOSA_TRUE when the packet is consumed, otherwise QOSA_FALSE.
 */
qosa_bool_t qosa_netdev_nat_wan_accept_pbuf(qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Accept one WAN packet provided as a raw frame.
 *
 * @param[in] qosa_uint8_t *data
 *          - Pointer to the packet data.
 * @param[in] qosa_uint32_t len
 *          - Packet length in bytes.
 *
 * @return Returns QOSA_TRUE when the packet is consumed, otherwise QOSA_FALSE.
 */
qosa_bool_t qosa_netdev_nat_wan_accept_raw(qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Return the LAN netif selected for NAT forwarding.
 *
 * @param[in] struct netif *netif
 *          - Source netif used for routing context.
 *
 * @return Returns the routed LAN netif pointer, or NULL when no route is available.
 */
struct netif *qosa_netdev_nat_lan_route(struct netif *netif);

#endif /* __QOSA_NETDEV_NAT_INTERNAL_H__ */
