/*****************************************************************/ /**
* @file qosa_dev1.h
* @brief Device operation interface
* @author Joe.tu@quectel.com
* @date 2024-05-11
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

#ifndef __QOSA_DEV1_H__
#define __QOSA_DEV1_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sim_config.h"
#include "qosa_modem.h"

typedef void (*dev_callback_ptr)(void *ctx, void *argv);

#define QOSA_ERRCODE_DEV_BASE                   (QOSA_COMPONENT_DEV << 16)

#define QOSA_QCFG_AIRPLANECONTROL_DEFAULT_VALUE 0

// clang-format off
/**
 * @enum qosa_dev1_error_e
 * @brief Device error codes
 *
 */
typedef enum dev_err
{
    QOSA_DEV_ERR_OK = 0,
    QOSA_DEV_ERR_OPERATION_NOT_ALLOWED   = 3 | QOSA_ERRCODE_DEV_BASE, /*!< operation not allowed */
    QOSA_DEV_ERR_OPERATION_NOT_SUPPORTED = 4 | QOSA_ERRCODE_DEV_BASE, /*!< operation not supported */
    QOSA_DEV_ERR_MEMORY_FULL             = 20 | QOSA_ERRCODE_DEV_BASE, /*!< memory full */
    QOSA_DEV_ERR_MEMORY_FAILURE          = 23 | QOSA_ERRCODE_DEV_BASE, /*!< memory failure */
    QOSA_DEV_ERR_INVALID_PARAM           = 53 | QOSA_ERRCODE_DEV_BASE, /*!< invalid parameter */
    QOSA_DEV_ERR_EXECUTE                 = 1 | (QOSA_ERRCODE_DEV_BASE + QOSA_AT_ERR_OFS),
} qosa_dev1_error_e;
// clang-format on

/**
 * @enum qosa_dev_func_e
 * @brief Device cfun level
 *
 */
typedef enum
{
    QOSA_DEV_FUNC_0 = 0, /*!< minimum functionality */
    QOSA_DEV_FUNC_1 = 1, /*!< full functionality */
    QOSA_DEV_FUNC_4 = 4, /*!< disable (turn off) both MT transmit and receive RF circuits */
} qosa_dev_func_e;

/**
 * @enum qosa_dev_airplane_state_e
 * @brief Device cfun level
 *
 */
typedef enum
{
    QOSA_DEV_NOT_IN_AIRPLANE_MODE = 0, /*!< Not in airplane mode */
    QOSA_DEV_IN_AIRPLANE_MODE = 1,     /*!< In airplane mode */
} qosa_dev_airplane_state_e;

/*-------------------------------------- basic struct --------------------------------------*/

/*-------------------------------------- event --------------------------------------*/

/**
 * @struct qosa_dev_airplane_event_t
 * @brief Data content carried by event QOSA_EVENT_DEV_AIRPLANE_CTRL_EVENT reporting
 *
 */
typedef struct
{
    qosa_dev_airplane_state_e status; /*!< Airplane mode status, 0: not in airplane mode, 1: in airplane mode */
} qosa_dev_airplane_event_t;

/*-------------------------------------- pa --------------------------------------*/

/**
 * @struct qosa_dev_airplanecontrol_param_t
 * @brief Airplane control configuration structure
 *
 */
typedef struct
{
    qosa_uint8_t              airplanecontrol_enable; /*!< 0-disable, 1-enable */
    qosa_dev_airplane_state_e status;                 /*!< Airplane mode status */
} qosa_dev_airplanecontrol_param_t;

/*-------------------------------------- cnf --------------------------------------*/

/* All cnf results need to ensure that the first two parameters of the structure are simid, err_code in order, that is OSA_MODEM_COMMON_CNF_HDR, user layer does not need to release parameter content, after executing the callback, it will be directly released internally */

/**
 * @struct qosa_dev_general_cnf_t
 * @brief General result return for dev module asynchronous interface, used to return simple success or failure, no additional result return type
 *
 */
typedef qosa_modem_general_cnf_t qosa_dev_general_cnf_t;

/*-------------------------------------- api --------------------------------------*/

/**
 * @brief Set the cfun status of the module, asynchronous interface, result returned through osa_dev_general_cnf_t
 *
 * @param[in] simid
 *          - SIM card ID
 * @param[in] func
 *          - Support cfun0 cfun1 and cfun4 (qosa_dev_func_e)
 * @param[in] cb
 *          - Callback function pointer
 * @param[in] ctx
 *          - Context pointer for callback function
 * @return qosa_dev1_error_e
 */
qosa_dev1_error_e qosa_dev_set_cfun(qosa_uint8_t simid, qosa_uint8_t func, dev_callback_ptr cb, void *ctx);

/**
 * @brief Get cfun status
 *
 * @param[in] simid
 *          - simid
 * @param[out] func
 *          - Return cfun status (qosa_dev_func_e)
 * @return qosa_dev1_error_e
 */
qosa_dev1_error_e qosa_dev_get_cfun(qosa_uint8_t simid, qosa_uint8_t *func);

/**
 * @brief Disable or enable airplane mode control through W_DISABLE#
 *
 * @param[in] simid
 *          - sim id
 * @param[in] airplane_ctl_enable
 *          - Enable/disable airplane mode control through W_DISABLE#, 0-disable, 1-enable
 * @return qosa_dev1_error_e
 */
qosa_dev1_error_e qosa_dev_set_airplanecontrol_config(qosa_uint8_t simid, qosa_uint8_t airplane_ctl_enable);

/**
 * @brief Query the configuration of airplane mode control through W_DISABLE#
 *
 * @param[in] simid
 *          - sim id
 * @param[out] config
 *          - Configuration of airplane mode control through W_DISABLE#
 * @return qosa_dev1_error_e
 */
qosa_dev1_error_e qosa_dev_get_airplanecontrol_config(qosa_uint8_t simid, qosa_dev_airplanecontrol_param_t *config);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_DEV1_H__ */
