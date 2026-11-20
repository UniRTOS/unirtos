#include "bk_video_player_jpeg_probe.h"

#include "components/bk_jpeg_decode/bk_jpeg_decode_utils.h"
#include "frame_buffer.h"

avdk_err_t bk_video_player_probe_jpeg_subsampling(const uint8_t *jpeg_buf,
                                                  uint32_t jpeg_len,
                                                  video_player_jpeg_subsampling_t *out_subsampling)
{
    if (jpeg_buf == NULL || out_subsampling == NULL || jpeg_len == 0)
    {
        return AVDK_ERR_INVAL;
    }

    frame_buffer_t temp_frame = {0};
    temp_frame.frame = (uint8_t *)jpeg_buf;
    temp_frame.length = jpeg_len;

    bk_jpeg_decode_img_info_t img_info = {0};
    img_info.frame = &temp_frame;

    avdk_err_t jpeg_ret = bk_get_jpeg_data_info(&img_info);
    if (jpeg_ret != AVDK_ERR_OK)
    {
        return jpeg_ret;
    }

    if ((uint32_t)img_info.format > 4U)
    {
        return AVDK_ERR_INVAL;
    }

    *out_subsampling = (img_info.format == 0) ? VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE
                                              : (video_player_jpeg_subsampling_t)img_info.format;
    return AVDK_ERR_OK;
}

