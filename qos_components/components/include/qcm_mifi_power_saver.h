/*****************************************************************/ /**
 * @file qcm_mifi_power_saver.h
 * @brief MiFi power-saver configuration interface
 * @author joe.tu@quectel.com
 * @date 2026-07-29
 *
 * @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description"
 * <tr><td>2026-07-29 <td>1.0 <td>joe.tu <td> Init
 * </table>
 **********************************************************************/

#ifndef __QCM_MIFI_POWER_SAVER_H__
#define __QCM_MIFI_POWER_SAVER_H__

#include "qosa_def.h"
#include "qcm_mifi_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum qcm_mifi_power_saver_mode_e
 * @brief MiFi automatic power-saving action selected after the device remains idle.
 */
typedef enum
{
    QCM_MIFI_POWER_SAVER_MODE_SUPER = 0,    /*!< Power off after shutdown_time_min of continuous idle time. */
    QCM_MIFI_POWER_SAVER_MODE_STANDARD = 1, /*!< Enter recoverable low power after sleep_time_min of continuous idle time. */
    QCM_MIFI_POWER_SAVER_MODE_MAX            /*!< Invalid mode sentinel; not a configurable mode. */
} qcm_mifi_power_saver_mode_e;

#define QCM_MIFI_POWER_SAVER_DEFAULT_MODE         QCM_MIFI_POWER_SAVER_MODE_STANDARD
#define QCM_MIFI_POWER_SAVER_DEFAULT_SHUTDOWN_MIN 0U
#define QCM_MIFI_POWER_SAVER_DEFAULT_SLEEP_MIN    30U
#define QCM_MIFI_POWER_SAVER_TIME_MAX_MIN         1440U

/**
 * @struct qcm_mifi_power_saver_cfg_t
 * @brief MiFi automatic power-saving policy.
 *
 * The two time values are idle wait thresholds before an action, not the
 * duration of shutdown or sleep. Only the threshold selected by mode is used.
 */
typedef struct
{
    qosa_uint8_t  mode;              /*!< Power-saving action; see qcm_mifi_power_saver_mode_e. */
    qosa_uint16_t shutdown_time_min; /*!< Idle minutes before power-off in SUPER mode; 0 disables automatic power-off. */
    qosa_uint16_t sleep_time_min;    /*!< Idle minutes before standard low power in STANDARD mode; 0 disables automatic sleep. */
} qcm_mifi_power_saver_cfg_t;

/**
 * @brief Get the configured continuous-idle timeout in minutes.
 *
 * SUPER mode selects shutdown_time_min; STANDARD mode selects sleep_time_min.
 * cfg_ptr must be a non-NULL pointer without side effects because the macro may
 * evaluate it more than once.
 */
#define QCM_MIFI_POWER_SAVER_GET_IDLE_TIME_MIN(cfg_ptr)                 \
    (((cfg_ptr)->mode == QCM_MIFI_POWER_SAVER_MODE_SUPER)               \
        ? (cfg_ptr)->shutdown_time_min : (cfg_ptr)->sleep_time_min)

/**
 * @brief Check whether automatic power saving is enabled for the current mode.
 *
 * A non-zero mode-specific idle timeout enables the automatic action. This
 * indicates policy enablement, not whether the device is currently asleep.
 */
#define QCM_MIFI_POWER_SAVER_IS_ENABLED(cfg_ptr) \
    (QCM_MIFI_POWER_SAVER_GET_IDLE_TIME_MIN(cfg_ptr) != 0U)

/**
 * @brief Get the persisted MiFi power-saver configuration.
 *
 * @param[out] qcm_mifi_power_saver_cfg_t *cfg
 *          - Receives the complete power-saver policy.
 *
 * @return qcm_mifi_err_e
 *          - QCM_MIFI_ERR_OK on success; an error code otherwise.
 */
qcm_mifi_err_e qcm_mifi_power_saver_get_cfg(qcm_mifi_power_saver_cfg_t *cfg);

/**
 * @brief Validate and persist the complete MiFi power-saver configuration.
 *
 * @param[in] const qcm_mifi_power_saver_cfg_t *cfg
 *          - Complete policy to store; each time value must not exceed
 *            QCM_MIFI_POWER_SAVER_TIME_MAX_MIN.
 *
 * @return qcm_mifi_err_e
 *          - QCM_MIFI_ERR_OK on success; an error code otherwise.
 */
qcm_mifi_err_e qcm_mifi_power_saver_set_cfg(const qcm_mifi_power_saver_cfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* __QCM_MIFI_POWER_SAVER_H__ */
