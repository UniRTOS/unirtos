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

#ifndef __BLE_PROVISIONING_PRIV_H__
#define __BLE_PROVISIONING_PRIV_H__

#include <stdint.h>
#include "ble_provisioning.h"

enum
{
    BOARDING_DEBUG_LEVEL_ERROR,
    BOARDING_DEBUG_LEVEL_WARNING,
    BOARDING_DEBUG_LEVEL_INFO,
    BOARDING_DEBUG_LEVEL_DEBUG,
    BOARDING_DEBUG_LEVEL_VERBOSE,
};

#define BOARDING_DEBUG_LEVEL_INFO BOARDING_DEBUG_LEVEL_INFO

#define wboard_loge(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_ERROR)   BK_LOGE("bk-gen", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logw(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_WARNING) BK_LOGW("bk-gen", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logi(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_INFO)    BK_LOGI("bk-gen", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logd(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_DEBUG)   BK_LOGI("bk-gen", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logv(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_VERBOSE) BK_LOGI("bk-gen", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

typedef struct
{
    uint32_t enabled : 1;
    uint32_t service : 6;

    char *id;
    beken_thread_t thd;
    beken_queue_t queue;
} bk_ble_provisioning_msg_info_t;

int bk_ble_np_wifi_boarding_init(ble_provisioning_info_t *info);
int bk_ble_np_wifi_boarding_deinit(void);
int bk_ble_np_wifi_boarding_adv_start(void);
int bk_ble_np_wifi_boarding_adv_stop(void);
int bk_ble_np_wifi_boarding_notify(uint8_t *data, uint16_t length);

void bk_ble_np_init(void);
void bk_ble_np_deinit(void);

#endif
