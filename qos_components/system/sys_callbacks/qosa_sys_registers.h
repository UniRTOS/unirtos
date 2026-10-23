/*****************************************************************/ /**
* @file qosa_sys_registers.h
* @brief System registration interface definitions
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
#ifndef __QOSA_SYS_REGISTERS_H__
#define __QOSA_SYS_REGISTERS_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#include "qosa_sys.h"
#include "qosa_at_sio.h"
#include "qosa_at_param.h"
#include "qosa_urc.h"

// at command qcfg
typedef void (*qosa_at_qcfg_func_ptr)(qosa_at_cmd_t *cmd);

// urc refresh
typedef int (*qosa_qcm_urc_flush_cache_func_ptr)(void);

// wait for urc send completion
typedef void (*qosa_qcm_urc_send_wait_complete_ptr)(qosa_uint16_t delay_ms);

// send urc
typedef qosa_urc_errcode_e (*qosa_qcm_urc_send_report_ptr)(
    qosa_at_dev_type_e    dev_port,
    char                 *data,
    qosa_uint32_t         len,
    qosa_urc_out_method_e opt_mthod,
    qosa_ri_category_e    urc_cate,
    qosa_urc_option_t     option
);

// get urc configuration items
typedef qosa_urc_errcode_e (*qosa_qcm_urc_config_get_opt_ptr)(qosa_urc_cfg_opt_e urc_cfg_opt, void *option);

/**
 * @brief Used for APP side to register qcfg function pointer to implement dual qcfg solution
 *
 */
void qosa_regedit_at_qcfg_func(qosa_at_qcfg_func_ptr at_qcfg_func_ptr);

/**
 * @brief Used for APP side to register urc refresh
 *
 */
void qosa_regedit_qcm_urc_flush_cache_func(qosa_qcm_urc_flush_cache_func_ptr urc_flush_cache_func_ptr);

/**
 * @brief Used for APP side to register urc wait for completion
 *
 */
void qosa_regedit_qcm_urc_send_wait_complete_func(qosa_qcm_urc_send_wait_complete_ptr qcm_urc_send_wait_complete_ptr);

/**
 * @brief Used for APP side to register urc send
 *
 */
void qosa_regedit_qcm_urc_send_report_func(qosa_qcm_urc_send_report_ptr urc_send_report_ptr);

/**
 * @brief Used for APP side to register urc get configuration items
 *
 */
void qosa_regedit_qcm_urc_config_get_opt_func(qosa_qcm_urc_config_get_opt_ptr qcm_urc_config_get_opt_ptr);

// The function pointer that reports the execution result of the virtual AT
typedef void (*qosa_qcm_virt_at_result_func_ptr)(const char *data, qosa_uint32_t size);

/**
 * @brief Register a callback function that reports the execution results of the virtual AT
 *
 * @param Incoming callback function pointer
 *
 */
void qosa_regedit_qcm_virt_at_result_func(qosa_qcm_virt_at_result_func_ptr virt_at_result_func_ptr);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC))
#endif /* __QOSA_SYS_REGISTERS_H__ */
