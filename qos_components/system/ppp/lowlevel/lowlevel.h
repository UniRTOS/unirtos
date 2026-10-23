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
#ifndef __LOWLEVEL_H__
#define __LOWLEVEL_H__

#include "qosa_def.h"

#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#define LOWLEVEL_NOTIFY_CONNECT 1

typedef struct lowlevel_data lowlevel_context_t;
typedef int (*lowlevel_tunnel_4_cb)(qosa_uint8_t simid, qosa_uint8_t pdpid);
typedef int (*lowlevel_notify)(struct lowlevel_data* lowlevel_data, int notify, void* argv);

struct lowlevel_data
{
    qosa_uint8_t         simid;       /*!< SIMID */
    qosa_uint8_t         pdpid;       /*!< PDPID */
    qosa_int32_t         dev_hd;      /*!< Device channel */
    qosa_uint8_t         num;         /*!< This value increments each time a PPP is initiated */
    lowlevel_tunnel_4_cb tunnel_4_cb; /*!< Callback function for tunnel_4 data arrival */
    lowlevel_notify      notify;      /*!< PPP service layer sends notification to lowlevel */
};

struct lowlevel_data* lowlevel_init(qosa_uint8_t simid, qosa_uint8_t pdpid, qosa_int32_t dev_hd, lowlevel_tunnel_4_cb tunnel_4_cb);
void                  lowlevel_deinit(struct lowlevel_data* lowlevel_data);
int                   lowlevel_tunnel_3(struct lowlevel_data* lowlevel_data, qosa_uint8_t* data, qosa_uint32_t len);
int                   lowlevel_tunnel_4_read(struct lowlevel_data* lowlevel_data, qosa_uint8_t* data, qosa_uint32_t len);

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __LOWLEVEL_H__ */
