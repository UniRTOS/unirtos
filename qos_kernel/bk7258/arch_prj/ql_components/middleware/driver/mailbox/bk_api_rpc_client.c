// Copyright 2025 Beken
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

#include <os/mem.h>
#include <os/os.h>
#include <string.h>
#include <common/bk_err.h>
#include "bk_api_rpc.h"
#include "bk_api_ipc.h"



#define TAG "bk_rpc"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


BK_IPC_CHANNEL_DEF(bk_api_rpc);
BK_IPC_CHANNEL_REGISTER(bk_api_rpc, IPC_ROUTE_CPU0_CPU1, NULL, NULL, NULL);

static beken_mutex_t s_bk_api_rpc_mutex = NULL;

bk_err_t bk_api_rpc_init(void)
{
    bk_err_t ret;

    // Initialize mutex if not already initialized
    if (s_bk_api_rpc_mutex == NULL) {
        ret = rtos_init_mutex(&s_bk_api_rpc_mutex);
        if (ret != BK_OK) {
            LOGE("%s failed to init mutex, ret=%d\n", __func__, ret);
            return ret;
        }
    }
    return BK_OK;
}

bk_err_t bk_api_rpc_get_uid(unsigned char data[32])
{
    bk_err_t ret;
    uint32_t result = 0;

    if (data == NULL) {
        LOGE("%s invalid parameter: data is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    if (s_bk_api_rpc_mutex == NULL) {
        ret = bk_api_rpc_init();
        if (ret != BK_OK) {
            LOGE("%s failed to init rpc, ret=%d\n", __func__, ret);
            return ret;
        }
    }

    bk_api_rpc_get_uid_msg_t proxy = {0};
    proxy.event = BK_API_RPC_EVENT_GET_CHIP_UID;
    proxy.out_para_size = BK_UID_SIZE;

    ret = rtos_lock_mutex(&s_bk_api_rpc_mutex);
    if (ret != BK_OK) {
        LOGE("%s failed to lock mutex, ret=%d\n", __func__, ret);
        return ret;
    }

    ret = bk_ipc_send(&bk_api_rpc, &proxy, sizeof(proxy), MIPC_CHAN_SEND_FLAG_SYNC, &result);
    if (ret == BK_OK) {
        // Copy the UID data from the response buffer to user's buffer
        memcpy(data, proxy.out_para, BK_UID_SIZE);
    }

    rtos_unlock_mutex(&s_bk_api_rpc_mutex);

    return ret;
}






