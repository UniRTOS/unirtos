/*****************************************************************/ /**
* @file qosa_lan.h
* @brief LAN operation interface
* @author larson.li@quectel.com
* @date 2025-05-09
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-05-09 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_LAN_H__
#define __QOSA_LAN_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/** Maximum IPv4 string length including the terminator. */
#define QOSA_LAN_MAX_IP_LEN 16
/** Maximum MAC string length including the terminator. */
#define QOSA_LAN_MAX_MAC_LEN 18
/** Maximum number of static bindings shared by all SSIDs. */
#define QOSA_LAN_STATIC_BINDING_MAX 30

/** @enum qosa_lan_error_e
 *  @brief LAN API error codes.
 */
typedef enum {
    QOSA_LAN_ERR_OK = 0,                                      /*!< Operation succeeded. */
    QOSA_LAN_ERR_INVALID_PARAM = 1 | QOSA_COMPONENT_API_LAN, /*!< Null, empty, or unsupported parameter. */
    QOSA_LAN_ERR_NOT_INITIALIZED,                             /*!< LAN service is not initialized. */
    QOSA_LAN_ERR_INVALID_STATE,                               /*!< LAN runtime state does not allow the operation. */
    QOSA_LAN_ERR_INVALID_IP,                                  /*!< IPv4 or MAC address format is invalid. */
    QOSA_LAN_ERR_INVALID_SUBNET,                              /*!< Address does not belong to the configured /24 subnet. */
    QOSA_LAN_ERR_INVALID_RANGE,                               /*!< DHCP pool or lease range is invalid. */
    QOSA_LAN_ERR_CONFLICT,                                    /*!< Address or binding conflicts with an existing resource. */
    QOSA_LAN_ERR_OPERATION_DISABLED,                          /*!< Address-pool editing is disabled. */
    QOSA_LAN_ERR_NOT_FOUND,                                   /*!< Requested binding or device was not found. */
    QOSA_LAN_ERR_NO_RESOURCE,                                 /*!< Memory, table capacity, or platform resource is unavailable. */
    QOSA_LAN_ERR_PLATFORM,                                    /*!< Platform DHCP, DNS, or netif operation failed. */
    QOSA_LAN_ERR_NVM,                                         /*!< Persistent configuration access failed. */
    QOSA_LAN_ERR_WIFI,                                        /*!< Wi-Fi stop, start, or status operation failed. */
    QOSA_LAN_ERR_USBNET,                                      /*!< USB network stop, start, or query operation failed. */
    QOSA_LAN_ERR_NOT_SUPPORTED,                               /*!< Operation is not implemented on this platform. */
    QOSA_LAN_ERR_GENERAL                                      /*!< Unclassified LAN failure. */
} qosa_lan_error_e;

/** @enum qosa_lan_state_e
 *  @brief LAN configuration runtime state.
 */
typedef enum {
    QOSA_LAN_STATE_UNINITIALIZED = 0, /*!< LAN service is not initialized. */
    QOSA_LAN_STATE_LOADED,            /*!< Configuration is loaded; resources may be pending. */
    QOSA_LAN_STATE_APPLYING,          /*!< A complete candidate configuration is being applied. */
    QOSA_LAN_STATE_READY,             /*!< Committed configuration is active. */
    QOSA_LAN_STATE_DEGRADED           /*!< Runtime state cannot be proven consistent. */
} qosa_lan_state_e;

/** @enum qosa_lan_changed_field_e
 *  @brief Bit mask describing fields explicitly changed by a caller.
 */
typedef enum {
    QOSA_LAN_FIELD_NONE          = 0U,         /*!< No field was explicitly changed. */
    QOSA_LAN_FIELD_GATEWAY       = (1U << 0), /*!< Gateway was changed. */
    QOSA_LAN_FIELD_SUBNET_MASK   = (1U << 1), /*!< Subnet mask was changed. */
    QOSA_LAN_FIELD_DHCP_ENABLE   = (1U << 2), /*!< Address-pool editing permission changed. */
    QOSA_LAN_FIELD_DHCP_START    = (1U << 3), /*!< Pool minimum changed. */
    QOSA_LAN_FIELD_DHCP_END      = (1U << 4), /*!< Pool maximum changed. */
    QOSA_LAN_FIELD_LEASE_TIME    = (1U << 5), /*!< Lease time changed. */
    QOSA_LAN_FIELD_DNS_ENABLE    = (1U << 6), /*!< User DNS switch changed. */
    QOSA_LAN_FIELD_DNS1          = (1U << 7), /*!< Primary user DNS changed. */
    QOSA_LAN_FIELD_DNS2          = (1U << 8)  /*!< Secondary user DNS changed. */
} qosa_lan_changed_field_e;

/** @struct qosa_lan_config_t
 *  @brief Complete LAN configuration used for validation and atomic application.
 */
typedef struct {
    char gateway[QOSA_LAN_MAX_IP_LEN];        /*!< LAN gateway IPv4 address. */
    char subnet_mask[QOSA_LAN_MAX_IP_LEN];    /*!< LAN subnet mask; fixed to /24 this release. */
    int dhcp_enabled;                         /*!< Address-pool editing permission, not DHCP runtime state. */
    char dhcp_start_ip[QOSA_LAN_MAX_IP_LEN];  /*!< Configured pool minimum and USB reserved address. */
    char dhcp_end_ip[QOSA_LAN_MAX_IP_LEN];    /*!< Configured pool maximum address. */
    unsigned int dhcp_lease_time;             /*!< DHCP lease time in seconds. */
    int dns_custom_enabled;                   /*!< User-defined DNS switch. */
    char dns1[QOSA_LAN_MAX_IP_LEN];           /*!< User primary DNS; an empty value means fallback. */
    char dns2[QOSA_LAN_MAX_IP_LEN];           /*!< User secondary DNS; an empty value means fallback. */
} qosa_lan_config_t;

/*===========================================================================
 * DHCP
 *===========================================================================*/

/**
 * @brief Get DHCP server enable status
 *
 * @param[out] enable
 *          - Pointer to store the DHCP server enable status
 *              - 0: DHCP server disabled
 *              - 1: DHCP server enabled
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_dhcp_enable(int *enable);

/**
 * @brief Set DHCP server enable status
 *
 * @param[in] enable
 *          - DHCP server enable status to set
 *              - 0: Disable DHCP server
 *              - 1: Enable DHCP server
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_dhcp_enable(int enable);

/*===========================================================================
 * Gateway
 *===========================================================================*/

/**
 * @brief Set LAN gateway IP address (module platform only)
 *
 * @param[in] ip
 *          - Gateway IP address string to set (IPv4 format)
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_gateway_ip(const char *ip);

/*===========================================================================
 * DHCP Start IP
 *===========================================================================*/

/**
 * @brief Get DHCP IP address pool start address
 *
 * @param[out] buf
 *          - Buffer to store the DHCP start IP address string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_dhcp_start_ip(char *buf, unsigned int buf_len);

/**
 * @brief Set DHCP IP address pool start address
 *
 * @param[in] ip
 *          - DHCP start IP address string to set (IPv4 format)
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_dhcp_start_ip(const char *ip);

/*===========================================================================
 * DHCP End IP
 *===========================================================================*/

/**
 * @brief Get DHCP IP address pool end address
 *
 * @param[out] buf
 *          - Buffer to store the DHCP end IP address string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_dhcp_end_ip(char *buf, unsigned int buf_len);

/**
 * @brief Set DHCP IP address pool end address
 *
 * @param[in] ip
 *          - DHCP end IP address string to set (IPv4 format)
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_dhcp_end_ip(const char *ip);

/*===========================================================================
 * DHCP Lease Time
 *===========================================================================*/

/**
 * @brief Get DHCP IP address lease time
 *
 * @param[out] seconds
 *          - Pointer to store the lease time in seconds
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_dhcp_lease_time(unsigned int *seconds);

/**
 * @brief Set DHCP IP address lease time
 *
 * @param[in] seconds
 *          - Lease time to set, in seconds
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_dhcp_lease_time(unsigned int seconds);

/*===========================================================================
 * DNS
 *===========================================================================*/

/**
 * @brief Get DNS proxy enable status
 *
 * @param[out] enable
 *          - Pointer to store the DNS proxy enable status
 *              - 0: DNS proxy disabled
 *              - 1: DNS proxy enabled
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_dns_enable(int *enable);

/**
 * @brief Set DNS proxy enable status
 *
 * @param[in] enable
 *          - DNS proxy enable status to set
 *              - 0: Disable DNS proxy
 *              - 1: Enable DNS proxy
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_dns_enable(int enable);

/**
 * @brief Get primary DNS server address
 *
 * @param[out] buf
 *          - Buffer to store the primary DNS IP address string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_dns1(char *buf, unsigned int buf_len);

/**
 * @brief Set primary DNS server address
 *
 * @param[in] ip
 *          - Primary DNS IP address string to set (IPv4 format)
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_dns1(const char *ip);

/**
 * @brief Get secondary DNS server address
 *
 * @param[out] buf
 *          - Buffer to store the secondary DNS IP address string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_dns2(char *buf, unsigned int buf_len);

/**
 * @brief Set secondary DNS server address
 *
 * @param[in] ip
 *          - Secondary DNS IP address string to set (IPv4 format)
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_dns2(const char *ip);

/*===========================================================================
 * Static IP (MAC-IP Binding)
 *===========================================================================*/
/**
 * @struct qosa_lan_static_ip_info_s
 * @brief Static IP binding entry
 */
typedef struct {
    char mac[QOSA_LAN_MAX_MAC_LEN]; /*!< MAC address */
    char ip[QOSA_LAN_MAX_IP_LEN];   /*!< IP address */
} qosa_lan_static_ip_info_s;

/**
 * @brief Get the number of static IP binding entries
 *
 * @param[out] count
 *          - Pointer to store the number of static IP binding entries
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_static_ip_count(unsigned int *count);

/**
 * @brief Get static IP binding entry by index
 *
 * @param[in] index
 *          - Binding entry index to query (zero-based)
 * @param[out] info
 *          - Pointer to qosa_lan_static_ip_info_s structure to fill with binding information
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_static_ip_info(unsigned int index, qosa_lan_static_ip_info_s *info);

/**
 * @brief Add a static IP binding entry
 *
 * This function adds a MAC-to-IP binding so that the DHCP server always
 * assigns the specified IP address to the device with the given MAC address.
 *
 * @param[in] mac
 *          - MAC address string to bind
 * @param[in] ip
 *          - IP address string to assign to the specified MAC address
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_add_static_ip(const char *mac, const char *ip);

/**
 * @brief Delete a static IP binding entry by MAC address
 *
 * @param[in] mac
 *          - MAC address string of the binding entry to delete
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_delete_static_ip(const char *mac);

/**
 * @brief Get a LAN client MAC address by IPv4 address.
 *
 * @param[in] ip
 *          - IPv4 address string to query
 * @param[out] mac
 *          - Buffer to store the MAC address string
 * @param[in] mac_len
 *          - Length of the MAC address buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_device_mac_by_ip(const char *ip, char *mac, unsigned int mac_len);

/**
 * @brief Get the complete committed LAN configuration.
 *
 * @param[out] qosa_lan_config_t *config
 *          - Output configuration buffer.
 * @return int
 *          - 0 on success; other values on failure.
 */
qosa_lan_error_e qosa_lan_get_config(qosa_lan_config_t *config);

/**
 * @brief Validate a complete LAN candidate configuration.
 *
 * @param[in] const qosa_lan_config_t *candidate
 *          - Candidate configuration to validate.
 * @return int
 *          - 0 when valid; other values when invalid.
 */
qosa_lan_error_e qosa_lan_validate_config(const qosa_lan_config_t *candidate);

/**
 * @brief Apply and commit one complete LAN candidate configuration.
 *
 * @param[in] const qosa_lan_config_t *candidate
 *          - Complete candidate configuration.
 * @param[in] unsigned int changed_fields
 *          - Bit mask from qosa_lan_changed_field_e.
 * @return int
 *          - 0 on success; other values on failure.
 */
qosa_lan_error_e qosa_lan_apply_config(const qosa_lan_config_t *candidate,
                          unsigned int changed_fields);

/**
 * @brief Get the current LAN runtime state.
 *
 * @param[out] qosa_lan_state_e *state
 *          - Output runtime state.
 * @return int
 *          - 0 on success; other values on failure.
 */
qosa_lan_error_e qosa_lan_get_state(qosa_lan_state_e *state);

/**
 * @brief Get LAN gateway IP address
 *
 * @param[out] buf
 *          - Buffer to store the gateway IP address string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_gateway_ip(char *buf, unsigned int buf_len);

/*===========================================================================
 * Subnet Mask
 *===========================================================================*/

/**
 * @brief Get LAN subnet mask
 *
 * @param[out] buf
 *          - Buffer to store the subnet mask string (e.g., "255.255.255.0")
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_get_subnet_mask(char *buf, unsigned int buf_len);

/**
 * @brief Set LAN subnet mask
 *
 * @param[in] mask
 *          - Subnet mask string to set (IPv4 format)
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_set_subnet_mask(const char *mask);

/*===========================================================================
 * Lifecycle
 *===========================================================================*/

/**
 * @brief Initialize LAN subsystem
 *
 * Reads configuration from NVM, initializes DHCP and DNS modules,
 * and applies all settings to the platform.
 *
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_lan_error_e qosa_lan_init(void);
#endif /* __QOSA_LAN_H__ */
