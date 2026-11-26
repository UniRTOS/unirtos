/*****************************************************************/ /**
* @file unirtos_atcmd_agora_rtc.h
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
#ifndef __UNIRTOS_ATCMD_AGORA_RTC_H__
#define __UNIRTOS_ATCMD_AGORA_RTC_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"


void qstd_exec_qartccfg_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qartcopen_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qartcjoin_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qartcleave_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qartcclose_cmd(qosa_at_cmd_t *cmd);

#endif /* __UNIRTOS_ATCMD_AGORA_RTC_H__*/
