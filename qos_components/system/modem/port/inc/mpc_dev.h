/*****************************************************************/ /**
* @file mpc_network.h
* @brief Device operation interface for modem port layer
* @author Joe.tu@quectel.com
* @date 2024-03-18
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-16 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __MPC_DEV__H__
#define __MPC_DEV__H__

#include "mpc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_dev1.h"
#include "qosa_network.h"
#include "qosa_network_param.h"

/** Error code conversion, add corresponding function when adapting to platform, 0 means success */
int err_dev_mpc2ms(int err);

/**
 * @brief Get cfun status
 *
 * @param[in] simid
 *          - simid
 * @param[out] func (qosa_dev_func_e)
 *          - Return cfun status
 * @return qosa_dev1_error_e
 */
int mpc_dev_get_cfun(qosa_uint8_t simid, qosa_uint8_t *func);

/**
 * @brief Set cfun, asynchronous interface
 * @result
 * +-----------------+------------+--------+--------+
 * | ret             | param1     | param2 | param3 |
 * +-----------------+------------+--------+--------+
 * | MPC_RET_SUCCESS | QOSA_OK     | null   | null   |
 * | MPC_RET_FAIL    | error code | null   | null   |
 * +-----------------+------------+--------+--------+
 * @param[in] simid
 *          -
 * @param[in] func (qosa_dev_func_e) Support 0,1,4
 *          - cfun value
 * @param[in] reset
 *          - Whether to restart, QOSA_TRUE: restart QOSA_FALSE: do not restart
 * @param[in] cb
 *          -
 * @return int
 */
int mpc_dev_set_cfun(qosa_uint8_t simid, qosa_uint8_t func, mpc_cb_t cb);

/**
 * @brief Disable or enable airplane mode control through W_DISABLE#
 *
 * @param[in] simid
 *          - sim id
 * @param[in] airplane_ctl_enable
 *          - Enable/disable airplane mode control through W_DISABLE#, 0-disable, 1-enable
 * @return qosa_dev1_error_e
 */
int mpc_dev_set_airplanecontrol_config(qosa_uint8_t simid, qosa_uint8_t airplane_ctl_enable);
/**
 * @brief Query the configuration of airplane mode control through W_DISABLE#
 *
 * @param[in] simid
 *          - sim id
 * @param[out] config
 *          - Configuration of airplane mode control through W_DISABLE#
 * @return qosa_dev1_error_e
 */
int mpc_dev_get_airplanecontrol_config(qosa_uint8_t simid, qosa_dev_airplanecontrol_param_t *config);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __MPC_DEV__H__ */
