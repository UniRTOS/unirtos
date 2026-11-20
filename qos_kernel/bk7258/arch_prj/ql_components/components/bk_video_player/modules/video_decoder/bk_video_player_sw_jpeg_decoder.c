#include "os/os.h"
#include "os/mem.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/media_types.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_sw.h"
#include "frame_buffer.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"

#define TAG "sw_jpeg_decoder"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Software JPEG decoder private context
typedef struct sw_jpeg_decoder_ctx_s
{
    bk_jpeg_decode_sw_handle_t sw_decoder_handle;  // Software JPEG decoder handle
    video_player_video_params_t video_params;      // Video parameters (width, height, fps, format)
    bool is_initialized;                           // Initialization flag
    pixel_format_t last_out_fmt;                   // Last configured output format
} sw_jpeg_decoder_ctx_t;

static video_player_video_decoder_ops_t s_ops_template;

// Allocate ops + context in a single block to avoid double os_malloc.
// Keep ops as the first member so we can pass ops as the instance pointer.
typedef struct
{
    video_player_video_decoder_ops_t ops;
    sw_jpeg_decoder_ctx_t ctx;
} sw_jpeg_decoder_instance_t;

static avdk_err_t sw_jpeg_decoder_get_supported_formats(struct video_player_video_decoder_ops_s *ops,
                                                        const video_player_video_format_t **formats,
                                                        uint32_t *format_count)
{
    (void)ops;

    if (formats == NULL || format_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const video_player_video_format_t s_formats[] = {
        VIDEO_PLAYER_VIDEO_FORMAT_MJPEG,
    };

    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

static bool sw_jpeg_map_out_format(pixel_format_t out_fmt, bk_jpeg_decode_sw_out_format_t *out_format)
{
    if (out_format == NULL)
    {
        return false;
    }

    switch (out_fmt)
    {
        case PIXEL_FMT_YUYV:
            *out_format = JPEG_DECODE_SW_OUT_FORMAT_YUYV;
            return true;
        case PIXEL_FMT_RGB565:
            *out_format = JPEG_DECODE_SW_OUT_FORMAT_RGB565;
            return true;
        case PIXEL_FMT_RGB888:
            *out_format = JPEG_DECODE_SW_OUT_FORMAT_RGB888;
            return true;
        default:
            return false;
    }
}

// Callback functions for software JPEG decoder
// These callbacks are used for asynchronous decoding scenarios.
// For synchronous decoding (which is what we use), these can be simple stubs.

static bk_err_t sw_jpeg_decoder_in_complete(frame_buffer_t *in_frame)
{
    // Input frame decoding complete callback
    // For synchronous decoding, this is called after input processing
    // No action needed for our use case
    (void)in_frame;
    return BK_OK;
}

static frame_buffer_t *sw_jpeg_decoder_out_malloc(uint32_t size)
{
    // Output buffer allocation callback
    // For synchronous decoding, output buffer is pre-allocated by caller
    // Return NULL to indicate we don't allocate here
    (void)size;
    return NULL;
}

static bk_err_t sw_jpeg_decoder_out_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    // Output frame processing complete callback
    // For synchronous decoding, this is called after output processing
    // No action needed for our use case
    (void)format_type;
    (void)result;
    (void)out_frame;
    return BK_OK;
}

static avdk_err_t sw_jpeg_decoder_deinit(struct video_player_video_decoder_ops_s *ops);

static avdk_err_t sw_jpeg_decoder_init(struct video_player_video_decoder_ops_s *ops, video_player_video_params_t *params)
{
    sw_jpeg_decoder_instance_t *sw_instance = __containerof(ops, sw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(sw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    sw_jpeg_decoder_ctx_t *ctx = &sw_instance->ctx;
    AVDK_RETURN_ON_FALSE(params, AVDK_ERR_INVAL, TAG, "params is NULL");

    // Software JPEG decoder has no restrictions:
    // - No width/height alignment requirements
    // - Supports all JPEG formats (YUV422, YUV420, etc.)
    // - Used as fallback when hardware decoder cannot handle the frame
    LOGI("%s: Initializing software JPEG decoder, width=%d, height=%d, format=%d\n",
         __func__, params->width, params->height, params->format);

    if (ctx->is_initialized)
    {
        LOGW("%s: Software JPEG decoder already initialized, deinitializing first\n", __func__);
        sw_jpeg_decoder_deinit(ops);
    }

    os_memcpy(&ctx->video_params, params, sizeof(video_player_video_params_t));
    ctx->sw_decoder_handle = NULL;
    ctx->last_out_fmt = PIXEL_FMT_UNKNOW;

    // Create software JPEG decoder instance.
    // NOTE: output format is configured on demand in decode() based on out_fmt.
#if 0
    bk_jpeg_decode_sw_config_t cfg = {0};
    cfg.core_id = JPEG_DECODE_CORE_ID_1;
    cfg.out_format = JPEG_DECODE_SW_OUT_FORMAT_YUYV;
    cfg.byte_order = JPEG_DECODE_LITTLE_ENDIAN;

    avdk_err_t ret = bk_software_jpeg_decode_new(&ctx->sw_decoder_handle, &cfg);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to create software JPEG decoder, ret=%d\n", __func__, ret);
        ctx->sw_decoder_handle = NULL;
        return ret;
    }
#else
    bk_jpeg_decode_sw_config_t cfg = {0};
    cfg.core_id = JPEG_DECODE_CORE_ID_2;
    cfg.out_format = JPEG_DECODE_SW_OUT_FORMAT_YUYV;
    cfg.byte_order = JPEG_DECODE_LITTLE_ENDIAN;
    cfg.decode_cbs.in_complete = sw_jpeg_decoder_in_complete;
    cfg.decode_cbs.out_malloc = sw_jpeg_decoder_out_malloc;
    cfg.decode_cbs.out_complete = sw_jpeg_decoder_out_complete;
    avdk_err_t ret = bk_software_jpeg_decode_on_multi_core_new(&ctx->sw_decoder_handle, &cfg);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to create software JPEG decoder, ret=%d\n", __func__, ret);
        ctx->sw_decoder_handle = NULL;
        return ret;
    }
#endif
    ret = bk_jpeg_decode_sw_open(ctx->sw_decoder_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to open software JPEG decoder, ret=%d\n", __func__, ret);
        (void)bk_jpeg_decode_sw_delete(ctx->sw_decoder_handle);
        ctx->sw_decoder_handle = NULL;
        return ret;
    }

    ctx->is_initialized = true;
    LOGI("%s: Software JPEG decoder initialized successfully\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t sw_jpeg_decoder_deinit(struct video_player_video_decoder_ops_s *ops)
{
    sw_jpeg_decoder_instance_t *sw_instance = __containerof(ops, sw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(sw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    sw_jpeg_decoder_ctx_t *ctx = &sw_instance->ctx;

    if (ctx->sw_decoder_handle != NULL)
    {
        (void)bk_jpeg_decode_sw_close(ctx->sw_decoder_handle);
        (void)bk_jpeg_decode_sw_delete(ctx->sw_decoder_handle);
        ctx->sw_decoder_handle = NULL;
    }

    ctx->is_initialized = false;
    ctx->last_out_fmt = PIXEL_FMT_UNKNOW;
    return AVDK_ERR_OK;
}

static avdk_err_t sw_jpeg_decoder_decode(struct video_player_video_decoder_ops_s *ops,
                                        video_player_buffer_t *in_buffer,
                                        video_player_buffer_t *out_buffer,
                                        pixel_format_t out_fmt)
{
    sw_jpeg_decoder_instance_t *sw_instance = __containerof(ops, sw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(sw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    sw_jpeg_decoder_ctx_t *ctx = &sw_instance->ctx;
    AVDK_RETURN_ON_FALSE(in_buffer, AVDK_ERR_INVAL, TAG, "in_buffer is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer, AVDK_ERR_INVAL, TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data, AVDK_ERR_INVAL, TAG, "in_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->data, AVDK_ERR_INVAL, TAG, "out_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->frame_buffer, AVDK_ERR_INVAL, TAG, "in_buffer->frame_buffer is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->frame_buffer, AVDK_ERR_INVAL, TAG, "out_buffer->frame_buffer is NULL");
    AVDK_RETURN_ON_FALSE(ctx->sw_decoder_handle, AVDK_ERR_IO, TAG, "Software decoder not initialized");
    AVDK_RETURN_ON_FALSE(ctx->is_initialized, AVDK_ERR_IO, TAG, "Software decoder not initialized");

    bk_jpeg_decode_sw_out_format_t sw_out_fmt = JPEG_DECODE_SW_OUT_FORMAT_YUYV;
    AVDK_RETURN_ON_FALSE(sw_jpeg_map_out_format(out_fmt, &sw_out_fmt), AVDK_ERR_UNSUPPORTED, TAG, "unsupported out_fmt");

    // Update sw decoder output format only when changed.
    if (ctx->last_out_fmt != out_fmt)
    {
        bk_jpeg_decode_sw_out_frame_info_t out_cfg = {0};
        out_cfg.out_format = sw_out_fmt;
        out_cfg.byte_order = JPEG_DECODE_LITTLE_ENDIAN;
        avdk_err_t cfg_ret = bk_jpeg_decode_sw_set_config(ctx->sw_decoder_handle, &out_cfg);
        if (cfg_ret != AVDK_ERR_OK)
        {
            LOGE("%s: Failed to set sw jpeg output config, ret=%d\n", __func__, cfg_ret);
            return cfg_ret;
        }
        ctx->last_out_fmt = out_fmt;
    }

    frame_buffer_t *in_fb = (frame_buffer_t *)in_buffer->frame_buffer;
    frame_buffer_t *out_fb = (frame_buffer_t *)out_buffer->frame_buffer;

    // Fill frame buffer fields from video_player_buffer_t to make the decoder independent from caller details.
    in_fb->frame = in_buffer->data;
    in_fb->size = in_buffer->length;
    in_fb->length = in_buffer->length;
    in_fb->width = (uint16_t)ctx->video_params.width;
    in_fb->height = (uint16_t)ctx->video_params.height;

    out_fb->frame = out_buffer->data;
    out_fb->size = out_buffer->length;
    out_fb->length = 0;
    out_fb->fmt = out_fmt;
    out_fb->width = (uint16_t)ctx->video_params.width;
    out_fb->height = (uint16_t)ctx->video_params.height;

    avdk_err_t ret = bk_jpeg_decode_sw_decode(ctx->sw_decoder_handle, in_fb, out_fb);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Software JPEG decode failed, ret=%d\n", __func__, ret);
        return ret;
    }

    // Sync decoded metadata back to video_player_buffer_t.
    out_buffer->pts = in_buffer->pts;
    out_buffer->length = out_fb->length;

    return AVDK_ERR_OK;
}

static video_player_video_decoder_ops_t *sw_jpeg_decoder_create(void)
{
    sw_jpeg_decoder_instance_t *instance = os_malloc(sizeof(sw_jpeg_decoder_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate software JPEG decoder instance\n", __func__);
        return NULL;
    }
    os_memset(instance, 0, sizeof(sw_jpeg_decoder_instance_t));
    instance->ctx.sw_decoder_handle = NULL;
    instance->ctx.is_initialized = false;
    instance->ctx.last_out_fmt = PIXEL_FMT_UNKNOW;

    os_memcpy(&instance->ops, &s_ops_template, sizeof(video_player_video_decoder_ops_t));
    return &instance->ops;
}

static void sw_jpeg_decoder_destroy(video_player_video_decoder_ops_t *ops)
{
    if (ops == NULL)
    {
        return;
    }
    if (ops == &s_ops_template)
    {
        return;
    }
    sw_jpeg_decoder_instance_t *instance = __containerof(ops, sw_jpeg_decoder_instance_t, ops);
    os_free(instance);
}

static video_player_video_decoder_ops_t s_ops_template = {
    .create = sw_jpeg_decoder_create,
    .destroy = sw_jpeg_decoder_destroy,
    .get_supported_formats = sw_jpeg_decoder_get_supported_formats,
    .init = sw_jpeg_decoder_init,
    .deinit = sw_jpeg_decoder_deinit,
    .decode = sw_jpeg_decoder_decode,
};

video_player_video_decoder_ops_t *bk_video_player_get_sw_jpeg_decoder_ops(void)
{
    return &s_ops_template;
}



