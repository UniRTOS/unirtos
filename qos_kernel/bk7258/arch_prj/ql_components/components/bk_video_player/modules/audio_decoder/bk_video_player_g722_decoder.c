#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"

#include "modules/bk_g722.h"

#define TAG "vp_g722_dec"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct
{
    g722_decode_state_t state;
    bool inited;
} vp_g722_decoder_ctx_t;

static const video_player_audio_decoder_ops_t s_g722_decoder_ops;

typedef struct
{
    video_player_audio_decoder_ops_t ops;
    vp_g722_decoder_ctx_t ctx;
} vp_g722_decoder_instance_t;

static avdk_err_t vp_g722_decoder_get_supported_formats(const struct video_player_audio_decoder_ops_s *ops,
                                                        const video_player_audio_format_t **formats,
                                                        uint32_t *format_count)
{
    (void)ops;

    if (formats == NULL || format_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const video_player_audio_format_t s_formats[] = {
        VIDEO_PLAYER_AUDIO_FORMAT_G722,
    };

    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

static avdk_err_t vp_g722_decoder_init(struct video_player_audio_decoder_ops_s *ops, video_player_audio_params_t *params)
{
    vp_g722_decoder_instance_t *g722_instance = __containerof(ops, vp_g722_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(g722_instance != NULL, AVDK_ERR_INVAL, TAG, "instance is NULL");
    vp_g722_decoder_ctx_t *ctx = &g722_instance->ctx;
    AVDK_RETURN_ON_FALSE(params != NULL, AVDK_ERR_INVAL, TAG, "params is NULL");

    if (params->format != VIDEO_PLAYER_AUDIO_FORMAT_G722)
    {
        return AVDK_ERR_NODEV;
    }

    if (ctx->inited)
    {
        (void)bk_g722_decode_release(&ctx->state);
        ctx->inited = false;
    }

    /*
     * NOTE:
     * - bk_g722 implementation supports 48/56/64 kbps only (rate param: 48000/56000/64000).
     * - G722_SAMPLE_RATE_8000 only switches sample mode, it does NOT halve bitrate.
     *
     * Current recorder path uses 64 kbps, so default decode rate is 64000.
     */
    int options = 0;
    int bitrate = 64000;
    if (params->sample_rate == 8000)
    {
        // Decode to 8k samples/second (narrowband output mode).
        options |= G722_SAMPLE_RATE_8000;
    }

    int rc = bk_g722_decode_init(&ctx->state, bitrate, options);
    if (rc != 0)
    {
        LOGE("%s: bk_g722_decode_init failed, rc=%d\n", __func__, rc);
        return AVDK_ERR_INVAL;
    }

    ctx->inited = true;
    LOGI("%s: G722 decoder init ok (sr=%u, bitrate=%u, options=0x%x)\n",
         __func__, (unsigned)params->sample_rate, (unsigned)bitrate, (unsigned)options);
    return AVDK_ERR_OK;
}

static avdk_err_t vp_g722_decoder_deinit(struct video_player_audio_decoder_ops_s *ops)
{
    if (ops == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    vp_g722_decoder_instance_t *g722_instance = __containerof(ops, vp_g722_decoder_instance_t, ops);
    if (g722_instance == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    vp_g722_decoder_ctx_t *ctx = &g722_instance->ctx;

    if (ctx->inited)
    {
        (void)bk_g722_decode_release(&ctx->state);
        ctx->inited = false;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t vp_g722_decoder_decode(struct video_player_audio_decoder_ops_s *ops,
                                        video_player_buffer_t *in_buffer,
                                        video_player_buffer_t *out_buffer)
{
    vp_g722_decoder_instance_t *g722_instance = __containerof(ops, vp_g722_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(g722_instance != NULL, AVDK_ERR_INVAL, TAG, "instance is NULL");
    vp_g722_decoder_ctx_t *ctx = &g722_instance->ctx;
    AVDK_RETURN_ON_FALSE(in_buffer != NULL && out_buffer != NULL, AVDK_ERR_INVAL, TAG, "buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data != NULL && in_buffer->length > 0, AVDK_ERR_INVAL, TAG, "input is empty");
    AVDK_RETURN_ON_FALSE(out_buffer->data != NULL && out_buffer->length > 0, AVDK_ERR_INVAL, TAG, "output is empty");
    AVDK_RETURN_ON_FALSE(ctx->inited, AVDK_ERR_INVAL, TAG, "decoder not initialized");

    // Worst-case expansion is 2 PCM16 samples per input byte => 4 bytes.
    uint32_t max_need = in_buffer->length * 4U;
    if (max_need > out_buffer->length)
    {
        LOGE("%s: Output buffer too small, need>=%u, got=%u\n", __func__, max_need, out_buffer->length);
        return AVDK_ERR_NOMEM;
    }

    int out_samples = bk_g722_decode(&ctx->state,
                                    (int16_t *)out_buffer->data,
                                    (const uint8_t *)in_buffer->data,
                                    (int)in_buffer->length);
    if (out_samples <= 0)
    {
        LOGE("%s: bk_g722_decode failed, out_samples=%d, in_len=%u\n", __func__, out_samples, in_buffer->length);
        return AVDK_ERR_GENERIC;
    }

    uint32_t out_bytes = (uint32_t)out_samples * 2U;
    out_buffer->length = out_bytes;
    out_buffer->pts = in_buffer->pts;
    return AVDK_ERR_OK;
}

static video_player_audio_decoder_ops_t *vp_g722_decoder_create(void)
{
    vp_g722_decoder_instance_t *instance = os_malloc(sizeof(vp_g722_decoder_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate G722 decoder instance\n", __func__);
        return NULL;
    }

    os_memset(instance, 0, sizeof(vp_g722_decoder_instance_t));
    os_memcpy(&instance->ops, &s_g722_decoder_ops, sizeof(video_player_audio_decoder_ops_t));
    return &instance->ops;
}

static void vp_g722_decoder_destroy(video_player_audio_decoder_ops_t *ops)
{
    if (ops == NULL)
    {
        return;
    }

    if (ops == &s_g722_decoder_ops)
    {
        return;
    }

    vp_g722_decoder_instance_t *instance = __containerof(ops, vp_g722_decoder_instance_t, ops);
    os_free(instance);
}

static const video_player_audio_decoder_ops_t s_g722_decoder_ops = {
    .create = vp_g722_decoder_create,
    .destroy = vp_g722_decoder_destroy,
    .get_supported_formats = vp_g722_decoder_get_supported_formats,
    .init = vp_g722_decoder_init,
    .deinit = vp_g722_decoder_deinit,
    .decode = vp_g722_decoder_decode,
};

const video_player_audio_decoder_ops_t *bk_video_player_get_g722_audio_decoder_ops(void)
{
    return &s_g722_decoder_ops;
}

