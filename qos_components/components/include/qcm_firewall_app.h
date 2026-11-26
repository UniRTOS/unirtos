/*****************************************************************/ /**
* @file qcm_firewall_app.h
* @brief  Public API definitions, configuration enumeration, and error codes for the firewall module.
* @author alan.tuo@quectel.com
* @date 2026-05-13
*
* @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-05-13 <td>1.0 <td>alan.tuo <td> Init
* </table>
*/
/*****************************************************************/

#ifndef __QCM_FIREWALL_APP_H__
#define __QCM_FIREWALL_APP_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_sio.h"

// ========== Valid Parameter Ranges ==========
#define QCM_FIREWALL_ENABLE_MIN   0
#define QCM_FIREWALL_ENABLE_MAX   1
//time Ranges
#define QCM_CLEAN_TIME_MIN        1
#define QCM_CLEAN_TIME_MAX        65535

#define QCM_ACCESS_PERCENTAGE_MIN 1
#define QCM_ACCESS_PERCENTAGE_MAX 65535

#define QCM_INPUT_SCALE_MIN       1
#define QCM_INPUT_SCALE_MAX       65535

#define QCM_INPUT_INTERVAL_MIN    1
#define QCM_INPUT_INTERVAL_MAX    65535

#define QCM_SAVE_LOG_TIME_MIN     1
#define QCM_SAVE_LOG_TIME_MAX     65535

#define QCM_SAVE_LOG_SIZE_MIN     (10 * 1024)
#define QCM_SAVE_LOG_SIZE_MAX     (200 * 1024)

#define QCM_DEBUG_LOG_MIN         0
#define QCM_DEBUG_LOG_MAX         1

#define QCM_FIREWALL_ERR_START    590

typedef enum
{
    QCM_FIREWALL_ENABLE = 0,          /**< Firewall master switch: 0=disabled 1=enabled */
    QCM_TCP_BLACKLIST_CLEAN_TIME = 1, /**< TCP blacklist auto-clean interval (seconds) */
    QCM_TCP_RECORD_LIST_CLEAN_TIME,   /**< TCP connection record list auto-clean interval (seconds) */
    QCM_ICMP_BLACKLIST_CLEAN_TIME,    /**< ICMP blacklist auto-clean interval (seconds) */
    QCM_ICMP_RECORD_LIST_CLEAN_TIME,  /**< ICMP record list auto-clean interval (seconds) */
    QCM_TCP_ACCESS_PERCENTAGE,        /**< TCP traffic access threshold percentage (for anomaly detection) */
    QCM_TCP_INPUT_SCALE = 6,          /**< TCP input traffic scaling factor (rate calculation/throttling) */
    QCM_ICMP_INPUT_SCALE,             /**< ICMP input traffic scaling factor (rate calculation/throttling) */
    QCM_ICMP_INPUT_INTERVAL,          /**< ICMP input detection interval (milliseconds) */
    QCM_SAVE_FILE_LOG_TIME,           /**< Log file time-based rotation interval (seconds) */
    QCM_SAVE_FILE_LOG_SIZE,           /**< Log file size-based rotation threshold (bytes) */
    QCM_FIREWALL_PATCH_DATA,          /**< Patch data configuration (explicit number, skipped original 11=deleted FILE_LOG_FULL_CB) */
    QCM_DEBUG_LOG,                    /**< Debug log enable/disable (0=disable, 1=enable) */
    QCM_FIREWALL_CONFIG_MAX           /**< Total config item count (array boundary check) */
} qcm_firewall_config_type;

/**
 * @enum qstd_firewall_resp_cmd_e
 * @brief Firewall service execution result reporting message event
 */
typedef enum
{
    QCM_FIREWALL_QFCFG_RSP_MSG = 0, /*!< Firewall configuration command response */
    QCM_FIREWALL_FROM_SIO_DATA_CMD, /*!< Data read from SIO */
    QCM_FIREWALL_ACK_BACK_MSG,      /*!< Acknowledgment back message */
    QCM_FIREWALL_LOG_FILE_FULL,     /*!< Log file full notification (others exist alongside this, can be removed) */
    QCM_FIREWALL_RSP_MAX
} qstd_firewall_resp_cmd_e;

/**
 * @enum qosa_tcpip_err_e
 * @brief TCP service application error codes
 */
/** Patch data configuration (for external set/get_config use) */
typedef struct
{
    qosa_uint32_t data_len; /**< Patch data length */
    qosa_int8_t  *data;     /**< Patch data pointer */
} qcm_firewall_patch_data_t;

typedef enum
{
    QCM_FIREWALL_NEGATIVE = -1,
    QCM_FIREWALL_OK = 0,
    QCM_FIREWALL_ERR_FAILURE = QCM_FIREWALL_ERR_START,
    QCM_FIREWALL_ERR_INVALID_PARAM,
    QCM_FIREWALL_ERR_OUT_OF_MEMORY,
} qosa_firewall_err_e;

/* Outer layer call layer event notification registration callback function */
typedef void (*atcmd_event_response)(qstd_firewall_resp_cmd_e cmd_id, void *msg);

/**
 * @brief   Firewall module initialization entry function.
 *
 *          Initializes the global management structure g_firewall_mgr, creates the
 *          control Task (QFIRET) and write Task (QFIRWR), and initializes all
 *          subsystems including default config values, semaphores, mutexes, and timers.
 *
 * @note    This function should be called during system startup. The lwIP protocol
 *          stack and AT framework must be ready before calling.
 */
void qstd_firewall_app_init(void);

/**
 * @brief   Check whether initialization was successful.
 *
 * @return  Non-zero if initialized successfully, 0 if not initialized.
 */
qosa_int32_t qstd_firewall_app_check(void);

/**
 * @brief   Register an event notification callback function.
 *          Currently, only notifications for when the log storage is full can be configured.
 *
 * @param[in] cb  Callback function for event reporting.
 */
void qstd_firewall_event_report(atcmd_event_response cb);

/**
 * @brief   Set a firewall configuration item.
 *
 * @param[in]  type      Configuration item type (qcm_firewall_config_type)
 * @param[in]  val       Configuration value (numeric value passed directly;
 *                       for QCM_FIREWALL_PATCH_DATA, pass intptr_t-converted
 *                       qcm_firewall_patch_data_t*)
 * @param[in]  val_len   Length of the configuration value
 * @param[in]  save_to_nv Whether to save to non-volatile storage
 *
 * @return qosa_firewall_err_e
 * - Returns QCM_FIREWALL_OK on success
 * - Returns other values on failure
 */
qosa_firewall_err_e qstd_firewall_set_config(qcm_firewall_config_type type, void *val, qosa_int32_t val_len, qosa_int32_t save_to_nv);

/**
 * @brief   Get a firewall configuration item.
 *
 * @param[in]  type      Configuration item type (qcm_firewall_config_type)
 * @param[out] val       Output buffer for the configuration value
 * @param[out] val_len   Returns the actual length of the configuration value;
 *                       when type is QCM_FIREWALL_PATCH_DATA, val should point
 *                       to qcm_firewall_patch_data_t, with data/data_len used as
 *                       output buffer and capacity
 *
 * @return qosa_firewall_err_e
 * - Returns QCM_FIREWALL_OK on success
 * - Returns other values on failure
 */
qosa_firewall_err_e qstd_firewall_get_config(qcm_firewall_config_type type, void *val, qosa_int32_t *val_len);

#endif /* __QCM_FIREWALL_APP_H__ */
