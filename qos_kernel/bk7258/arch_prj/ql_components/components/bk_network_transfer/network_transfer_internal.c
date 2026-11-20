#include <os/mem.h>
#include <os/os.h>
#include <components/log.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>

#include "network_transfer_internal.h"
#include "network_transfer.h"
#include "network_type.h"
#include "ntwk_pack.h"

#define TAG "ntwk-trans"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static ntwk_in_cfg_t *ntwk_in_cfg = NULL;

bk_err_t ntwk_msg_send(ntwk_trans_event_t *msg)
{
    bk_err_t ret = BK_OK;

    if (ntwk_in_cfg && ntwk_in_cfg->queue)
    {
        ret = rtos_push_to_queue(&ntwk_in_cfg->queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            LOGE("%s failed\n", __func__);
            return BK_FAIL;
        }

        return ret;
    }

    return ret;
}

static void ntwk_msg_message_handle(void)
{
    bk_err_t ret = BK_OK;
    ntwk_trans_event_t msg;

    while (1)
    {
        // Check if queue is still valid
        if (ntwk_in_cfg == NULL || ntwk_in_cfg->queue == NULL)
        {
            LOGD("%s, queue is NULL, exiting\n", __func__);
            break;
        }

        ret = rtos_pop_from_queue(&ntwk_in_cfg->queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            // Check if this is an exit signal (queue is being deinitialized)
            if (ntwk_in_cfg == NULL || ntwk_in_cfg->queue == NULL)
            {
                LOGD("%s, queue deinitialized, exiting\n", __func__);
                break;
            }

            switch (msg.code)
            {
                case NTWK_TRANS_EVT_START:
                case NTWK_TRANS_EVT_CONNECTED:
                case NTWK_TRANS_EVT_DISCONNECTED:
                case NTWK_TRANS_EVT_STOP:
                {
                    if ((msg.code == NTWK_TRANS_EVT_DISCONNECTED) ||
                        (msg.code == NTWK_TRANS_EVT_STOP))
                    {
                        ntwk_pack_clear_ccount(msg.chan_type);
                    }

                    // Call user registered event callback
                    if (ntwk_in_cfg && ntwk_in_cfg->event_cb != NULL)
                    {
                        LOGV("%s, event:%d, param:%d, chan_type:%d\n", __func__, msg.code, msg.param, msg.chan_type);
                        // Convert internal msg format to ntwk_trans_event_t
                        ntwk_trans_event_t event;
                        event.chan_type = msg.chan_type;
                        event.code = msg.code;
                        event.param = msg.param;
                        ntwk_in_cfg->event_cb(&event);
                    }
                    else
                    {
                        LOGW("%s, no event callback registered for event:%d\n", __func__, msg.code);
                    }
                }
                break;

                default:
                    LOGW("%s, unknown event:%d\n", __func__, msg.code);
                    break;
            }
        }
        else
        {
            // Queue error (queue deleted or invalid), exit the thread
            LOGD("%s, queue pop error (ret:%d), exiting\n", __func__, ret);
            break;
        }
    }

    LOGD("%s exit\n", __func__);
    rtos_delete_thread(NULL);
}


bk_err_t ntwk_msg_start(void)
{
    bk_err_t ret = BK_OK;

    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_trans_msg_info is NULL\n", __func__);
        return BK_FAIL;
    }

    ret = rtos_init_queue(&ntwk_in_cfg->queue,
                          "ntwk_trans_msg_info->queue",
                          sizeof(ntwk_trans_event_t),
                          20);

    if (ret != BK_OK)
    {
        LOGE("%s, create network message queue failed\n", __func__);
        goto error;
    }

    ret = rtos_create_thread(&ntwk_in_cfg->thd,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "ntwk_trans_msg_info->thd",
                             (beken_thread_function_t)ntwk_msg_message_handle,
                             1024 * 4,
                             NULL);

    if (ret != BK_OK)
    {
        LOGE("create network message thread fail\n");
        goto error;
    }

    LOGV("%s success\n", __func__);
    return BK_OK;

error:
    ntwk_msg_stop();
    return ret;
}

bk_err_t ntwk_msg_stop(void)
{
    bk_err_t ret = BK_OK;

    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_trans_msg_info is NULL\n", __func__);
        return BK_FAIL;
    }

    /* clear event callback first */
    ntwk_in_cfg->event_cb = NULL;

    /* delete message queue - this will wake up the thread waiting on rtos_pop_from_queue */
    if (ntwk_in_cfg->queue)
    {
        ret = rtos_deinit_queue(&ntwk_in_cfg->queue);
        if (ret != kNoErr)
        {
            LOGE("delete message queue fail\n");
        }
        ntwk_in_cfg->queue = NULL;
    }

    /* wait for thread to exit naturally */
    if (ntwk_in_cfg->thd)
    {
        rtos_thread_join(ntwk_in_cfg->thd);
        ntwk_in_cfg->thd = NULL;
    }

    LOGV("%s complete\n", __func__);
    return BK_OK;
}

bk_err_t ntwk_msg_init(void)
{
    bk_err_t ret = BK_OK;

    if (ntwk_in_cfg != NULL)
    {
        LOGE("%s, ntwk_trans_msg_info already init\n", __func__);
        return BK_OK;
    }

    ntwk_in_cfg = ntwk_malloc(sizeof(ntwk_in_cfg_t));

    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, malloc ntwk_trans_msg_info failed\n", __func__);
        return BK_FAIL;
    }

    os_memset(ntwk_in_cfg, 0, sizeof(ntwk_in_cfg_t));

    return ret;
}

bk_err_t ntwk_msg_deinit(void)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGW("%s, ntwk_trans_msg_info is NULL\n", __func__);
        return BK_OK;
    }

    os_free(ntwk_in_cfg);
    ntwk_in_cfg = NULL;

    return BK_OK;
}

void ntwk_msg_event_report(uint32_t event, uint32_t param, uint32_t chan_type)
{
    ntwk_trans_event_t msg;

    msg.code = event;
    msg.param = param;
    msg.chan_type = chan_type;

    bk_err_t ret = ntwk_msg_send(&msg);

    if (ret != BK_OK)
    {
        LOGE("%s, failed to send event:%d, chan_type:%d\n", __func__, event, chan_type);
    }
}

bk_err_t ntwk_msg_register_event_cb(ntwk_trans_msg_event_cb_t cb)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_trans_msg_info is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    ntwk_in_cfg->event_cb = cb;

    return BK_OK;
}

bk_err_t ntwk_in_register_ctrl_start_cb(ntwk_in_start_cb_t cb)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }

    ntwk_in_cfg->ctrl_start = cb;

    return BK_OK;
}

bk_err_t ntwk_in_register_ctrl_stop_cb(ntwk_in_stop_cb_t cb)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }

    ntwk_in_cfg->ctrl_stop = cb;

    return BK_OK;
}

bk_err_t ntwk_in_register_video_start_cb(ntwk_in_start_cb_t cb)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }

    ntwk_in_cfg->video_start = cb;

    return BK_OK;
}

bk_err_t ntwk_in_register_video_stop_cb(ntwk_in_stop_cb_t cb)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }

    ntwk_in_cfg->video_stop = cb;

    return BK_OK;
}

bk_err_t ntwk_in_register_audio_start_cb(ntwk_in_start_cb_t cb)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }

    ntwk_in_cfg->audio_start = cb;

    return BK_OK;
}

bk_err_t ntwk_in_register_audio_stop_cb(ntwk_in_stop_cb_t cb)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }

    ntwk_in_cfg->audio_stop = cb;

    return BK_OK;
}

bk_err_t ntwk_in_start(chan_type_t chan_type, void *param)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }

    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            return ntwk_in_cfg->ctrl_start(param);
        }
        case NTWK_TRANS_CHAN_VIDEO:
        {
            return ntwk_in_cfg->video_start(param);
        }
        case NTWK_TRANS_CHAN_AUDIO:
        {
            return ntwk_in_cfg->audio_start(param);
        }
        default:
        {
            LOGE("%s, invalid channel type: %d\n", __func__, chan_type);
            return BK_FAIL;
        }
    }

    return BK_FAIL;
}

bk_err_t ntwk_in_stop(chan_type_t chan_type)
{
    if (ntwk_in_cfg == NULL)
    {
        LOGE("%s, ntwk_in_cfg is NULL\n", __func__);
        return BK_FAIL;
    }
    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            return ntwk_in_cfg->ctrl_stop();
        }
        case NTWK_TRANS_CHAN_VIDEO:
        {
            return ntwk_in_cfg->video_stop();
        }
        case NTWK_TRANS_CHAN_AUDIO:
        {
            return ntwk_in_cfg->audio_stop();
        }
        default:
        {
            LOGE("%s, invalid channel type: %d\n", __func__, chan_type);
            return BK_FAIL;
        }
    }

    return BK_FAIL;
}