/*****************************************************************/ /**
 * @file unirtos_atcmd_uis8360s.h
 * @brief UIS8360S WiFi manufacturing test AT command declarations
 * @author joe.tu@quectel.com
 * @date 2026-06-28
 *
 * @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description"
 * <tr><td>2026-06-28 <td>1.0 <td>joe.tu <td> Init
 * </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_UIS8360S_H__
#define __UNIRTOS_ATCMD_UIS8360S_H__

#include "qosa_at_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

void qstd_exec_qwififtmtest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwifitest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwsetcali_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwsetssid_uis_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwsetmac_uis_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qnvr_uis_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qnvw_uis_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qledtest_cmd(qosa_at_cmd_t *cmd);
#ifdef __cplusplus
}
#endif

#endif /* __UNIRTOS_ATCMD_UIS8360S_H__ */
