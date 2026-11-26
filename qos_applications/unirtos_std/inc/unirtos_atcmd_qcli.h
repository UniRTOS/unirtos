/*****************************************************************/ /**
 * @file unirtos_atcmd_qcli.h
 * @brief AT+QCLI command interface
 * @author joe.tu@quectel.com
 * @date 2026-06-28
 *
 * @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
 * All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
 *
 * @par EDIT HISTORY FOR MODULE
 * <table>
 * <tr><th>Date <th>Version <th>Author <th>Description"
 * <tr><td>2026-06-28 <td>1.0 <td>Joe.Tu <td> Init
 * </table>
 **********************************************************************/
#ifndef __UNIRTOS_ATCMD_QCLI_H__
#define __UNIRTOS_ATCMD_QCLI_H__

#include "qosa_at_param.h"
#include "qosa_def.h"
#include "qosa_sys.h"

/**
 * @brief Execute AT+QCLI command.
 *
 * @param[in] qosa_at_cmd_t *cmd
 *          - Current AT command context.
 */
void qstd_exec_qcli_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief Initialize AT+QCLI related CLI resources.
 */
void qstd_qcli_at_init(void);

#endif /* __UNIRTOS_ATCMD_QCLI_H__ */
