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

#include "bk_dm_bluetooth_types.h"
#include "bk_dm_pan_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dm_pan PAN API
 * @{
 */

/**
 *
 * @brief           Initialize the bluetooth PAN module
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_pan_init(uint8_t role);

/**
 * @brief           Register application callbacks to AVRCP module.
 *
 * @param[in]       callback: AVRCP controller callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_bt_pan_register_callback(bk_pan_cb_t callback);

/**
 *
 * @brief           Connect to remote PAN device. This API must be called
 *                  after bk_bt_pan_init()
 *
 * @param[in]       addr: remote bluetooth device address
 *
 * @param[in]       src_role: local pan role
 *
 * @param[in]       dst_role: remote pan role
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_pan_connect(uint8_t *bda, uint8_t src_role, uint8_t dst_role);

/**
 *
 * @brief           send Ethernet data to remote device over bt pan. This API must be called
 *                  after bk_bt_pan_init()
 *
 * @param[in]       addr: remote bluetooth device address
 *
 * @param[in]       eth_data: Ethernet data
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: write request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_pan_write(uint8_t *bda, eth_data_t *eth_data);

/**
 *
 * @brief           Disconnect to remote PAN device. This API must be called
 *                  after bk_bt_pan_init()
 *
 * @param[in]       addr: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: disconnect request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_pan_disconnect(uint8_t *bda);

/**
 *
 * @brief           filter network protocol type to remote PAN device. This API must be called
 *                  after bk_bt_pan_init()
 *
 * @param[in]       addr: remote bluetooth device address
 *
 * @param[in]       np_type: network Protocol filters parameters
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: the filter request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_pan_set_protocol_filters(uint8_t *bda, np_type_filter_t *np_type);

///@}

#ifdef __cplusplus
}
#endif


