// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



#pragma once

#include "components/bluetooth/bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dm_pan_types PAN DEFINES
 * @{
 */
 
/// PAN connection state
enum
{
    BK_BTPAN_STATE_DISCONNECTED = 0,                /*!< disconnected */
    BK_BTPAN_STATE_CONNECTED = 1,                /*!< connected */
};

/// PAN role
enum
{
    BK_PAN_ROLE_NONE = 0x00,               /* none role */
    BK_PAN_ROLE_PANU = 0x01,               /* PANU role */
    BK_PAN_ROLE_GN = 0x02,                /* GN role */
    BK_PAN_ROLE_NAP = 0x04,                /* NAP role */
};

/* Define the result codes from PAN */
enum
{
    BK_PAN_SUCCESS = 0,                                                /* Success      */
    BK_PAN_FAIL,                                                       /*  failed       */
};

/// PAN callback events
typedef enum
{
    BK_PAN_CONNECTION_STATE_EVT = 0,            /*!< connection state changed event */
    BK_PAN_DATA_READY_EVT,          /*!< pan data received event */
    BK_PAN_WRITE_DATA_CNF_EVT,          /*!< pan data received event */
} bk_pan_cb_event_t;

/// Ethernet Data
typedef struct
{
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t protocol;
    uint16_t payload_len;
    uint8_t payload[0];
} eth_data_t;


/// Network Protocol filters parameters
typedef struct
{
    uint16_t num_filters;        /*!< Number of Filter Ranges */
    uint16_t start[10];            /*! Ethernet Network Type Start */
    uint16_t end[10];              /*! Ethernet Network Type End */
} np_type_filter_t;

/// PAN callback parameters
typedef union
{
    /**
     * @brief BK_PAN_CONNECTION_STATE_EVT
     */
    struct pan_open_state_param
    {
        uint8_t con_state;                       /*!< connection state*/
        uint8_t status;                          /*!< status*/
        uint8_t remote_bda[6];                   /*!< remote bluetooth device address */
    } conn_state;                                /*!< PAN connection state */

    /**
     * @brief BK_PAN_DATA_READY_EVT
     */
    struct pan_data_ready_param
    {
        uint8_t remote_bda[6];                   /*!< remote bluetooth device address */
        eth_data_t *eth_data;                     /*!< Ethernet Data */
    } pan_data;                                /*!< PAN data ind */

    /**
     * @brief BK_PAN_WRITE_DATA_CNF_EVT
     */
    struct pan_write_data_cnf_param
    {
        uint8_t remote_bda[6];                   /*!< remote bluetooth device address */
    } write_cnf;                                /*!< PAN write data cnf */

} bk_pan_cb_param_t;

/**
 * @brief           PAN callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter union
 */
typedef void (* bk_pan_cb_t)(bk_pan_cb_event_t event, bk_pan_cb_param_t *param);

///@}

#ifdef __cplusplus
}
#endif

