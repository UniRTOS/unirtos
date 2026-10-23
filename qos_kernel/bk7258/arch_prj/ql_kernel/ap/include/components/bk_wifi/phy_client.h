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

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize PHY driver client
 *
 * This API initializes the PHY driver client, including:
 *  - Initialize IPC communication
 *  - Create mutex for thread safety
 *  - Establish connection with PHY server
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_phy_driver_init(void);

/**
 * @brief Deinitialize PHY driver client
 *
 * This API deinitializes the PHY driver client
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_phy_driver_deinit(void);

/**
 * @brief Get MAC Address
 *
 * This API to get mac addr
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_ap_get_mac(uint8_t *mac, mac_type_t type);
/**
 * @brief Get Sta MAC Address
 *
 * This API to get sta mac addr
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_ap_wifi_sta_get_mac(uint8_t *mac);
/**
 * @brief Get AP MAC Address
 *
 * This API to get ap mac addr
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_ap_wifi_ap_get_mac(uint8_t *mac);
/**
 * @brief Set MAC Address
 *
 * This API to set mac addr
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_ap_set_base_mac(uint8_t *mac);

#ifdef __cplusplus
}
#endif