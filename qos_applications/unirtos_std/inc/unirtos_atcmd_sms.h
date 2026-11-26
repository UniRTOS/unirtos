/*****************************************************************/ /**
* @file unirtos_atcmd_sms.h
* @brief
* @author Joe.tu@quectel.com
* @date 2024-06-23
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-06-23 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_SMS_H__
#define __UNIRTOS_ATCMD_SMS_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

void qstd_exec_cmgd_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cmgf_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cmgl_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cmgr_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcmgr_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cmgs_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qcmgs_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cmgw_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cmms_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cmss_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cnma_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cnmi_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cpms_cmd(qosa_at_cmd_t *cmd);
#ifdef CONFIG_QAPP_UNIRTOS_AT_CPNER_CMD
void qstd_exec_cpner_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_CPNER_CMD */
void qstd_exec_csca_cmd(qosa_at_cmd_t *cmd);
/* TODO: +CSCB not yet implemented; remove this declaration when implemented */
// void qstd_exec_cscb_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_csdh_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_csmp_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_csms_cmd(qosa_at_cmd_t *cmd);

#endif /* __UNIRTOS_ATCMD_SMS_H__ */
