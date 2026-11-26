/*****************************************************************/ /**
* @file unirtos_atcmd_mifi.h
* @brief MiFi compatibility AT commands
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_MIFI_H__
#define __UNIRTOS_ATCMD_MIFI_H__

#include "qosa_at_param.h"
#include "qosa_def.h"
#include "unirtos_atcmd_cfg.h"

#ifdef CONFIG_QAPP_UNIRTOS_MIFI_AT_FUNC
void qstd_exec_qmifinet_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qlapptest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qlapptestex_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qlapptestact_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qsleeptest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qledtest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qkeytest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qbatterytest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qbatterysoc_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_ogmr_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qodmver_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qodmsub_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qopauthsiminfo_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwifitest_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_wifi_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwssid_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qwauth_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qswsim_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qsimban_cmd(qosa_at_cmd_t *cmd);
void qstd_exec_qsimpoll_cmd(qosa_at_cmd_t *cmd);

#endif /* CONFIG_QAPP_UNIRTOS_MIFI_AT_FUNC */

#endif /* __UNIRTOS_ATCMD_MIFI_H__ */
