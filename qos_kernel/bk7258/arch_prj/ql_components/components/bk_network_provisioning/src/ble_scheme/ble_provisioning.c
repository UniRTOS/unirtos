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

#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/log.h"
#include "os/mem.h"
#include "ble_provisioning_priv.h"
#if CONFIG_NET_PAN
#include "pan_service.h"
#endif

#define TAG "ble_np"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static bk_ble_provisioning_info_t *bk_ble_provisioning_info = NULL;
static bk_ble_provisioning_msg_info_t *db_info = NULL;

bk_err_t bk_ble_provisioning_send_msg(ble_prov_msg_t *msg)
{
    bk_err_t ret = BK_OK;

    if (db_info->queue)
    {
        ret = rtos_push_to_queue(&db_info->queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            LOGE("%s failed\n", __func__);
            return BK_FAIL;
        }

        return ret;
    }

    return ret;
}

bk_ble_provisioning_info_t * bk_ble_provisioning_get_boarding_info(void)
{
    return bk_ble_provisioning_info;
}

void bk_ble_provisioning_event_notify(uint16_t opcode, int status)
{
    uint8_t data[] =
    {
        opcode & 0xFF, opcode >> 8,     /* opcode           */
                              status & 0xFF,                                                          /* status           */
                              0, 0,                                                                   /* payload length   */
    };

    LOGV("%s: %d, %d\n", __func__, opcode, status);
    bk_ble_np_wifi_boarding_notify(data, sizeof(data));
}

void bk_ble_provisioning_event_notify_with_data(uint16_t opcode, int status, char *payload, uint16_t length)
{
    uint8_t data[1024] =
    {
        opcode & 0xFF, opcode >> 8,     /* opcode           */
                              status & 0xFF,                  /* status           */
                              length & 0xFF, length >> 8,     /* payload length   */
                              0,
    };

    if (length > 1024 - 5)
    {
        LOGE("size %d over flow\n", length);
        return;
    }

    os_memcpy(&data[5], payload, length);

    LOGI("%s: %d, %d\n", __func__, opcode, status);
    bk_ble_np_wifi_boarding_notify(data, length + 5);
}

void bk_ble_provisioning_operation_handle(uint16_t opcode, uint16_t length, uint8_t *data)
{
    ble_prov_msg_t msg;
    bk_err_t ret = BK_OK;

    /* Ensure that all fields are clean before populating metadata */
    os_memset(&msg, 0, sizeof(msg));
    LOGV("%s, opcode: %04X, length: %u\n", __func__, opcode, length);

    msg.event = opcode;
    /* Record payload length for consumers that need binary buffers */
    msg.length = length;

    if ((length > 0) && (data == NULL))
    {
        LOGE("%s, payload is NULL while length=%u\n", __func__, length);
        return;
    }

    if (length > 0) {
        char *payload = os_zalloc(length + 1);

        if (payload == NULL)
        {
            LOGE("%s, malloc %u bytes failed\n", __func__, length + 1);
            return;
        }

        os_memcpy(payload, data, length);
        msg.param = (uint32_t)payload;
    }
    else
        msg.param = 0;

    ret = bk_ble_provisioning_send_msg(&msg);
    if ((ret != BK_OK) && (msg.param != 0))
    {
        os_free((void *)msg.param);
    }
}

static int bk_ble_provisioning_init(void)
{
    LOGI("%s\n", __func__);

    if (bk_ble_provisioning_info == NULL)
    {
        bk_ble_provisioning_info = os_malloc(sizeof(bk_ble_provisioning_info_t));

        if (bk_ble_provisioning_info == NULL)
        {
            LOGE("bk_ble_provisioning_info malloc failed\n");

            goto error;
        }

        os_memset(bk_ble_provisioning_info, 0, sizeof(bk_ble_provisioning_info_t));
    }
    else
    {
        LOGI("%s already initialised\n", __func__);
        return BK_OK;
    }

    bk_ble_provisioning_info->ble_prov_info.cb = bk_ble_provisioning_operation_handle;

    bk_ble_np_wifi_boarding_init(&bk_ble_provisioning_info->ble_prov_info);

#if CONFIG_NET_PAN
    pan_service_init();
#endif
    return BK_OK;
error:
    return BK_FAIL;
}

static int bk_ble_provisioning_deinit(void)
{
    LOGI("%s\n", __func__);

    bk_ble_np_wifi_boarding_deinit();

    if (bk_ble_provisioning_info)
    {
        os_free(bk_ble_provisioning_info);
        bk_ble_provisioning_info = NULL;
    }

#if CONFIG_NET_PAN
    pan_service_deinit();
#endif
    return BK_OK;
}

static ble_msg_handle_cb_t ble_msg_handle_cb = NULL;

void bk_ble_provisioning_set_msg_handle_cb(ble_msg_handle_cb_t cb)
{
    ble_msg_handle_cb = cb;
}

static void bk_ble_provisioning_message_handle(void)
{
    bk_err_t ret = BK_OK;
    ble_prov_msg_t msg;

    LOGI("%s tart\n", __func__);

    while (1)
    {

        ret = rtos_pop_from_queue(&db_info->queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            LOGV("msg.event: %d, msg.param: %d, msg.length: %u\n", msg.event, msg.param, msg.length);

            if(msg.event == -1)
            {
                LOGW("%s exit evt\n", __func__);
                break;
            }

            if (ble_msg_handle_cb) {
                ble_msg_handle_cb(&msg);
            }
            else
            {
                LOGW("ble_msg_handle_cb is NULL\n");
            }
            if (msg.param) {
                os_free((void *)msg.param);
            }
        }
    }


    LOGE("%s end\n", __func__);
    rtos_delete_thread(NULL);
}

static void bk_ble_provisioning_core_init(void)
{
    bk_err_t ret = BK_OK;

    LOGI("%s start\n", __func__);

    if (db_info == NULL)
    {
        db_info = os_malloc(sizeof(bk_ble_provisioning_msg_info_t));

        if (db_info == NULL)
        {
            LOGE("%s, malloc db_info failed\n", __func__);
            goto error;
        }

        os_memset(db_info, 0, sizeof(bk_ble_provisioning_msg_info_t));
    }


    if (db_info->queue != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, db_info->queue allready init, exit!\n", __func__);
        goto error;
    }

    if (db_info->thd != NULL)
    {
        ret = BK_FAIL;
        LOGE("%s, db_info->thd allready init, exit!\n", __func__);
        goto error;
    }

    ret = rtos_init_queue(&db_info->queue,
                          "db_info->queue",
                          sizeof(ble_prov_msg_t),
                          10);

    if (ret != BK_OK)
    {
        LOGE("%s, ceate doorbell message queue failed\n");
        goto error;
    }

    ret = rtos_create_thread(&db_info->thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "db_info->thd",
                             (beken_thread_function_t)bk_ble_provisioning_message_handle,
                             4096,
                             NULL);

    if (ret != BK_OK)
    {
        LOGE("create media major thread fail\n");
        goto error;
    }

    db_info->enabled = BK_TRUE;

    LOGE("%s success\n", __func__);

    return;

error:

    LOGE("%s fail\n", __func__);
}

static void bk_ble_provisioning_core_deinit(void)
{
    bk_err_t ret = BK_OK;

    LOGI("%s start\n", __func__);

    if(db_info)
    {
        if(db_info->thd)
        {
            ble_prov_msg_t msg = {.event = -1};

            if (db_info->queue)
            {
                ret = rtos_push_to_queue(&db_info->queue, &msg, BEKEN_WAIT_FOREVER);

                if (BK_OK != ret)
                {
                    LOGE("%s push failed\n", __func__);
                    return;
                }

                rtos_thread_join(db_info->thd);
            }
            else
            {
                LOGE("%s task exist but queue not exist !!!\n", __func__);
            }
        }

        if(db_info->queue)
        {
            ret = rtos_deinit_queue(&db_info->queue);

            if (ret != kNoErr)
            {
                LOGE("%s delete message queue fail\n", __func__);
            }
        }

        os_free(db_info);
        db_info = NULL;
    }

    LOGE("%s end\n", __func__);

    return;
}

void bk_ble_np_init(void)
{
    bk_ble_provisioning_core_init();
    if(bk_bluetooth_init())
    {
        BK_LOGE(TAG, "bluetooth init err\n");
    }
#if CONFIG_NET_PAN && !CONFIG_A2DP_SINK_DEMO && !CONFIG_HFP_HF_DEMO
    bk_bt_enter_pairing_mode(0);
#else
    BK_LOGW(TAG, "%s pan disable !!!\n", __func__);
#endif

#if CONFIG_BK_BLE_PROVISIONING
    extern bool ate_is_enabled(void);

    //TODO np
    //if (!ate_is_enabled())
    if (1)
    {
        bk_ble_provisioning_init();
        bk_ble_np_wifi_boarding_adv_stop();
        bk_ble_np_wifi_boarding_adv_start();
    }
    else
    {
        BK_LOGW(TAG, "%s ATE is enable, ble will not enable!!!!!!\n", __func__);
    }
#endif
}

void bk_ble_np_deinit(void)
{
    if(!bk_ble_provisioning_info)
    {
        LOGE("%s no need deinit !!!\n", __func__);
        return;
    }

    bk_ble_np_wifi_boarding_adv_stop();
    bk_ble_provisioning_deinit();
    bk_ble_provisioning_core_deinit();

    if(bk_bluetooth_deinit())
    {
        LOGE("%s bluetooth deinit err\n", __func__);
    }
}
