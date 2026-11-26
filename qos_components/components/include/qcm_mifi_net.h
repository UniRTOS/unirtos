/*****************************************************************/ /**
 * @file qcm_mifi_net.h
* @brief Network operation interface
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

#ifndef __QCM_MIFI_NET_H__
#define __QCM_MIFI_NET_H__

#include "qosa_def.h"
#include "qosa_modem.h"
#include "qosa_network_config.h"
#include "qosa_network_param.h"

#define QCM_ERRCODE_MIFI_BASE (QOSA_COMPONENT_MIFI << 16)

// clang-format off
/**
 * @enum qosa_nw_err_e
 * @brief network error codes
 *
 */
typedef enum
{
    QCM_MIFI_ERR_OK = 0,

    QCM_MIFI_ERR_OPERATION_NOT_ALLOWED   = 3 | QCM_ERRCODE_MIFI_BASE, /*!< operation not allowed */
    QCM_MIFI_ERR_OPERATION_NOT_SUPPORTED = 4 | QCM_ERRCODE_MIFI_BASE, /*!< operation not supported */
    QCM_MIFI_ERR_MEMORY_FULL             = 20 | QCM_ERRCODE_MIFI_BASE, /*!< memory full */
    QCM_MIFI_ERR_MEMORY_FAILURE          = 23 | QCM_ERRCODE_MIFI_BASE, /*!< memory failure */
    QCM_MIFI_ERR_INVALID_PARAM           = 53 | QCM_ERRCODE_MIFI_BASE, /*!< invalid parameter */
    QCM_MIFI_ERR_EXECUTE                 = 1 | (QCM_ERRCODE_MIFI_BASE + QOSA_AT_ERR_OFS),
} qcm_mifi_err_e;
// clang-format on

/**
 * @brief MiFi network connection state
 */
typedef enum
{
    QCM_MIFI_NET_SIM_NOT_READY,  ///< SIM card not ready
    QCM_MIFI_NET_STOP,           ///< SIM ready, but network not connected
    QCM_MIFI_NET_CONNECTING,     ///< Network connecting
    QCM_MIFI_NET_BAD,            ///< Unexpected network connection
    QCM_MIFI_NET_OK,             ///< Network registered and data call established
    QCM_MIFI_NET_MAX,
} qcm_mifi_net_state_e;

/**
 * @brief           Initialize MiFi network manager task and context
 */
void qcm_mifi_net_manage_init(void);

/**
 * @brief           Set roaming allowance configuration
 * @param[in]       simid       SIM card slot ID
 * @param[in]       enable      QOSA_TRUE to allow roaming, QOSA_FALSE to disallow
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_net_set_roam_cfg(qosa_uint8_t simid, qosa_bool_t enable);

/**
 * @brief           Get roaming allowance configuration
 * @param[in]       simid       SIM card slot ID
 * @param[out]      enable      QOSA_TRUE if roaming is allowed, QOSA_FALSE otherwise
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_net_get_roam_cfg(qosa_uint8_t simid, qosa_bool_t *enable);

/**
 * @brief           Initiate network data call connection
 * @param[in]       simid       SIM card slot ID
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_net_connect(qosa_uint8_t simid);

/**
 * @brief           Disconnect network data call
 * @param[in]       simid       SIM card slot ID
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_net_disconnect(qosa_uint8_t simid);

/**
 * @brief           Get current network connection status
 * @param[in]       simid       SIM card slot ID
 * @param[out]      status      Current network state, see qcm_mifi_net_state_e
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_net_get_status(qosa_uint8_t simid, qcm_mifi_net_state_e *status);

/**
 * @brief           Reconnect the MiFi data call after APN/profile changes
 * @param[in]       simid       SIM card slot ID
 * @return          QCM_MIFI_ERR_OK on success, error code otherwise
 */
qcm_mifi_err_e qcm_mifi_net_modify_apn(qosa_uint8_t simid);

/**
 * @brief Check whether the current MIFI cellular data path is healthy.
 *
 * @param[in] simid
 *          - SIM card slot ID.
 * @return qosa_bool_t
 *          - QOSA_TRUE only when the network health state is QCM_MIFI_NET_OK.
 */
qosa_bool_t qcm_mifi_net_is_healthy(qosa_uint8_t simid);

/**
 * @brief Invalidate network health after the configured SIM becomes unavailable.
 */
void qcm_mifi_net_notify_sim_not_ready(qosa_uint8_t simid);

#endif /* __QCM_MIFI_NET_H__ */
