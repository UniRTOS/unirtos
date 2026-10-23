/*****************************************************************/ /**
* @brief
* @author Joe.tu@quectel.com
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
#ifndef __QPPP_SESSION_H__
#define __QPPP_SESSION_H__

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qppp.h"
#include "ppp.h"

struct qppp_auth
{
    qosa_uint16_t auth_method;  // Authentication method; PPP_PAP, PPP_CHAP
    union
    {
#if PAP_SUPPORT
        struct
        {
            qosa_uint8_t user[MAXNAMELEN + 1];
            qosa_uint8_t passwd[MAXSECRETLEN + 1];
            qosa_uint8_t id;
        } pap;
#endif
#if CHAP_SUPPORT
        struct
        {
            qosa_uint8_t* our_name;
            qosa_uint8_t  our_name_len;
            qosa_uint8_t* peer_name;
            qosa_uint8_t  peer_name_len;
            qosa_uint8_t  challenge[CHAL_MAX_PKTLEN]; /*!< Complete challenge packet, containing id, length, challenge, name */
            qosa_uint8_t  challenge_len;
            qosa_uint8_t  response[RESP_MAX_PKTLEN];  /*!< Complete response packet, containing id, length, response, name */
            qosa_uint8_t  response_len;
            qosa_uint8_t  id;
        } chap;
#endif
    };
};

struct qppp_session
{
    qosa_uint8_t simid;                               /*!< sim id */
    qosa_uint8_t pdpid;                               /*!< pdp id */
    qosa_int32_t dev_hd;                              /*!< PPP AT Channel */

    qppp_createjob_cb         createjob_cb;           /*!< PPP creation result callback function */
    qppp_output_cb            tunnel_2_output_cb;     /*!< Tunnel 2 output callback function, sends from pppos to AT */
    qppp_end_cb               end_cb;                 /*!< PPP termination callback function */
    qppp_tunnel_2_flowctrl_cb tunnel_2_flowctrl_cb;   /*!< Tunnel 2 flow control callback function */
    void*                     job_ctx;                /*!< PPP application context */

    qosa_buffer_watermark_t* tunnel_1_wm;             /*!< Uplink tunnel 1 watermark */
    qosa_buffer_block_t*     current_tunnel_1_wm_ptr; /*!< Current tunnel 1 watermark pointer */

    ppp_pcb*              pcb;
    struct lowlevel_data* low_ctx;  // Lower-level transaction context

    struct qppp_auth auth;          /*!< PPP authentication context */
    qosa_bool_t      act_by_ppp;    /*!< Whether activated by ppp */
};

struct qppp_session* _get_tls_session(qosa_uint8_t simid, qosa_uint8_t pdpid);

void qppp_core_lock(void);
void qppp_core_unlock(void);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif  // !__QPPP_SESSION_H__
