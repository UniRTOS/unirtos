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
#ifndef __PPPBUF__H__
#define __PPPBUF__H__
#include <stdint.h>
#include <stdlib.h>

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#if UNIRTOS_PPP_USE_EXTERNAL_PBUF

#include "lwip/pbuf.h"

#else  // !UNIRTOS_PPP_USE_EXTERNAL_PBUF

#define QPPP_PBUF_RAW                                   0x0001
#define QPPP_PBUF_RAM                                   0x0002
#define QPPP_PBUF_POOL                                  0x0003

#define QPPP_PBUF_POOL_BUFSIZE                          1700

#define qpppbuf_alloc(l, length, type)                  pppbuf_alloc_impl(l, length, type)
#define qpppbuf_free(p)                                 pppbuf_free_impl(p)
#define qpppbuf_realloc(p, new_len)                     pppbuf_realloc_impl(p, new_len)
#define qpppbuf_add_header(p, header_size_increment)    pppbuf_add_header_impl(p, header_size_increment)
#define qpppbuf_remove_header(p, header_size_decrement) pppbuf_remove_header_impl(p, header_size_decrement)
#define qpppbuf_cat(head, tail)                         pppbuf_cat_impl(head, tail)
#define qpppbuf_chain(head, tail)                       pppbuf_chain_impl(head, tail)
#define qpppbuf_take(buf, dataptr, len)                 pppbuf_take_impl(buf, dataptr, len)
#define qpppbuf_copy(p_to, p_from)                      pppbuf_copy_impl(p_to, p_from)
#define qpppbuf_clone(layer, type, p)                   pppbuf_clone_impl(layer, type, p)
#define qpppbuf_coalesce(p, layer)                      pppbuf_coalesce_impl(p, layer)

struct qpppbuf
{
    struct qpppbuf *next;
    void           *payload;
    uint16_t        tot_len;
    uint16_t        len;
};

/** Definitions for error constants. */
typedef enum
{
    /** No error, everything OK. */
    QPPP_ERR_OK = 0,
    /** Out of memory error.     */
    QPPP_ERR_MEM = -1,
    /** Buffer error.            */
    QPPP_ERR_BUF = -2,
    /** Timeout.                 */
    QPPP_ERR_TIMEOUT = -3,
    /** Routing problem.         */
    QPPP_ERR_RTE = -4,
    /** Operation in progress    */
    QPPP_ERR_INPROGRESS = -5,
    /** Illegal value.           */
    QPPP_ERR_VAL = -6,
    /** Operation would block.   */
    QPPP_ERR_WOULDBLOCK = -7,
    /** Address in use.          */
    QPPP_ERR_USE = -8,
    /** Already connecting.      */
    QPPP_ERR_ALREADY = -9,
    /** Conn already established.*/
    QPPP_ERR_ISCONN = -10,
    /** Not connected.           */
    QPPP_ERR_CONN = -11,
    /** Low-level netif error    */
    QPPP_ERR_IF = -12,

    /** Connection aborted.      */
    QPPP_ERR_ABRT = -13,
    /** Connection reset.        */
    QPPP_ERR_RST = -14,
    /** Connection closed.       */
    QPPP_ERR_CLSD = -15,
    /** Illegal argument.        */
    QPPP_ERR_ARG = -16
} qppp_err_enum_t;

struct qpppbuf *pppbuf_alloc_impl(uint32_t layer, uint16_t length, uint32_t type);
uint8_t         pppbuf_free_impl(struct qpppbuf *p);
void            pppbuf_realloc_impl(struct qpppbuf *p, uint16_t new_len);
int8_t          pppbuf_add_header_impl(struct qpppbuf *p, uint16_t header_size_increment);
int8_t          pppbuf_remove_header_impl(struct qpppbuf *p, uint16_t header_size_decrement);
void            pppbuf_cat_impl(struct qpppbuf *head, struct qpppbuf *tail);
void            pppbuf_chain_impl(struct qpppbuf *head, struct qpppbuf *tail);
int8_t          pppbuf_take_impl(struct qpppbuf *buf, const void *dataptr, uint16_t len);
int8_t          pppbuf_copy_impl(struct qpppbuf *p_to, const struct qpppbuf *p_from);
struct qpppbuf *pppbuf_clone_impl(uint32_t layer, uint32_t type, struct qpppbuf *p);
struct qpppbuf *pppbuf_coalesce_impl(struct qpppbuf *p, uint32_t layer);

#endif  // !UNIRTOS_PPP_USE_EXTERNAL_PBUF
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif  // !__PPPBUF__H__
