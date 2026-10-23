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

#include <common/sys_config.h>
#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief  Send customer command
 *
 * @param cmd_id    command id
 * @param data    command data
 * @param len    length of the command data
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_FAIL: customer command fail.
 *    - others: other errors
 */
bk_err_t bk_wdrv_customer_transfer(uint16_t cmd_id, uint8_t *data, uint16_t len);

/**
 * @brief Send customer command to CP and wait for response
 *
 * @param cmd_id Customer command ID
 * @param data Command data (can be NULL if len is 0)
 * @param len Command data length
 * @param response_buf Buffer to store response data
 * @param response_buf_size Size of response buffer
 * @param response_len Output parameter: actual response length received
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_PARAM: invalid parameters
 *    - BK_ERR_NO_MEM: memory allocation failed
 *    - BK_ERR_TIMEOUT: command timeout
 *    - others: other errors
 */
bk_err_t bk_wdrv_customer_transfer_rsp(uint16_t cmd_id, uint8_t *data, uint16_t len,
                                                  uint8_t *response_buf, uint16_t response_buf_size, 
                                                  uint16_t *response_len);

#ifdef __cplusplus
}
#endif
