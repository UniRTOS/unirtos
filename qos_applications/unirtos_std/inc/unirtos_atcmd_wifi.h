/*****************************************************************/ /**
* @file unirtos_atcmd_wifi.h
* @brief
* @author GitHub Copilot
* @date 2026-06-05
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-06-05 <td>1.0 <td>GitHub Copilot <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_WIFI_H__
#define __UNIRTOS_ATCMD_WIFI_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"

void qstd_exec_qwifi_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwssid_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwauth_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwmoch_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwclip_cmd(qosa_at_cmd_t *cmd);

#endif /* __UNIRTOS_ATCMD_WIFI_H__ */