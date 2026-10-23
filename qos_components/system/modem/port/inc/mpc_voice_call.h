/*****************************************************************/ /**
* @file mpc_voice_call.h
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
#ifndef __MPC_VOICE_CALL__H__
#define __MPC_VOICE_CALL__H__

#include "mpc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
int mpc_start_voice_call(const char *phone_number);

int mpc_stop_voice_call(qosa_uint8_t  simid);

int mpc_answer_call(void);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __MPC_VOICE_CALL__H__ */
