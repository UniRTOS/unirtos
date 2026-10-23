/*****************************************************************/ /**
* @file qosa_modem_feature_diff.h
* @brief Modem feature difference configuration interface
* @author Joe.tu@quectel.com
* @date 2025-03-17
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-03-17 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_MODEM_FEATURE_DIFF_H__
#define __QOSA_MODEM_FEATURE_DIFF_H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
typedef enum
{
    QOSA_MFD_NW_CFG_CREG_EMERGENCY, /*!< Whether to display cell information in creg command under emergency registration; Default: not display */
    QOSA_MFD_NW_CFG_CREG_MODE,      /*!< Whether to use PS domain status when creg is in rejected status (3); Default: use PS domain status */
} qosa_mfd_e;

typedef struct
{
    qosa_uint8_t allow_query_emergency_in_creg;
    qosa_uint8_t creg_denied_using_ps_reg_status;
} qosa_ms_nw_feature_cfg_t;

/**
 * 0: not display (default)
 * 1: display
 * Whether to allow displaying cell lac cid and other information in CREG command and CREG URC when there is no card and the module is registered to a cell
 */
#define qosa_nw_cfg_creg_emergency qosa_mfd_get_cfg(QOSA_MFD_NW_CFG_CREG_EMERGENCY)

/**
 * 0: when creg is in rejected status (3), if PS domain is in registered status, use PS domain status (default)
 * 1: when creg is in rejected status (3), display rejected status
 * Background: China Telecom does not support CS domain, but customers in the 2G era use CREG command to check network registration status, for compatibility with old customers
 *
 */
#define qosa_nw_cfg_creg_mode      qosa_mfd_get_cfg(QOSA_MFD_NW_CFG_CREG_MODE)

qosa_uint32_t qosa_mfd_get_cfg(qosa_mfd_e feature);

void qosa_mfd_set_cfg(qosa_mfd_e feature, qosa_uint32_t value);
#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_MODEM_FEATURE_DIFF_H__ */
