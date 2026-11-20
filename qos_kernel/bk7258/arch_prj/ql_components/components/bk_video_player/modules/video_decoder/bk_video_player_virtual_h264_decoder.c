#include "os/os.h"
#include "os/mem.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"
#include "frame_buffer.h"

#define TAG "virt_h264_decoder"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Virtual H264 decoder private context (log-only, no real decode).
typedef struct virt_h264_decoder_ctx_s
{
    video_player_video_params_t video_params;
    bool is_initialized;
} virt_h264_decoder_ctx_t;

static video_player_video_decoder_ops_t s_ops_template;

typedef struct
{
    video_player_video_decoder_ops_t ops;
    virt_h264_decoder_ctx_t ctx;
} virt_h264_decoder_instance_t;

static avdk_err_t virt_h264_decoder_get_supported_formats(struct video_player_video_decoder_ops_s *ops,
                                                          const video_player_video_format_t **formats,
                                                          uint32_t *format_count)
{
    (void)ops;
    if (formats == NULL || format_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const video_player_video_format_t s_formats[] = {
        VIDEO_PLAYER_VIDEO_FORMAT_H264,
    };
    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

static bool h264_has_annexb_start_code(const uint8_t *data, uint32_t length)
{
    if (data == NULL || length < 4)
    {
        return false;
    }
    for (uint32_t i = 0; i + 3 < length; i++)
    {
        if (i + 4 < length &&
            data[i] == 0x00 && data[i + 1] == 0x00 &&
            data[i + 2] == 0x00 && data[i + 3] == 0x01)
        {
            return true;
        }
        if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01)
        {
            return true;
        }
    }
    return false;
}

static bool h264_find_idr_annexb(const uint8_t *data, uint32_t length)
{
    if (data == NULL || length < 4)
    {
        return false;
    }

    uint32_t i = 0;
    while (i + 3 < length)
    {
        uint32_t nal_header = 0;
        if (i + 4 < length &&
            data[i] == 0x00 && data[i + 1] == 0x00 &&
            data[i + 2] == 0x00 && data[i + 3] == 0x01)
        {
            nal_header = i + 4;
            i = i + 4;
        }
        else if (data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x01)
        {
            nal_header = i + 3;
            i = i + 3;
        }
        else
        {
            i++;
            continue;
        }

        if (nal_header >= length)
        {
            break;
        }

        uint8_t nal_type = data[nal_header] & 0x1F;
        if (nal_type == 5)
        {
            return true;
        }
    }

    return false;
}

static bool h264_find_idr_avcc(const uint8_t *data, uint32_t length, bool *out_valid)
{
    if (out_valid != NULL)
    {
        *out_valid = false;
    }
    if (data == NULL || length < 4)
    {
        return false;
    }

    uint32_t offset = 0;
    while (offset + 4 <= length)
    {
        uint32_t nal_len = (uint32_t)data[offset] << 24 |
                           (uint32_t)data[offset + 1] << 16 |
                           (uint32_t)data[offset + 2] << 8 |
                           (uint32_t)data[offset + 3];
        if (nal_len == 0 || offset + 4 + nal_len > length)
        {
            return false;
        }
        uint8_t nal_type = data[offset + 4] & 0x1F;
        if (nal_type == 5)
        {
            if (out_valid != NULL)
            {
                *out_valid = true;
            }
            return true;
        }
        offset += 4 + nal_len;
    }

    if (out_valid != NULL)
    {
        *out_valid = (offset == length);
    }
    return false;
}

static avdk_err_t virt_h264_decoder_init(struct video_player_video_decoder_ops_s *ops,
                                         video_player_video_params_t *params)
{
    virt_h264_decoder_instance_t *instance = __containerof(ops, virt_h264_decoder_instance_t, ops);
    if (instance == NULL || params == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    instance->ctx.video_params = *params;
    instance->ctx.is_initialized = true;
    return AVDK_ERR_OK;
}

static avdk_err_t virt_h264_decoder_deinit(struct video_player_video_decoder_ops_s *ops)
{
    virt_h264_decoder_instance_t *instance = __containerof(ops, virt_h264_decoder_instance_t, ops);
    if (instance == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    instance->ctx.is_initialized = false;
    return AVDK_ERR_OK;
}

static avdk_err_t virt_h264_decoder_decode(struct video_player_video_decoder_ops_s *ops,
                                           video_player_buffer_t *in_buffer,
                                           video_player_buffer_t *out_buffer,
                                           pixel_format_t out_fmt)
{
    virt_h264_decoder_instance_t *instance = __containerof(ops, virt_h264_decoder_instance_t, ops);
    if (instance == NULL || in_buffer == NULL || out_buffer == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    if (!instance->ctx.is_initialized)
    {
        return AVDK_ERR_INVAL;
    }

    bool is_idr = false;
    if (h264_has_annexb_start_code(in_buffer->data, in_buffer->length))
    {
        is_idr = h264_find_idr_annexb(in_buffer->data, in_buffer->length);
    }
    else
    {
        bool avcc_valid = false;
        is_idr = h264_find_idr_avcc(in_buffer->data, in_buffer->length, &avcc_valid);
        if (!avcc_valid)
        {
            LOGW("%s: Unrecognized H264 stream format, size=%u\n", __func__, in_buffer->length);
        }
    }

    LOGI("%s: H264 frame=%s, pts=%llu ms, size=%u\n",
         __func__, is_idr ? "I" : "P",
         (unsigned long long)in_buffer->pts, in_buffer->length);

    out_buffer->pts = in_buffer->pts;
    (void)out_fmt;

    if (out_buffer->data != NULL && out_buffer->length > 0)
    {
        os_memset(out_buffer->data, 0, out_buffer->length);
    }
    if (out_buffer->frame_buffer != NULL)
    {
        frame_buffer_t *fb = (frame_buffer_t *)out_buffer->frame_buffer;
        fb->length = out_buffer->length;
        fb->width = (uint16_t)instance->ctx.video_params.width;
        fb->height = (uint16_t)instance->ctx.video_params.height;
    }

    return AVDK_ERR_OK;
}

static video_player_video_decoder_ops_t *virt_h264_decoder_create(void)
{
    virt_h264_decoder_instance_t *instance = os_malloc(sizeof(virt_h264_decoder_instance_t));
    if (instance == NULL)
    {
        return NULL;
    }
    os_memset(instance, 0, sizeof(*instance));
    instance->ops = s_ops_template;
    return &instance->ops;
}

static void virt_h264_decoder_destroy(struct video_player_video_decoder_ops_s *ops)
{
    if (ops == NULL)
    {
        return;
    }
    virt_h264_decoder_instance_t *instance = __containerof(ops, virt_h264_decoder_instance_t, ops);
    os_free(instance);
}

static video_player_video_decoder_ops_t s_ops_template = {
    .create = virt_h264_decoder_create,
    .destroy = virt_h264_decoder_destroy,
    .get_supported_formats = virt_h264_decoder_get_supported_formats,
    .init = virt_h264_decoder_init,
    .deinit = virt_h264_decoder_deinit,
    .decode = virt_h264_decoder_decode,
};

video_player_video_decoder_ops_t *bk_video_player_get_virtual_h264_decoder_ops(void)
{
    return &s_ops_template;
}
