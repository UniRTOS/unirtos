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
#ifndef __QPPP_IP__H__
#define __QPPP_IP__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
/** Macros related to IP addresses used in PPP */
#define IP_CLASSA_NSHIFT   24
#define IP_LOOPBACKNET     127

#define IP_CLASSD(a)       (((qosa_uint32_t)(a)&0xf0000000UL) == 0xe0000000UL)
#define IP_CLASSD_NET      0xf0000000 /* These ones aren't really */
#define IP_CLASSD_NSHIFT   28         /*   net and host fields, but */
#define IP_CLASSD_HOST     0x0fffffff /*   routing needn't know. */
#define IP_MULTICAST(a)    IP_CLASSD(a)

#define IP_EXPERIMENTAL(a) (((qosa_uint32_t)(a)&0xf0000000UL) == 0xf0000000UL)
#define IP_BADCLASS(a)     (((qosa_uint32_t)(a)&0xf0000000UL) == 0xf0000000UL)
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QPPP_IP__H__ */
