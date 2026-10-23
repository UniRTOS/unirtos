/*****************************************************************/ /**
* @brief
* @author Joe.tu@om
* @date 2025-04-07
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-04-07 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/

#ifndef __QPPP_QOSA_ARCH__H__
#define __QPPP_QOSA_ARCH__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
typedef void (*qppp_timeout_handler)(void *arg);

void qppp_timeout_init(void);
void qppp_timeout(qosa_uint32_t msecs, qppp_timeout_handler handler, void *arg);
void qppp_untimeout(qppp_timeout_handler handler, void *arg);

qosa_uint32_t qppp_tick(void);
qosa_bool_t   ppp_isdigit(qosa_uint8_t c);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QPPP_QOSA_ARCH__H__ */
