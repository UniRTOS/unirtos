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
#ifndef __UNIRTOS_PLATFORM_H__
#define __UNIRTOS_PLATFORM_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
typedef uint8_t  qosa_uint8_t;
typedef uint16_t qosa_uint16_t;
typedef uint32_t qosa_uint32_t;
// typedef bool qosa_bool_t;
typedef int qppp_err_t;

#define QOSA_NULL     0

#define OSA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define OSA_MIN(a, b) ((a) < (b) ? (a) : (b))

#ifndef QOSA_UNUSED
#define QOSA_UNUSED(x) (void)(x)
#endif  // ! QOSA_UNUSED

#define PPP_DECL_PROTECT(x)
#define PPP_PROTECT(x)
#define PPP_UNPROTECT(x)

#ifndef qosa_malloc
// #define qosa_malloc(x) malloc(x)
#endif  // !qosa_malloc

#ifndef qosa_free
// #define qosa_free(x) free(x)
#endif  // !qosa_free

#ifndef offsetof
#define offsetof(type, member) ((int)&((type *)0)->member)
#endif

#define QOSA_TRUE  1
#define QOSA_FALSE 0

#define QPPP_ASSERT_CORE_LOCKED()

// #define PPP_DECL_PROTECT(x) spinlock_t x
// #define PPP_PROTECT(x) local_irq_save(x)
// #define PPP_UNPROTECT(x) local_irq_restore(x)

qosa_uint32_t _ppp_ntohl(qosa_uint32_t x);

#ifdef UNIRTOS_BIG_ENDIAN
#define ppp_htonl(x) (x)
#else
#define ppp_htonl(x) _ppp_ntohl(x)
#endif

#ifdef UNIRTOS_BIG_ENDIAN
#define ppp_ntohl(x) (x)
#else
#define ppp_ntohl(x) _ppp_ntohl(x)
#endif

// TODO: Add log printing for assertions
#define PPP_ASSERT(message, x)                                                                                                                                 \
    do {                                                                                                                                                       \
        QOSA_ASSERT((x));                                                                                                                                      \
    } while (0)
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif  //__UNIRTOS_PLATFORM_H__
