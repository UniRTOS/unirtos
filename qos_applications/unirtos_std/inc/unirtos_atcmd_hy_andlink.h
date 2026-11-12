/*****************************************************************/ /**
* @file unirtos_atcmd_hy_andlink.h
* @brief
* @author garrett.huang@quectel.com
* @date 2026-01-14
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-01-14 <td>1.0 <td>garrett.tang <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_HY_ANDLINK_H__
#define __UNIRTOS_ATCMD_HY_ANDLINK_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

void qstd_exec_hy_andlink_qadlcfg_cmd_func(qosa_at_cmd_t *cmd);

void qstd_exec_hy_andlink_qadlstart_cmd_func(qosa_at_cmd_t *cmd);

void qstd_exec_hy_andlink_qadlstatus_cmd_func(qosa_at_cmd_t *cmd);

void qstd_exec_hy_andlink_qadlreport_cmd_func(qosa_at_cmd_t *cmd);

void qstd_exec_hy_andlink_qadlstop_cmd_func(qosa_at_cmd_t *cmd);

void qstd_exec_hy_andlink_qadlbindstate_cmd_func(qosa_at_cmd_t *cmd);

void qstd_exec_hy_andlink_qadlreset_cmd_func(qosa_at_cmd_t *cmd);

#endif /* __UNIRTOS_ATCMD_HY_ANDLINK_H__*/
