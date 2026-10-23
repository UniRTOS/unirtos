/*****************************************************************/ /**
* @file qosa_usbnet.h
* @brief USB network operation interface
* @author Joe.tu@quectel.com
* @date 2024-03-14
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-03-14 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_USBNET_H__
#define __QOSA_USBNET_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_modem.h"
#include "qosa_ip_addr.h"
#include "qosa_usbnet_config.h"

/**
 * USBNET callback function type definition
 * @param[in] ctx User context pointer
 * @param[in] argv Callback parameter pointer
 */
typedef void (*usbnet_callback_t)(void *ctx, void *argv);

/** USBNET component error code base value, used to generate USBNET-related error codes */
#define QOSA_ERRCODE_USBNET_BASE          (QOSA_COMPONENT_USBNET << 16)
/** USBNET subnet configuration parameter - IP address configuration bit */
#define QOSA_USBNET_SUBNET_CONFIG_IPADDR  0x01
/** USBNET subnet configuration parameter - Gateway configuration bit */
#define QOSA_USBNET_SUBNET_CONFIG_GATEWAY 0x02
/** USBNET subnet configuration parameter - Subnet mask configuration bit */
#define QOSA_USBNET_SUBNET_CONFIG_MASK    0x04

// clang-format off
/**
 * @enum qosa_usbnet_err_e
 * @brief Error code of USBNET module.
 *
 */
typedef enum usbnet_err
{
    QOSA_USBNET_ERR_OK = 0,

    QOSA_USBNET_ERR_OPERATION_NOT_ALLOWED   = 3 | QOSA_ERRCODE_USBNET_BASE, /*!< operation not allowed */
    QOSA_USBNET_ERR_OPERATION_NOT_SUPPORTED = 4 | QOSA_ERRCODE_USBNET_BASE, /*!< operation not supported */
    QOSA_USBNET_ERR_MEMORY_FULL             = 20 | QOSA_ERRCODE_USBNET_BASE, /*!< memory full */
    QOSA_USBNET_ERR_MEMORY_FAILURE          = 23 | QOSA_ERRCODE_USBNET_BASE, /*!< memory failure */
    QOSA_USBNET_ERR_INVALID_PARAM           = 53 | QOSA_ERRCODE_USBNET_BASE, /*!< invalid parameter */
    QOSA_USBNET_ERR_EXECUTE                 = 1 | (QOSA_ERRCODE_USBNET_BASE + QOSA_AT_ERR_OFS),
    QOSA_USBNET_ERR_BUSY                    = 2 | (QOSA_ERRCODE_USBNET_BASE + QOSA_AT_ERR_OFS),
    QOSA_USBNET_ERR_ALREADY_CONNECTED       = 3 | (QOSA_ERRCODE_USBNET_BASE + QOSA_AT_ERR_OFS),
} qosa_usbnet_err_e;
// clang-format on

/**
 * @enum qosa_usbnet_type_e
 * @brief USBNET connection protocol type
 *
 */
typedef enum
{
    QOSA_USBNET_TYPE_ECM = 1,   /*!< ECM */
    QOSA_USBNET_TYPE_MBIM = 2,  /*!< MBIM */
    QOSA_USBNET_TYPE_RNDIS = 3, /*!< RNDIS */
    QOSA_USBNET_TYPE_NCM = 5,   /*!< NCM */
} qosa_usbnet_type_e;

/**
 * @enum qosa_usbnet_method_e
 * @brief Connection method
 *
 */
typedef enum
{
    QOSA_USBNET_METHOD_DISABLE = 0,  /*!< Disable */
    QOSA_USBNET_METHOD_ONE_SHOT = 1, /*!< One-shot connection */
    QOSA_USBNET_METHOD_AUTO_CONNECT
        = 3, /*!< Auto-reconnect, USBNET will automatically reconnect when PDP disconnects/power on, etc., reconnection interval gradually increases */
} qosa_usbnet_method_e;

/**
 * @enum qosa_usbnet_status_e
 * @brief USBNET connection status
 *
 */
typedef enum
{
    QOSA_USBNET_STATUS_IDLE = 0,      /*!< Idle state */
    QOSA_USBNET_STATUS_CONNECTED = 1, /*!< Connected */
} qosa_usbnet_status_e;

/*-------------------------------------- event --------------------------------------*/

/**
 * @struct qosa_usbnet_status_event_t
 * @brief Data content carried by event QOSA_EVENT_USBNET_STATUS report
 *
 */
typedef struct
{
    qosa_uint8_t simid;  /*!< SIM card ID */
    qosa_uint8_t pdpid;  /*!< PDP ID */
    qosa_uint8_t status; /*!< USBNET status, 0: idle, 1: connected */
} qosa_usbnet_status_event_t;

/*-------------------------------------- pa --------------------------------------*/

/**
 * @struct qosa_usbnet_config_t
 * @brief USBNET configuration parameters
 *
 */
typedef struct
{
    qosa_uint8_t         simid;  /*!< SIM card ID */
    qosa_uint8_t         pdpid;  /*!< PDP ID */
    qosa_usbnet_method_e method; /*!< Connection method, 0: disable, 1: one-shot, 3: auto-connect */
} qosa_usbnet_config_t;

/**
 * @struct qosa_usbnet_netmask_config_t
 * @brief USBNET subnet mask configuration parameters
 *
 */
typedef struct
{
    qosa_uint8_t simid;   /*!< SIM card ID */
    qosa_bool_t  enable;  /*!< Whether to enable subnet mask configuration */
    char         mask[4]; /*!< Subnet mask, such as {255, 255, 255, 0}*/
} qosa_usbnet_netmask_config_t;

/**
 * @struct qosa_usbnet_subnet_config_t
 * @brief USBNET subnet configuration parameters
 *
 */
typedef struct
{
    qosa_uint8_t flags : 4;  /*!< Bit field indicating which parameters are valid.
                             - In set operations, it specifies which fields should be configured.
                             - In get operations, it indicates which fields contain valid data.
                             Use QOSA_USBNET_SUBNET_CONFIG_* macros for bitwise combination. */
    char         ipAddr[4];  /*!< IP address, such as {192, 168, 22, 10} */
    char         gateway[4]; /*!< Gateway, such as {192, 168, 22, 1} */
    char         mask[4];    /*!< Subnet mask, such as {255, 255, 255, 0} */
} qosa_usbnet_subnet_config_t;

/**
 * @struct qosa_usbnet_qdmz_cfg_t
 * @brief Single-instance configuration structure for DMZ function
 *
 */
typedef struct
{
    qosa_uint8_t   status;          /*!< Enable status of DMZ function (0=Disabled, 1=Enabled) */
    qosa_uint8_t   ip_ver;          /*!< IP type identifier corresponding to DMZ (4=IPv4, 6=IPv6) */
    qosa_ip_addr_t ip_addr;         /*!< IP address information of the DMZ forwarding target */
    qosa_ip_addr_t dns_server;      /*!< DNS server address corresponding to DMZ */
    qosa_ip_addr_t virt_dns_server; /*!< Virtual DNS server address corresponding to DMZ */
} qosa_usbnet_qdmz_cfg_t;

/**
 * @struct qosa_qdmz_global_cfg_t
 * @brief Global DMZ configuration container structure
 *
 */
typedef struct
{
    qosa_usbnet_qdmz_cfg_t ipv4; /*!< Dedicated DMZ configuration instance for IPv4 */
    qosa_usbnet_qdmz_cfg_t ipv6; /*!< Dedicated DMZ configuration instance for IPv6 */
} qosa_qdmz_global_cfg_t;

/*-------------------------------------- cnf --------------------------------------*/

/* All cnf results need to ensure the first two parameters of the structure are simid and err_code in order, which is OSA_MODEM_COMMON_CNF_HDR, user layer does not need to release parameter content, after executing the callback, internally it will be released directly */
typedef qosa_modem_general_cnf_t qosa_usbnet_general_cnf_t;

/*-------------------------------------- api --------------------------------------*/
/**
 * @brief Set usbnet mac address
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] pmac
 *          - MAC address pointer, for example, if the input content is "0x2289846a96ab", the MAC address is "22:89:84:6a:96:ab"
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_set_usbnetmac(qosa_uint8_t simid, qosa_uint8_t *pmac);

/**
 * @brief Get usbnet mac address configuration
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] pmac
 *          - MAC address pointer, for example, if the obtained content is "0x2289846a96ab", the MAC address is "22:89:84:6a:96:ab"
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_get_usbnetmac(qosa_uint8_t simid, qosa_uint8_t *pmac);

/**
 * @brief Set nat mode
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] nat_mode
 *          - NAT mode enable, QOSA_FALSE: disable, QOSA_TRUE: enable
 * @param[in] cid
 *          - CID that enables NAT mode
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_set_nat_mode(qosa_uint8_t simid, qosa_bool_t nat_mode, qosa_uint32_t cid);

/**
 * @brief Get nat mode configuration
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] nat_mode
 *          - NAT mode enable, QOSA_FALSE: disable, QOSA_TRUE: enable
 * @param[out] cid
 *          - CID that enables NAT mode, could be NULL if not needed
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_get_nat_mode(qosa_uint8_t simid, qosa_bool_t *nat_mode, qosa_uint32_t *cid);

/**
 * @brief Set USBNET application protocol, such as ECM, MBIM, RNDIS, NCM
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] type
 *          - Protocol type, 1: ECM, 2: MBIM, 3: RNDIS, 5: NCM
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_set_type(qosa_uint8_t simid, qosa_usbnet_type_e type);

/**
 * @brief Get USBNET application protocol
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] type
 *          - Protocol type, 1: ECM, 2: MBIM, 3: RNDIS, 5: NCM
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_get_type(qosa_uint8_t simid, qosa_usbnet_type_e *type);

/**
 * @brief Set USBNET connection configuration items, such as simid, pdpid, whether to auto-reconnect, etc.
 *
 * @param[in] simid
 *          - SIM card ID (specified in the config structure, this parameter is reserved for future use)
 * @param[in] config
 *          - Connection configuration item structure pointer
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_set_config(qosa_uint8_t simid, qosa_usbnet_config_t *config);

/**
 * @brief Get USBNET connection configuration items
 *
 * @param[in] simid
 *          - SIM card ID (specified in the config structure, this parameter is reserved for future use)
 * @param[out] config
 *          - Connection configuration item structure pointer
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_get_config(qosa_uint8_t simid, qosa_usbnet_config_t *config);

/**
 * @brief Start USBNET connection
 * @result Result is returned through qosa_usbnet_general_cnf_t
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - Callback function after asynchronous operation is completed
 * @param[in] ctx
 *          - User context pointer of the callback function
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_start(qosa_uint8_t simid, usbnet_callback_t cb, void *ctx);

/**
 * @brief Stop USBNET connection, executing this function will modify method to QOSA_USBNET_METHOD_DISABLE by default
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - Callback function after asynchronous operation is completed
 * @param[in] ctx
 *          - User context pointer of the callback function
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_stop(qosa_uint8_t simid, usbnet_callback_t cb, void *ctx);

/**
 * @brief Get USBNET connection status
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] status
 *          - Connection status, 0: idle, 1: connected
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_get_status(qosa_uint8_t simid, qosa_usbnet_status_e *status);

/**
 * @brief Configure IP, gateway, and mask allocated to the terminal in routing mode
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] config
 *          - Subnet configuration parameter structure
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_set_subnet(qosa_uint8_t simid, qosa_usbnet_subnet_config_t config);

/**
 * @brief Get IP, gateway, and mask allocated to the terminal
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] config
 *          - Subnet configuration parameter structure pointer
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_get_subnet(qosa_uint8_t simid, qosa_usbnet_subnet_config_t *config);

/**
 * @brief Set QDMZ configuration parameters
 *
 * @param[in] qdmz_cfg
 *          - New QDMZ configuration (contains IP address, status, IP version)
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_set_qdmz(qosa_qdmz_global_cfg_t qdmz_cfg);

/**
 * @brief Get QDMZ configuration parameters
 *
 * @param[in] ip_ver
 *          - IP version
 * @param[in] qdmz_cfg
 *          - QDMZ configuration (contains IP address, status, IP version)
 * @return qosa_usbnet_err_e
 *          - Return QOSA_USBNET_ERR_OK on success, return corresponding error code on failure
 */
qosa_usbnet_err_e qosa_usbnet_get_qdmz(qosa_uint8_t ip_ver, qosa_usbnet_qdmz_cfg_t* qdmz_cfg);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_USBNET_H__ */
