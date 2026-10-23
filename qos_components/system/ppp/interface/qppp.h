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
#ifndef __QPPP__H__
#define __QPPP__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_watermark.h"

#define QPPP_TERMINAL_NEED_TERM QOSA_TRUE
#define QPPP_TERMINAL_NO_TERM   QOSA_FALSE

enum qppp_status
{
    QPPP_STATUS_DISCONNECTED = 0x0,
    QPPP_STATUS_CONNECTED = 0x1,
};

// 0: Success, other values indicate failure
typedef void (*qppp_createjob_cb)(void* ctx, qosa_int32_t err);
typedef void (*qppp_end_cb)(void* ctx, qosa_int32_t err);
typedef int (*qppp_output_cb)(void* ctx, qosa_uint8_t* data, qosa_uint32_t len);
typedef qosa_bool_t (*qppp_tunnel_2_flowctrl_cb)(void* ctx);

typedef struct qppp_job_s
{
    qosa_uint8_t simid;
    qosa_uint8_t pdpid;
    qosa_int32_t dev_hd;

    qppp_createjob_cb         job_cb;
    qppp_end_cb               end_cb;
    qppp_output_cb            tunnel_2_output_cb;   /*!< Tunnel 2 output callback function, sends from pppos to AT */
    qppp_tunnel_2_flowctrl_cb tunnel_2_flowctrl_cb; /*!< Tunnel 2 flow control callback function */

    void* job_ctx;

    qosa_buffer_watermark_t* tunnel_1_wm; /*!< Uplink tunnel 1 watermark */
} qppp_job_t;

qosa_bool_t  qppp_createjob(qppp_job_t* job);
void         qppp_terminaljob(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_bool_t need_term);
qosa_uint8_t qppp_get_status(qosa_uint8_t simid, qosa_uint8_t pdpid);
#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __QPPP__H__ */
