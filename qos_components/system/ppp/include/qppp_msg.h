/*****************************************************************/ /**
* @brief
* @author Joe.tu@quectel.com
* @date 2025-04-27
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-04-27 <td>1.0 <td>Joe.tu <td> Init
* </table>
**********************************************************************/
#ifndef __QPPP_MSG__H__
#define __QPPP_MSG__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
/* unir ppp inter msg */
#define QPPP_MSG_TUNNEL_1_PACKET_IND 201
#define QPPP_MSG_TUNNEL_4_PACKET_IND 204
#define QPPP_MSG_TIMEOUT             200
#define QPPP_MSG_TERMINAL_REQ        208
#define QPPP_MSG_SESSION_END         209

/** Register message handler function */
#define QPPP_MSG_REGISTER_HANDLERS(msg, i, h)                                                                                                                  \
    (&msg)->msg_id = i;                                                                                                                                        \
    (&msg)->handler = h;

#define QPPP_MSG_SEND(msg) qppp_send_msg(&msg)

typedef struct qppp_msg qppp_msg_t;
typedef void (*qppp_msg_handler_t)(qppp_msg_t* msg);
typedef struct qppp_session qppp_session_t;

struct qppp_msg
{
    qosa_uint32_t msg_id;
    union
    {
        qosa_uint32_t data;
    };
    qppp_msg_handler_t handler;
    qppp_session_t*    session;
    qosa_uint8_t       simid; /*!< Message passing from other threads */
    qosa_uint8_t       pdpid; /*!< Message passing from other threads */
};

void qppp_send_msg(struct qppp_msg* msg);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif
