/*****************************************************************/ /**
* @file qosa_wifi.h
* @brief WiFi operation interface
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
#ifndef __QOSA_WIFI_H__
#define __QOSA_WIFI_H__

#include "qosa_def.h"
#include "qosa_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSA_WIFI_SSID_MAX_LEN          32U
#define QOSA_WIFI_SSID_BUF_LEN          (QOSA_WIFI_SSID_MAX_LEN + 1U)
#define QOSA_WIFI_PASSWORD_MAX_LEN      65U
#define QOSA_WIFI_SECURITY_MODE_MAX_LEN 16U
#define QOSA_WIFI_MAC_LEN               6U
#define QOSA_MAX_MAC_FILTER             16U

#ifndef QOSA_WIFI_MAX_NUM_CLIENTS
#define QOSA_WIFI_MAX_NUM_CLIENTS        16U
#endif
#ifndef QOSA_WIFI_MAX_BLACKLIST_NUM
#define QOSA_WIFI_MAX_BLACKLIST_NUM      16U
#endif
#ifndef QOSA_WIFI_DEVICE_NAME_MAX_LEN
#define QOSA_WIFI_DEVICE_NAME_MAX_LEN    32U
#endif
#ifndef QOSA_WIFI_DEVICE_HISTORY_MAX_NUM
#define QOSA_WIFI_DEVICE_HISTORY_MAX_NUM 32U
#endif
#ifndef QOSA_WIFI_DEVICE_MAC_STR_LEN
#define QOSA_WIFI_DEVICE_MAC_STR_LEN     18U
#endif
#ifndef QOSA_WIFI_DEVICE_IP_STR_LEN
#define QOSA_WIFI_DEVICE_IP_STR_LEN      16U
#endif
#ifndef QOSA_WIFI_DEVICE_NAME_STR_LEN
#define QOSA_WIFI_DEVICE_NAME_STR_LEN    32U
#endif

#define QOSA_WIFI_CFG_STATUS         ((qosa_uint32_t)1U << 0)
#define QOSA_WIFI_CFG_BAND           ((qosa_uint32_t)1U << 1)
#define QOSA_WIFI_CFG_SSID           ((qosa_uint32_t)1U << 2)
#define QOSA_WIFI_CFG_PASSWORD       ((qosa_uint32_t)1U << 3)
#define QOSA_WIFI_CFG_BROADCAST      ((qosa_uint32_t)1U << 4)
#define QOSA_WIFI_CFG_ISOLATION      ((qosa_uint32_t)1U << 5)
#define QOSA_WIFI_CFG_SECURITY_MODE  ((qosa_uint32_t)1U << 6)
#define QOSA_WIFI_CFG_MAX_CLIENTS    ((qosa_uint32_t)1U << 7)
#define QOSA_WIFI_CFG_BEACON_PERIOD  ((qosa_uint32_t)1U << 8)
#define QOSA_WIFI_CFG_DTIM_INTERVAL  ((qosa_uint32_t)1U << 9)
#define QOSA_WIFI_CFG_WIRELESS_MODE  ((qosa_uint32_t)1U << 10)
#define QOSA_WIFI_CFG_CHANNEL        ((qosa_uint32_t)1U << 11)
#define QOSA_WIFI_CFG_WIDTH          ((qosa_uint32_t)1U << 12)
#define QOSA_WIFI_CFG_ALL            (((qosa_uint32_t)1U << 13) - 1U)

/** WiFi PHY mode */
#define QOSA_WIFI_PHY_MODE_B          ((qosa_uint32_t)0x01U << 0)
#define QOSA_WIFI_PHY_MODE_G          ((qosa_uint32_t)0x01U << 1)
#define QOSA_WIFI_PHY_MODE_A          ((qosa_uint32_t)0x01U << 2)
#define QOSA_WIFI_PHY_MODE_GN         ((qosa_uint32_t)0x01U << 3)
#define QOSA_WIFI_PHY_MODE_AN         ((qosa_uint32_t)0x01U << 4)
#define QOSA_WIFI_PHY_MODE_AX         ((qosa_uint32_t)0x01U << 5)
#define QOSA_WIFI_PHY_MODE_AC         ((qosa_uint32_t)0x01U << 6)

#define QOSA_WIFI_BAND_B        QOSA_WIFI_PHY_MODE_B
#define QOSA_WIFI_BAND_G        QOSA_WIFI_PHY_MODE_G
#define QOSA_WIFI_BAND_A        QOSA_WIFI_PHY_MODE_A
#define QOSA_WIFI_BAND_GN       QOSA_WIFI_PHY_MODE_GN
#define QOSA_WIFI_BAND_AN       QOSA_WIFI_PHY_MODE_AN
#define QOSA_WIFI_BAND_AX       QOSA_WIFI_PHY_MODE_AX
#define QOSA_WIFI_BAND_AC       QOSA_WIFI_PHY_MODE_AC


/*===========================================================================
 * WiFi Device Manager - Constants
 *===========================================================================*/

/*===========================================================================
 * WiFi Device Manager - Error Codes
 *===========================================================================*/

typedef enum
{
    QOSA_WIFI_SUCCESS = 0,
    QOSA_WIFI_ERR_OK = 0,
    QOSA_WIFI_ERROR = -1,
    QOSA_WIFI_INVALID_PARAM_ERR = -2,
    QOSA_WIFI_MEM_ADDR_NULL_ERR = -3,
    QOSA_WIFI_EXECUTE_ERR = -4,
    QOSA_WIFI_NO_MEMORY_ERR = -5,
    QOSA_WIFI_NO_SPACE_ERR = -6,
    QOSA_WIFI_NOT_FOUND_ERR = -7,
    QOSA_WIFI_BUSY_ERR = -8,
    QOSA_WIFI_NAME_TOO_LONG_ERR = -9,
} qosa_wifi_err_e;

#ifndef QOSA_WIFI_DEV_MGR_OK
#define QOSA_WIFI_DEV_MGR_OK                  QOSA_WIFI_SUCCESS
#define QOSA_WIFI_DEV_MGR_ERROR               QOSA_WIFI_ERROR
#define QOSA_WIFI_DEV_MGR_ERROR_PARAM_INVALID QOSA_WIFI_INVALID_PARAM_ERR
#define QOSA_WIFI_DEV_MGR_ERROR_MAC_INVALID   (-1002)
#define QOSA_WIFI_DEV_MGR_ERROR_NAME_TOO_LONG QOSA_WIFI_NAME_TOO_LONG_ERR
#define QOSA_WIFI_DEV_MGR_ERROR_NOT_FOUND     QOSA_WIFI_NOT_FOUND_ERR
#define QOSA_WIFI_DEV_MGR_ERROR_PSM           (-1005)
#define QOSA_WIFI_DEV_MGR_ERROR_BUSY          QOSA_WIFI_BUSY_ERR
#define QOSA_WIFI_DEV_MGR_ERROR_NOT_SUPPORTED  (-1006)
#endif


/*===========================================================================
 * WiFi Wireless Mode
 *===========================================================================*/

/**
 * @enum qosa_wifi_wireless_mode_e
 * @brief WiFi wireless mode
 */
typedef enum {
    QOSA_WIFI_WIRELESS_LEGACY, /*!< Maximum 802.11b/g capability */
    QOSA_WIFI_WIRELESS_4,      /*!< Maximum 802.11n capability; backward compatible with 802.11b/g */
    QOSA_WIFI_WIRELESS_5,      /*!< Maximum 802.11ac capability; backward compatible with 802.11n/b/g */
    QOSA_WIFI_WIRELESS_6,      /*!< Maximum 802.11ax capability; backward compatible with 802.11ac/n/b/g */
    QOSA_WIFI_WIRELESS_MAX,
} qosa_wifi_wireless_mode_e;

typedef qosa_uint8_t qosa_wifi_mac_addr_t[QOSA_WIFI_MAC_LEN];
typedef char qosa_wifi_mac_name_t[QOSA_WIFI_SSID_MAX_LEN];

typedef enum
{
    QOSA_PROTOCOL_NO_SECURITY,
    QOSA_PROTOCOL_WPA2,
    QOSA_PROTOCOL_WPA3,
    QOSA_PROTOCOL_WPA2_WPA_MIXED,
    QOSA_PROTOCOL_WPA3_WPA2_MIXED,
    QOSA_PROTOCOL_MAX
} qosa_wifi_protocol_e;


/*===========================================================================
 * WiFi MAC Filter Structure
 *===========================================================================*/

/**
 * @struct qosa_wifi_mac_filter_s
 * @brief WiFi MAC filter configuration
 */
typedef struct {
    qosa_int16_t filter_mode;                                        /*!< Filter mode: 0=disable, 1=whitelist, 2=blacklist */
    qosa_int16_t allow_count;
    qosa_wifi_mac_addr_t allow_mac_list[QOSA_MAX_MAC_FILTER];
    qosa_wifi_mac_name_t allow_name_list[QOSA_MAX_MAC_FILTER];
    qosa_int16_t deny_count;                                         /*!< Number of deny entries */
    qosa_wifi_mac_addr_t deny_mac_list[QOSA_MAX_MAC_FILTER];  /*!< Deny MAC list (binary format) */
    qosa_wifi_mac_name_t deny_name_list[QOSA_MAX_MAC_FILTER];  /*!< Deny device name list */
} qosa_wifi_mac_filter_t;

typedef qosa_wifi_mac_filter_t qosa_wifi_mac_filter_s;


typedef struct
{
    qosa_uint32_t mask;
    qosa_bool_t wifi_status;
    qosa_int32_t wifi_band;
    char ssid[QOSA_WIFI_SSID_BUF_LEN];
    char passwd[QOSA_WIFI_PASSWORD_MAX_LEN];
    qosa_bool_t broadcast;
    qosa_bool_t isolation;
    qosa_wifi_protocol_e protocol;
    qosa_uint32_t max_clients;
    qosa_uint32_t beacon_period;
    qosa_uint32_t dtim_interval;
    qosa_wifi_wireless_mode_e wireless_mode;
    qosa_uint32_t channel;
    qosa_int32_t width;
    qosa_wifi_mac_addr_t mac_addr;
} qosa_wifi_common_config_t;

typedef enum {
    QOSA_WIFI_RADIO_BAND_2G4 = 1,
    QOSA_WIFI_RADIO_BAND_5G = 2,
} qosa_wifi_radio_band_e;

#define QOSA_WIFI_BAND_MASK_2G4               (1U << 0)
#define QOSA_WIFI_BAND_MASK_5G                (1U << 1)

typedef struct {
    qosa_uint32_t field_mask;
    char ssid[QOSA_WIFI_SSID_BUF_LEN];
    char passwd[QOSA_WIFI_PASSWORD_MAX_LEN];
    qosa_bool_t broadcast;
    qosa_wifi_protocol_e protocol;
    qosa_wifi_wireless_mode_e wireless_mode;
    qosa_uint32_t channel;
    qosa_int32_t width;
} qosa_wifi_band_config_t;


/*===========================================================================
 * WiFi Status
 *===========================================================================*/

/**
 * @brief Open or close WiFi
 *
 * This function is used to control the WiFi power state. When WiFi is turned off
 * for configuration changes, changes will not take effect until WiFi is restarted.
 *
 * @param[in] on_off
 *          - WiFi power state
 *              - 0: Close WiFi
 *              - 1: Open WiFi
 */
qosa_wifi_err_e qosa_wifi_open(qosa_bool_t enable);

qosa_wifi_err_e qosa_wifi_insmod(const char *modname);
qosa_wifi_err_e qosa_wifi_rmmod(const char *modname);

qosa_wifi_err_e qosa_wifi_get_common_config(qosa_wifi_common_config_t *config);
qosa_wifi_err_e qosa_wifi_set_common_config(const qosa_wifi_common_config_t *config);

/**
 * @brief Get WiFi enable status
 *
 * @param[out] enable
 *          - Pointer to store the WiFi enable status
 *              - 0: WiFi disabled
 *              - 1: WiFi enabled
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_status(qosa_bool_t *enable);

/**
 * @brief Set WiFi enable status
 *
 * @param[in] enable
 *          - WiFi enable status to set
 *              - 0: Disable WiFi
 *              - 1: Enable WiFi
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_set_status(qosa_bool_t enable);

/*===========================================================================
 * WiFi Band
 *===========================================================================*/

/**
 * @brief Get WiFi frequency band
 *
 * @param[out] band
 *          - Pointer to store the current WiFi band
 *              - 1: 2.4GHz
 *              - 2: 5GHz
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_band(qosa_int32_t *band);

/**
 * @brief Set WiFi frequency band
 *
 * @param[in] band
 *          - WiFi band to set
 *              - 1: 2.4GHz
 *              - 2: 5GHz
 * @return int
 *          - 0 on success
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_band(qosa_int32_t band);

/**
 * @note field_mask 复用 QOSA_WIFI_CFG_SSID/PASSWORD/BROADCAST/SECURITY_MODE/
 *       WIRELESS_MODE/CHANNEL/WIDTH。QOSA 频段值 1=2.4G、2=5G；WebUI 映射由
 *       web_comm_wlan.c 负责。
 */
qosa_wifi_err_e qosa_wifi_get_supported_band_mask(qosa_uint32_t *band_mask);
qosa_wifi_err_e qosa_wifi_get_band_config(qosa_wifi_radio_band_e band,
                                           qosa_wifi_band_config_t *config);
qosa_wifi_err_e qosa_wifi_set_band_config(qosa_wifi_radio_band_e band,
                                           const qosa_wifi_band_config_t *config);

/*===========================================================================
 * WiFi SSID
 *===========================================================================*/

/**
 * @brief Get WiFi SSID
 *
 * @param[out] buf
 *          - Buffer to store the SSID string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_ssid(char *buf, qosa_uint32_t *buf_len);

/**
 * @brief Set WiFi SSID
 *
 * @param[in] ssid
 *          - SSID string to set
 * @return int
 *          - 0 on success
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_ssid(const char *ssid);

/*===========================================================================
 * WiFi Password
 *===========================================================================*/

/**
 * @brief Get WiFi password
 *
 * @param[out] buf
 *          - Buffer to store the password string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_password(char *buf, qosa_uint32_t *buf_len);

/**
 * @brief Set WiFi password
 *
 * @param[in] password
 *          - Password string to set
 * @return int
 *          - 0 on success
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_password(const char *password);

/*===========================================================================
 * WiFi Broadcast (SSID broadcast)
 *===========================================================================*/

/**
 * @brief Get WiFi SSID broadcast status
 *
 * @param[out] enable
 *          - Pointer to store the broadcast status
 *              - 0: SSID broadcast disabled (hidden)
 *              - 1: SSID broadcast enabled
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_broadcast(qosa_bool_t *enable);

/**
 * @brief Set WiFi SSID broadcast status
 *
 * @param[in] enable
 *          - Broadcast status to set
 *              - 0: Disable SSID broadcast (hide SSID)
 *              - 1: Enable SSID broadcast
 * @return int
 *          - 0 on success
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_broadcast(qosa_bool_t enable);

/*===========================================================================
 * WiFi AP Isolation
 *===========================================================================*/

/**
 * @brief Get WiFi AP isolation status
 *
 * AP isolation prevents connected clients from communicating with each other
 * through the access point.
 *
 * @param[out] enable
 *          - Pointer to store the isolation status
 *              - 0: AP isolation disabled
 *              - 1: AP isolation enabled
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_isolation(qosa_bool_t *enable);

/**
 * @brief Set WiFi AP isolation status
 *
 * AP isolation prevents connected clients from communicating with each other
 * through the access point.
 *
 * @param[in] enable
 *          - Isolation status to set
 *              - 0: Disable AP isolation
 *              - 1: Enable AP isolation
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_set_isolation(qosa_bool_t enable);

/*===========================================================================
 * WiFi Security Mode
 *===========================================================================*/

qosa_wifi_err_e qosa_wifi_get_security_mode(qosa_wifi_protocol_e *protocol);
qosa_wifi_err_e qosa_wifi_set_security_mode(qosa_wifi_protocol_e protocol);

/**
 * @brief Get WiFi security mode
 *
 * @param[out] buf
 *          - Buffer to store the security mode string
 * @param[in] buf_len
 *          - Length of the buffer
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_security_mode_ex(char *buf, qosa_uint32_t *buf_len);

/**
 * @brief Set WiFi security mode
 *
 * @param[in] mode
 *          - Security mode string to set
 * @return int
 *          - 0 on success
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_security_mode_ex(const char *mode);

/*===========================================================================
 * WiFi Max Clients
 *===========================================================================*/

/**
 * @brief Get WiFi maximum connected clients
 *
 * @param[out] count
 *          - Pointer to store the maximum number of clients
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_max_clients(qosa_uint32_t *count);

/**
 * @brief Set WiFi maximum connected clients
 *
 * @param[in] count
 *          - Maximum number of clients to set
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_set_max_clients(qosa_uint32_t count);

qosa_wifi_err_e qosa_wifi_get_beacon_period(qosa_uint32_t *period);
qosa_wifi_err_e qosa_wifi_set_beacon_period(qosa_uint32_t period);
qosa_wifi_err_e qosa_wifi_get_dtim_interval(qosa_uint32_t *interval);
qosa_wifi_err_e qosa_wifi_set_dtim_interval(qosa_uint32_t interval);


/*===========================================================================
 * WiFi Wireless Mode
 *===========================================================================*/

/**
 * @brief Get WiFi wireless mode
 *
 * @param[out] mode
 *          - Pointer to store the wireless mode, refer to qosa_wifi_wireless_mode_e
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_wireless_mode(qosa_wifi_wireless_mode_e *mode);

/**
 * @brief Set WiFi wireless mode
 *
 * @param[in] mode
 *          - Wireless mode to set, refer to qosa_wifi_wireless_mode_e
 * @return int
 *          - 0 on success
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_wireless_mode(qosa_wifi_wireless_mode_e mode);

/*===========================================================================
 * WiFi Channel
 *===========================================================================*/

/**
 * @brief Get WiFi channel
 *
 * @param[out] channel
 *          - Pointer to store the current channel number; 0 means automatic selection
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_channel(qosa_uint32_t *channel);

/**
 * @brief Set WiFi channel
 *
 * @param[in] channel
 *          - Channel number to set; 0 means automatic selection
 * @return int
 *          - 0 on success
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_channel(qosa_uint32_t channel);

/*===========================================================================
 * WiFi Width
 *===========================================================================*/

/**
 * @brief Get WiFi channel bandwidth
 *
 * @param[out] width
 *          - Pointer to store the channel bandwidth value
 *              - 1: 20MHz
 *              - 2: 40MHz
 * @return qosa_wifi_err_e
 *          - QOSA_WIFI_SUCCESS on success
 *          - QOSA_WIFI_INVALID_PARAM_ERR if width is NULL
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_width(qosa_int32_t *width);

/**
 * @brief Set WiFi channel bandwidth
 *
 * @param[in] width
 *          - Channel bandwidth to set
 *              - 1: 20MHz
 *              - 2: 40MHz
 * @return qosa_wifi_err_e
 *          - QOSA_WIFI_SUCCESS on success
 *          - QOSA_WIFI_INVALID_PARAM_ERR if width value is invalid
 *          - Other values on failure
 * @note This configuration requires WiFi restart to take effect.
 */
qosa_wifi_err_e qosa_wifi_set_width(qosa_int32_t width);


/*===========================================================================
 * WiFi MAC Filter (complex API)
 *===========================================================================*/

/**
 * @brief Get WiFi MAC filter configuration
 *
 * Retrieves the current MAC address filtering settings, including the filter
 * mode (disable/whitelist/blacklist) and deny list entries.
 *
 * @param[out] filter
 *          - Pointer to qosa_wifi_mac_filter_s structure to fill with filter configuration
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_filter(qosa_wifi_mac_filter_t *filter);

/**
 * @brief Set WiFi MAC filter configuration
 *
 * Configures the MAC address filtering settings. The filter mode can be set
 * to disable filtering, use a whitelist, or use a blacklist.
 *
 * @param[in] filter
 *          - Pointer to qosa_wifi_mac_filter_s structure containing filter configuration
 * @return int
 *          - 0 on success
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_set_filter(const qosa_wifi_mac_filter_t *filter);

qosa_wifi_err_e qosa_wifi_reset_wlan_client_traffic(void);

/**
 * @brief Get WiFi MAC address
 *
 * @param[out] mac_addr
 *          - Pointer to a caller-provided 6-byte buffer for the MAC address
 * @return qosa_wifi_err_e
 *          - QOSA_WIFI_SUCCESS on success
 *          - QOSA_WIFI_INVALID_PARAM_ERR if mac_addr is NULL
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_get_mac(qosa_wifi_mac_addr_t mac_addr);

/**
 * @brief Set WiFi MAC address
 *
 * @param[in] mac_addr
 *          - Pointer to a 6-byte MAC address; the input buffer is not modified
 * @return qosa_wifi_err_e
 *          - QOSA_WIFI_SUCCESS on success
 *          - QOSA_WIFI_INVALID_PARAM_ERR if mac_addr is NULL
 *          - Other values on failure
 */
qosa_wifi_err_e qosa_wifi_set_mac(const qosa_wifi_mac_addr_t mac_addr);

#ifdef __cplusplus
}
#endif

#endif /* __QOSA_WIFI_H__ */
