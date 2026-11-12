/*****************************************************************/ /**
* @file unirtos_atcmd_dev.h
* @brief
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
#ifndef __UNIRTOS_ATCMD_FACTORY_H__
#define __UNIRTOS_ATCMD_FACTORY_H__

#include "qosa_at_param.h"
#include "unirtos_atcmd_cfg.h"

#ifdef CONFIG_QAPP_UNIRTOS_AT_EGMR_CMD
void qstd_exec_egmr_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_EGMR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QPROD_CMD
void qstd_exec_qprod_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QPROD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_PROD_CMD
void qstd_exec_prod_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_PROD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QSVN_CMD
void qstd_exec_qsvn_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QSVN_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QWSETMAC_CMD
void qstd_exec_qwsetmac_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QWSETMAC_CMD */

#endif /* __UNIRTOS_ATCMD_FACTORY_H__ */
