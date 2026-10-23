/*****************************************************************/ /**
* @file mpc_usbnet.h
* @brief USB network operation interface for modem port layer
* @author Joe.tu@quectel.com
* @date 2024-12-24
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-12-24 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __MPC_USBNET__H__
#define __MPC_USBNET__H__

#include "mpc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_usbnet.h"

/**
 * @brief Parameter type of MPC_EVENT_USBNET_STATUS_IND event
 * @note status: 0: idle 1:connected
 *
 * +-----------------------------+--------+--------+--------+
 * | ind_id                      | param1 | param2 | param3 |
 * +-----------------------------+--------+--------+--------+
 * | MPC_EVENT_USBNET_STATUS_IND | status | null   | null   |
 * +-----------------------------+--------+--------+--------+
 *
 */

/** Error code conversion, add corresponding function when adapting to platform, 0 means success */
int err_usbnet_mpc2ms(int err);

/**
 * @brief Set usbnet mac address
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] pmac
 *          - MAC address pointer
 * @return int
 */
int mpc_usbnet_set_usbnetmac(qosa_uint8_t simid, qosa_uint8_t *pmac);

/**
 * @brief Get usbnet mac address configuration
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] pmac
 *          - MAC address pointer, for example, if obtained content is "0x2289846a96ab", MAC address is "22:89:84:6a:96:ab"
 * @return int
 */
int mpc_usbnet_get_usbnetmac(qosa_uint8_t simid, qosa_uint8_t *pmac);

/**
 * @brief Set nat mode
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] nat_mode
 *          - NAT mode enable, 0 disable, 1 enable
 * @param[in] cid
 *          - CID that enables NAT mode
 * @return int
 */
int mpc_usbnet_set_nat_mode(qosa_uint8_t simid, qosa_bool_t nat_mode, qosa_uint32_t cid);

/**
 * @brief Get nat mode configuration
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] nat_mode
 *          - NAT mode enable, 0 disable, 1 enable
 * @param[out] cid
 *          - CID that enables NAT mode
 * @return int
 */
int mpc_usbnet_get_nat_mode(qosa_uint8_t simid, qosa_bool_t *nat_mode, qosa_uint32_t *cid);

/**
 * @brief Set USBNET application protocol, such as ECM, MBIM, RNDIS, NCM
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] type
 *          - Protocol type, 1: ECM, 2: MBIM, 3: RNDIS, 5: NCM
 * @return int
 */
int mpc_usbnet_set_type(qosa_uint8_t simid, qosa_usbnet_type_e type);

/**
 * @brief Get USBNET application protocol
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[out] type
 *          - Protocol type, 1: ECM, 2: MBIM, 3: RNDIS, 5: NCM
 * @return int
 */
int mpc_usbnet_get_type(qosa_uint8_t simid, qosa_usbnet_type_e *type);

/**
 * @brief Get usbnet connection status
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] status
 *          - Connection status, 0: not connected, 1: connected
 * @return int
 */
int mpc_usbnet_get_status(qosa_uint8_t simid, qosa_usbnet_status_e *status);

/**
 * @brief Get USBNET configuration information
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] config
 *          - Configuration information
 * @return int
 */
int mpc_usbnet_get_config(qosa_uint8_t simid, qosa_usbnet_config_t *config);

/**
 * @brief Set USBNET configuration information
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] config
 *          - Configuration information
 * @return int
 */
int mpc_usbnet_set_config(qosa_uint8_t simid, qosa_usbnet_config_t *config);

/**
 * @brief Set usbnet ip, gateway, mask
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] config
 *          - Configuration information
 * @return int
 */
int mpc_usbnet_set_subnet(qosa_uint8_t simid, qosa_usbnet_subnet_config_t sconfig);

/**
 * @brief Set usbnet ip, gateway, mask
 *
 * @param[in] simid
 *          - sim id
 * @param[in] config
 *          - qosa_usbnet_subnet_config_t
 * @return int
 */
int mpc_usbnet_get_subnet(qosa_uint8_t simid, qosa_usbnet_subnet_config_t *config);

/**
 * @brief Execute USBNET operation
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] cb
 *          - Callback function
 * @return int
 */
int mpc_usbnet_action(qosa_uint8_t simid, mpc_cb_t cb);

/**
 * @brief Get global configuration of QDMZ function
 *
 * @param[in]  qosa_uint8_t ip_ver
 *          - IP version identifier (4 for IPv4, 6 for IPv6)
 *
 * @param[out] qosa_usbnet_qdmz_cfg_t * global_cfg
 *          - Pointer to the structure for storing QDMZ global configuration (output parameter)
 *
 * @return int
 */
int mpc_datacall_get_qdmz(qosa_uint8_t ip_ver, qosa_usbnet_qdmz_cfg_t* cfg);

/**
 * @brief Set global configuration of QDMZ function
 *
 * @param[in] qosa_qdmz_global_cfg_t global_cfg
 *          - Input parameter, the QDMZ global configuration structure to be set
 *
 * @return int
 */
int mpc_datacall_set_qdmz(qosa_qdmz_global_cfg_t cfg);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __MPC_USBNET__H__ */
