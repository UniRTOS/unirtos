/*****************************************************************/ /**
* @file qosa_voice_call.h
* @brief VOICE CALL Function
* @author Liaz.liao@quectel.com
* @date 2026-03-05
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-03-05 <td>1.0 <td>Liaz.liao <td> Init
* </table>
**********************************************************************/

#ifndef __QOSA_VOICE_CALL__H__
#define __QOSA_VOICE_CALL__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"

/**
 * @brief
 *
 * @param[in] phone_number
 *          - phone number
 * @return int
 */
int qosa_start_voice_call(const char *phone_number);

int qosa_stop_voice_call(qosa_uint8_t  simid);

int qosa_answer_call(void);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QOSA_VOICE_CALL__H__ */
