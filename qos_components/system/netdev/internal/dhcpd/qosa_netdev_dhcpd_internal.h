/*****************************************************************/ /**
 * @file qosa_netdev_dhcpd_internal.h
 * @brief Internal DHCP server declarations for netdev.
 * @author felix.lin@quectel.com
 * @date 2026-07-03
 *
 * @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description
 * <tr><td>2026-07-03 <td>1.0 <td>felix.lin <td> Comment cleanup and English normalization
 * </table>
 **********************************************************************/

#ifndef __QOSA_NETDEV_DHCPD_INTERNAL_H__
#define __QOSA_NETDEV_DHCPD_INTERNAL_H__

#include "qosa_netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 *  Configuration defaults (override via compiler -D or a config header)
 * ---------------------------------------------------------------- */

/** First host octet in the client pool. Default: .2 (.1 is the gateway). */
#ifndef QOSA_DHCPD_CLIENT_IP_MIN
#define QOSA_DHCPD_CLIENT_IP_MIN   2
#endif

/** Last host octet in the client pool, inclusive. Default: .254. */
#ifndef QOSA_DHCPD_CLIENT_IP_MAX
#define QOSA_DHCPD_CLIENT_IP_MAX   254
#endif

/** Default server IP when no static address is configured. */
#ifndef QOSA_DHCPD_SERVER_IP
#define QOSA_DHCPD_SERVER_IP       "192.168.100.1"
#endif

/** Default lease time in seconds. */
#ifndef QOSA_DHCPD_LEASE_TIME_S
#define QOSA_DHCPD_LEASE_TIME_S    86400U
#endif

/** Minimum DHCP reply size. RFC 2131 clients must accept 576-byte messages. */
#define QOSA_DHCPD_MIN_REPLY_LEN   576U

/* ----------------------------------------------------------------
 *  Mode B — reply descriptor
 * ---------------------------------------------------------------- */

/**
 * @struct qosa_netdev_dhcpd_reply_t
 * @brief Reply buffer descriptor for raw DHCP processing.
 */
typedef struct
{
    /*!< Caller-owned output buffer with at least QOSA_DHCPD_MIN_REPLY_LEN bytes. */
    qosa_uint8_t  *reply_buf;
    /*!< Number of bytes written to reply_buf. A value of 0 means no reply. */
    qosa_uint16_t  reply_len;
} qosa_netdev_dhcpd_reply_t;

/* ----------------------------------------------------------------
 *  Mode A — built-in UDP listener
 * ---------------------------------------------------------------- */

/**
 * @brief Start the DHCP server with the built-in UDP listener.
 *
 * @param[in] qosa_uint8_t idx
 *          - Netdev index.
 * @param[in] const qosa_netdev_dhcpd_cfg_t *cfg
 *          - DHCP server configuration.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_dhcpd_start(qosa_uint8_t idx, const qosa_netdev_dhcpd_cfg_t *cfg);

/* ----------------------------------------------------------------
 *  Common control
 * ---------------------------------------------------------------- */

/**
 * @brief Stop the DHCP server on a netdev.
 *
 * @param[in] qosa_uint8_t idx
 *          - Netdev index.
 *
 * @return Returns qosa_netdev_err_e status code.
 */
qosa_netdev_err_e qosa_netdev_dhcpd_stop(qosa_uint8_t idx);

/**
 * @brief Return whether the DHCP server is running on a netdev.
 *
 * @param[in] qosa_uint8_t idx
 *          - Netdev index.
 *
 * @return Returns QOSA_TRUE if the server is running, otherwise QOSA_FALSE.
 */
qosa_bool_t qosa_netdev_dhcpd_is_running(qosa_uint8_t idx);

/**
 * @brief Consume one Ethernet-framed DHCP request in raw mode and emit a reply through the configured path.
 *
 * @param[in] qosa_uint8_t idx
 *          - Netdev index.
 * @param[in] const qosa_uint8_t *data
 *          - Pointer to the Ethernet frame data.
 * @param[in] qosa_uint32_t len
 *          - Frame length in bytes.
 *
 * @return Returns QOSA_TRUE if the frame is consumed, otherwise QOSA_FALSE.
 */
qosa_bool_t qosa_netdev_dhcpd_frame_input(qosa_uint8_t        idx,
                                            const qosa_uint8_t *data,
                                            qosa_uint32_t       len);

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_NETDEV_DHCPD_INTERNAL_H__ */

