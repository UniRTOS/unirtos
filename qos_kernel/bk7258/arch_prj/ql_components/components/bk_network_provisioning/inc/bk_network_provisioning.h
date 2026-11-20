// Copyright 2020-2025 Beken
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

#ifndef __BK_NETWORK_PROVISIONING_H__
#define __BK_NETWORK_PROVISIONING_H__

#include <common/bk_include.h>
#include <modules/wifi_types.h>
#include <components/netif_types.h>
#if CONFIG_BK_BLE_PROVISIONING
#include "ble_provisioning.h"
#endif

typedef enum
{
    BK_NETWORK_PROVISIONING_TYPE_BLE = 0,
    BK_NETWORK_PROVISIONING_TYPE_AIRKISS = 1,
    BK_NETWORK_PROVISIONING_TYPE_WEB = 2,
    BK_NETWORK_PROVISIONING_TYPE_CONSOLE = 4,
    BK_NETWORK_PROVISIONING_TYPE_MAX,
} bk_network_provisioning_type_t;

typedef enum
{
    BK_NETWORK_PROVISIONING_STATUS_IDLE = 0,
    /*for network provisioning*/
    BK_NETWORK_PROVISIONING_STATUS_RUNNING = 1,
    BK_NETWORK_PROVISIONING_STATUS_SUCCEED = 2,
    BK_NETWORK_PROVISIONING_STATUS_FAILED = 3,
    /*for network reconnect after hetwork provisioning succeed*/
    BK_NETWORK_PROVISIONING_STATUS_RECONNECTING = 4,
    BK_NETWORK_PROVISIONING_STATUS_RECONNECT_SUCCEED = 5,
    BK_NETWORK_PROVISIONING_STATUS_RECONNECT_FAILED = 6,
    BK_NETWORK_PROVISIONING_STATUS_MAX,
} bk_network_provisioning_status_t;

typedef struct bk_fast_connect_d
{
    uint8_t sta_ssid[33];
    uint8_t sta_pwd[65];
    uint8_t ap_ssid[33];
    uint8_t ap_pwd[65];
    uint8_t ap_channel;
    uint16_t flag;		//to check if netif_if_t is configed, default 0
    uint8_t p2p_dev_name[33];
    uint8_t p2p_go_intent;	/* P2P GO intent (1-15), 0 = use default 15 on reconnect */
}BK_FAST_CONNECT_D;

#define BK_NW_PRO_P2P_GO_INTENT_DEFAULT 15

typedef void (*network_provisioning_status_cb_t)(bk_network_provisioning_status_t status, void *user_data);

/**
 * @brief       get supported network type, WiFi, 4G, BT PAN
 *
 * @return      supported network type, 0: WiFi, 1: 4G, 2: BT PAN
 */
uint8_t * bk_sconf_get_supported_network(uint8_t *len);

/**
 * @brief       get network provisioning type
 *
 * @return      network provisioning type
 */
bk_network_provisioning_type_t bk_network_provisioning_get_type(void);

/**
 * @brief       register network provisioning status callback, to receive network provisioning status
 *
 * @param       cb: callback function
 *
 * @return      BK_OK: success, others: fail
 */
bk_err_t bk_register_network_provisioning_status_cb(network_provisioning_status_cb_t cb);

/**
 * @brief       unregister network provisioning status callback
 *
 * @return      BK_OK: success, others: fail
 */
bk_err_t bk_unregister_network_provisioning_status_cb(void);

/**
 * @brief       start network provisioning, used when user click provisioning button
 *
 * @param       type: network provisioning type
 *
 * @return      BK_OK: success, others: fail
 */
bk_err_t bk_network_provisioning_start(bk_network_provisioning_type_t type);

/**
 * @brief       init network provisioning, used when system boot up
 *
 * example:
 *      projects/app/ap/ap_main.c
 *      void ap_main(void)
 *      {
 *          ......
 *          bk_network_provisioning_init(BK_NETWORK_PROVISIONING_TYPE_BLE);
 *      }
 *
 * @param       default_type: default network provisioning type
 *
 * @return      BK_OK: success, others: fail
 */
bk_err_t bk_network_provisioning_init(bk_network_provisioning_type_t default_type);

/**
 * @brief       erase network auto reconnect info, used when user click erase button
 *
 * @return      none
 */
void erase_network_auto_reconnect_info(void);

/**
 * @brief       get np send method
 *
 * @param       send: send op
 * @param       send_with_data: send op with data
 *
 * @return      none
 */
void bk_network_provisioning_get_send_cb(
    void (**send)(uint16_t opcode, int status),
    void (**send_with_data)(uint16_t opcode, int status, char *payload, uint16_t length)
);

#if CONFIG_BK_BLE_PROVISIONING
/**
 * @brief       set ble msg handle callback, for user to handle their own msg
 *
 * @param       cb: callback function
 *
 * @return      none
 */
void bk_ble_provisioning_set_msg_handle_cb(ble_msg_handle_cb_t cb);
#endif

#endif
