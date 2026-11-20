#include <common/bk_include.h>
#include "bk_uart.h"
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <common/bk_kernel_err.h>
#include <components/video_types.h>
#include <stdlib.h>
#include <driver/h264.h>
#include "network_transfer_internal.h"

#include "video_drop.h"

#define TAG "video-drop"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static ntwk_h264_drop_info_t *s_h264_drop_info = NULL;

bk_err_t ntwk_video_drop_init(void)
{
    if (s_h264_drop_info != NULL)
    {
        LOGW("%s: already initialized\n", __func__);
        return BK_OK;
    }

    s_h264_drop_info = (ntwk_h264_drop_info_t *)ntwk_malloc(sizeof(ntwk_h264_drop_info_t));
    if (s_h264_drop_info == NULL)
    {
        LOGE("%s: malloc failed\n", __func__);
        return BK_ERR_NO_MEM;
    }

    os_memset(s_h264_drop_info, 0, sizeof(ntwk_h264_drop_info_t));

    return BK_OK;
}

bk_err_t ntwk_video_drop_deinit(void)
{
    if (s_h264_drop_info == NULL)
    {
        LOGW("%s: not initialized\n", __func__);
        return BK_OK;
    }

    os_free(s_h264_drop_info);
    s_h264_drop_info = NULL;

    return BK_OK;
}

bk_err_t ntwk_video_drop_start(uint32_t buffer_size)
{
    if (s_h264_drop_info == NULL)
    {
        LOGE("%s: not initialized\n", __func__);
        return BK_ERR_PARAM;
    }

    if (buffer_size == 0)
    {
        return BK_ERR_PARAM;
    }

    s_h264_drop_info->buffer_thd = buffer_size;
    s_h264_drop_info->start_drop_thd = buffer_size / 2;
    s_h264_drop_info->drop_level_interval = (buffer_size / 2) / 7;

    s_h264_drop_info->initialized = true;

    return BK_OK;
}

bk_err_t ntwk_video_drop_stop(void)
{
    if (s_h264_drop_info == NULL)
    {
        LOGE("%s: not initialized\n", __func__);
        return BK_ERR_PARAM;
    }

    s_h264_drop_info->initialized = false;

    ntwk_video_drop_deinit();

    return BK_OK;
}


void ntwk_video_h264_record_map(bool is_I_frame)
{
    if (s_h264_drop_info == NULL)
    {
        return;
    }

    LOGV("[map] is_I_frame %d I_num %d P_num %d drop_level %d drop_period %d\n", is_I_frame,
         s_h264_drop_info->I_num,
         s_h264_drop_info->P_num,
         s_h264_drop_info->drop_level,
         s_h264_drop_info->drop_period);

    if (is_I_frame && s_h264_drop_info->drop_level)
    {
        s_h264_drop_info->drop_period++;
    }

    if (is_I_frame && (s_h264_drop_info->I_num >= MEDIA_H264_I_FRAME_MAX_NUM))
    {
        s_h264_drop_info->I_num = 0;
    }

    if (is_I_frame)
    {
        s_h264_drop_info->I_num++;
        s_h264_drop_info->P_num = 0;
    }
    else
    {
        s_h264_drop_info->P_num++;
    }
}

void ntwk_h264_down_drop_level(uint32_t cur_drop_level, bool recovery, bool is_I_frame)
{
    uint32_t status_avg_size;
    int32_t avg_drop_level;

    if (s_h264_drop_info == NULL)
    {
        return;
    }

    LOGV("[dec] cur_drop_level %d recovery %d is_I_frame %d\n", cur_drop_level, recovery, is_I_frame);
    LOGV("[dec] drop period %d total_size %d stats cnt %d\n", s_h264_drop_info->drop_period, s_h264_drop_info->status_total_size, s_h264_drop_info->status_cnt);

    if ((s_h264_drop_info->drop_period <= MEDIA_H264_DOWN_DROP_LEVEL_MIN_PERIOD) || !is_I_frame || !s_h264_drop_info->status_cnt)
    {
        return;
    }

    status_avg_size = (s_h264_drop_info->status_total_size * 1024) / s_h264_drop_info->status_cnt;

    LOGV("status_avg_size %d\n", status_avg_size);

    if (status_avg_size < s_h264_drop_info->start_drop_thd)
    {
        s_h264_drop_info->drop_level = recovery ? 0 : cur_drop_level;
    }
    else
    {
        avg_drop_level = (status_avg_size - s_h264_drop_info->start_drop_thd) / s_h264_drop_info->drop_level_interval + 1;
        if (avg_drop_level >= MEDIA_H264_MAX_DROP_LEVEL)
        {
            avg_drop_level = MEDIA_H264_MAX_DROP_LEVEL - 1;
        }
        s_h264_drop_info->drop_level = (avg_drop_level > cur_drop_level) ? avg_drop_level : cur_drop_level;
        LOGV("[dec] avg_drop_level %d drop_level %d\n", avg_drop_level, s_h264_drop_info->drop_level);
    }

    s_h264_drop_info->drop_period = 0;
    s_h264_drop_info->status_total_size = 0;
    s_h264_drop_info->status_cnt = 0;
}

bool ntwk_video_h264_drop_level_check(uint32_t pre_size, UINT32 WriteSize, bool is_I_frame)
{
    uint32_t cur_size = pre_size + WriteSize;
    uint32_t cur_drop_level;

    if (s_h264_drop_info == NULL)
    {
        return false;
    }

    bool need_drop = (cur_size > s_h264_drop_info->start_drop_thd) ? true : false;

    LOGV("pre_size %d WriteSize %d is_I_frame %d need_drop %d\n", pre_size, WriteSize, is_I_frame, need_drop);
    LOGV("drop_level %d drop_period %d is_I_frame %d need_drop %d\n", s_h264_drop_info->drop_level, s_h264_drop_info->drop_period);
    LOGV("drop thd %d offset %d\n", s_h264_drop_info->start_drop_thd, s_h264_drop_info->drop_level_interval);

    if ((!s_h264_drop_info->drop_level) && !need_drop)
    {
        return false;
    }

    if (s_h264_drop_info->drop_period > MEDIA_H264_EVERY_DROP_LEVEL_MAX_STATUS_CNT)
    {
        s_h264_drop_info->drop_period = 0;
        s_h264_drop_info->status_total_size = 0;
        s_h264_drop_info->status_cnt = 0;
    }

    if (s_h264_drop_info->drop_period)
    {
        s_h264_drop_info->status_total_size += WriteSize / 1024;
        s_h264_drop_info->status_cnt++;
    }

    if (need_drop)
    {
        cur_drop_level = (cur_size - s_h264_drop_info->start_drop_thd) / s_h264_drop_info->drop_level_interval + 1;

        if (cur_drop_level >= MEDIA_H264_MAX_DROP_LEVEL)
        {
            cur_drop_level = MEDIA_H264_MAX_DROP_LEVEL - 1;
        }

        if (s_h264_drop_info->drop_level > cur_drop_level)
        {
            ntwk_h264_down_drop_level(cur_drop_level, false, is_I_frame);
        }
        else
        {
            s_h264_drop_info->drop_level = cur_drop_level;
        }
    }
    else
    {
        ntwk_h264_down_drop_level(s_h264_drop_info->drop_level, true, is_I_frame);
    }

    if (s_h264_drop_info->drop_level && !s_h264_drop_info->drop_period)
    {
        s_h264_drop_info->drop_period++;
    }

    if (is_I_frame)
    {
        if ((s_h264_drop_info->drop_level >= MEDIA_H264_MAX_DROP_LEVEL) && (s_h264_drop_info->I_num == MEDIA_H264_I_FRAME_MAX_NUM))
        {
            LOGV("Drop cur I frame\n");
            s_h264_drop_info->I_frame_droped = true;
            return true;
        }
        s_h264_drop_info->I_frame_droped = false;
    }
    else
    {
        if ((s_h264_drop_info->P_num >= (MEDIA_H264_MAX_DROP_LEVEL - s_h264_drop_info->drop_level)))
        {
            LOGV("Drop cur P frame\n");
            return true;
        }
    }

    return false;
}

bool ntwk_video_h264_drop_check(frame_buffer_t *frame)
{
    static bool drop_other_gop_frame = false;
#ifdef CONFIG_H264
    int Check_wsize = 0;

    uint32_t WriteSize = 0;
    bool is_NAL_I_frame = false;
    uint32_t pre_size = 0;

    if (s_h264_drop_info == NULL ||  s_h264_drop_info->buffer_thd == 0)
    {
        return false;
    }

    if (!s_h264_drop_info->check_type)
    {
        h264_base_config_t media_config = {0};
        int ret = 0;

        ret = bk_h264_get_h264_base_config(&media_config);
        if (ret == BK_OK)
        {
            s_h264_drop_info->check_type = true;
            s_h264_drop_info->support_type = (media_config.p_frame_cnt == MEDIA_H264_SUPPORT_MAX_P_FRAME_NUM) ? true : false;
            LOGD("check support cnt %d support type %d\n", media_config.p_frame_cnt, s_h264_drop_info->support_type);
        }
    }

    if (frame == NULL || !s_h264_drop_info->support_type)
    {
        LOGV("frame %d support_type %d\n", frame, s_h264_drop_info->support_type);
        return false;
    }

    if (frame->h264_type & (0x1 << H264_NAL_I_FRAME))
    {
        is_NAL_I_frame = true;
        drop_other_gop_frame = false;
    }
    else
    {
        is_NAL_I_frame = false;
    }

    ntwk_video_h264_record_map(is_NAL_I_frame);

    LOGV("drop_check seq:%d len:%d type:%s\n", frame->sequence, frame->length, is_NAL_I_frame ? "I" : "P");

    if ( s_h264_drop_info->write_size)
    {
        if ( s_h264_drop_info->write_size(&WriteSize) != BK_OK)
        {
            drop_other_gop_frame = false;
            return false;
        }
        Check_wsize = WriteSize;
    }

    if (Check_wsize < 0)
    {
        drop_other_gop_frame = false;
        return false;
    }

    WriteSize = Check_wsize;

    //Recovery send frame immditely when write size is less than THD/2 and one gop no drop I frame
    if (!s_h264_drop_info->I_frame_droped && drop_other_gop_frame && (WriteSize < s_h264_drop_info->start_drop_thd / 2))
    {
        LOGV("[drop] restore p frame\r\n");
        drop_other_gop_frame = false;
    }

    if (drop_other_gop_frame)
    {
        LOGV("GOP Drop frame[%s]\n", is_NAL_I_frame ? "I" : "P");
        return true;
    }

    pre_size = frame->length;
    drop_other_gop_frame = ntwk_video_h264_drop_level_check(pre_size, WriteSize, is_NAL_I_frame);
#endif
    return drop_other_gop_frame;

}

bool ntwk_video_drop_check(frame_buffer_t *frame)
{
    bool ret = false;

    if (frame == NULL)
    {
        return false;
    }

    switch (frame->fmt)
    {
        case PIXEL_FMT_H264:
            ret = ntwk_video_h264_drop_check(frame);
            break;
        default:
            break;
    }

    return ret;
}

bk_err_t ntwk_register_get_drop_size_cb(ntwk_get_write_size_cb_t cb)
{
    if (cb == NULL)
    {
        LOGE("%s: callback is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    s_h264_drop_info->write_size = cb;

    return BK_OK;
}

