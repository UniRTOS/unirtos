/*****************************************************************/ /**
* @file unirtos_atcmd_voice_call.h
* @brief
* @author Liaz.liao@quectel.com
* @date 2026-03-05
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-03-05 <td>1.0 <td>Liaz.liao <td> Init
* </table>
**********************************************************************/
#ifndef __UNIRTOS_ATCMD_VOICE_CALL_H__
#define __UNIRTOS_ATCMD_VOICE_CALL_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_at_param.h"
#include "qosa_at_config.h"
#include "unirtos_atcmd_cfg.h"

void unir_exec_atd_cmd(qosa_at_cmd_t *cmd);

void unir_exec_ath_cmd(qosa_at_cmd_t *cmd);

void unir_exec_ata_cmd(qosa_at_cmd_t *cmd);

#endif /* __UNIRTOS_ATCMD_VOICE_CALL_H__ */
