/*****************************************************************/ /**
* @file mpc_ims.h
* @brief ims operation interface for modem port layer
* @author elmer.tang@quectel.com
* @date 2026-03-06
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2026-03-06 <td>1.0 <td>elmer.tang <td> Init
* </table>
**********************************************************************/
#ifndef __MPC_IMS__H__
#define __MPC_IMS__H__

#include "mpc.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_ims.h"

typedef struct
{
    qosa_uint8_t call_type;
} mpc_ims_ring_event_ind_t;

typedef struct
{
    qosa_uint8_t no_carrier;
} mpc_ims_disconnect_event_ind_t;

typedef struct
{
    qosa_uint8_t type;
    char         dialnumstr[80];
} mpc_ims_conn_id_event_ind_t;

#endif// !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif /* __MPC_IMS__H__ */
