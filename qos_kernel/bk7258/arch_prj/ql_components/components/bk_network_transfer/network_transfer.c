#include <os/mem.h>
#include <os/os.h>
#include <components/log.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>

#include "network_transfer.h"
#include "network_type.h"
#include "network_transfer_internal.h"
#include "video_drop.h"
#include "ntwk_pack.h"
#include "ntwk_fragmentation.h"

#define TAG "ntwk-trans"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static ntwk_trans_ctxt_t *s_ntwk_trans_ctxt = NULL;
#if CONFIG_NTWK_CLIENT_SERVICE_ENABLE
static ntwk_server_net_info_t s_ntwk_server_net_info = {0};
#endif

ntwk_trans_ctxt_t *ntwk_trans_get_ctxt(void)
{
	return s_ntwk_trans_ctxt;
}

const char *ntwk_trans_get_service_name(void)
{
	if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
	{
		LOGE("%s, context not initialized\n", __func__);
		return NULL;
	}

	return s_ntwk_trans_ctxt->service_name;
}

bk_err_t ntwk_trans_register_msg_event_cb(ntwk_trans_msg_event_cb_t cb)
{
    return ntwk_msg_register_event_cb(cb);
}

int ntwk_trans_ctrl_recv_handler(uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan == NULL)
    {
        LOGE("%s, control channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->unpack != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->unpack(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->unfragment != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->unfragment(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->recive != NULL)
    {
        return s_ntwk_trans_ctxt->cntrl_chan->recive(data, length);
    }

    return BK_FAIL;
}

int ntwk_trans_video_recv_handler(uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan == NULL)
    {
        LOGE("%s, video channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan->unpack != NULL)
    {
        if(s_ntwk_trans_ctxt->video_chan->unpack(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->video_chan->unfragment != NULL)
    {
        if(s_ntwk_trans_ctxt->video_chan->unfragment(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->video_chan->recive != NULL)
    {
        return s_ntwk_trans_ctxt->video_chan->recive(data, length);
    }

    return BK_FAIL;
}

int ntwk_trans_audio_recv_handler(uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan == NULL)
    {
        LOGE("%s, audio channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan->unpack != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->unpack(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->audio_chan->unfragment != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->unfragment(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->audio_chan->recive != NULL)
    {
        return s_ntwk_trans_ctxt->audio_chan->recive(data, length);
    }

    return BK_FAIL;
}

int ntwk_trans_fragment_rx_handler(chan_type_t chan, uint8_t *data, uint32_t length)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;

    switch (chan)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (s_ntwk_trans_ctxt->cntrl_chan && s_ntwk_trans_ctxt->cntrl_chan->send != NULL)
            {
                if (s_ntwk_trans_ctxt->cntrl_chan->pack != NULL)
                {
                  if(s_ntwk_trans_ctxt->cntrl_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
                  {
                    (s_ntwk_trans_ctxt->cntrl_chan->send)(pack_ptr, pack_ptr_length);
                    break;
                  }
                }

                (s_ntwk_trans_ctxt->cntrl_chan->send)(data, length);
            }
        }break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (s_ntwk_trans_ctxt->video_chan && s_ntwk_trans_ctxt->video_chan->send != NULL)
            {
                if (s_ntwk_trans_ctxt->video_chan->pack != NULL)
                {
                    if(s_ntwk_trans_ctxt->video_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
                    {
                        (s_ntwk_trans_ctxt->video_chan->send)(pack_ptr, pack_ptr_length, s_ntwk_trans_ctxt->video_chan->vid_type);
                        break;
                    }
                }
                (s_ntwk_trans_ctxt->video_chan->send)(data, length, s_ntwk_trans_ctxt->video_chan->vid_type);
            }
        }break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (s_ntwk_trans_ctxt->audio_chan && s_ntwk_trans_ctxt->audio_chan->send != NULL)
            {
                if (s_ntwk_trans_ctxt->audio_chan->pack != NULL)
                {
                    if(s_ntwk_trans_ctxt->audio_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
                    {
                        (s_ntwk_trans_ctxt->audio_chan->send)(pack_ptr, pack_ptr_length, s_ntwk_trans_ctxt->audio_chan->aud_type);
                        break;
                    }
                }
                (s_ntwk_trans_ctxt->audio_chan->send)(data, length, s_ntwk_trans_ctxt->audio_chan->aud_type);
            }
        }break;
        default:
            LOGE("%s, invalid channel type: %d\n", __func__, chan);
            return BK_ERR_PARAM;
    }

    return BK_OK;
}

bk_err_t ntwk_trans_ctxt_init(ntwk_trans_ctxt_t *ctxt)
{
    LOGV("%s start\r\n", __func__);
    if (s_ntwk_trans_ctxt != NULL)
    {
        LOGW("%s, context already initialized\n", __func__);
        return BK_FAIL;
    }

    if (ctxt != NULL)
    {
        LOGE("%s, context is NULL\n", __func__);
        os_memcpy(s_ntwk_trans_ctxt, ctxt, sizeof(ntwk_trans_ctxt_t));
    }
    else
    {
        s_ntwk_trans_ctxt = ntwk_malloc(sizeof(ntwk_trans_ctxt_t));
        if (s_ntwk_trans_ctxt == NULL)
        {
            LOGE("malloc ctxt failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt, 0, sizeof(ntwk_trans_ctxt_t));

        s_ntwk_trans_ctxt->cntrl_chan = ntwk_malloc(sizeof(ntwk_trans_ctrl_chan_t));
        if (s_ntwk_trans_ctxt->cntrl_chan == NULL)
        {
            LOGE("malloc cntrl_chan failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt->cntrl_chan, 0, sizeof(ntwk_trans_ctrl_chan_t));
        
        s_ntwk_trans_ctxt->video_chan = ntwk_malloc(sizeof(ntwk_trans_video_chan_t));
        if (s_ntwk_trans_ctxt->video_chan == NULL)
        {
            LOGE("malloc video_chan failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt->video_chan, 0, sizeof(ntwk_trans_video_chan_t));
        
        s_ntwk_trans_ctxt->audio_chan = ntwk_malloc(sizeof(ntwk_trans_audio_chan_t));
        if (s_ntwk_trans_ctxt->audio_chan == NULL)
        {
            LOGE("malloc audio_chan failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt->audio_chan, 0, sizeof(ntwk_trans_audio_chan_t));
    }

    ntwk_msg_init();
    ntwk_msg_start();
    ntwk_video_drop_init();

    ntwk_pack_init(NTWK_TRANS_CHAN_CTRL);
    ntwk_pack_init(NTWK_TRANS_CHAN_VIDEO);
    ntwk_pack_init(NTWK_TRANS_CHAN_AUDIO);

    ntwk_fragmentation_init(NTWK_TRANS_CHAN_CTRL);
    ntwk_fragmentation_init(NTWK_TRANS_CHAN_VIDEO);
    ntwk_fragmentation_init(NTWK_TRANS_CHAN_AUDIO);

    s_ntwk_trans_ctxt->initialized = true;

    LOGV("%s, service: %s\n", __func__, s_ntwk_trans_ctxt->service_name);

    return BK_OK;
}

bk_err_t ntwk_trans_ctxt_deinit(void)
{
    LOGV("%s start\r\n", __func__);

    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGW("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    ntwk_msg_stop();
    ntwk_msg_deinit();
    ntwk_video_drop_deinit();
    ntwk_pack_deinit(NTWK_TRANS_CHAN_CTRL);
    ntwk_pack_deinit(NTWK_TRANS_CHAN_VIDEO);
    ntwk_pack_deinit(NTWK_TRANS_CHAN_AUDIO);

    ntwk_fragmentation_deinit(NTWK_TRANS_CHAN_CTRL);
    ntwk_fragmentation_deinit(NTWK_TRANS_CHAN_VIDEO);
    ntwk_fragmentation_deinit(NTWK_TRANS_CHAN_AUDIO);

    if (s_ntwk_trans_ctxt->cntrl_chan != NULL)
    {
        os_free(s_ntwk_trans_ctxt->cntrl_chan);
        s_ntwk_trans_ctxt->cntrl_chan = NULL;
    }
    if (s_ntwk_trans_ctxt->video_chan != NULL)
    {
        os_free(s_ntwk_trans_ctxt->video_chan);
        s_ntwk_trans_ctxt->video_chan = NULL;
    }
    if (s_ntwk_trans_ctxt->audio_chan != NULL)
    {
        os_free(s_ntwk_trans_ctxt->audio_chan);
        s_ntwk_trans_ctxt->audio_chan = NULL;
    }

    s_ntwk_trans_ctxt->initialized = false;

    os_free(s_ntwk_trans_ctxt);
    s_ntwk_trans_ctxt = NULL;

    LOGV("%s, completed\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_trans_chan_start(chan_type_t chan_type, void *param)
{
    LOGV("%s start\r\n", __func__);

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_FAIL;

    ret = ntwk_in_start(chan_type, param);

    if (ret == BK_OK)
    {
        LOGV("%s, channel %d started\n", __func__, chan_type);
    }
    else
    {
        LOGE("%s, channel %d start failed: %d\n", __func__, chan_type, ret);
    }

    return ret;
}

bk_err_t ntwk_trans_chan_stop(chan_type_t chan_type)
{
    LOGV("%s start\r\n", __func__);

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_FAIL;

    ret = ntwk_in_stop(chan_type);

    if (ret != BK_OK)
    {
        LOGE("%s, channel %d stop failed: %d\n", __func__, chan_type, ret);
    }

    return ret;
}

int ntwk_trans_ctrl_send(uint8_t *data, uint32_t length)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;

    if (data == NULL || length == 0)
    {
        LOGE("%s, invalid parameters\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan == NULL || s_ntwk_trans_ctxt->cntrl_chan->send == NULL)
    {
        LOGE("%s, control channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->fragment != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->fragment(data, length) >= 0)
        {
            return BK_OK;
        }
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->pack != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
        {
            (s_ntwk_trans_ctxt->cntrl_chan->send)(pack_ptr, pack_ptr_length);
            return BK_OK;
        }
    }

    return (s_ntwk_trans_ctxt->cntrl_chan->send)(data, length);
}

int ntwk_trans_video_send(uint8_t *data, uint32_t length, image_format_t video_type)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        LOGE("%s, invalid parameters\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan == NULL || s_ntwk_trans_ctxt->video_chan->send == NULL)
    {
        LOGE("%s, video channel not configured\n", __func__);
        return BK_FAIL;
    }
    s_ntwk_trans_ctxt->video_chan->vid_type = video_type;

    if (s_ntwk_trans_ctxt->video_chan->drop_check != NULL)
    {
        if (s_ntwk_trans_ctxt->video_chan->drop_check((frame_buffer_t *)data) == true)
        {
            return BK_OK;
        }
    }

    if (s_ntwk_trans_ctxt->video_chan->fragment != NULL)
    {
        if(s_ntwk_trans_ctxt->video_chan->fragment(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->video_chan->pack != NULL)
    {
        if(s_ntwk_trans_ctxt->video_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
        {
            return (s_ntwk_trans_ctxt->video_chan->send)(pack_ptr, pack_ptr_length,s_ntwk_trans_ctxt->video_chan->vid_type);
        }
    }

    return (s_ntwk_trans_ctxt->video_chan->send)(data, length, s_ntwk_trans_ctxt->video_chan->vid_type);
}

int ntwk_trans_audio_send(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        LOGE("%s, invalid parameters\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan == NULL || s_ntwk_trans_ctxt->audio_chan->send == NULL)
    {
        LOGE("%s, audio channel not configured\n", __func__);
        return BK_FAIL;
    }

    s_ntwk_trans_ctxt->audio_chan->aud_type = audio_type;

    if (s_ntwk_trans_ctxt->audio_chan->fragment != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->fragment(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->audio_chan->pack != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
        {
            return (s_ntwk_trans_ctxt->audio_chan->send)(pack_ptr, pack_ptr_length, s_ntwk_trans_ctxt->audio_chan->aud_type);
        }
    }

    return (s_ntwk_trans_ctxt->audio_chan->send)(data, length, s_ntwk_trans_ctxt->audio_chan->aud_type);
}

int ntwk_trans_pack_rx_handler(chan_type_t chan_type, uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (s_ntwk_trans_ctxt->cntrl_chan == NULL)
            {
                LOGE("%s, control channel not configured\n", __func__);
                return -1;
            }

            if (s_ntwk_trans_ctxt->cntrl_chan->unfragment != NULL)
            {
                if(s_ntwk_trans_ctxt->cntrl_chan->unfragment(data, length) >= 0)
                {
                    return length;
                }
            }

            if (s_ntwk_trans_ctxt->cntrl_chan->recive != NULL)
            {
                return s_ntwk_trans_ctxt->cntrl_chan->recive(data, length);
            }
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (s_ntwk_trans_ctxt->video_chan == NULL)
            {
                LOGE("%s, video channel not configured\n", __func__);
                return BK_FAIL;
            }
            if (s_ntwk_trans_ctxt->video_chan->unfragment != NULL)
            {
                if(s_ntwk_trans_ctxt->video_chan->unfragment(data, length) >= 0)
                {
                    return length;
                }
            }
            if (s_ntwk_trans_ctxt->video_chan->recive != NULL)
            {
                return s_ntwk_trans_ctxt->video_chan->recive(data, length);
            }
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (s_ntwk_trans_ctxt->audio_chan == NULL)
            {
                LOGE("%s, audio channel not configured\n", __func__);
                return BK_FAIL;
            }
            if (s_ntwk_trans_ctxt->audio_chan->unfragment != NULL)
            {
                if(s_ntwk_trans_ctxt->audio_chan->unfragment(data, length) >= 0)
                {
                    return length;
                }
            }
            if (s_ntwk_trans_ctxt->audio_chan->recive != NULL)
            {
                return s_ntwk_trans_ctxt->audio_chan->recive(data, length);
            }
        } break;
        default:
            LOGE("%s, invalid channel type: %d\n", __func__, chan_type);
            return BK_FAIL;
    }

    return BK_FAIL;
}

bk_err_t ntwk_trans_register_ctrl_recv_cb(ntwk_trans_recv_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan != NULL)
    {
        s_ntwk_trans_ctxt->cntrl_chan->recive = cb;
        return BK_OK;
    }

    return BK_FAIL;
}
bk_err_t ntwk_trans_register_video_recv_cb(ntwk_trans_recv_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan != NULL)
    {
        s_ntwk_trans_ctxt->video_chan->recive = cb;
        return BK_OK;
    }

    return BK_FAIL;
}
bk_err_t ntwk_trans_register_audio_recv_cb(ntwk_trans_recv_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan != NULL)
    {
        s_ntwk_trans_ctxt->audio_chan->recive = cb;
        return BK_OK;
    }

    return BK_FAIL;
}


bk_err_t ntwk_trans_register_unfragment_malloc_cb(chan_type_t chan, ntwk_trans_unfragment_malloc_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    return ntwk_unfragment_register_malloc_cb(chan, cb);
}

bk_err_t ntwk_trans_register_unfragment_send_cb(chan_type_t chan, ntwk_trans_unfragment_send_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    return ntwk_unfragment_register_send_cb(chan, cb);
}

bk_err_t ntwk_trans_register_unfragment_free_cb(chan_type_t chan, ntwk_trans_unfragment_free_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    return ntwk_unfragment_register_free_cb(chan, cb);
}

#if CONFIG_NTWK_CLIENT_SERVICE_ENABLE
bk_err_t ntwk_trans_set_server_net_info(ntwk_server_net_info_t *net_info)
{
    if (!net_info)
    {
        LOGE("%s, invalid parameter\n", __func__);
        return BK_ERR_PARAM;
    }

    os_memset(&s_ntwk_server_net_info, 0, sizeof(ntwk_server_net_info_t));

    os_memcpy(&s_ntwk_server_net_info, net_info, sizeof(ntwk_server_net_info_t));

    LOGI("%s, server net info configured\n", __func__);

    return BK_OK;
}

ntwk_server_net_info_t *ntwk_trans_get_server_net_info(void)
{
    return &s_ntwk_server_net_info;
}

#endif