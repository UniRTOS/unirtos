/*****************************************************************/ /**
* @file qosa_sys_callbacks.h
* @brief System callback interface definitions
* @author harry.li@quectel.com
* @date 2025-05-26
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-05-26 <td>1.0 <td>harry.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QOSA_SYS_CALLBACKS_H__
#define __QOSA_SYS_CALLBACKS_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#include "qosa_sys.h"
#include "qosa_at_sio.h"
#include "qosa_at_param.h"
#include "qosa_urc.h"

/**
 * @brief Used for system side to call qcfg function pointer
 *
 */
void qosa_run_at_qcfg_func(struct _qosa_at_cmd *cmd);

/**
 * @brief Used for system side to call urc refresh function
 *
 */
int qosa_run_qcm_urc_flush_cache_func(void);

/**
 * @brief Used for system platform side to call urc wait for send completion function pointer
 *
 */
void qosa_run_qcm_urc_send_wait_complete_func(qosa_uint16_t delay_ms);

/**
 * @brief Used for system platform side to call urc send function
 *
 */
qosa_urc_errcode_e qosa_run_qcm_urc_send_report_func(
    qosa_at_dev_type_e    dev_port,
    char                 *data,
    qosa_uint32_t         len,
    qosa_urc_out_method_e opt_mthod,
    qosa_ri_category_e    urc_cate,
    qosa_urc_option_t     option
);

/**
 * @brief Used for system platform side to call urc get configuration
 *
 */
void qosa_run_qcm_urc_config_get_opt_func(qosa_urc_cfg_opt_e urc_cfg_opt, void *option);

/**
 * @brief It is used to report the results of virtual AT execution, and the virtual AT instructions correspond to the URC results
 *
 * @param data  The virtual AT executes the response, along with the corresponding URC notification
 * @param size  Response length
 *
 */
void qosa_virt_at_result_func(const char *data, qosa_uint32_t size);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#endif /* __QOSA_SYS_CALLBACKS_H__ */
