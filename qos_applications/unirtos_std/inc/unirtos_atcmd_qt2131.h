/*****************************************************************/ /**
* @file unirtos_atcmd_qt2131.h
* @brief
* @author kevin.wang@quectel.com
* @date 2023-06-13
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-06-13 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_QT2131_H__
#define __UNIRTOS_ATCMD_QT2131_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"


void qstd_exec_nflst_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nqstate_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nchiptype_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nrdtest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nrdctl_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nrdset_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nrdexec_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nrcupd_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ngold_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nlogmodft_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nvetosuperds_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nuicc_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nsimac_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nforceburn_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nbackup_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_nrecovery_cmd(qosa_at_cmd_t *cmd);
#endif
