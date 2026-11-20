#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"

#include "modules/g711.h"

#define TAG "vp_g711_dec"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct
{
    video_player_audio_format_t format;
} vp_g711_decoder_ctx_t;

static const video_player_audio_decoder_ops_t s_g711_decoder_ops;

typedef struct
{
    video_player_audio_decoder_ops_t ops;
    vp_g711_decoder_ctx_t ctx;
} vp_g711_decoder_instance_t;

static avdk_err_t vp_g711_decoder_get_supported_formats(const struct video_player_audio_decoder_ops_s *ops,
                                                        const video_player_audio_format_t **formats,
                                                        uint32_t *format_count)
{
    (void)ops;

    if (formats == NULL || format_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const video_player_audio_format_t s_formats[] = {
        VIDEO_PLAYER_AUDIO_FORMAT_G711A,
        VIDEO_PLAYER_AUDIO_FORMAT_G711U,
    };

    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

static avdk_err_t vp_g711_decoder_init(struct video_player_audio_decoder_ops_s *ops, video_player_audio_params_t *params)
{
    vp_g711_decoder_instance_t *g711_instance = __containerof(ops, vp_g711_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(g711_instance != NULL, AVDK_ERR_INVAL, TAG, "instance is NULL");
    vp_g711_decoder_ctx_t *ctx = &g711_instance->ctx;
    AVDK_RETURN_ON_FALSE(params != NULL, AVDK_ERR_INVAL, TAG, "params is NULL");

    if (params->format != VIDEO_PLAYER_AUDIO_FORMAT_G711A &&
        params->format != VIDEO_PLAYER_AUDIO_FORMAT_G711U)
    {
        return AVDK_ERR_NODEV;
    }

    ctx->format = params->format;
    LOGI("%s: G711 decoder init ok (format=%u)\n", __func__, (unsigned)ctx->format);
    return AVDK_ERR_OK;
}

static avdk_err_t vp_g711_decoder_deinit(struct video_player_audio_decoder_ops_s *ops)
{
    if (ops == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

static avdk_err_t vp_g711_decoder_decode(struct video_player_audio_decoder_ops_s *ops,
                                        video_player_buffer_t *in_buffer,
                                        video_player_buffer_t *out_buffer)
{
    vp_g711_decoder_instance_t *g711_instance = __containerof(ops, vp_g711_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(g711_instance != NULL, AVDK_ERR_INVAL, TAG, "instance is NULL");
    vp_g711_decoder_ctx_t *ctx = &g711_instance->ctx;
    AVDK_RETURN_ON_FALSE(in_buffer != NULL && out_buffer != NULL, AVDK_ERR_INVAL, TAG, "buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data != NULL && in_buffer->length > 0, AVDK_ERR_INVAL, TAG, "input is empty");
    AVDK_RETURN_ON_FALSE(out_buffer->data != NULL && out_buffer->length > 0, AVDK_ERR_INVAL, TAG, "output is empty");

    uint32_t need = in_buffer->length * 2U;
    if (need > out_buffer->length)
    {
        LOGE("%s: Output buffer too small, need=%u, got=%u\n", __func__, need, out_buffer->length);
        return AVDK_ERR_NOMEM;
    }

    int16_t *dst = (int16_t *)out_buffer->data;
    const uint8_t *src = (const uint8_t *)in_buffer->data;

    if (ctx->format == VIDEO_PLAYER_AUDIO_FORMAT_G711A)
    {
        for (uint32_t i = 0; i < in_buffer->length; i++)
        {
            dst[i] = (int16_t)alaw2linear(src[i]);
        }
    }
    else
    {
        for (uint32_t i = 0; i < in_buffer->length; i++)
        {
            dst[i] = (int16_t)ulaw2linear(src[i]);
        }
    }

    out_buffer->length = need;
    out_buffer->pts = in_buffer->pts;
    return AVDK_ERR_OK;
}

static video_player_audio_decoder_ops_t *vp_g711_decoder_create(void)
{
    vp_g711_decoder_instance_t *instance = os_malloc(sizeof(vp_g711_decoder_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate G711 decoder instance\n", __func__);
        return NULL;
    }

    os_memset(instance, 0, sizeof(vp_g711_decoder_instance_t));
    os_memcpy(&instance->ops, &s_g711_decoder_ops, sizeof(video_player_audio_decoder_ops_t));
    return &instance->ops;
}

static void vp_g711_decoder_destroy(video_player_audio_decoder_ops_t *ops)
{
    if (ops == NULL)
    {
        return;
    }

    if (ops == &s_g711_decoder_ops)
    {
        return;
    }

    vp_g711_decoder_instance_t *instance = __containerof(ops, vp_g711_decoder_instance_t, ops);
    os_free(instance);
}

static const video_player_audio_decoder_ops_t s_g711_decoder_ops = {
    .create = vp_g711_decoder_create,
    .destroy = vp_g711_decoder_destroy,
    .get_supported_formats = vp_g711_decoder_get_supported_formats,
    .init = vp_g711_decoder_init,
    .deinit = vp_g711_decoder_deinit,
    .decode = vp_g711_decoder_decode,
};

const video_player_audio_decoder_ops_t *bk_video_player_get_g711_audio_decoder_ops(void)
{
    return &s_g711_decoder_ops;
}

