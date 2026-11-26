/*****************************************************************/ /**
 * @file qosa_netdev.h
 * @brief Public API for network device registration, control, DHCP, and NAT.
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

#ifndef __QOSA_NETDEV_H__
#define __QOSA_NETDEV_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** Maximum number of simultaneously registered network devices. */
#ifndef QOSA_NETDEV_MAX
#define QOSA_NETDEV_MAX 2
#endif

/* ----------------------------------------------------------------
 *  Error codes
 * ---------------------------------------------------------------- */
/**
 * @enum qosa_netdev_err_e
 * @brief Netdev operation status codes.
 */
typedef enum netdev_err
{
    QOSA_NETDEV_ERR_OK = 0, /*!< Operation completed successfully. */
    QOSA_NETDEV_ERR_PARAM,  /*!< Invalid input parameter. */
    QOSA_NETDEV_ERR_BUSY,   /*!< Requested resource is busy. */
    QOSA_NETDEV_ERR_TIMEOUT, /*!< Operation timed out. */
    QOSA_NETDEV_ERR_EXECUTE, /*!< Internal execution failed. */
} qosa_netdev_err_e;

/* ----------------------------------------------------------------
 *  Network device type
 * ---------------------------------------------------------------- */
/**
 * @enum qosa_netdev_type_e
 * @brief Supported network device classes.
 */
typedef enum
{
    QOSA_NETDEV_TYPE_ETHERNET = 0, /*!< Ethernet netdev. */
    QOSA_NETDEV_TYPE_WIFI,         /*!< Wi-Fi netdev. */
    QOSA_NETDEV_TYPE_PDN,          /*!< Cellular PDN netdev. */
} qosa_netdev_type_e;

/* ----------------------------------------------------------------
 *  Status events forwarded to the caller's status_cb
 * ---------------------------------------------------------------- */
/**
 * @enum qosa_netdev_status_e
 * @brief Status events delivered through status_cb.
 */
typedef enum
{
    QOSA_NETDEV_STATUS_IP_ASSIGNED = 0, /*!< DHCP lease obtained or static IPv4 is ready. */
    QOSA_NETDEV_STATUS_LINK_UP,         /*!< Physical link is up. */
    QOSA_NETDEV_STATUS_LINK_DOWN,       /*!< Physical link is down. */
} qosa_netdev_status_e;

/* ----------------------------------------------------------------
 *  Route preference
 * ---------------------------------------------------------------- */
/**
 * @enum qosa_netdev_route_prefer_e
 * @brief Preferred egress route selection.
 */
typedef enum netdev_route_prefer
{
    QOSA_NETDEV_ROUTE_PREFER_NONE = 0, /*!< No preferred route override. */
    QOSA_NETDEV_ROUTE_PREFER_ETH,      /*!< Prefer Ethernet route. */
    QOSA_NETDEV_ROUTE_PREFER_WIFI,     /*!< Prefer Wi-Fi route. */
    QOSA_NETDEV_ROUTE_PREFER_PDN,      /*!< Prefer PDN route. */
    QOSA_NETDEV_ROUTE_PREFER_MAX,      /*!< Boundary check guard. */
} qosa_netdev_route_prefer_e;

/* ----------------------------------------------------------------
 *  Option types for opt_set / opt_get
 * ---------------------------------------------------------------- */
/**
 * @enum qosa_netdev_opt_e
 * @brief Option selectors used by qosa_netdev_opt_set() and qosa_netdev_opt_get().
 */
typedef enum
{
    QOSA_NETDEV_OPT_DNS = 0,     /*!< DNS configuration option. */
    QOSA_NETDEV_OPT_DHCP_CLIENT, /*!< Start or stop the DHCP client. */
    QOSA_NETDEV_OPT_SCI,         /*!< SCI-related option. */
    QOSA_NETDEV_OPT_ROUTE_PREFER, /*!< Route preference option. */
    QOSA_NETDEV_OPT_MAC,         /*!< MAC address option. */
    QOSA_NETDEV_OPT_DHCP_SERVER, /*!< DHCP server configuration option. */
    QOSA_NETDEV_OPT_MODE,        /*!< Netdev mode option. */
    QOSA_NETDEV_OPT_ID,          /*!< Netdev identifier option. */
    QOSA_NETDEV_OPT_IPADDR,      /*!< Static IPv4 address, netmask, and gateway option. */
    QOSA_NETDEV_OPT_NONCOPY,    /*!< fix len to lower,avoid to mult-copy. */
    QOSA_NETDEV_OPT_MAX,         /*!< Boundary check guard. */
} qosa_netdev_opt_e;

/* ----------------------------------------------------------------
 *  DHCP server configuration embedded in opt_set / opt_get
 * ---------------------------------------------------------------- */

/**
 * @enum qosa_netdev_dhcpd_mode_e
 * @brief DHCP server operating mode, selected via opt_set(QOSA_NETDEV_OPT_DHCP_SERVER).
 *
 * QOSA_NETDEV_DHCPD_MODE_UDP (default, 0):
 *   The netdev layer opens a lwip UDP PCB on port 67.  DHCP traffic that
 *   arrives as normal Ethernet frames on the netif is handled automatically.
 *   Use for the standard MiFi / W5500 scenario.
 *
 * QOSA_NETDEV_DHCPD_MODE_RAW:
 *   No UDP PCB is created. The pool and lease table are initialised, but
 *   the caller is responsible for receiving DHCP request bytes from any
 *   transport and passing them to qosa_netdev_dhcpd_frame_input() one packet
 *   at a time. Use when the downstream channel does not go through lwip UDP
 *   (e.g. a raw SPI slave bus, a custom tunnel, etc.).
 */
typedef enum
{
    QOSA_NETDEV_DHCPD_MODE_UDP = 0, /**< Built-in UDP listener (Mode A) */
    QOSA_NETDEV_DHCPD_MODE_RAW,     /**< Caller-owned transport (Mode B) */
} qosa_netdev_dhcpd_mode_e;

/**
 * @brief DHCP server configuration used with QOSA_NETDEV_OPT_DHCP_SERVER.
 */
typedef struct
{
    qosa_uint8_t             enable;        /**< 1 = start DHCP server; 0 = stop */
    qosa_netdev_dhcpd_mode_e mode;          /**< UDP listener or raw packet mode */
    qosa_uint32_t            pool_start_ip; /**< Pool start — host-byte-order (0 = auto-derive) */
    qosa_uint32_t            pool_end_ip;   /**< Pool end   — host-byte-order (0 = auto-derive) */
    qosa_uint32_t            lease_time_s;  /**< Lease lifetime in seconds (0 = default) */
} qosa_netdev_dhcpd_cfg_t;

/**
 * @brief DHCP server configuration used with QOSA_NETDEV_OPT_DHCP_SERVER.
 */
typedef struct
{
    qosa_uint8_t             enable;        /**< 1 = start DHCP server; 0 = stop */
    qosa_uint16_t            offset;
    qosa_uint16_t            xter_len;    
} qosa_netdev_noncopy_t;

/* ----------------------------------------------------------------
 *  Option value container — passed to opt_set / opt_get
 * ---------------------------------------------------------------- */
/**
 * @brief Option value container passed to qosa_netdev_opt_set() and qosa_netdev_opt_get().
 */
typedef struct
{
    qosa_netdev_opt_e type; /**< Option selector describing which union member is valid. */
    union
    {
        qosa_netdev_route_prefer_e route_prefer; /**< QOSA_NETDEV_OPT_ROUTE_PREFER */
        qosa_uint8_t               mac[6];       /**< QOSA_NETDEV_OPT_MAC */
        qosa_uint8_t               dhcpc_enable; /**< QOSA_NETDEV_OPT_DHCP_CLIENT: 1=start, 0=stop */
        qosa_netdev_dhcpd_cfg_t    dhcpd;        /**< QOSA_NETDEV_OPT_DHCP_SERVER */
        struct {
            qosa_uint32_t ip;      /**< IPv4 address in host byte order */
            qosa_uint32_t netmask; /**< Subnet mask in host byte order  */
            qosa_uint32_t gw;      /**< Default gateway in host byte order */
        } ipaddr;                                /**< QOSA_NETDEV_OPT_IPADDR */
        qosa_netdev_noncopy_t  noncopy; /*!< fix len to lower,avoid to mult-copy. */
    } u;                                        /**< Option payload selected by type. */
} qosa_netdev_opt_t;

/* ----------------------------------------------------------------
 *  Callback types
 *
 *  output_cb: called by lwip linkoutput to transmit one Ethernet frame.
 *             idx      = netdev index assigned at register time
 *             data     = frame bytes (Ethernet II, including header)
 *             len      = frame length in bytes
 *
 *  status_cb: called when interface status changes (e.g. IP assigned).
 *             idx      = netdev index
 *             status   = event code
 *             arg      = cb_arg supplied at register time
 * ---------------------------------------------------------------- */
typedef qosa_netdev_err_e (*qosa_netdev_output_cb_t)(qosa_uint8_t        idx,
                                                      const qosa_uint8_t *data,
                                                      qosa_uint32_t       len);

typedef void (*qosa_netdev_status_cb_t)(qosa_uint8_t       idx,
                                        qosa_netdev_status_e status,
                                        void               *arg);

/* ----------------------------------------------------------------
 *  Registration descriptor — filled by the caller, passed to
 *  qosa_netdev_register().  The layer copies this struct internally.
 *
 *  This is a pure hardware descriptor: interface type, MAC address,
 *  and callbacks.  All DHCP policy (client start/stop, server start/stop)
 *  is controlled after registration via opt_set(QOSA_NETDEV_OPT_DHCP_CLIENT)
 *  and opt_set(QOSA_NETDEV_OPT_DHCP_SERVER).
 * ---------------------------------------------------------------- */
typedef struct
{
    qosa_netdev_type_e      type;       /**< Interface type (Ethernet / Wi-Fi / PDN) */
    qosa_netdev_output_cb_t output_cb;  /**< TX callback: lwip → caller's driver */
    qosa_netdev_status_cb_t status_cb;  /**< Status change notification */
    void                   *cb_arg;     /**< Opaque arg forwarded to status_cb */
    qosa_uint8_t            mac[6];     /**< Source MAC address */
} qosa_netdev_reg_t;

/* ----------------------------------------------------------------
 *  Public API
 * ---------------------------------------------------------------- */

/**
 * @brief Register a new network device into the lwip stack.
 *
 * Auto-allocates a free slot and returns the assigned index via idx_out.
 * Calls netifapi_netif_add() on the TCPIP thread — blocks until complete.
 *
 * @param reg     Registration descriptor (copied internally).
 * @param idx_out Assigned device index (used for all subsequent calls).
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_register(qosa_netdev_reg_t *reg, qosa_uint8_t *idx_out);

/**
 * @brief Unregister a previously registered network device.
 *
 * @param idx Netdev index returned by qosa_netdev_register().
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_unregister(qosa_uint8_t idx);

/**
 * @brief Inject a received byte buffer into the netdev.
 *
 * Handles all three input cases transparently:
 *   - Normal Ethernet frame  → forwarded to lwip ethernet_input
 *   - B1: Ethernet-framed DHCP (when server is in RAW mode) → intercepted,
 *         reply sent via output_cb; frame not forwarded to lwip
 *   - B2: Unframed DHCP bytes (no Ethernet/IP/UDP wrapper, when server is
 *         in RAW mode) → processed, reply wrapped in Eth+IP+UDP and sent
 *         via output_cb
 *
 * All processing happens on the TCPIP thread via tcpip_callback.
 *
 * @param idx   Netdev index.
 * @param data  Frame or raw bytes.
 * @param len   Length in bytes.
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_input(qosa_uint8_t idx, qosa_uint8_t *data, qosa_uint32_t len);

/**
 * @brief Bring the interface administratively UP.
 *
 * Calls netifapi_netif_set_up().  DHCP client is NOT started automatically;
 * call opt_set(QOSA_NETDEV_OPT_DHCP_CLIENT, enable=1) before or after set_up()
 * to enable the DHCP client.
 *
 * @param idx Netdev index returned by qosa_netdev_register().
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_set_up(qosa_uint8_t idx);

/**
 * @brief Bring the interface administratively DOWN.
 *
 * Stops DHCP client (if running), then calls netifapi_netif_set_down().
 *
 * @param idx Netdev index returned by qosa_netdev_register().
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_set_down(qosa_uint8_t idx);

/**
 * @brief Signal physical link UP.
 *
 * lwip will auto-call dhcp_network_changed() if the DHCP client is running.
 *
 * @param idx Netdev index returned by qosa_netdev_register().
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_set_link_up(qosa_uint8_t idx);

/**
 * @brief Signal physical link DOWN.
 *
 * DHCP client is suspended but not destroyed; reconnect (set_link_up) is fast.
 *
 * @param idx Netdev index returned by qosa_netdev_register().
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_set_link_down(qosa_uint8_t idx);

/**
 * @brief Set a network option.
 *
 * @param idx Netdev index returned by qosa_netdev_register().
 * @param ctx Pointer to qosa_netdev_opt_t with .type set to the desired option.
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_opt_set(qosa_uint8_t idx, void *ctx);

/**
 * @brief Get a network option.
 *
 * @param idx Netdev index returned by qosa_netdev_register().
 * @param ctx Pointer to qosa_netdev_opt_t with .type set to the desired option.
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_opt_get(qosa_uint8_t idx, void *ctx);

/* ----------------------------------------------------------------
 *  NAT API
 *
 *  Wire a registered LAN netdev to the lwip ip4_nat SNAT/DNAT engine,
 *  routing LAN traffic out via the cellular PDN identified by (simid, cid).
 *
 *  Prerequisites before calling nat_bind():
 *    - LAN netif must have a static IP+netmask set via opt_set(IPADDR).
 *    - LAN netdev must be set_up() + set_link_up().
 *    - The data call for (simid, cid) must be active (IP assigned).
 * ---------------------------------------------------------------- */

/**
 * @brief Bind LAN netdev to WAN via NAT.
 *
 * @param lan_idx  Netdev index of the LAN interface.
 * @param simid    SIM id of the active data call.
 * @param cid      CID of the active data call (PDP context).
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_nat_bind(qosa_uint8_t lan_idx,
                                        qosa_uint8_t simid,
                                        qosa_uint8_t cid);

/**
 * @brief Unbind NAT for a LAN netdev.  Idempotent if not bound.
 *
 * @param lan_idx  Netdev index of the LAN interface.
 * @param simid    SIM id used at bind time.
 * @param cid      CID used at bind time.
 * @return QOSA_NETDEV_ERR_OK on success.
 */
qosa_netdev_err_e qosa_netdev_nat_unbind(qosa_uint8_t lan_idx,
                                          qosa_uint8_t simid,
                                          qosa_uint8_t cid);

/**
 * @brief Query whether NAT is currently bound for a LAN netdev.
 *
 * @param lan_idx  Netdev index of the LAN interface.
 * @return QOSA_TRUE if bound, QOSA_FALSE otherwise.
 */
qosa_bool_t qosa_netdev_nat_is_bound(qosa_uint8_t lan_idx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __QOSA_NETDEV_H__ */
