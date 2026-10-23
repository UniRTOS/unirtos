/*
 * Copyright 2020-2025 Beken

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "os/os.h"
#include "wdrv_co_list.h"
#include "wdrv_cntrl.h"
#include "wifi_log.h"

#define WIFI_API_IPC_COM_REQ_MAX_ARGC           6

enum BK_WIFI_API_CMD_TYPE
{
    // SCAN management command section
    SCAN_START                          = 0x300,  //BK_CMD_WIFI_API_START
    SCAN_STOP                           = 0x301,
    SCAN_RESULT                         = 0x302,
    SCAN_CONTRY_CODE                    = 0x303,
    SCAN_RESULT_FREE                    = 0x304,

    // STA management command section
    STA_SET_CONFIG                      = 0x310,
    STA_START                           = 0x311,
    STA_STOP                            = 0x312,
    STA_GET_CONFIG                      = 0x313,
    STA_GET_LINK_STATUS                 = 0x314,
    STA_GET_LISTEN_INTERVAL             = 0x315,
    STA_SET_LISTEN_INTERVAL             = 0x316,
    STA_SET_BCN_LOSS_INT                = 0x317,
    STA_SET_BCN_RECV_WIN                = 0x318,
    STA_SET_BCN_LOSS_TIME               = 0x319,
    STA_SET_BCN_MISS_TIME               = 0x31A,
    STA_GET_LINK_STATE_WITH_REASON      = 0x31B,
    STA_SET_IP4_STATIC_IP               = 0x31C,
    STA_GET_NETIF_IP4_CONFIG            = 0x31D,

    // AP management command section
    AP_SET_CONFIG                       = 0x320,
    AP_START                            = 0x321,
    AP_STOP                             = 0x322,
    AP_NETIF_IP4_CONFIG                 = 0x323,
    AP_GET_NETIF_IP4_CONFIG             = 0x329,
    AP_SET_CHANNEL                      = 0x324,
    AP_SET_CSA_COUNT                    = 0x325,
    AP_SET_CHANNEL_STOP                 = 0x326,
    AP_GET_STA_LIST                     = 0x327,
    FREE_GET_STA_LIST_MEMORY            = 0x328,
    P2P_GET_MAC                         = 0x32C,

    // PM management Wi-Fi command section
    STA_PM_ENABLE                       = 0x330,
    STA_PM_DISABLE                      = 0x331,
    PS_CONFIG                           = 0x332,

    // MONITOR Wi-Fi command section
    MONITOR_START                       = 0x340,
    MONITOR_STOP                        = 0x341,
    MONITOR_SET_CONFIG                  = 0x342,
    MONITOR_GET_CONFIG                  = 0x343,
    MONITOR_REGISTER_CB                 = 0x344,
    MONITOR_SET_CHANNEL                 = 0x345,
    MONITOR_RESUME                      = 0x346,
    MONITOR_SUSPEND                     = 0x347,
    FILTER_REGISTER_CB                  = 0x348,
    FILTER_FREE_PBUF                    = 0x349,
    MONITOR_FREE_PBUF                   = 0x34A,
    MONITOR_GET_RESULT                  = 0x34B,

    // Common Wi-Fi command section
    WIFI_GET_CHANNEL                    = 0x360,
    WIFI_SET_COUNTRY                    = 0x361,
    WIFI_GET_COUNTRY                    = 0x362,
    WIFI_CAPA_CONFIG                    = 0x363,
    SEND_ARP_SET_RATE_REQ               = 0x364,
    SET_MAC_ADDRESS                     = 0x365,
    STA_GET_MAC                         = 0x366,
    AP_GET_MAC                          = 0x367,
    GET_STATUS                          = 0x368,
    WIFI_SET_MEDIA_MODE                 = 0x369,
    WIFI_SET_VIDEO_QUALITY              = 0x36A,
    WIFI_SET_CSA_COEXIST_MODE_FLAG      = 0x36B,
    WIFI_GET_SUPPORT_MODE               = 0x36C,
    WIFI_GET_BCN_CC                     = 0x36D,
    WIFI_SET_BLOCK_BCMC_EN              = 0x36E,
    WIFI_GET_BLOCK_BCMC_EN              = 0x36F,
    WIFI_SET_RC_CONFIG                  = 0x370,
    WIFI_GET_AP_OPER_CHANNEL            = 0x371,

    //Common PHY command section
    PHY_CAL_RFCALI                      = 0x380,

    // RAW Wi-Fi command section
    SEND_RAW                            = 0x390,

    CHECK_CLIENT_MAC_CONNECTED          = 0x3A0,
    SET_BRIDGE_SYNC_STATE               = 0x3A1,

    // FTM command section
    FTM_START                           = 0x3B0,
    FTM_FREE_RESULT                     = 0x3B1,

    // CSI command section
    CSI_ALG_CONFIG                      = 0x3C0,
    CSI_START                           = 0x3C1,
    CSI_STOP                            = 0x3C2,
    CSI_STATIC_PARAM_RESET              = 0x3C3,
    CSI_INFO_GET                        = 0x3C4,
    CSI_DEMO_LIGHT                      = 0x3C5,

    // P2P command section
    P2P_ENABLE                          = 0x3D0,
    P2P_FIND                            = 0x3D1,
    P2P_LISTEN                          = 0x3D2,
    P2P_STOP_FIND                       = 0x3D3,
    P2P_CONNECT                         = 0x3D4,
    P2P_CANCEL                          = 0x3D5,
    P2P_DISABLE                         = 0x3D6,
    P2P_ENABLE_WITH_INTENT              = 0x3D7,
    P2P_GET_ROLE                        = 0x3D8,
    P2P_GET_GC_NETIF_IP4_CONFIG         = 0x3D9,
    P2P_GET_GO_NETIF_IP4_CONFIG         = 0x3DA,
    P2P_GET_GROUP_CHANNEL               = 0x3DB,

    // RLK command section
    RLK_REGISTER_SEND_CB                = 0x400,
    RLK_UNREGISTER_SEND_CB              = 0x401,
    RLK_REGISTER_RECV_CB                = 0x402,
    RLK_UNREGISTER_RECV_CB              = 0x403,
    RLK_REGISTER_ACS_CFM_CB             = 0x404,
    RLK_UNREGISTER_ACS_CFM_CB           = 0x405,
    RLK_REGISTER_SCAN_CFM_CB            = 0x406,
    RLK_UNREGISTER_SCAN_CFM_CB          = 0x407,
    RLK_INIT                            = 0x408,
    RLK_DEINIT                          = 0x409,
    RLK_SET_CHANNEL                     = 0x40A,
    RLK_GET_CHANNEL                     = 0x40B,
    //RLK_SEND                            = 0x40C,
    RLK_SEND_EX                         = 0x40D,
    RLK_SEND_BY_OUI                     = 0x40E,
    RLK_ADD_PEER                        = 0x40F,
    RLK_DEL_PEER                        = 0x410,
    RLK_GET_PEER                        = 0x411,
    RLK_IS_PEER_EXIST                   = 0x412,
    RLK_GET_PEER_NUM                    = 0x413,
    RLK_SET_TX_AC                       = 0x414,
    RLK_SET_TX_TIMEOUT_MS               = 0x415,
    RLK_SET_TX_POWER                    = 0x416,
    RLK_SET_TX_RATE                     = 0x417,
    RLK_SET_TX_RETRY_CNT                = 0x418,
    RLK_SLEEP                           = 0x419,
    RLK_WAKEUP                          = 0x41A,
    RLK_ADD_WHITE_LIST                  = 0x41B,
    RLK_DEL_WHITE_LIST                  = 0x41C,
    RLK_SET_MAC_HDR_TYPE                = 0x41D,
    RLK_MAC_HDR_REINIT                  = 0x41E,
    RLK_ACS_CHECK                       = 0x41F,
    RLK_SCAN                            = 0x420,
    RLK_SET_ROLE                        = 0x421,
    RLK_SLAVE_APP_INIT                  = 0x422,
    RLK_SLAVE_BSSID_APP_INIT            = 0x423,
    RLK_SET_ACS_AUTO_SWITCH_CHAN        = 0x424,

    BK_WIFI_API_CMD_BUTT                = BK_CMD_WIFI_API_END
};

enum BK_WIFI_API_EVT_TYPE
{
    // Wi-Fi event
    STA_EVT_XXX_0                       = 0x300,  //BK_EVT_WIFI_API_START
    STA_EVT_XXX_1                       = 0x301,

    MONITOR_REGISTER_CB_IND             = 0x310,
    FILER_REGISTER_CB_IND               = 0x311,

    RLK_REGISTER_SEND_CB_IND            = 0x320,
    RLK_REGISTER_SEND_EX_CB_IND         = 0x321,
    RLK_REGISTER_RECV_CB_IND            = 0x322,
    RLK_REGISTER_ACS_CFM_CB_IND         = 0x323,
    RLK_REGISTER_SCAN_CFM_CB_IND        = 0x324,
    RLK_TX_SEND_EVT                     = 0x325,

    BK_WIFI_API_EVT_BUTT                = BK_EVT_WIFI_API_END
};

struct wifi_api_req_no_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_1_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_2_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_3_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_4_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_5_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
    wdrv_cmd_cfm cmd_cfm;
};
struct wifi_api_req_6_arg
{
    wdrv_cmd_hdr cmd_hdr;
    uint32_t argc;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
    uint32_t arg6;
    wdrv_cmd_cfm cmd_cfm;
};
typedef struct wifi_arg_ipc_info
{
    uint32_t argc;
    uint32_t args[WIFI_API_IPC_COM_REQ_MAX_ARGC];
} wifi_api_arg_info_t;

struct wifi_api_com_req
{
    wdrv_cmd_hdr cmd_hdr;
    wifi_api_arg_info_t arg_info;
    wdrv_cmd_cfm cmd_cfm;
};

bk_err_t wifi_send_com_api_cmd(uint32_t cmd_id, uint32_t argc, ...);
bk_err_t wifi_handle_api_evt(uint32_t evt_id, uint8_t *evt_data, uint16_t evt_len);

bk_err_t bk_wifi_filter_register_ind(uint8_t * msg_payload);
bk_err_t bk_wifi_monitor_register_ind(uint8_t * msg_payload);

#ifdef __cplusplus
}
#endif

