/*****************************************************************/ /**
* @file qosa_ftm.h
* @brief Factory Test Mode interface
* @author joe.tu@quectel.com
* @date 2025-09-03
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-09-03 <td>1.0 <td>joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_FTM_H__
#define __QOSA_FTM_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"

/**
 * @enum qosa_ftm_err_e
 * @brief Error codes for rftest function component
 */
typedef enum
{
    QOSA_FTM_SUCCESS = 0,                                  /*!< Execution successful */
    QOSA_FTM_ERR_GENERAL = (QOSA_COMPONENT_FTM << 16) | 1, /*!< Execution failed */
    QOSA_FTM_ERR_WAIT_RESULT, /*!< Asynchronous operation, actual execution result needs to be returned through callback function */
} qosa_ftm_err_e;

/**
 * @enum qosa_ftm_event_e
 * @brief FTM events
 *
 */
typedef enum
{
    QOSA_FTM_EVENT_SET_MODE_RESULT, /*!< Set mode result */
    QOSA_FTM_EVENT_TX_RESULT,       /*!< Transmission result */
    QOSA_FTM_EVENT_RX_RESULT,       /*!< Reception result */
} qosa_ftm_event_e;

/**
 * @brief FTM callback function
 * @param event FTM event type
 * @param result Event result data pointer
 * @param user_data User-defined data pointer
 */
typedef void (*qosa_ftm_callback)(qosa_ftm_event_e event, void *result, void *user_data);

/**
 * @struct qosa_ftm_tx_config_t
 * @brief FTM transmission test configuration
 *
 */
typedef struct
{
    int          band;         /*!< Network type: 1=GSM, 2=WCDMA, 3=LTE */
    char         band_str[20]; /*!< Band string, such as "GSM900", "LTE BAND3" */
    int          tx_channel;   /*!< Uplink channel, actually frequency point number (arfcn) */
    qosa_bool_t  enable;       /*!< QOSA_TRUE=on, QOSA_FALSE=off */
    int          tx_power;     /*!< Transmission power value, unit determined by platform */
    qosa_uint8_t mode;         /*!< Transmission mode, optional, value is 0xff when not configured */
    qosa_uint8_t lte_bw;       /*!< LTE bandwidth 0~5, optional, LTE only, value is 0xff when not configured */
    qosa_uint8_t type;         /*!< Modulation type, optional, value is 0xff when not configured */
} qosa_ftm_tx_config_t;

/**
 * @struct qosa_ftm_rx_config_t
 * @brief FTM reception test configuration
 *
 */
typedef struct
{
    int          band;         /*!< Network type: 1=GSM, 2=WCDMA, 3=LTE */
    char         band_str[20]; /*!< Band string, such as "GSM900", "LTE BAND3" */
    int          rx_channel;   /*!< Downlink channel, actually frequency point number (arfcn) */
    qosa_bool_t  enable;       /*!< QOSA_TRUE=on, QOSA_FALSE=off */
    int          rx_power;     /*!< Expected reception power (dBm) */
    qosa_uint8_t mode;         /*!< Reception mode, optional, value is 0xff when not configured */
    qosa_uint8_t lte_bw;       /*!< LTE bandwidth 0~5, optional, LTE only, value is 0xff when not configured */
    qosa_uint8_t type;         /*!< Modulation type, optional, value is 0xff when not configured */
    int          rx_rssi_pri;  /*!< Primary set received signal strength */
    int          rx_rssi_sec;  /*!< Diversity set received signal strength */
} qosa_ftm_rx_config_t;

/**
 * @struct qosa_ftm_general_result_t
 * @brief FTM general result
 *
 */
typedef struct
{
    qosa_ftm_err_e err; /*!< Execution result */
} qosa_ftm_general_result_t;

/**
 * @struct qosa_ftm_rx_result_t
 * @brief FTM reception test result
 *
 */
typedef struct
{
    qosa_ftm_err_e err; /*!< Execution result */
    int rx_rssi_pri; /*!< Output: Primary set received signal strength, some platforms do not distinguish primary and diversity sets, this represents single antenna received signal strength, unit is not constrained, defined by platform side */
    int rx_rssi_sec; /*!< Output: Diversity set received signal strength, optional, invalid on some platforms that do not distinguish primary and diversity sets */
} qosa_ftm_rx_result_t;

typedef qosa_ftm_general_result_t qosa_ftm_set_mode_result_t;

/**
 * @brief Register FTM callback function
 *
 * This function is used to register a FTM (Factory Test Mode) callback function and bind user context data.
 * Only one callback is allowed to be registered in the system, if a callback is already registered, an error is returned.
 *
 * @param[in] callback
 *          - Callback function pointer, type is qosa_ftm_callback
 * @param[in] user_data
 *          - User-defined data pointer, which will be passed as a parameter during callback
 * @return qosa_ftm_err_e
 *       - If operation is successful, return QOSA_FTM_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_ftm_err_e qosa_ftm_register_callback(qosa_ftm_callback callback, void *user_data);

/**
 * @brief Unregister FTM callback function
 *
 * @param[in] callback
 *          - Callback function pointer, type is qosa_ftm_callback
 * @param[in] user_data
 *          - User-defined data pointer, used to match the context data during registration
 * @return qosa_ftm_err_e
 *       - If operation is successful, return QOSA_FTM_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_ftm_err_e qosa_ftm_unregister_callback(qosa_ftm_callback callback, void *user_data);

/**
 * @brief Configure whether to enter FTM mode
 *
 * @param[in] mode
 *          - 0: Exit FTM mode
 *          - 1: Enter FTM mode
 * @return qosa_ftm_err_e
 *       - If operation is successful, return QOSA_FTM_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_ftm_err_e qosa_ftm_set_mode(qosa_uint32_t mode);

/**
 * @brief Get current FTM mode
 *
 * @param[out] mode
 *          - 0: Exit FTM mode
 *          - 1: Enter FTM mode
 * @return qosa_ftm_err_e
 *       - If operation is successful, return QOSA_FTM_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_ftm_err_e qosa_ftm_get_mode(qosa_uint32_t *mode);

/**
 * @brief Configure FTM transmission test parameters
 *
 * @param[in] config
 *          - Transmission configuration parameter pointer
 * @return qosa_ftm_err_e
 *       - If operation is successful, return QOSA_FTM_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_ftm_err_e qosa_ftm_tx_set(qosa_ftm_tx_config_t *config);

/**
 * @brief Configure FTM reception parameters
 *
 * @param[in] config
 *          - Reception configuration parameter pointer
 * @return qosa_ftm_err_e
 *       - If operation is successful, return QOSA_FTM_SUCCESS
 *       - Return corresponding error code if failed
 */
qosa_ftm_err_e qosa_ftm_rx_set(qosa_ftm_rx_config_t *config);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_FTM_H__ */
