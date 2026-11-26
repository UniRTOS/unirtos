/*****************************************************************/ /**
* @file unirtos_atcmd_firewall.h
* @brief
* @author alan.tuo@quectel.com
* @date 2026-05-13
*
* @copyright Copyright (c) 2026 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-05-13 <td>1.0 <td>alan.tuo <td> Init
* </table>
*/
/*****************************************************************/

#ifndef __UNIRTOS_ATCMD_FIREWALL_H__
#define __UNIRTOS_ATCMD_FIREWALL_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_sio.h"
#include "qosa_asyn_dns.h"
#include "unirtos_atcmd_cfg.h"
#include "qcm_firewall_app.h"

/**
 * @struct unir_at_firewall_resp_info_t
 * @brief  Used by the Firewall task to return reporting and processing information to the AT task context.
 */
typedef struct
{
    qstd_firewall_resp_cmd_e cmd_id;  /*!< qstd_firewall_resp_cmd_e */
    void                    *ptr_msg; /*!< Pointer to send_rsp_t/read_rsp_t/stat_rsp_t/accept_rsp_t/... */
} unir_at_firewall_resp_info_t;

/**
 * @brief   Execute firewall QFCFG AT command
 *
 * @param[in] cmd  AT command handle
 */
void qstd_exec_firewall_qfcfg_cmd(qosa_at_cmd_t *cmd);

/**
 * @brief   Firewall AT module initialization
 */
void unir_firewall_at_init(void);

#endif /* __UNIRTOS_ATCMD_FIREWALL_H__ */
