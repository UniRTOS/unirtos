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
#if CONFIG_BK_RAW_LINK
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "os/os.h"
#include <modules/raw_link.h>

#define RLKD_TAG "RLK_DRV"
#define RLKD_LOGI(...)       BK_LOGI(RLKD_TAG, ##__VA_ARGS__)
#define RLKD_LOGW(...)       BK_LOGW(RLKD_TAG, ##__VA_ARGS__)
#define RLKD_LOGE(...)       BK_LOGE(RLKD_TAG, ##__VA_ARGS__)
#define RLKD_LOGD(...)       BK_LOGD(RLKD_TAG, ##__VA_ARGS__)
#define RLKD_LOGV(...)       BK_LOGV(RLKD_TAG, ##__VA_ARGS__)
#define RLKD_LOG_RAW(...)    BK_LOG_RAW(RLKD_TAG, ##__VA_ARGS__)


#define RLKD_QUEUE_LEN                          192
#define RLKD_TASK_PRIO                          2
#define RLKD_TX_MEM_LIMIT_BYTES                 (50 * 1024)

/**
 * WIFI_API_MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> #define WIFI_API_MEM_ALIGNMENT 4
 *    2 byte alignment -> #define WIFI_API_MEM_ALIGNMENT 2
 */
#define RLKD_API_MEM_ALIGNMENT                   4

/** Calculate memory size for an aligned buffer - returns the next highest
 * multiple of WIFI_API_MEM_ALIGNMENT (e.g. LWIP_MEM_ALIGN_SIZE(3) and
 * LWIP_MEM_ALIGN_SIZE(4) will both yield 4 for WIFI_API_MEM_ALIGNMENT == 4).
 */
#define RLKD_API_MEM_ALIGN_SIZE(size) (((size) + RLKD_API_MEM_ALIGNMENT - 1U) & ~(RLKD_API_MEM_ALIGNMENT-1U))

enum rlkd_task_msg_evt
{
    RLKD_TASK_MSG_SEND_CB = 1,
    RLKD_TASK_MSG_SEND_EX_CB = 2,
    RLKD_TASK_MSG_RECV_CB = 3,
};

struct rlkd_msg {
    uint32_t msg_id;
    uint32_t arg;
    uint32_t len;
    void *cb;
    void *param;
};

struct rlkd_env_t
{
    void *io_queue;
    void *handle;
    uint32_t is_init;
    beken_mutex_t mem_lock;
    uint32_t tx_mem_in_use;
    uint32_t tx_mem_limit;
    uint8_t mem_lock_init;
};

extern struct rlkd_env_t rlkd_env;


bk_err_t bk_rlk_send_register_ind(uint8_t * msg_payload);
bk_err_t bk_rlk_send_ex_register_ind(uint8_t * msg_payload);
bk_err_t bk_rlk_recv_register_ind(uint8_t * msg_payload);
bk_err_t bk_rlk_acs_cfm_register_ind(uint8_t * msg_payload);
bk_err_t bk_rlk_scan_cfm_register_ind(uint8_t * msg_payload);

bk_err_t rlkd_msg_send_cb_ind(const uint8_t *peer_mac_addr, bk_rlk_send_status_t status);
bk_err_t rlkd_msg_send_ex_cb_ind(void *args, bool status);
bk_err_t rlkd_msg_recv_cb_ind(bk_rlk_recv_info_t *rx_info);
bk_err_t ap_rlk_drv_init(void);
bk_err_t rlkd_deinit(void);
bk_err_t rlkd_handle_free_mem_req(uint32_t mem_addr);
#ifdef __cplusplus
}
#endif

#endif // CONFIG_BK_RAW_LINK
