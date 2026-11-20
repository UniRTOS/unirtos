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

#include "components/bluetooth/bk_dm_pan.h"

#ifdef __cplusplus
extern "C" {
#endif

bt_err_t bt_pan_register_callback_internal(bk_pan_cb_t callback);
bt_err_t bt_pan_init_internal(uint8_t role);
bt_err_t bt_pan_connect_internal(uint8_t *bda, uint8_t src_role, uint8_t dst_role);
bt_err_t bt_pan_write_internal(uint8_t *bda, eth_data_t *eth_data);
bt_err_t bt_pan_disconnect_internal(uint8_t *bda);
bt_err_t bt_pan_set_protocol_filters_internal(uint8_t *bda, np_type_filter_t *np_type);


#ifdef __cplusplus
}
#endif
