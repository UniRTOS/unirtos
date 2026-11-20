#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include <sys/stat.h>

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"
#include "modules/mp4lib.h"
#include "bk_video_player_jpeg_probe.h"

#define TAG "mp4_parser"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static video_player_container_parser_ops_t s_mp4_parser_ops;

// MP4 container parser private context
typedef struct mp4_parser_ctx_s
{
    mp4_t *mp4_handle;
    const char *file_path;
    uint64_t file_size_bytes;    // Cached file size (best-effort)
    uint64_t duration_ms;        // Cached duration in ms (best-effort)
    
    // Video state
    uint32_t current_video_sample;
    uint32_t total_video_samples;
    uint64_t video_pts_base;
    bool video_eof;
    
    // Audio state
    uint32_t current_audio_sample;
    uint32_t total_audio_samples;
    uint64_t audio_pts_base;
    uint64_t audio_bytes_read;
    bool audio_eof;

    // Audio seek base for PTS generation (ms).
    // When upper layer requests a seek (target_pts != VIDEO_PLAYER_PTS_INVALID),
    // we reset these counters so audio PTS starts from target_pts and increases monotonically.
    uint64_t audio_seek_pts_ms;
    uint64_t audio_time_units_since_seek;
    
    // Video info (cached)
    uint32_t video_width;
    uint32_t video_height;
    double video_fps;
    uint32_t video_codec;
    video_player_jpeg_subsampling_t jpeg_subsampling;
    
    // Audio info (cached)
    uint16_t audio_channels;
    uint32_t audio_rate;
    uint16_t audio_bits;
    uint32_t audio_codec;
    
    // Thread safety
    beken_mutex_t mutex;
    
    // Index availability flags
    bool has_index;

    // Upper-layer encoded video packet buffer callbacks (optional).
    // Used to allocate temporary buffers (e.g., MJPEG first-frame probe) without using os_malloc/os_free.
    void *video_packet_user_data;
    video_player_video_packet_buffer_alloc_cb_t video_packet_alloc_cb;
    video_player_video_packet_buffer_free_cb_t video_packet_free_cb;
} mp4_parser_ctx_t;

// Allocate ops + context in a single block to avoid double os_malloc.
// Keep ops as the first member so we can return &instance->ops.
typedef struct
{
    video_player_container_parser_ops_t ops;
    mp4_parser_ctx_t ctx;
} mp4_parser_instance_t;

// Caller must hold ctx->mutex.
static uint64_t mp4_audio_frame_ms(const mp4_parser_ctx_t *ctx)
{
    if (ctx == NULL || ctx->mp4_handle == NULL)
    {
        return 0;
    }

    uint32_t ts = MP4_audio_timescale(ctx->mp4_handle);
    uint32_t dur = MP4_audio_duration(ctx->mp4_handle);
    uint32_t cnt = MP4_audio_samples(ctx->mp4_handle);
    if (ts == 0 || dur == 0 || cnt == 0)
    {
        return 0;
    }

    uint64_t denom = (uint64_t)ts * (uint64_t)cnt;
    if (denom == 0)
    {
        return 0;
    }

    // Derive per-sample duration from mdhd duration and sample_count.
    // This avoids hardcoding AAC(1024 samples/frame) and matches writer's constant STTS model.
    uint64_t num = (uint64_t)dur * 1000ULL;
    uint64_t frame_ms = (num + denom / 2ULL) / denom;
    if (frame_ms == 0)
    {
        frame_ms = 1;
    }
    return frame_ms;
}

// Caller must hold ctx->mutex.
static uint32_t mp4_find_video_sample_by_pts(mp4_parser_ctx_t *ctx, uint64_t target_pts_ms)
{
    if (ctx == NULL || ctx->mp4_handle == NULL)
    {
        return 0;
    }

    if (ctx->total_video_samples == 0)
    {
        return 0;
    }

    if (ctx->video_fps > 0)
    {
        // IMPORTANT:
        // ctx->video_fps can be fractional (e.g. 19.42). Do NOT cast it to integer here.
        // Otherwise, PTS/sample mapping drifts and video may appear longer/shorter than audio.
        double idx_d = ((double)target_pts_ms * ctx->video_fps) / 1000.0;
        if (idx_d < 0.0)
        {
            idx_d = 0.0;
        }
        uint64_t idx64 = (uint64_t)(idx_d + 0.5);
        if (idx64 >= (uint64_t)ctx->total_video_samples)
        {
            idx64 = (uint64_t)ctx->total_video_samples - 1ULL;
        }
        return (uint32_t)idx64;
    }

    // Fallback: proportional mapping by duration (if available).
    if (ctx->duration_ms > 0)
    {
        uint64_t scaled = (target_pts_ms >= ctx->duration_ms) ? (uint64_t)(ctx->total_video_samples - 1U)
                                                              : (target_pts_ms * (uint64_t)ctx->total_video_samples) / ctx->duration_ms;
        if (scaled >= (uint64_t)ctx->total_video_samples)
        {
            scaled = (uint64_t)ctx->total_video_samples - 1ULL;
        }
        return (uint32_t)scaled;
    }
    return 0;
}

// Caller must hold ctx->mutex.
static uint32_t mp4_find_prev_video_sync_sample_index(mp4_parser_ctx_t *ctx, uint32_t sample)
{
    if (ctx == NULL || ctx->mp4_handle == NULL) {
        return 0;
    }

    // Use mp4lib helper to support segmented sample tables.
    return MP4_find_prev_video_sync_sample(ctx->mp4_handle, sample);
}

// Caller must hold ctx->mutex.
static uint64_t mp4_video_sample_pts_ms(const mp4_parser_ctx_t *ctx, uint32_t sample)
{
    if (ctx == NULL) {
        return 0;
    }
    if (ctx->video_fps > 0) {
        // Use fractional fps to avoid accumulating drift (e.g. 19.42fps -> 19fps).
        double pts_d = ((double)sample * 1000.0) / ctx->video_fps;
        if (pts_d < 0.0)
        {
            return 0;
        }
        return (uint64_t)(pts_d + 0.5);
    }
    if (ctx->duration_ms > 0 && ctx->total_video_samples > 0) {
        return ((uint64_t)sample * ctx->duration_ms) / (uint64_t)ctx->total_video_samples;
    }
    return 0;
}

// Caller must hold ctx->mutex.
static uint64_t mp4_adjust_seek_pts_for_h264_sync_sample(mp4_parser_ctx_t *ctx, uint64_t target_pts_ms)
{
    if (ctx == NULL || ctx->mp4_handle == NULL) {
        return target_pts_ms;
    }
    if (target_pts_ms == VIDEO_PLAYER_PTS_INVALID) {
        return target_pts_ms;
    }
    if (!ctx->has_index || ctx->video_codec != MP4_CODEC_H264 || ctx->total_video_samples == 0) {
        return target_pts_ms;
    }

    uint32_t desired = mp4_find_video_sample_by_pts(ctx, target_pts_ms);
    uint32_t key = mp4_find_prev_video_sync_sample_index(ctx, desired);
    return mp4_video_sample_pts_ms(ctx, key);
}

// Caller must hold ctx->mutex.
static uint32_t mp4_find_audio_sample_by_pts(mp4_parser_ctx_t *ctx, uint64_t target_pts_ms)
{
    if (ctx == NULL || ctx->mp4_handle == NULL)
    {
        return 0;
    }

    if (ctx->total_audio_samples == 0)
    {
        return 0;
    }

    uint32_t sample = 0;

    if (ctx->audio_codec == MP4_CODEC_AAC)
    {
        uint64_t frame_ms = mp4_audio_frame_ms(ctx);
        if (frame_ms > 0)
        {
            sample = (uint32_t)(target_pts_ms / frame_ms);
        }
        else
        {
            sample = 0;
        }
    }
    else if (ctx->duration_ms > 0)
    {
        // Best-effort mapping for other codecs: proportionally map by container duration.
        // This makes target_pts "actually effective" even when we don't have per-sample audio PTS.
        uint64_t scaled = (target_pts_ms >= ctx->duration_ms) ? (uint64_t)(ctx->total_audio_samples - 1U)
                                                              : (target_pts_ms * (uint64_t)ctx->total_audio_samples) / ctx->duration_ms;
        sample = (uint32_t)scaled;
    }
    else
    {
        sample = 0;
    }

    if (sample >= ctx->total_audio_samples)
    {
        sample = ctx->total_audio_samples - 1U;
    }
    return sample;
}

// Open MP4 container
static avdk_err_t mp4_parser_open(struct video_player_container_parser_ops_s *ops, const char *file_path)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");
    
    LOGV("%s: Opening MP4 container: %s\n", __func__, file_path);
    
    if (ctx->mp4_handle != NULL)
    {
        // Close previous file if any to avoid leaking state across sessions.
        MP4_close_input_file((mp4_t *)ctx->mp4_handle);
        ctx->mp4_handle = NULL;
    }
    if (ctx->mutex != NULL)
    {
        rtos_deinit_mutex(&ctx->mutex);
        ctx->mutex = NULL;
    }
    
    ctx->mp4_handle = MP4_open_input_file(file_path, 1, MP4_MEM_PSRAM);
    if (ctx->mp4_handle == NULL)
    {
        LOGE("%s: Failed to open MP4 file: %s\n", __func__, file_path);
        return AVDK_ERR_IO;
    }
    
    LOGV("%s: MP4 file opened successfully\n", __func__);
    
    ctx->file_path = file_path;
    ctx->file_size_bytes = 0;
    ctx->duration_ms = 0;

    // Cache file size best-effort.
    struct stat st;
    if (stat(file_path, &st) == 0 && st.st_size > 0)
    {
        ctx->file_size_bytes = (uint64_t)st.st_size;
    }
    
    // Get video info
    ctx->video_width = MP4_video_width(ctx->mp4_handle);
    ctx->video_height = MP4_video_height(ctx->mp4_handle);
    ctx->video_fps = MP4_video_frame_rate(ctx->mp4_handle);
    ctx->total_video_samples = MP4_video_frames(ctx->mp4_handle);
    ctx->video_codec = MP4_video_codec(ctx->mp4_handle);
    ctx->jpeg_subsampling = VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE;
    
    LOGV("%s: Video info retrieved: width=%u, height=%u, fps=%.2f, samples=%u, codec=0x%08x\n",
         __func__, ctx->video_width, ctx->video_height, ctx->video_fps, ctx->total_video_samples, ctx->video_codec);

    // MJPEG: probe first frame subsampling once during open (best-effort).
    // Keep codec-specific probing out of parse_video_info() to avoid side effects during playback.
    if (ctx->video_codec == MP4_CODEC_MJPEG && ctx->total_video_samples > 0)
    {
        uint32_t first_frame_size = 0;
        if (MP4_set_video_read_index(ctx->mp4_handle, 0, &first_frame_size) == 0 && first_frame_size > 0)
        {
            if (ctx->video_packet_alloc_cb == NULL || ctx->video_packet_free_cb == NULL)
            {
                LOGW("%s: video packet buffer callbacks not set, skip MJPEG subsampling probe\n", __func__);
            }
            else
            {
                video_player_buffer_t tmp = {0};
                tmp.length = first_frame_size;
                avdk_err_t alloc_ret = ctx->video_packet_alloc_cb(ctx->video_packet_user_data, &tmp);
                if (alloc_ret == AVDK_ERR_OK && tmp.data != NULL && tmp.length >= first_frame_size)
                {
                    uint32_t bytes_read = MP4_read_next_video_sample(ctx->mp4_handle, tmp.data, first_frame_size);
                    if (bytes_read == first_frame_size)
                    {
                        video_player_jpeg_subsampling_t subsampling = VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE;
                        avdk_err_t jpeg_ret = bk_video_player_probe_jpeg_subsampling(tmp.data, first_frame_size, &subsampling);
                        if (jpeg_ret == AVDK_ERR_OK)
                        {
                            ctx->jpeg_subsampling = subsampling;
                            LOGI("%s: Detected JPEG subsampling=%u (NONE=0,YUV444=1,YUV422=2,YUV420=3,YUV400=4)\n",
                                 __func__, (uint32_t)ctx->jpeg_subsampling);
                        }
                        else
                        {
                            LOGW("%s: Failed to parse JPEG header from first frame, ret=%d\n", __func__, jpeg_ret);
                        }
                    }
                    else
                    {
                        LOGW("%s: Failed to read first frame, expected %u bytes, got %u\n",
                             __func__, first_frame_size, bytes_read);
                    }

                    ctx->video_packet_free_cb(ctx->video_packet_user_data, &tmp);
                }
                else
                {
                    LOGW("%s: video_packet_alloc_cb failed, ret=%d, size=%u\n", __func__, alloc_ret, first_frame_size);
                    if (alloc_ret == AVDK_ERR_OK && tmp.data != NULL)
                    {
                        ctx->video_packet_free_cb(ctx->video_packet_user_data, &tmp);
                    }
                }

                // Restore read index for normal playback after probing the first frame.
                if (MP4_set_video_read_index(ctx->mp4_handle, 0, NULL) != 0)
                {
                    LOGW("%s: Failed to reset video read index after MJPEG probe\n", __func__);
                }
            }
        }
        else
        {
            LOGW("%s: Invalid first_frame_size=%u for MJPEG probe\n", __func__, first_frame_size);
        }
    }
    
    // Get audio info
    ctx->audio_channels = MP4_audio_channels(ctx->mp4_handle);
    ctx->audio_rate = MP4_audio_rate(ctx->mp4_handle);
    ctx->audio_bits = MP4_audio_bits(ctx->mp4_handle);
    ctx->audio_codec = MP4_audio_format(ctx->mp4_handle);
    
    LOGV("%s: Audio info retrieved: channels=%u, rate=%u, bits=%u, codec=0x%08x\n",
         __func__, ctx->audio_channels, ctx->audio_rate, ctx->audio_bits, ctx->audio_codec);
    
    // Get audio sample count from MP4 handle
    ctx->total_audio_samples = MP4_audio_samples(ctx->mp4_handle);
    
    // Initialize decode state
    ctx->current_video_sample = 0;
    ctx->current_audio_sample = 0;
    ctx->audio_bytes_read = 0;
    ctx->video_pts_base = 0;
    ctx->audio_pts_base = 0;
    ctx->audio_seek_pts_ms = 0;
    ctx->audio_time_units_since_seek = 0;
    ctx->video_eof = false;
    ctx->audio_eof = false;
    
    // Check if index is available
    ctx->has_index = MP4_has_index(ctx->mp4_handle);

    // Cache duration best-effort.
    if (ctx->video_fps > 0 && ctx->total_video_samples > 0)
    {
        ctx->duration_ms = (uint64_t)((double)ctx->total_video_samples / ctx->video_fps * 1000.0 + 0.5);
    }
    
    // Initialize mutex for thread safety
    if (rtos_init_mutex(&ctx->mutex) != BK_OK)
    {
        LOGE("%s: Failed to init mutex\n", __func__);
        // Note: Need to close MP4 file here when close function is available
        ctx->mp4_handle = NULL;
        return AVDK_ERR_IO;
    }
    
    LOGI("%s: video: %ux%u@%.2ffps, samples=%u, audio: %uch@%uHz@%ubits has_index=%s\n",
         __func__, ctx->video_width, ctx->video_height, ctx->video_fps, ctx->total_video_samples,
         ctx->audio_channels, ctx->audio_rate, ctx->audio_bits, ctx->has_index ? "yes" : "no");
    
    return AVDK_ERR_OK;
}

// Close MP4 container
static avdk_err_t mp4_parser_close(struct video_player_container_parser_ops_s *ops)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    if (mp4_instance == NULL)
    {
        return AVDK_ERR_INVAL;
    }
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    
    LOGI("%s: Closing MP4 container\n", __func__);
    
    if (ctx->mp4_handle != NULL)
    {
        MP4_close_input_file((mp4_t *)ctx->mp4_handle);
        ctx->mp4_handle = NULL;
    }

    if (ctx->mutex != NULL)
    {
        rtos_deinit_mutex(&ctx->mutex);
        ctx->mutex = NULL;
    }
    
    return AVDK_ERR_OK;
}

// Parse audio info from MP4 container
static avdk_err_t mp4_parser_parse_audio_info(struct video_player_container_parser_ops_s *ops, video_player_audio_params_t *audio_params)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "instance is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(audio_params, AVDK_ERR_INVAL, TAG, "audio_params is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");
    
    if (ctx->audio_channels == 0 || ctx->audio_rate == 0)
    {
        LOGW("%s: MP4 file has no audio stream\n", __func__);
        return AVDK_ERR_NODEV;
    }
    
    audio_params->channels = ctx->audio_channels;
    audio_params->sample_rate = ctx->audio_rate;
    audio_params->bits_per_sample = ctx->audio_bits;
    audio_params->codec_config = NULL;
    audio_params->codec_config_size = 0;
    
    // Map MP4 codec type to audio format
    if (ctx->audio_codec == MP4_CODEC_AAC)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_AAC;
    }
    else if (ctx->audio_codec == MP4_CODEC_MP3)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_MP3;
    }
    else if (ctx->audio_codec == MP4_CODEC_PCM_SOWT || ctx->audio_codec == MP4_CODEC_PCM_TWOS)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_PCM;
    }
    else if (ctx->audio_codec == MP4_CODEC_ALAW)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_G711A;
    }
    else if (ctx->audio_codec == MP4_CODEC_ULAW)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_G711U;
    }
    else if (ctx->audio_codec == MP4_CODEC_G722)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_G722;
    }
    else
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_UNKNOWN;
    }

    audio_params->codec_config = MP4_audio_codec_config(ctx->mp4_handle, &audio_params->codec_config_size);
    
    if (audio_params->format == VIDEO_PLAYER_AUDIO_FORMAT_UNKNOWN)
    {
        LOGE("%s: Unsupported MP4 audio codec=0x%08x\n", __func__, (unsigned)ctx->audio_codec);
        return AVDK_ERR_NODEV;
    }

    return AVDK_ERR_OK;
}

// Parse video info from MP4 file
static avdk_err_t mp4_parser_parse_video_info(struct video_player_container_parser_ops_s *ops, video_player_video_params_t *video_params)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(video_params, AVDK_ERR_INVAL, TAG, "video_params is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");
    
    if (ctx->video_width == 0 || ctx->video_height == 0)
    {
        LOGW("%s: MP4 file has no video stream\n", __func__);
        return AVDK_ERR_NODEV;
    }
    
    video_params->width = ctx->video_width;
    video_params->height = ctx->video_height;
    video_params->fps = (uint32_t)(ctx->video_fps + 0.5);
    video_params->jpeg_subsampling = ctx->jpeg_subsampling;
    
    // Map MP4 codec type to video format
    if (ctx->video_codec == MP4_CODEC_H264)
    {
        video_params->format = VIDEO_PLAYER_VIDEO_FORMAT_H264;
    }
    else if (ctx->video_codec == MP4_CODEC_MJPEG)
    {
        video_params->format = VIDEO_PLAYER_VIDEO_FORMAT_MJPEG;
    }
    else
    {
        video_params->format = VIDEO_PLAYER_VIDEO_FORMAT_UNKNOWN;
    }
    
    LOGV("%s: Video info: width=%u, height=%u, fps=%u, format=%u, jpeg_subsampling=%u\n",
         __func__, video_params->width, video_params->height,
         video_params->fps, video_params->format, video_params->jpeg_subsampling);
    
    return AVDK_ERR_OK;
}

// Get encoded video packet size
static avdk_err_t mp4_parser_get_video_packet_size(struct video_player_container_parser_ops_s *ops, uint32_t *packet_size, uint64_t target_pts)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(packet_size, AVDK_ERR_INVAL, TAG, "packet_size is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");
    
    *packet_size = 0;
    
    rtos_lock_mutex(&ctx->mutex);
    
    uint32_t target_sample = ctx->current_video_sample;
    if (ctx->has_index)
    {
        if (target_pts != VIDEO_PLAYER_PTS_INVALID)
        {
            uint32_t desired = mp4_find_video_sample_by_pts(ctx, target_pts);

            if (ctx->video_codec == MP4_CODEC_H264)
            {
                uint32_t key = mp4_find_prev_video_sync_sample_index(ctx, desired);
                target_sample = (key < desired) ? key : desired;
            }
            else
            {
                target_sample = desired;
            }
        }
    }

    if (ctx->total_video_samples > 0 && target_sample >= ctx->total_video_samples)
    {
        ctx->video_eof = true;
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_EOF; // EOF
    }

    uint32_t sample_size = 0;
    if (MP4_set_video_read_index(ctx->mp4_handle, target_sample, &sample_size) != 0 || sample_size == 0)
    {
        ctx->video_eof = true;
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_EOF; // EOF
    }
    *packet_size = sample_size;
    
    rtos_unlock_mutex(&ctx->mutex);
    
    return AVDK_ERR_OK;
}

// Get encoded audio packet size
static avdk_err_t mp4_parser_get_audio_packet_size(struct video_player_container_parser_ops_s *ops, uint32_t *packet_size, uint64_t target_pts)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(packet_size, AVDK_ERR_INVAL, TAG, "packet_size is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");
    
    *packet_size = 0;

    rtos_lock_mutex(&ctx->mutex);

    if (ctx->has_index && target_pts != VIDEO_PLAYER_PTS_INVALID)
    {
        uint64_t anchor_pts = mp4_adjust_seek_pts_for_h264_sync_sample(ctx, target_pts);
        uint32_t target_sample = mp4_find_audio_sample_by_pts(ctx, anchor_pts);
        ctx->current_audio_sample = target_sample;
        ctx->audio_eof = false;
        ctx->audio_seek_pts_ms = anchor_pts;
        ctx->audio_time_units_since_seek = 0;
    }

    if (ctx->current_audio_sample >= ctx->total_audio_samples)
    {
        ctx->audio_eof = true;
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_EOF;
    }

    uint32_t sample_size = 0;
    if (MP4_set_audio_read_index(ctx->mp4_handle, ctx->current_audio_sample, &sample_size) != 0 || sample_size == 0)
    {
        ctx->audio_eof = true;
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_EOF;
    }

    *packet_size = sample_size;
    rtos_unlock_mutex(&ctx->mutex);

    return AVDK_ERR_OK;
}

// Read encoded video packet from MP4 container
static avdk_err_t mp4_parser_read_video_packet(struct video_player_container_parser_ops_s *ops, video_player_buffer_t *out_buffer, uint64_t target_pts)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(out_buffer, AVDK_ERR_INVAL, TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");
    AVDK_RETURN_ON_FALSE(out_buffer->data, AVDK_ERR_INVAL, TAG, "out_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->length > 0, AVDK_ERR_INVAL, TAG, "out_buffer->length is 0");
    
    rtos_lock_mutex(&ctx->mutex);
    
    if (ctx->has_index)
    {
        uint32_t target_sample = ctx->current_video_sample;
        
        if (target_pts != VIDEO_PLAYER_PTS_INVALID)
        {
            uint32_t desired = mp4_find_video_sample_by_pts(ctx, target_pts);
            target_sample = desired;

            if (ctx->video_codec == MP4_CODEC_H264)
            {
                uint32_t key = mp4_find_prev_video_sync_sample_index(ctx, desired);
                target_sample = (key < desired) ? key : desired;
            }
            if (target_sample >= ctx->total_video_samples)
            {
                ctx->video_eof = true;
                out_buffer->length = 0;
                rtos_unlock_mutex(&ctx->mutex);
                return AVDK_ERR_EOF; // EOF
            }
            /*
             * NOTE:
             * Upper layer may call read_video_packet() multiple times with the same target_pts while
             * A/V clock is not progressing fast enough (e.g., buffer backpressure or audio-driven clock jitter).
             * This would cause repeated "Seek video ..." logs although it is not an actual issue.
             * Keep this log at verbose level to avoid confusing repeated INFO prints.
             */
            ctx->current_video_sample = target_sample;
            LOGV("%s: Seek video to target_pts=%llu ms -> sample=%u\n",
                 __func__, (unsigned long long)target_pts, target_sample);
        }
        
        if (ctx->current_video_sample >= ctx->total_video_samples)
        {
            ctx->video_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_EOF; // EOF
        }
        
        uint32_t sample_size = 0;
        if (MP4_set_video_read_index(ctx->mp4_handle, ctx->current_video_sample, &sample_size) != 0)
        {
            LOGE("%s: Failed to set video position to sample %u\n", __func__, ctx->current_video_sample);
            ctx->video_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }
        
        if (out_buffer->length < sample_size)
        {
            LOGE("%s: Buffer too small, need %u bytes, got %u bytes. This should not happen if buffer was allocated correctly.\n",
                 __func__, sample_size, out_buffer->length);
            LOGE("%s: Sample %u size mismatch. Please check buffer allocation callback.\n", 
                 __func__, ctx->current_video_sample);
            // Return error instead of truncating, let upper layer handle it
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_NOMEM;
        }
        
        uint32_t bytes_read = MP4_read_next_video_sample(ctx->mp4_handle, out_buffer->data, sample_size);
        if (bytes_read == 0)
        {
            LOGE("%s: Failed to read video sample %u\n", __func__, ctx->current_video_sample);
            ctx->video_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }
        
        out_buffer->length = bytes_read;
        
        // Use sample PTS (ms) derived from fractional fps for seek/sync.
        uint32_t sample_num = ctx->current_video_sample;
        if (ctx->video_fps > 0)
        {
            double pts_d = ((double)sample_num * 1000.0) / ctx->video_fps;
            out_buffer->pts = (pts_d < 0.0) ? 0 : (uint64_t)(pts_d + 0.5);
        }
        else if (ctx->duration_ms > 0 && ctx->total_video_samples > 0)
        {
            out_buffer->pts = ((uint64_t)sample_num * ctx->duration_ms) / (uint64_t)ctx->total_video_samples;
        }
        else
        {
            out_buffer->pts = 0;
        }
        
        ctx->current_video_sample++;
        
        if (ctx->current_video_sample >= ctx->total_video_samples)
        {
            ctx->video_eof = true;
        }
        
        rtos_unlock_mutex(&ctx->mutex);
        LOGV("%s: Read video sample %u, size=%u, pts=%llu\n",
             __func__, sample_num, out_buffer->length, out_buffer->pts);
        return AVDK_ERR_OK;
    }
    else
    {
        LOGE("%s: No index available, cannot decode video\n", __func__);
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_IO;
    }
}

// Decode audio data from MP4 file
static avdk_err_t mp4_parser_read_audio_packet(struct video_player_container_parser_ops_s *ops, video_player_buffer_t *out_buffer, uint64_t target_pts)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(out_buffer, AVDK_ERR_INVAL, TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");
    AVDK_RETURN_ON_FALSE(out_buffer->data, AVDK_ERR_INVAL, TAG, "out_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->length > 0, AVDK_ERR_INVAL, TAG, "out_buffer->length is 0");
    
    rtos_lock_mutex(&ctx->mutex);
    
    if (ctx->has_index)
    {
        if (target_pts != VIDEO_PLAYER_PTS_INVALID)
        {
            uint64_t anchor_pts = mp4_adjust_seek_pts_for_h264_sync_sample(ctx, target_pts);
            uint32_t target_sample = mp4_find_audio_sample_by_pts(ctx, anchor_pts);
            ctx->current_audio_sample = target_sample;
            ctx->audio_eof = false;
            ctx->audio_seek_pts_ms = anchor_pts;
            ctx->audio_time_units_since_seek = 0;
        }

        if (ctx->current_audio_sample >= ctx->total_audio_samples)
        {
            ctx->audio_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_EOF; // EOF
        }
        
        uint32_t sample_size = 0;
        if (MP4_set_audio_read_index(ctx->mp4_handle, ctx->current_audio_sample, &sample_size) != 0)
        {
            LOGE("%s: Failed to set audio position to sample %u\n", __func__, ctx->current_audio_sample);
            ctx->audio_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }
        
        if (out_buffer->length < sample_size)
        {
            LOGW("%s: Buffer too small, need %u bytes, got %u bytes\n",
                 __func__, sample_size, out_buffer->length);
            sample_size = out_buffer->length;
        }
        
        uint32_t bytes_read = MP4_read_next_audio_sample(ctx->mp4_handle, out_buffer->data, sample_size);
        if (bytes_read == 0)
        {
            LOGE("%s: Failed to read audio sample %u\n", __func__, ctx->current_audio_sample);
            ctx->audio_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }

        out_buffer->length = bytes_read;

        if (ctx->audio_rate > 0)
        {
            if (ctx->audio_codec == MP4_CODEC_AAC)
            {
                uint64_t frame_ms = mp4_audio_frame_ms(ctx);
                out_buffer->pts = ctx->audio_seek_pts_ms + ctx->audio_time_units_since_seek * frame_ms;
                ctx->audio_time_units_since_seek++;
            }
            else
            {
                /*
                 * Non-AAC audio in MP4 can be either PCM or a packetized codec (e.g., G711/G722).
                 *
                 * Root cause fix:
                 * Some packetized codecs have audio_bits == 0 (compressed), so we must NOT treat them as PCM,
                 * otherwise bytes_per_sample becomes 0 and PTS stays 0, causing audio to run ahead of video.
                 */
                uint32_t bytes_per_sample = (uint32_t)ctx->audio_channels * ((uint32_t)ctx->audio_bits / 8U);
                if (bytes_per_sample > 0)
                {
                    // PCM: derive sample count from bytes.
                    uint32_t samples_in_buffer = bytes_read / bytes_per_sample;
                    uint64_t denom = (uint64_t)ctx->audio_rate;
                    out_buffer->pts = ctx->audio_seek_pts_ms +
                                      ((ctx->audio_time_units_since_seek * 1000ULL + denom / 2ULL) / denom);
                    ctx->audio_time_units_since_seek += (uint64_t)samples_in_buffer;
                }
                else if (ctx->duration_ms > 0 && ctx->total_audio_samples > 0)
                {
                    // Packetized/compressed: estimate per-sample PTS from total duration and sample index.
                    uint32_t sample_num = ctx->current_audio_sample;
                    out_buffer->pts = ((uint64_t)sample_num * ctx->duration_ms + (uint64_t)ctx->total_audio_samples / 2ULL) /
                                      (uint64_t)ctx->total_audio_samples;
                    ctx->audio_time_units_since_seek++;
                }
                else
                {
                    out_buffer->pts = 0;
                }
            }
        }
        else
        {
            out_buffer->pts = 0;
        }
        
        ctx->current_audio_sample++;
        
        if (ctx->current_audio_sample >= ctx->total_audio_samples)
        {
            ctx->audio_eof = true;
        }
        
        rtos_unlock_mutex(&ctx->mutex);
        LOGV("%s: Read audio sample %u, size=%u, pts=%llu\n",
             __func__, ctx->current_audio_sample - 1, out_buffer->length, out_buffer->pts);
        return AVDK_ERR_OK;
    }
    else
    {
        LOGE("%s: No index available, cannot read audio packet\n", __func__);
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_IO;
    }
}

// Get total duration of MP4 file (in milliseconds)
static avdk_err_t mp4_parser_get_duration(struct video_player_container_parser_ops_s *ops, uint64_t *duration_ms)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(duration_ms, AVDK_ERR_INVAL, TAG, "duration_ms is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");

    *duration_ms = ctx->duration_ms;
    return (*duration_ms > 0) ? AVDK_ERR_OK : AVDK_ERR_UNSUPPORTED;
}

static avdk_err_t mp4_parser_get_media_info(struct video_player_container_parser_ops_s *ops, video_player_media_info_t *media_info)
{
    mp4_parser_instance_t *mp4_instance = __containerof(ops, mp4_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(mp4_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    mp4_parser_ctx_t *ctx = &mp4_instance->ctx;
    AVDK_RETURN_ON_FALSE(media_info, AVDK_ERR_INVAL, TAG, "media_info is NULL");
    AVDK_RETURN_ON_FALSE(ctx->mp4_handle, AVDK_ERR_IO, TAG, "MP4 file not opened");

    os_memset(media_info, 0, sizeof(*media_info));
    bool got_any = false;

    // File-level info is cached during open(), get_media_info() only copies.
    media_info->duration_ms = ctx->duration_ms;
    media_info->file_size_bytes = ctx->file_size_bytes;

    if (mp4_parser_parse_video_info(ops, &media_info->video) == AVDK_ERR_OK)
    {
        got_any = true;
    }
    if (mp4_parser_parse_audio_info(ops, &media_info->audio) == AVDK_ERR_OK)
    {
        got_any = true;
    }

    return got_any ? AVDK_ERR_OK : AVDK_ERR_NODEV;
}

static video_player_container_parser_ops_t *mp4_parser_create(void *video_packet_user_data,
                                                              video_player_video_packet_buffer_alloc_cb_t video_packet_alloc_cb,
                                                              video_player_video_packet_buffer_free_cb_t video_packet_free_cb)
{
    mp4_parser_instance_t *instance = os_malloc(sizeof(mp4_parser_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate MP4 parser instance\n", __func__);
        return NULL;
    }
    os_memset(instance, 0, sizeof(mp4_parser_instance_t));

    os_memcpy(&instance->ops, &s_mp4_parser_ops, sizeof(video_player_container_parser_ops_t));

    // Inject optional packet buffer callbacks at create-time (dependency injection).
    instance->ctx.video_packet_user_data = video_packet_user_data;
    instance->ctx.video_packet_alloc_cb = video_packet_alloc_cb;
    instance->ctx.video_packet_free_cb = video_packet_free_cb;

    return &instance->ops;
}

static void mp4_parser_destroy(struct video_player_container_parser_ops_s *ops)
{
    if (ops == NULL || ops == &s_mp4_parser_ops)
    {
        return;
    }
    mp4_parser_instance_t *instance = __containerof(ops, mp4_parser_instance_t, ops);
    os_free(instance);
}

static avdk_err_t mp4_parser_get_supported_file_extensions(const struct video_player_container_parser_ops_s *ops,
                                                           const char * const **exts,
                                                           uint32_t *ext_count)
{
    (void)ops;
    if (exts == NULL || ext_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const char * const s_exts[] = {
        ".mp4",
        ".m4a",
        ".mov",
    };

    *exts = s_exts;
    *ext_count = (uint32_t)(sizeof(s_exts) / sizeof(s_exts[0]));
    return AVDK_ERR_OK;
}

// MP4 container parser operations
static video_player_container_parser_ops_t s_mp4_parser_ops = {
    .create = mp4_parser_create,
    .destroy = mp4_parser_destroy,
    .open = mp4_parser_open,
    .close = mp4_parser_close,
    .get_media_info = mp4_parser_get_media_info,
    .get_video_packet_size = mp4_parser_get_video_packet_size,
    .get_audio_packet_size = mp4_parser_get_audio_packet_size,
    .read_video_packet = mp4_parser_read_video_packet,
    .read_audio_packet = mp4_parser_read_audio_packet,
    .get_supported_file_extensions = mp4_parser_get_supported_file_extensions,
};

// Get MP4 container parser operations
video_player_container_parser_ops_t *bk_video_player_get_mp4_parser_ops(void)
{
    return &s_mp4_parser_ops;
}

