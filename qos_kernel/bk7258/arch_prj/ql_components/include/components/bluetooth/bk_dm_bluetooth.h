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

#ifdef __cplusplus
extern"C" {
#endif

/**
 * @defgroup dm_bluetooth BLUETOOTH API
 * @{
 */

/**
 * @brief     Get bluetooth status
 *
 * @return    Bluetooth status
 *
 */
bk_bluetooth_status_t bk_bluetooth_get_status(void);

/**
 *
 * @brief           init bluetooth.
 *
 *
 * @return
 *                  - BT_OK: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bluetooth_init(void);

/**
 *
 * @brief           deinit bluetooth.
 *
 *
 * @return
 *                  - BT_OK: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bluetooth_deinit(void);


/**
 *
 * @brief      Get bluetooth device address.  Must use after "bk_bluetooth_init".
 *
 * @param[out]      addr - bluetooth device address
 *
 * @return
 *                  - BT_OK: success
 *                  -  others: fail
 */
bt_err_t bk_bluetooth_get_address(uint8_t *addr);

/**
 * @brief  register hci callback for dual mode host only
 *
 * @param
 *    - cb: hci callback function used to recv hci data from host
 *
 * @attention used for dual mode host only
 *
 * @return
 *    - BT_OK: succeed
 *    - others: other errors.
 */
bt_err_t bk_dual_host_register_hci_callback(dual_hci_to_cp_cb cb);

/**
 * @brief send hci data to host.
 *
 * @param
 * - buf: payload
 * - len: buf's len
 *
 * @attention used for dual mode host only
 *
 * @return
 * - BT_OK: succeed
**/
bt_err_t bk_dual_hci_send_to_host(uint8_t *buf, uint32_t len);

/**
 * @brief reg secondary controller.
 *
 * @param
 * - cb: callback
 *
 * @attention used for dual mode host only
 *
 * @return
 * - BT_OK: succeed
**/
bt_err_t bk_bluetooth_reg_secondary_controller(bk_bluetooth_secondary_callback_t *cb);

/**
 * @brief init h5 module.
 *
 * @param
 * - cb: callback
 *
 * @attention this module only encode/decode h5 package
 *
 * @return
 * - BT_OK: succeed
**/
int32_t bk_bluetooth_h5_init(bluetooth_h5_cb_t *cb);

/**
 * @brief deinit h5 module.
 *
 * @attention this module only encode/decode h5 package
 *
 * @return
 * - BT_OK: succeed
**/
int32_t bk_bluetooth_h5_deinit(void);

/**
 * @brief enable h5 module.
 *
 * @param
 * - enable: enable or disable
 *
 *
 * @return
 * - BT_OK: succeed
**/
int32_t bk_bluetooth_h5_set_h5_enable(uint8_t enable);

/**
 * @brief get module enable status.
 *
 *
 * @return
 * - BT_OK: succeed
**/
uint8_t bk_bluetooth_h5_get_h5_enable(void);

/**
 * @brief encode h4 data to h5 and send.
 *
 * @param
 * - payload: hci data payload
 * - type: cmd/evt/acl/sco/iso
 * - len: payload len
 *
 * @attention when encode completed, bluetooth_h5_cb_t->encode_data_cb will be called.
 *
 * @return
 * - BT_OK: succeed
**/
int8_t bk_bluetooth_h5_send(const uint8_t *payload, uint8_t type, uint16_t len);

/**
 * @brief decode h5 data.
 *
 * @param
 * - payload: h5 data payload
 * - count: data len
 *
 * @attention when decode completed, bluetooth_h5_cb_t->decode_data_ext_cb will be called.
 *
 * @return
 * - BT_OK: succeed
**/
int32_t bk_bluetooth_h5_recv_data(uint8_t *data, uint32_t count);

/**
 * @brief send h5 sync/config to peer.
 *
 * @param
 * - win_size: win size
 * - fc: flow control, no need to set
 * - dic: if enable crc check
 *
 * @attention when recv nego completed from peer, bluetooth_h5_cb_t->notify_h5_nego_completed will be called.
 *
 * @return
 * - BT_OK: succeed
**/
int8_t bk_bluetooth_h5_send_sync_req(uint8_t win_size, uint8_t fc, uint8_t dic);

/**
 * @brief check if have unack packaged in h5 queue.
 *
 *
 * @attention This function must be called in bluetooth_h5_cb_t->notify_tx_ack_tout_cb context.
 *
 * @return
 * - BT_OK: succeed
**/
int32_t bk_bluetooth_h5_check_retran(void);

///@}

#ifdef __cplusplus
}
#endif
