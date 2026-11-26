/*****************************************************************/ /**
* @file unirtos_atcmd_sim.h
* @brief
* @author Joe.tu@quectel.com
* @date 2024-12-14
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2024-12-14 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_USBNET_H__
#define __UNIRTOS_ATCMD_USBNET_H__

#include "qosa_def.h"
#include "qosa_at_param.h"
void unirtos_usbnet_at_init(void);
void qstd_exec_qnetdevctl_cmd(qosa_at_cmd_t *cmd);
void unirtos_exec_qcfg_usbnet_cmd(qosa_at_cmd_t *cmd);
void unirtos_exec_qcfg_nat_cmd(qosa_at_cmd_t *cmd);
void unirtos_exec_qcfg_usbnet_ri_cmd(qosa_at_cmd_t *cmd);
void unirtos_exec_qcfg_netmaskset_cmd(qosa_at_cmd_t *cmd);
void unirtos_exec_qcfg_usbnetmac_cmd(qosa_at_cmd_t *cmd);

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCFG_DHCP_CFG_CMD
void unirtos_exec_qcfg_dhcp_cfg_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCFG_DHCP_CFG_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_QDMZ_AT_FUNC
void qstd_exec_qdmz_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_QDMZ_AT_FUNC */

#endif /* __UNIRTOS_ATCMD_USBNET_H__ */
