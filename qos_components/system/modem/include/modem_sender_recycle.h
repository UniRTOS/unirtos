/*****************************************************************/ /**
* @file modem_sender_recycle.h
* @brief Modem sender recycle interface
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

#ifndef __MODEM_SENDER_RECYCLE__H__
#define __MODEM_SENDER_RECYCLE__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#include "qosa_sys.h"

/** Application layer receives cnf signal and transfers it to the target thread for processing */
typedef void (*ms_sender_recycle_callback_ptr)(void* cnf_sig);

/**
 * @brief Application layer uses this interface to register cnf signal recycling job
 *
 * @param[in] taskid
 *          - task id where cnf signal should be parsed
 * @param[in] callback
 *          - In this callback, this callback is executed in the ms layer environment, the application layer should transfer the received cnf to the specified task id in this callback, and call the cnf parser in that task environment.
 * @return qosa_common_error_e
 */
qosa_common_error_e qosa_ms_register_recycle(qosa_task_t taskid, ms_sender_recycle_callback_ptr callback);

qosa_common_error_e qosa_ms_cnf_signal_parsing(void* cnf_signal);

#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 

#endif /*__MODEM_SENDER_RECYCLE__H__ */
