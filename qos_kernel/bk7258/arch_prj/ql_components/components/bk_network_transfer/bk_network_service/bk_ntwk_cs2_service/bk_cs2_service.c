#include "bk_private/bk_init.h"
#include <os/os.h>

#include "common/network_transfer_common.h"

#include "network_transfer.h"
#include "network_type.h"
#include "common/bk_ntwk_pack/ntwk_pack.h"
#include "common/bk_ntwk_pack/ntwk_fragmentation.h"
#include "common/bk_video_drop_policy/video_drop.h"
#include "common/bk_ntwk_sdp/ntwk_sdp.h"
#include "network_transfer_internal.h"
#include "bk_network_service/bk_ntwk_cs2_service/ntwk_cs2_service.h"

#define TAG "bk-cs2-svc"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


bk_err_t bk_cs2_trans_service_init(char *service_name)
{
    bk_err_t ret = BK_OK;
    int payload_size = 0;
    ntwk_trans_ctxt_t *ctxt = NULL;

    LOGI("%s start\r\n", __func__);

    ret = ntwk_trans_ctxt_init(ctxt);
    if (ret != BK_OK)
    {
        LOGE("ntwk_trans_ctxt_init failed\n");
        goto error;
    }

    ret = ntwk_cs2_init();
    if (ret != BK_OK)
    {
        LOGE("ntwk_cs2_init failed\n");
        goto error;
    }

    ctxt = ntwk_trans_get_ctxt();

    if (ctxt == NULL)
    {
        LOGE("%s, ctxt is NULL\n", __func__);
        goto error;
    }

    strncpy(ctxt->service_name, service_name, strlen(service_name));

    if (ctxt->cntrl_chan != NULL)
    {
        ctxt->cntrl_chan->type = NTWK_TRANS_CHAN_CTRL;
       // ctxt->cntrl_chan->start = ntwk_cs2_ctrl_chan_start;
       // ctxt->cntrl_chan->stop = ntwk_cs2_ctrl_chan_stop;
        ctxt->cntrl_chan->send = ntwk_cs2_p2p_ctrl_send;
        ctxt->cntrl_chan->pack = ntwk_pack_ctrl_pack;
        ctxt->cntrl_chan->unpack = ntwk_pack_ctrl_unpack;
        ctxt->cntrl_chan->fragment = NULL;
        ctxt->cntrl_chan->unfragment = NULL;

        
        ntwk_in_register_ctrl_start_cb(ntwk_cs2_ctrl_chan_start);
        ntwk_in_register_ctrl_stop_cb(ntwk_cs2_ctrl_chan_stop);
        ntwk_pack_register_recv_cb(ctxt->cntrl_chan->type, ntwk_trans_pack_rx_handler);
           //configure ctrl channel
        ntwk_pack_chan_start(ctxt->cntrl_chan->type, NTWK_TRANS_DATA_MAX_SIZE, NTWK_TRANS_DATA_MAX_SIZE);

        ntwk_cs2_ctrl_register_receive_cb(ntwk_trans_ctrl_recv_handler);
    }

    if (ctxt->video_chan != NULL)
    {
        ctxt->video_chan->type = NTWK_TRANS_CHAN_VIDEO;
        ctxt->video_chan->vid_type = IMAGE_H264;
       // ctxt->video_chan->start = ntwk_cs2_video_chan_start;
       // ctxt->video_chan->stop = ntwk_cs2_video_chan_stop;
        ctxt->video_chan->send = ntwk_cs2_video_send_packet;
        ctxt->video_chan->pack = ntwk_pack_video_pack;
        ctxt->video_chan->unpack = ntwk_pack_video_unpack;
        ctxt->video_chan->fragment = ntwk_fragment_video_fragment;
        ctxt->video_chan->unfragment = ntwk_fragment_video_unfragment;
        ctxt->video_chan->drop_check = ntwk_video_drop_check;

        ntwk_in_register_video_start_cb(ntwk_cs2_video_chan_start);
        ntwk_in_register_video_stop_cb(ntwk_cs2_video_chan_stop);
        ntwk_register_get_drop_size_cb(ntwk_cs2_get_current_write_size);
        ntwk_video_drop_start(CONFIG_CS2_TX_BUFFER_THD);

        payload_size = NTWK_TRANS_TCP_DATA_MAX_SIZE - ntwk_pack_get_header_size() - ntwk_fragm_get_header_size();
        ntwk_fragment_start(ctxt->video_chan->type, payload_size, NULL);

        ntwk_pack_register_recv_cb(ctxt->video_chan->type, ntwk_trans_pack_rx_handler);
                //configure video channel
        ntwk_pack_chan_start(ctxt->video_chan->type, NTWK_TRANS_TCP_DATA_MAX_SIZE,NTWK_TRANS_TCP_DATA_MAX_SIZE);

        ntwk_fragment_register_recv_cb(ctxt->video_chan->type, ntwk_trans_fragment_rx_handler);
        ntwk_cs2_video_register_receive_cb(ntwk_trans_video_recv_handler);
    }

    if (ctxt->audio_chan != NULL)
    {
        ctxt->audio_chan->type = NTWK_TRANS_CHAN_AUDIO;
        ctxt->audio_chan->aud_type = AUDIO_ENC_TYPE_G711A;
        //ctxt->audio_chan->start = ntwk_cs2_audio_chan_start;
        //ctxt->audio_chan->stop = ntwk_cs2_audio_chan_stop;
        ctxt->audio_chan->send = ntwk_cs2_audio_send_packet;
        ctxt->audio_chan->pack = ntwk_pack_audio_pack;
        ctxt->audio_chan->unpack = ntwk_pack_audio_unpack;
        ctxt->audio_chan->fragment = NULL;
        ctxt->audio_chan->unfragment = NULL;

        ntwk_in_register_audio_start_cb(ntwk_cs2_audio_chan_start);
        ntwk_in_register_audio_stop_cb(ntwk_cs2_audio_chan_stop);
        ntwk_pack_register_recv_cb(ctxt->audio_chan->type, ntwk_trans_pack_rx_handler);
        ntwk_pack_chan_start(ctxt->audio_chan->type, NTWK_TRANS_TCP_DATA_MAX_SIZE,NTWK_TRANS_TCP_DATA_MAX_SIZE);
        ntwk_cs2_audio_register_receive_cb(ntwk_trans_audio_recv_handler);
    }

    LOGI("%s end\r\n", __func__);
    return BK_OK;

error:
    LOGI("%s error\r\n", __func__);
    bk_cs2_trans_service_deinit();

    return BK_FAIL;
}

bk_err_t bk_cs2_trans_service_deinit(void)
{
   ntwk_trans_ctxt_t *ctxt = ntwk_trans_get_ctxt();

    LOGI("%s start\r\n", __func__);

    if (ctxt == NULL)
    {
        LOGE("ctxt is NULL\n");
        return BK_FAIL;
    }

    ntwk_pack_chan_stop(NTWK_TRANS_CHAN_CTRL);
    ntwk_video_drop_stop();
    ntwk_fragment_stop(NTWK_TRANS_CHAN_VIDEO);
    ntwk_pack_chan_stop(NTWK_TRANS_CHAN_VIDEO);
    ntwk_pack_chan_stop(NTWK_TRANS_CHAN_AUDIO);
    ntwk_cs2_deinit();
    ntwk_trans_ctxt_deinit();

    LOGI("%s end\r\n", __func__);
    return BK_OK;
}

