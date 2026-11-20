#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"

#include "modules/aacdec.h"

#define TAG "vp_aac_dec"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct
{
    HAACDecoder h;
    bool is_raw; // true: MP4 raw AAC access units; false: ADTS stream
    uint32_t channels;
    uint32_t sample_rate;
    uint32_t bits_per_sample;
} vp_aac_decoder_ctx_t;

static const video_player_audio_decoder_ops_t s_aac_decoder_ops;

// Allocate ops + context in a single block to avoid double os_malloc.
// Keep ops as the first member so we can pass ops as the instance pointer.
typedef struct
{
    video_player_audio_decoder_ops_t ops;
    vp_aac_decoder_ctx_t ctx;
} vp_aac_decoder_instance_t;

static avdk_err_t vp_aac_decoder_get_supported_formats(const struct video_player_audio_decoder_ops_s *ops,
                                                       const video_player_audio_format_t **formats,
                                                       uint32_t *format_count)
{
    (void)ops;

    if (formats == NULL || format_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const video_player_audio_format_t s_formats[] = {
        VIDEO_PLAYER_AUDIO_FORMAT_AAC,
    };

    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

// Parse AAC AudioSpecificConfig (MPEG-4) to extract aot/sampleRate/channels (common 2-byte case).
static bool parse_aac_audio_specific_config(const uint8_t *cfg, uint32_t cfg_len,
                                           uint8_t *out_aot, uint32_t *out_sample_rate, uint8_t *out_channels)
{
    if (cfg == NULL || cfg_len < 2 || out_aot == NULL || out_sample_rate == NULL || out_channels == NULL)
    {
        return false;
    }

    // Bit layout:
    // audioObjectType: 5 bits
    // samplingFrequencyIndex: 4 bits
    // channelConfiguration: 4 bits
    uint16_t bits = ((uint16_t)cfg[0] << 8) | (uint16_t)cfg[1];
    uint8_t aot = (bits >> 11) & 0x1F;
    uint8_t sf_index = (bits >> 7) & 0x0F;
    uint8_t ch = (bits >> 3) & 0x0F;

    static const uint32_t sf_table[16] = {
        96000, 88200, 64000, 48000,
        44100, 32000, 24000, 22050,
        16000, 12000, 11025, 8000,
        7350,  0,     0,     0
    };

    uint32_t sr = 0;
    if (sf_index < 16)
    {
        sr = sf_table[sf_index];
    }
    if (sr == 0)
    {
        return false;
    }

    *out_aot = aot;
    *out_sample_rate = sr;
    *out_channels = ch;
    return true;
}

static avdk_err_t vp_aac_decoder_init(struct video_player_audio_decoder_ops_s *ops, video_player_audio_params_t *params)
{
    vp_aac_decoder_instance_t *aac_instance = __containerof(ops, vp_aac_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(aac_instance != NULL, AVDK_ERR_INVAL, TAG, "instance is NULL");
    vp_aac_decoder_ctx_t *ctx = &aac_instance->ctx;
    AVDK_RETURN_ON_FALSE(params != NULL, AVDK_ERR_INVAL, TAG, "params is NULL");

    if (params->format != 1)
    {
        return AVDK_ERR_NODEV;
    }

    if (ctx->h != NULL)
    {
        AACFreeDecoder(ctx->h);
        ctx->h = NULL;
    }

    ctx->h = AACInitDecoder();
    if (ctx->h == NULL)
    {
        LOGE("%s: AACInitDecoder failed\n", __func__);
        return AVDK_ERR_NOMEM;
    }

    ctx->channels = params->channels;
    ctx->sample_rate = params->sample_rate;
    ctx->bits_per_sample = params->bits_per_sample;

    // If we have AudioSpecificConfig, treat input as MP4 raw AAC and configure decoder.
    ctx->is_raw = (params->codec_config != NULL && params->codec_config_size > 0);
    if (ctx->is_raw)
    {
        uint8_t aot = 0;
        uint32_t sr = 0;
        uint8_t ch = 0;
        if (!parse_aac_audio_specific_config(params->codec_config, params->codec_config_size, &aot, &sr, &ch))
        {
            LOGW("%s: Invalid AAC codec_config, fallback to params (sr=%u,ch=%u)\n",
                 __func__, params->sample_rate, params->channels);
            aot = 2; // AAC-LC default
            sr = params->sample_rate;
            ch = (uint8_t)params->channels;
        }

        AACFrameInfo fi = {0};
        fi.nChans = ch;
        fi.sampRateCore = (int)sr;
        // Helix AAC profiles: 0(MP),1(LC),2(SSR). For AOT=2 use LC.
        fi.profile = AAC_PROFILE_LC;

        int rc = AACSetRawBlockParams(ctx->h, 0, &fi);
        if (rc != 0)
        {
            LOGE("%s: AACSetRawBlockParams failed, rc=%d\n", __func__, rc);
            AACFreeDecoder(ctx->h);
            ctx->h = NULL;
            return AVDK_ERR_INVAL;
        }
    }

    LOGI("%s: AAC decoder init ok (raw=%d, sr=%u, ch=%u)\n", __func__, ctx->is_raw, ctx->sample_rate, ctx->channels);
    return AVDK_ERR_OK;
}

static avdk_err_t vp_aac_decoder_deinit(struct video_player_audio_decoder_ops_s *ops)
{
    if (ops == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    vp_aac_decoder_instance_t *aac_instance = __containerof(ops, vp_aac_decoder_instance_t, ops);
    if (aac_instance == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    vp_aac_decoder_ctx_t *ctx = &aac_instance->ctx;

    if (ctx->h != NULL)
    {
        AACFreeDecoder(ctx->h);
        ctx->h = NULL;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t vp_aac_decoder_decode(struct video_player_audio_decoder_ops_s *ops, video_player_buffer_t *in_buffer, video_player_buffer_t *out_buffer)
{
    vp_aac_decoder_instance_t *aac_instance = __containerof(ops, vp_aac_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(aac_instance != NULL, AVDK_ERR_INVAL, TAG, "instance is NULL");
    vp_aac_decoder_ctx_t *ctx = &aac_instance->ctx;
    AVDK_RETURN_ON_FALSE(in_buffer != NULL && out_buffer != NULL, AVDK_ERR_INVAL, TAG, "buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data != NULL && in_buffer->length > 0, AVDK_ERR_INVAL, TAG, "input is empty");
    AVDK_RETURN_ON_FALSE(out_buffer->data != NULL && out_buffer->length > 0, AVDK_ERR_INVAL, TAG, "output is empty");
    AVDK_RETURN_ON_FALSE(ctx->h != NULL, AVDK_ERR_INVAL, TAG, "decoder not initialized");

    unsigned char *in_ptr = (unsigned char *)in_buffer->data;
    int bytes_left = (int)in_buffer->length;

    // If ADTS stream, align sync word if needed.
    if (!ctx->is_raw)
    {
        int off = AACFindSyncWord(in_ptr, bytes_left);
        if (off > 0)
        {
            in_ptr += off;
            bytes_left -= off;
        }
    }

    // Decode one AAC frame into out_buffer as PCM16.
    int rc = AACDecode(ctx->h, &in_ptr, &bytes_left, (short *)out_buffer->data);
    if (rc != 0)
    {
        LOGE("%s: AACDecode failed, rc=%d, in_len=%u, raw=%d\n", __func__, rc, in_buffer->length, ctx->is_raw);
        return AVDK_ERR_GENERIC;
    }

    AACFrameInfo fi = {0};
    AACGetLastFrameInfo(ctx->h, &fi);
    uint32_t pcm_bytes = (uint32_t)(fi.outputSamps * fi.bitsPerSample / 8);

    if (pcm_bytes > out_buffer->length)
    {
        LOGE("%s: Output buffer too small, need=%u, got=%u\n", __func__, pcm_bytes, out_buffer->length);
        return AVDK_ERR_NOMEM;
    }

    out_buffer->length = pcm_bytes;
    out_buffer->pts = in_buffer->pts;
    return AVDK_ERR_OK;
}



static video_player_audio_decoder_ops_t *vp_aac_decoder_create(void)
{
    vp_aac_decoder_instance_t *instance = os_malloc(sizeof(vp_aac_decoder_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate AAC decoder instance\n", __func__);
        return NULL;
    }

    os_memset(instance, 0, sizeof(vp_aac_decoder_instance_t));
    os_memcpy(&instance->ops, &s_aac_decoder_ops, sizeof(video_player_audio_decoder_ops_t));
    return &instance->ops;
}

static void vp_aac_decoder_destroy(video_player_audio_decoder_ops_t *ops)
{
    if (ops == NULL)
    {
        return;
    }

    // Do not destroy the static template ops.
    if (ops == &s_aac_decoder_ops)
    {
        return;
    }

    vp_aac_decoder_instance_t *instance = __containerof(ops, vp_aac_decoder_instance_t, ops);
    os_free(instance);
}

static const video_player_audio_decoder_ops_t s_aac_decoder_ops = {
    .create = vp_aac_decoder_create,
    .destroy = vp_aac_decoder_destroy,
    .get_supported_formats = vp_aac_decoder_get_supported_formats,
    .init = vp_aac_decoder_init,
    .deinit = vp_aac_decoder_deinit,
    .decode = vp_aac_decoder_decode,
};

const video_player_audio_decoder_ops_t *bk_video_player_get_aac_audio_decoder_ops(void)
{
    return &s_aac_decoder_ops;
}
