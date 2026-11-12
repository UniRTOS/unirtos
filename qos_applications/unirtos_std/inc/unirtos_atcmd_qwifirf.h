/*****************************************************************/ /**
* @file unirtos_atcmd_qwifirf.h
* @brief WiFi RF factory test AT command
* @author joe.tu@quectel.com
* @date 2026-06-16
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-06-16 <td>1.0 <td>joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_QWIFIRF_H__
#define __UNIRTOS_ATCMD_QWIFIRF_H__

#include "qosa_at_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

void qstd_exec_qwifirf_cmd(qosa_at_cmd_t *cmd);

#ifdef __cplusplus
}
#endif

#endif /* __UNIRTOS_ATCMD_QWIFIRF_H__ */
