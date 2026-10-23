/*****************************************************************/ /**
* @file qosa_modem.h
* @brief Modem operation interface
* @author Joe.tu@quectel.com
* @date 2024-03-29
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

#ifndef __QOSA_MODEM__H__
#define __QOSA_MODEM__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_sim_config.h"

/**
 * @brief Define QOSA_SIMID_NUM macro to represent the maximum value of simid in runtime
 * @note Note the difference with CONFIG_QOSA_SIM_HAL_SLOT_NUM:
 *       1. CONFIG_QOSA_SIM_HAL_SLOT_NUM is a literal value, representing the maximum number of SIM cards of the product, usually consistent with the number of SIM card slots on the board; because it is a literal value, it can be used to define the number of arrays
 *       2. QOSA_SIMID_NUM is the boundary value of simid in runtime; can be used to check if simid is legal
 *       3. QOSA_SIMID_NUM is less than or equal to CONFIG_QOSA_SIM_HAL_SLOT_NUM
 *
 *       If the SIM component is used, the SIM value checked by the application layer uses the g_ms_sim_num mechanism definition, if not, use CONFIG_QOSA_SIM_HAL_SLOT_NUM
 *
 */
#ifdef CONFIG_QOSA_SIM_FUNC

extern qosa_uint8_t g_ms_sim_num;

#if (CONFIG_QOSA_SIM_HAL_SLOT_NUM == 2)
#define QOSA_SIMID_NUM (CONFIG_QOSA_SIM_HAL_SLOT_NUM)
#else
#define QOSA_SIMID_NUM (1)
#endif

#else
#define QOSA_SIMID_NUM (CONFIG_QOSA_SIM_HAL_SLOT_NUM)
#endif  // CONFIG_QOSA_SIM_FUNC

/** osa modem api cnf return result structure common parameters, the first two parameters of each cnf structure must be this */
#define QOSA_MODEM_COMMON_CNF_HDR                                                                                                                              \
    qosa_uint8_t simid; /*!< sim id */                                                                                                                         \
    qosa_uint32_t                                                                                                                                              \
        err_code /*!< error code, success is OSA_OK, failure is others, when this value indicates failure, application layer is not allowed to parse other parameters */

/**
 * @struct qosa_modem_general_cnf_t
 * @brief Modem cnf general reply, used to return simple error and execution success reply.
 *
 */
typedef struct
{
    QOSA_MODEM_COMMON_CNF_HDR;
} qosa_modem_general_cnf_t;
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /*__QOSA_MODEM__H__*/
