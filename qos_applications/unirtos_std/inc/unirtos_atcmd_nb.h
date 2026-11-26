/*****************************************************************/ /**
* @file unir_atcmd_nb.h
* @brief
* @author elmre.tang@quectel.com
* @date 2024-05-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-04-16 <td>1.0 <td>elmer.tang <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_NB_H__
#define __UNIRTOS_ATCMD_NB_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

void qstd_exec_nuestats_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nb_cclk_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nrb_cmd(qosa_at_cmd_t *cmd);

#ifdef CONFIG_QAPP_UNIRTOS_AT_NBAND_CMD
void qstd_exec_nband_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_NBAND_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NPSMR_CMD
void qstd_exec_npsmr_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_NPSMR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NCSEARFCN_CMD
void qstd_exec_ncsearfcn_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_NCSEARFCN_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NEARFCN_CMD
void qstd_exec_nearfcn_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_NEARFCN_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NCONFIG_CMD
void qstd_exec_nconfig_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_NCONFIG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NPIN_CMD
void qstd_exec_npin_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_NPIN_CMD */

#endif /* __UNIRTOS_ATCMD_NB_H__ */
