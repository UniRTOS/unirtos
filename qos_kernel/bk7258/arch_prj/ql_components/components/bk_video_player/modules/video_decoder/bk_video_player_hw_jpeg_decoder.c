#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/media_types.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_hw.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_utils.h"
#include "components/bk_dma2d.h"
#include "components/bk_dma2d_types.h"
#include "frame_buffer.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"

#define TAG "hw_jpeg_decoder"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Hardware JPEG decoder private context
typedef struct hw_jpeg_decoder_ctx_s
{
    bk_jpeg_decode_hw_handle_t hw_decoder_handle;  // Hardware JPEG decoder handle
    video_player_video_params_t video_params;     // Video parameters (width, height, fps, format)
    bool is_initialized;                          // Initialization flag
    bk_dma2d_ctlr_handle_t dma2d_handle;          // DMA2D controller handle (for YUYV->RGB conversion)
    bool dma2d_opened;                            // DMA2D opened flag
    bool dma2d_owned;                             // DMA2D controller ownership (created by this decoder)
    frame_buffer_t *tmp_yuyv;                     // Temporary YUYV buffer for hardware decode
} hw_jpeg_decoder_ctx_t;

static video_player_video_decoder_ops_t s_ops_template;

// Allocate ops + context in a single block to avoid double os_malloc.
// Keep ops as the first member so we can pass ops as the instance pointer.
typedef struct
{
    video_player_video_decoder_ops_t ops;
    hw_jpeg_decoder_ctx_t ctx;
} hw_jpeg_decoder_instance_t;

static avdk_err_t hw_jpeg_decoder_get_supported_formats(struct video_player_video_decoder_ops_s *ops,
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

// Forward declaration
static avdk_err_t hw_jpeg_decoder_deinit(struct video_player_video_decoder_ops_s *ops);

static bool hw_jpeg_pixel_bytes(pixel_format_t fmt, uint32_t *out_bpp)
{
    if (out_bpp == NULL)
    {
        return false;
    }

    switch (fmt)
    {
        case PIXEL_FMT_RGB888:
            *out_bpp = 3;
            return true;
        case PIXEL_FMT_RGB565:
        case PIXEL_FMT_YUYV:
            *out_bpp = 2;
            return true;
        default:
            return false;
    }
}

static bool hw_jpeg_dma2d_map_output(pixel_format_t fmt, out_color_mode_t *out_mode, color_bytes_t *out_bytes)
{
    if (out_mode == NULL || out_bytes == NULL)
    {
        return false;
    }

    switch (fmt)
    {
        case PIXEL_FMT_RGB565:
            *out_mode = DMA2D_OUTPUT_RGB565;
            *out_bytes = TWO_BYTES;
            return true;
        case PIXEL_FMT_RGB888:
            *out_mode = DMA2D_OUTPUT_RGB888;
            *out_bytes = THREE_BYTES;
            return true;
        default:
            return false;
    }
}

// JPEG decode callback functions
// Note: These callbacks are called by hardware decoder, but we use synchronous decode
// so these may not be called in sync mode, but they're required for decoder creation
static bk_err_t hw_jpeg_decode_in_complete(frame_buffer_t *in_frame)
{
    // Input buffer processing complete callback
    // In synchronous mode, this may not be called
    LOGV("%s: Input frame processing complete\n", __func__);
    return BK_OK;
}

static frame_buffer_t *hw_jpeg_decode_out_malloc(uint32_t size)
{
    // Output buffer allocation callback
    // In synchronous mode, this may not be called as we provide the output buffer
    LOGV("%s: Request to allocate output buffer, size=%u\n", __func__, size);
    return NULL; // We provide output buffer in decode function
}

static bk_err_t hw_jpeg_decode_out_complete(uint32_t format_type, uint32_t result, frame_buffer_t *out_frame)
{
    // Output buffer processing complete callback
    // In synchronous mode, this may not be called
    LOGV("%s: Output frame processing complete, format=%u, result=%u\n", __func__, format_type, result);
    return BK_OK;
}

// Initialize hardware JPEG decoder
static avdk_err_t hw_jpeg_decoder_init(struct video_player_video_decoder_ops_s *ops, video_player_video_params_t *params)
{
    hw_jpeg_decoder_instance_t *hw_instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(hw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    hw_jpeg_decoder_ctx_t *ctx = &hw_instance->ctx;
    AVDK_RETURN_ON_FALSE(params, AVDK_ERR_INVAL, TAG, "params is NULL");

    LOGI("%s: Initializing hardware JPEG decoder, width=%d, height=%d, format=%d, jpeg_subsampling=%u\n",
         __func__, params->width, params->height, params->format, params->jpeg_subsampling);

    // Hardware JPEG decoder only supports MJPEG format
    if (params->format != VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
    {
        LOGW("%s: HW JPEG decoder only supports MJPEG format (got format=%u), will fallback to SW decoder\n",
             __func__, params->format);
        return AVDK_ERR_UNSUPPORTED;
    }

    // Hardware JPEG decoder requirements (strict):
    // - Image width must be multiple of 16
    // - Image height must be multiple of 8
    // - JPEG format must be YUV422 (checked in init if jpeg_subsampling is available, or per-frame in decode())
    // If any requirement is not met, return AVDK_ERR_UNSUPPORTED to allow fallback to software decoder
    if ((params->width % 16) != 0 || (params->height % 8) != 0)
    {
        LOGW("%s: HW JPEG decoder not supported for size %ux%u (require width%%16==0 && height%%8==0), will fallback to SW decoder\n",
             __func__, params->width, params->height);
        return AVDK_ERR_UNSUPPORTED;
    }

    // Check JPEG subsampling format if available (parsed from first frame in parse_video_info)
    // Only check if format is MJPEG and jpeg_subsampling was successfully parsed
    if (params->jpeg_subsampling != VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE)
    {
        if (params->jpeg_subsampling != VIDEO_PLAYER_JPEG_SUBSAMPLING_YUV422)
        {
            LOGW("%s: HW JPEG decoder not supported for subsampling format=%u (require VIDEO_PLAYER_JPEG_SUBSAMPLING_YUV422=%u), will fallback to SW decoder\n",
                 __func__, params->jpeg_subsampling, VIDEO_PLAYER_JPEG_SUBSAMPLING_YUV422);
            return AVDK_ERR_UNSUPPORTED;
        }
    }
    // If jpeg_subsampling is NONE, we'll check it per-frame in decode() as fallback

    // Check if already initialized
    if (ctx->is_initialized)
    {
        LOGW("%s: Hardware JPEG decoder already initialized, deinitializing first\n", __func__);
        hw_jpeg_decoder_deinit(ops);
    }

    // Save video parameters
    os_memcpy(&ctx->video_params, params, sizeof(video_player_video_params_t));

    ctx->dma2d_handle = NULL;
    ctx->dma2d_opened = false;
    ctx->dma2d_owned = false;
    ctx->tmp_yuyv = NULL;

    // Create hardware JPEG decoder instance
    bk_jpeg_decode_hw_config_t hw_config = {0};
    hw_config.decode_cbs.in_complete = hw_jpeg_decode_in_complete;
    hw_config.decode_cbs.out_malloc = hw_jpeg_decode_out_malloc;
    hw_config.decode_cbs.out_complete = hw_jpeg_decode_out_complete;

    avdk_err_t ret = bk_hardware_jpeg_decode_new(&ctx->hw_decoder_handle, &hw_config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to create hardware JPEG decoder, ret=%d\n", __func__, ret);
        return ret;
    }

    // Open hardware decoder
    ret = bk_jpeg_decode_hw_open(ctx->hw_decoder_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to open hardware JPEG decoder, ret=%d\n", __func__, ret);
        bk_jpeg_decode_hw_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
        return ret;
    }

    ctx->is_initialized = true;

    LOGI("%s: Hardware JPEG decoder initialized successfully\n", __func__);

    return AVDK_ERR_OK;
}

// Deinitialize hardware JPEG decoder
static avdk_err_t hw_jpeg_decoder_deinit(struct video_player_video_decoder_ops_s *ops)
{
    hw_jpeg_decoder_instance_t *hw_instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(hw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    hw_jpeg_decoder_ctx_t *ctx = &hw_instance->ctx;

    if (ctx->is_initialized && ctx->hw_decoder_handle != NULL)
    {
        LOGI("%s: Deinitializing hardware JPEG decoder\n", __func__);

        // Close hardware decoder
        bk_jpeg_decode_hw_close(ctx->hw_decoder_handle);

        // Delete hardware decoder instance
        bk_jpeg_decode_hw_delete(ctx->hw_decoder_handle);
        ctx->hw_decoder_handle = NULL;
        ctx->is_initialized = false;
    }

    if (ctx->tmp_yuyv != NULL)
    {
        frame_buffer_display_free(ctx->tmp_yuyv);
        ctx->tmp_yuyv = NULL;
    }

    if (ctx->dma2d_opened && ctx->dma2d_handle != NULL)
    {
        (void)bk_dma2d_close(ctx->dma2d_handle);
        ctx->dma2d_opened = false;
    }

    if (ctx->dma2d_owned && ctx->dma2d_handle != NULL)
    {
        LOGI("%s: Deinitializing DMA2D controller\n", __func__);
        (void)bk_dma2d_delete(ctx->dma2d_handle);
        ctx->dma2d_handle = NULL;
        ctx->dma2d_owned = false;
        LOGI("%s: DMA2D controller deinitialized successfully\n", __func__);
    }

    return AVDK_ERR_OK;
}

// Decode video data using hardware JPEG decoder
static avdk_err_t hw_jpeg_decoder_decode(struct video_player_video_decoder_ops_s *ops,
                                        video_player_buffer_t *in_buffer,
                                        video_player_buffer_t *out_buffer,
                                        pixel_format_t out_fmt)
{
    hw_jpeg_decoder_instance_t *hw_instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(hw_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    hw_jpeg_decoder_ctx_t *ctx = &hw_instance->ctx;
    AVDK_RETURN_ON_FALSE(in_buffer, AVDK_ERR_INVAL, TAG, "in_buffer is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer, AVDK_ERR_INVAL, TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data, AVDK_ERR_INVAL, TAG, "in_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->data, AVDK_ERR_INVAL, TAG, "out_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(ctx->hw_decoder_handle, AVDK_ERR_IO, TAG, "Hardware decoder not initialized");
    AVDK_RETURN_ON_FALSE(ctx->is_initialized, AVDK_ERR_IO, TAG, "Hardware decoder not initialized");
    AVDK_RETURN_ON_FALSE(in_buffer->frame_buffer, AVDK_ERR_INVAL, TAG, "in_buffer->frame_buffer is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->frame_buffer, AVDK_ERR_INVAL, TAG, "out_buffer->frame_buffer is NULL");

    // Output format is provided by caller via out_buffer->frame_buffer->fmt.
    // Hardware JPEG decode default output is YUYV. If caller requests RGB565/RGB888,
    // we decode into a temporary YUYV buffer first, then use DMA2D for pixel conversion.
    frame_buffer_t *in_frame = (frame_buffer_t *)in_buffer->frame_buffer;
    frame_buffer_t *out_frame = (frame_buffer_t *)out_buffer->frame_buffer;

    // out_fmt is the expected output pixel format for this decode call.
    // Keep backward compatibility: if out_fmt is not set, fall back to out_frame->fmt.
    pixel_format_t requested_fmt = out_fmt;
    if (requested_fmt == PIXEL_FMT_UNKNOW || requested_fmt == 0)
    {
        requested_fmt = out_frame->fmt;
    }
    if (requested_fmt == PIXEL_FMT_UNKNOW || requested_fmt == 0)
    {
        requested_fmt = PIXEL_FMT_YUYV;
    }

    // Setup input frame buffer using the provided frame_buffer_t
    in_frame->frame = in_buffer->data;
    in_frame->length = in_buffer->length;
    in_frame->size = in_buffer->length;
    in_frame->width = ctx->video_params.width;
    in_frame->height = ctx->video_params.height;
    in_frame->fmt = PIXEL_FMT_JPEG; // JPEG format for input
    in_frame->timestamp = (uint32_t)in_buffer->pts; // Use PTS as timestamp

    // Prepare output target size based on requested format.
    uint32_t bpp = 0;
    if (!hw_jpeg_pixel_bytes(requested_fmt, &bpp))
    {
        LOGE("%s: Unsupported requested output fmt=%d\n", __func__, requested_fmt);
        return AVDK_ERR_UNSUPPORTED;
    }

    // Get JPEG image info first to verify and get actual dimensions
    bk_jpeg_decode_img_info_t img_info = {0};
    img_info.frame = in_frame;
    avdk_err_t ret = bk_get_jpeg_data_info(&img_info);
    if (ret != AVDK_ERR_OK)
    {
        // If we cannot parse JPEG header on hardware path, treat it as "unsupported" so that
        // upper layer can fall back to the software JPEG decoder. This improves robustness for
        // cases where JPEG stream is valid but not supported by HW parser/decoder.
        LOGW("%s: Failed to get JPEG image info (HW path), fallback to SW, ret=%d\n", __func__, ret);
        return AVDK_ERR_UNSUPPORTED;
    }

    // Hardware JPEG decoder requirement: only supports JPEG YUV422 format
    // If format is not YUV422, return AVDK_ERR_UNSUPPORTED to allow fallback to software decoder
    if (img_info.format != JPEG_FMT_YUV422)
    {
        LOGW("%s: HW JPEG decoder does not support input format=%d (require JPEG_FMT_YUV422), will fallback to SW decoder\n",
             __func__, img_info.format);
        return AVDK_ERR_UNSUPPORTED;
    }

    // Update frame dimensions from JPEG info (may differ from params)
    in_frame->width = img_info.width;
    in_frame->height = img_info.height;

    uint32_t required_out_size = img_info.width * img_info.height * bpp;
    if (required_out_size > out_buffer->length)
    {
        LOGE("%s: Output buffer too small, need=%u, got=%u, fmt=%d\n",
             __func__, required_out_size, out_buffer->length, requested_fmt);
        return AVDK_ERR_NOMEM;
    }

    // If requested output is YUYV, decode directly into caller buffer.
    // Otherwise decode into temp YUYV and do DMA2D conversion to caller buffer.
    frame_buffer_t tmp_out = {0};
    frame_buffer_t *hw_out = out_frame;
    if (requested_fmt != PIXEL_FMT_YUYV)
    {
        uint32_t tmp_size = img_info.width * img_info.height * 2;
        if (ctx->tmp_yuyv == NULL || ctx->tmp_yuyv->size < tmp_size)
        {
            if (ctx->tmp_yuyv != NULL)
            {
                frame_buffer_display_free(ctx->tmp_yuyv);
                ctx->tmp_yuyv = NULL;
            }

            ctx->tmp_yuyv = frame_buffer_display_malloc(tmp_size);
            if (ctx->tmp_yuyv == NULL)
            {
                LOGE("%s: Failed to allocate tmp YUYV buffer, need=%u\n", __func__, tmp_size);
                return AVDK_ERR_NOMEM;
            }
        }

        os_memset(&tmp_out, 0, sizeof(tmp_out));
        tmp_out.frame = ctx->tmp_yuyv->frame;
        tmp_out.size = ctx->tmp_yuyv->size;
        tmp_out.length = 0;
        tmp_out.width = img_info.width;
        tmp_out.height = img_info.height;
        tmp_out.fmt = PIXEL_FMT_YUYV;
        tmp_out.timestamp = (uint32_t)in_buffer->pts;
        hw_out = &tmp_out;
    }

    // Setup output frame buffer for hardware decode.
    hw_out->width = img_info.width;
    hw_out->height = img_info.height;
    hw_out->timestamp = (uint32_t)in_buffer->pts;

    if (hw_out == out_frame)
    {
        out_frame->frame = out_buffer->data;
        out_frame->size = required_out_size;
        out_frame->length = 0;
        out_frame->fmt = PIXEL_FMT_YUYV;
    }

    // Perform hardware JPEG decoding (synchronous) into YUYV.
    ret = bk_jpeg_decode_hw_decode(ctx->hw_decoder_handle, in_frame, hw_out);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Hardware JPEG decode failed, ret=%d\n", __func__, ret);
        return ret;
    }

    // If requested RGB output, do DMA2D pixel conversion YUYV -> RGBxxx.
    if (requested_fmt != PIXEL_FMT_YUYV)
    {
        if (ctx->dma2d_handle == NULL)
        {
            // Try to get existing singleton controller first.
            ctx->dma2d_handle = bk_dma2d_handle_get();
            if (ctx->dma2d_handle == NULL)
            {
                // If controller is not created yet, create one here.
                avdk_err_t nret = bk_dma2d_new(&ctx->dma2d_handle);
                if (nret != AVDK_ERR_OK || ctx->dma2d_handle == NULL)
                {
                    LOGE("%s: bk_dma2d_new failed, ret=%d\n", __func__, nret);
                    return (nret != AVDK_ERR_OK) ? nret : AVDK_ERR_NODEV;
                }
                ctx->dma2d_owned = true;
            }
        }
        if (!ctx->dma2d_opened)
        {
            avdk_err_t dret = bk_dma2d_open(ctx->dma2d_handle);
            if (dret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_dma2d_open failed, ret=%d\n", __func__, dret);
                return dret;
            }
            ctx->dma2d_opened = true;
        }

        // Determine DMA2D output format based on requested_fmt (RGB565/RGB888).
        // Do NOT hardcode default here to avoid confusion; mapping function sets both fields.
        out_color_mode_t out_mode = 0;
        color_bytes_t out_bytes = 0;
        if (!hw_jpeg_dma2d_map_output(requested_fmt, &out_mode, &out_bytes))
        {
            LOGE("%s: Unsupported DMA2D dst fmt=%d\n", __func__, requested_fmt);
            return AVDK_ERR_UNSUPPORTED;
        }

        dma2d_pfc_memcpy_config_t pfc_cfg = {0};
        pfc_cfg.is_sync = true;
        pfc_cfg.pfc.mode = DMA2D_M2M_PFC;
        pfc_cfg.pfc.input_addr = hw_out->frame;
        pfc_cfg.pfc.src_frame_width = (uint16_t)img_info.width;
        pfc_cfg.pfc.src_frame_height = (uint16_t)img_info.height;
        pfc_cfg.pfc.src_frame_xpos = 0;
        pfc_cfg.pfc.src_frame_ypos = 0;
        pfc_cfg.pfc.input_color_mode = DMA2D_INPUT_YUYV;
        pfc_cfg.pfc.src_pixel_byte = TWO_BYTES;
        pfc_cfg.pfc.input_data_reverse = NO_REVERSE;
        pfc_cfg.pfc.input_red_blue_swap = DMA2D_RB_REGULAR;

        pfc_cfg.pfc.output_addr = out_buffer->data;
        pfc_cfg.pfc.dst_frame_width = (uint16_t)img_info.width;
        pfc_cfg.pfc.dst_frame_height = (uint16_t)img_info.height;
        pfc_cfg.pfc.dst_frame_xpos = 0;
        pfc_cfg.pfc.dst_frame_ypos = 0;
        pfc_cfg.pfc.output_color_mode = out_mode;
        pfc_cfg.pfc.dst_pixel_byte = out_bytes;
        pfc_cfg.pfc.output_red_blue_swap = DMA2D_RB_REGULAR;
        pfc_cfg.pfc.out_byte_by_byte_reverse = NO_REVERSE;

        pfc_cfg.pfc.dma2d_width = (uint16_t)img_info.width;
        pfc_cfg.pfc.dma2d_height = (uint16_t)img_info.height;
        pfc_cfg.pfc.input_alpha = 0xFF;
        pfc_cfg.pfc.output_alpha = 0xFF;

        avdk_err_t dret = bk_dma2d_pixel_conversion(ctx->dma2d_handle, &pfc_cfg);
        if (dret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_dma2d_pixel_conversion failed, ret=%d\n", __func__, dret);
            return dret;
        }

        // Fill out_frame metadata for caller output.
        out_frame->frame = out_buffer->data;
        out_frame->size = required_out_size;
        out_frame->length = required_out_size;
        out_frame->width = img_info.width;
        out_frame->height = img_info.height;
        out_frame->fmt = requested_fmt;
        out_frame->timestamp = (uint32_t)in_buffer->pts;
        out_buffer->length = required_out_size;
        out_buffer->pts = in_buffer->pts;
        return AVDK_ERR_OK;
    }

    // Update output buffer with decoded data
    out_frame->width = img_info.width;
    out_frame->height = img_info.height;
    out_frame->fmt = PIXEL_FMT_YUYV;
    out_frame->frame = out_buffer->data;
    out_frame->size = required_out_size;
    out_buffer->length = hw_out->length;
    out_buffer->pts = in_buffer->pts; // Preserve PTS from input
    // Note: out_frame is already out_buffer->frame_buffer, so width/height/length/fmt are already set

    LOGV("%s: Hardware JPEG decode successful, output size=%u\n", __func__, out_buffer->length);

    return AVDK_ERR_OK;
}

static video_player_video_decoder_ops_t *hw_jpeg_decoder_create(void)
{
    hw_jpeg_decoder_instance_t *instance = os_malloc(sizeof(hw_jpeg_decoder_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate hardware JPEG decoder instance\n", __func__);
        return NULL;
    }
    os_memset(instance, 0, sizeof(hw_jpeg_decoder_instance_t));

    os_memcpy(&instance->ops, &s_ops_template, sizeof(video_player_video_decoder_ops_t));
    return &instance->ops;
}

static void hw_jpeg_decoder_destroy(video_player_video_decoder_ops_t *ops)
{
    if (ops == NULL)
    {
        return;
    }
    if (ops == &s_ops_template)
    {
        return;
    }
    hw_jpeg_decoder_instance_t *instance = __containerof(ops, hw_jpeg_decoder_instance_t, ops);
    os_free(instance);
}

static video_player_video_decoder_ops_t s_ops_template = {
    .create = hw_jpeg_decoder_create,
    .destroy = hw_jpeg_decoder_destroy,
    .get_supported_formats = hw_jpeg_decoder_get_supported_formats,
    .init = hw_jpeg_decoder_init,
    .deinit = hw_jpeg_decoder_deinit,
    .decode = hw_jpeg_decoder_decode,
};

video_player_video_decoder_ops_t *bk_video_player_get_hw_jpeg_decoder_ops(void)
{
    return &s_ops_template;
}

