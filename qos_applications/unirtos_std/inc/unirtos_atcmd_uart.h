/*****************************************************************/ /**
* @file unirtos_atcmd_uart.h
* @brief
* @author larson.li@quectel.com
* @date 2024-04-06
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-04-06 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_UART_H__
#define __UNIRTOS_ATCMD_UART_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"
#include "unirtos_atcmd_cfg.h"

#ifdef CONFIG_QAPP_UNIRTOS_AT_IPR_CMD
void qstd_exec_ipr_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_IPR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CBAUD_CMD
void qstd_exec_cbaud_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_CBAUD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_ICF_CMD
void qstd_exec_icf_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_ICF_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_IFC_CMD
void qstd_exec_ifc_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_IFC_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_NATSPEED_CMD
void unir_exec_natspeed_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_NATSPEED_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD
/**
 * @brief AT+QDIAGPORT command handler (SET/READ/TEST). Switches the platform debug UART
 *        between debug-log console and AT-command port, persisting the choice to NV.
 *
 * - AT+QDIAGPORT=<mode> : persist and apply the debug-UART role (0/1).
 * - AT+QDIAGPORT?       : report the current mode as "+QDIAGPORT: <mode>".
 * - AT+QDIAGPORT=?      : report the supported range "+QDIAGPORT: (0,1)".
 *
 * @note <mode> values are qosa_at_debug_uart_mode_e (qosa_at_sio.h). The switch itself is
 *       deferred to the platform and applied after this command's response is emitted.
 */
void qstd_exec_qdiagport_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief Power-on init: read the persisted QDIAGPORT mode (default console) and apply it
 *        synchronously. Runs from unir_atcmd_init().
 */
void unir_qdiagport_at_init(void);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDIAGPORT_CMD */

#endif /* __UNIRTOS_ATCMD_UART_H__ */
