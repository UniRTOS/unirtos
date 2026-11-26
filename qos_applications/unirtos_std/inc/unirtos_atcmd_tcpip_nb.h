/*****************************************************************/ /**
* @file unirtos_atcmd_nb.h
* @brief
* @author garrett.huang@quectel.com
* @date 2025-08-21
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-08-21 <td>1.0 <td>Garrett.Huang <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_TCPIP_NB_H__
#define __UNIRTOS_ATCMD_TCPIP_NB_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

void unir_exec_nb_nsocr_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsoco_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsost_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsostex_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsosd_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsosdex_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nqsos_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsorf_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsocl_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsonmi_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_nsostatus_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_qidnscfg_cmd(qosa_at_cmd_t *cmd);
void unir_exec_nb_qdns_cmd(qosa_at_cmd_t *cmd);

#endif /* __UNIRTOS_ATCMD_TCPIP_NB_H__ */
