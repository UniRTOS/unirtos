/*****************************************************************/ /**
* @file unirtos_atcmd_general.h
* @brief
* @author larson.li@quectel.com
* @date 2023-05-25
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-05-25 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_GENERAL_H__
#define __UNIRTOS_ATCMD_GENERAL_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"
#include "unirtos_atcmd_cfg.h"

/*========================================================================
*  Type Definition
*========================================================================*/

#ifdef CONFIG_QAPP_UNIRTOS_AT_CSUB_CMD
void qstd_exec_csub_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_CSUB_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QINF_CMD
void qstd_exec_qinf_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QINF_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QGMR_CMD
void qstd_exec_qgmr_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QGMR_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_HVERSION_CMD
void qstd_exec_qcert_hversion_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_HVERSION_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_MODULE_CMD
void qstd_exec_qcert_module_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_MODULE_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QPOWD_CMD
void qstd_exec_qpowd_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QPOWD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QBOOTREASON_CMD
void qstd_exec_qbootreason_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QBOOTREASON_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CBC_CMD
void qstd_exec_cbc_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_CBC_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QTEMP_CMD
void qstd_exec_qtemp_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QTEMP_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QCHIPINFO_CMD
void unir_exec_qchipinfo_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QCHIPINFO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QADC_CMD
void qstd_exec_qadc_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QADC_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QRESET_CMD
void qstd_exec_qreset_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QRESET_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QDOWNLOAD_CMD
void qstd_exec_qdownload_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QDOWNLOAD_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_ATO_CMD
void qstd_exec_ato_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_ATO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_RETSET_CMD
void qstd_exec_rstset_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_RETSET_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_GPIO_CMD
void qstd_exec_qgpiocfg_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_GPIO_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_CVERSION_CMD
void qstd_exec_cversion_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_CVERSION_CMD */

#ifdef CONFIG_QAPP_UNIRTOS_AT_QVERINFO_CMD
void qstd_exec_qverinfo_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QVERINFO_CMD */

void qstd_exec_gmi_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cgmi_cmd(qosa_at_cmd_t *cmd);

void qstd_exec_gmm_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cgmm_cmd(qosa_at_cmd_t *cmd);

void qstd_exec_gmr_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_cgmr_cmd(qosa_at_cmd_t *cmd);

void qstd_exec_cgsn_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_gsn_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qgsn_cmd(qosa_at_cmd_t *cmd);
#ifdef CONFIG_QAPP_UNIRTOS_AT_QREDDAAVN_CMD
void qstd_exec_qreddavn_cmd(qosa_at_cmd_t *cmd);
#endif /* CONFIG_QAPP_UNIRTOS_AT_QREDDAAVN_CMD */
#endif /* __UNIRTOS_ATCMD_GENERAL_H__ */
