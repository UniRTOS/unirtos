/*****************************************************************/ /**
* @file unirtos_atcmd_nw_impl.h
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
#ifndef __UNIRTOS_ATCMD_IMPL_H__
#define __UNIRTOS_ATCMD_IMPL_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"
#include "qosa_at_config.h"

void qstd_exec_qcfg_nwscanmode_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcfg_nwscanmodeex_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcfg_nwscanseq_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcfg_band_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcfg_roamservice_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcfg_service_domain_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcfg_pdpremap_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcfg_qops_fmt_cmd(qosa_at_cmd_t *cmd);
#endif /* __UNIRTOS_ATCMD_IMPL_H__ */
