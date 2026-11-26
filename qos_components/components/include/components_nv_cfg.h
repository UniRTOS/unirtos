/*****************************************************************/ /**
* @file components_nv_cfg.h
* @brief
* @author larson.li@quectel.com
* @date 2024-03-29
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2024-03-29 <td>1.0 <td>Larson.Li <td> Init
* </table>
**********************************************************************/
#ifndef __QCM_COMPONENTS_NV_CFG_H__
#define __QCM_COMPONENTS_NV_CFG_H__

#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_nvitem.h"

// clang-format off

//json nv file
#define QCM_CONFIG_JSON_FILE                    "components_cfg.json"

//json config root node
#define QCM_CFG_JSON_ROOT_NODE_NAME             QCM_CONFIG_JSON_FILE"/quec_config"

#define QCM_AT_UART_CFG                         QCM_CFG_JSON_ROOT_NODE_NAME"/uart_cfg"
#define QCM_AT_SIM_CFG                          QCM_CFG_JSON_ROOT_NODE_NAME"/sim_at_cfg"
#define QCM_AT_NW_CFG                           QCM_CFG_JSON_ROOT_NODE_NAME"/nw_at_cfg"
#define QCM_AT_USBNET_CFG                       QCM_CFG_JSON_ROOT_NODE_NAME"/usbnet_at_cfg"

//RTC config
#define QCM_RTC_NV_CFG                          QCM_CFG_JSON_ROOT_NODE_NAME"/rtc_nv_cfg"

// sbfota config
#define QCM_AT_SBFOTA_CFG                       QCM_CFG_JSON_ROOT_NODE_NAME"/sbfota_at_cfg"

// URC config
#define QCM_URC_CFG                             QCM_CFG_JSON_ROOT_NODE_NAME"/qcm_urc_cfg"

//mifi config
#define QCM_MIFI_CFG                            QCM_CFG_JSON_ROOT_NODE_NAME"/mifi_net_cfg"
#define QCM_MIFI_SIM_INFO                       QCM_CFG_JSON_ROOT_NODE_NAME"/mifi_sim_info"
// clang-format on

// GNSS config
#define QCM_AT_GNSS_CFG                         QCM_CFG_JSON_ROOT_NODE_NAME"/gnss_at_cfg"

#endif /* __QCM_COMPONENTS_NV_CFG_H__ */
