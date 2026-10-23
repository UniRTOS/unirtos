/*****************************************************************/ /**
* @file mpc_clat.h
* @brief mpc clat interface definitions
* @author John.ou@quectel.com
* @date 2026-03-24
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2026-03-24 <td>1.0 <td>John.ou <td> Init
* </table>
**********************************************************************/

#ifndef __MPC_CLAT__H__
#define __MPC_CLAT__H__

#include "qosa_def.h"
#if !(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#include "qosa_sys.h"
#include "qosa_clat.h"

typedef struct qosa_clat_config qosa_clat_config_t;
qosa_nvm_error_e                mpc_clat_set_config(int profile_id, qosa_clat_config_t *cfg);
qosa_common_error_e             mpc_clat_get_config(int profile_id, qosa_clat_config_t *cfg);

#endif//!(defined(CONFIG_QOSA_BK7238_PLATFORM_FUNC) ||defined(CONFIG_QOSA_BK7258_PLATFORM_FUNC) || defined(CONFIG_QOSA_ECR6600_PLATFORM_FUNC)) 
#endif
