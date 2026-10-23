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
#ifndef __UNIRTOS_PPP_OPTS_H
#define __UNIRTOS_PPP_OPTS_H

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
/** Enable PPP function */
#define PPP_SUPPORT                   1

/** Enable PPPoE function */
#define PPPOE_SUPPORT                 0

/** Enable IPv4 functionality */
#define PPP_IPV4_SUPPORT              1

/** Enable IPv6 functionality */
#define PPP_IPV6_SUPPORT              1

/** Enable PAP authentication support */
#define PAP_SUPPORT                   1

/** Enable CHAP authentication */
#define CHAP_SUPPORT                  1

/** Enable PPP service mode */
#define PPP_SERVER                    1

/** Enable PPP phase notify mechanism */
#define PPP_NOTIFY_PHASE              1

/** First, close the LWIP PPP API */
#define LWIP_PPP_API                  0

/** Enable PPP debug function */
#define PPP_DEBUG                     1

/** Enable DNS function */
#define LWIP_DNS                      1

#define PRINTPKT_SUPPORT              1

#define LWIP_USE_EXTERNAL_MBEDTLS     1

/** Our name for authentication purposes */
#define PPP_OUR_NAME                  "QuecPPP"

/***********************************/
#define UNIRTOS_PPP_LOG_DEBUG         1
#define UNIRTOS_PPP_LOG_INFO          1
#define UNIRTOS_PPP_LOG_NOTICE        1
#define UNIRTOS_PPP_LOG_WARNING       1
#define UNIRTOS_PPP_LOG_ERR           1
#define UNIRTOS_PPP_LOG_CRITICAL      1

#define UNIRTOS_PPP_USE_EXTERNAL_PBUF 0
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif  //__UNIRTOS_PPP_OPTS_H
