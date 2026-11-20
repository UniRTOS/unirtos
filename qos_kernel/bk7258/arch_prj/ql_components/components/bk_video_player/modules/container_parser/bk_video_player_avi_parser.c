#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include <sys/stat.h>
#include <limits.h>

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"
#include "modules/avilib.h"
#include "bk_video_player_jpeg_probe.h"

#define TAG "avi_parser"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct avi_parser_ctx_s avi_parser_ctx_t;

// AVI container parser private context
typedef struct avi_parser_ctx_s
{
    avi_t *avi_handle;              // AVI file handle (shared between video and audio threads)
    const char *file_path;           // File path
    uint64_t file_size_bytes;        // Cached file size (best-effort)
    uint64_t duration_ms;            // Cached duration in ms (best-effort)

    // Video state
    long current_video_frame;        // Current video frame number
    long total_video_frames;         // Total video frames
    uint64_t video_pts_base;         // Base PTS for video (in milliseconds)
    bool video_eof;                  // Video end of file flag

    // Audio state
    long current_audio_chunk;        // Current audio chunk number
    long total_audio_chunks;         // Total audio chunks
    uint64_t audio_pts_base;         // Base PTS for audio (in milliseconds)
    long audio_bytes_read;           // Total audio bytes read
    bool audio_eof;                  // Audio end of file flag

    // AAC timing (ADTS framed AAC in AVI)
    uint64_t audio_aac_samples;      // Total decoded AAC samples (per channel) before current chunk
    bool audio_aac_is_adts;          // true: ADTS stream; false: raw AAC access unit per chunk

    // Video info (cached)
    int video_width;
    int video_height;
    double video_fps;
    video_player_video_format_t video_format;
    video_player_jpeg_subsampling_t jpeg_subsampling;

    // Audio info (cached)
    int audio_channels;
    long audio_rate;
    int audio_bits;
    int audio_format;

    // Thread safety (for pipeline mode with separate threads)
    beken_mutex_t mutex;             // Mutex for thread-safe access

    // Index availability flags
    bool has_video_index;            // Whether video index is available
    bool has_audio_index;            // Whether audio index is available

    // Upper-layer encoded video packet buffer callbacks (optional).
    // Used to allocate temporary buffers (e.g., MJPEG first-frame probe) without using os_malloc/os_free.
    void *video_packet_user_data;
    video_player_video_packet_buffer_alloc_cb_t video_packet_alloc_cb;
    video_player_video_packet_buffer_free_cb_t video_packet_free_cb;
} avi_parser_ctx_t;

// Allocate ops + context in a single block to avoid double os_malloc.
// Keep ops as the first member so we can return &instance->ops.
typedef struct
{
    video_player_container_parser_ops_t ops;
    avi_parser_ctx_t ctx;
} avi_parser_instance_t;

static video_player_container_parser_ops_t s_avi_parser_ops;

// -----------------------------------------------------------------------------
// PTS mapping helpers (parser-side only)
// -----------------------------------------------------------------------------
//
// NOTE:
// - Align with mp4lib: avilib provides "read index" primitives only.
// - PTS -> frame/chunk mapping is a player/parser responsibility.
//

// Map video PTS (ms) to frame index by cached fps.
// Return 0 on success, -1 on invalid params or pts beyond EOF.
static int avi_calc_video_frame_by_pts_ms(const avi_parser_ctx_t *ctx, uint64_t pts_ms, long *out_frame)
{
    if (ctx == NULL || out_frame == NULL) {
        return -1;
    }
    if (ctx->video_fps <= 0.0 || ctx->total_video_frames <= 0) {
        return -1;
    }
    // frame = pts_ms * fps / 1000 with rounding.
    double frame_d = ((double)pts_ms * ctx->video_fps) / 1000.0 + 0.5;
    if (frame_d < 0.0) {
        frame_d = 0.0;
    }
    if (frame_d > (double)LONG_MAX) {
        return -1;
    }
    long frame = (long)frame_d;
    if (frame < 0 || frame >= ctx->total_video_frames) {
        return -1;
    }
    *out_frame = frame;
    return 0;
}

// Map audio PTS (ms) to a global audio byte offset.
// Return 0 on success, -1 on invalid params or pts beyond EOF.
static int avi_calc_audio_byte_offset_by_pts_ms(const avi_parser_ctx_t *ctx, uint64_t pts_ms, uint64_t *out_byte_off)
{
    if (ctx == NULL || ctx->avi_handle == NULL || out_byte_off == NULL) {
        return -1;
    }
    if (ctx->avi_handle->audio_bytes <= 0) {
        return -1;
    }
    uint64_t audio_bytes_u64 = (uint64_t)ctx->avi_handle->audio_bytes;

    if (pts_ms == 0) {
        *out_byte_off = 0;
        return 0;
    }

    uint64_t byte_off = 0;
    if (ctx->audio_format == WAVE_FORMAT_PCM ||
        ctx->audio_format == WAVE_FORMAT_ALAW ||
        ctx->audio_format == WAVE_FORMAT_MULAW)
    {
        if (ctx->audio_rate <= 0 || ctx->audio_channels <= 0 || ctx->audio_bits <= 0) {
            return -1;
        }
        uint64_t bytes_per_sample = (uint64_t)((ctx->audio_bits + 7) / 8);
        if (bytes_per_sample == 0) {
            return -1;
        }
        uint64_t bytes_per_sec = (uint64_t)ctx->audio_rate * (uint64_t)ctx->audio_channels * bytes_per_sample;
        if (bytes_per_sec == 0) {
            return -1;
        }
        byte_off = (pts_ms * bytes_per_sec + 500ULL) / 1000ULL;
    }
    else if (ctx->avi_handle->a_byterate > 0)
    {
        uint64_t bytes_per_sec = (uint64_t)ctx->avi_handle->a_byterate;
        byte_off = (pts_ms * bytes_per_sec + 500ULL) / 1000ULL;
    }
    else
    {
        if (ctx->duration_ms == 0) {
            return -1;
        }
        byte_off = (pts_ms * audio_bytes_u64) / ctx->duration_ms;
    }

    if (byte_off >= audio_bytes_u64) {
        return -1;
    }
    *out_byte_off = byte_off;
    return 0;
}

// Map audio global byte offset to audio chunk index using chunk start offsets.
// Return 0 on success, -1 on error.
static int avi_find_audio_chunk_by_byte_offset(avi_parser_ctx_t *ctx, uint64_t byte_off, long *out_chunk)
{
    if (ctx == NULL || ctx->avi_handle == NULL || out_chunk == NULL) {
        return -1;
    }
    long total = AVI_audio_chunks(ctx->avi_handle);
    if (total <= 0) {
        return -1;
    }

    uint64_t lo = 0;
    uint64_t hi = (uint64_t)total; // [lo, hi)
    while (lo < hi)
    {
        uint64_t mid = lo + (hi - lo) / 2;
        uint64_t mid_off = 0;
        if (AVI_audio_byte_offset_of_chunk(ctx->avi_handle, (long)mid, &mid_off) != 0) {
            return -1;
        }
        if (mid_off <= byte_off) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo == 0) {
        *out_chunk = 0;
        return 0;
    }
    *out_chunk = (long)(lo - 1);
    return 0;
}

// Map audio PTS (ms) to audio chunk index.
//
// Implementation note:
// - AVI audio indexing is byte-offset oriented (idx1/segments). For most formats, a robust way to
//   locate the corresponding chunk is: PTS -> byte_offset -> chunk (by searching chunk byte offsets).
// - This helper keeps call sites clean while preserving the correct underlying model.
static int avi_calc_audio_chunk_by_pts_ms(avi_parser_ctx_t *ctx, uint64_t pts_ms, long *out_chunk)
{
    if (ctx == NULL || out_chunk == NULL) {
        return -1;
    }
    uint64_t byte_off = 0;
    if (avi_calc_audio_byte_offset_by_pts_ms(ctx, pts_ms, &byte_off) != 0) {
        return -1;
    }
    return avi_find_audio_chunk_by_byte_offset(ctx, byte_off, out_chunk);
}

// Parse ADTS header and return frame length in bytes.
// Return true on success.
static bool avi_parse_adts_frame_length(const uint8_t *buf, uint32_t len, uint16_t *out_frame_len)
{
    if (buf == NULL || out_frame_len == NULL || len < 7)
    {
        return false;
    }

    // Syncword 0xFFF (12 bits)
    if (buf[0] != 0xFF || ((buf[1] & 0xF0) != 0xF0))
    {
        return false;
    }

    uint16_t frame_len = (uint16_t)(((buf[3] & 0x03) << 11) | ((uint16_t)buf[4] << 3) | ((buf[5] & 0xE0) >> 5));
    if (frame_len < 7 || frame_len > len)
    {
        return false;
    }

    *out_frame_len = frame_len;
    return true;
}

// Count complete ADTS frames in a buffer.
static uint32_t avi_count_adts_frames(const uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len < 7)
    {
        return 0;
    }

    uint32_t off = 0;
    uint32_t frames = 0;
    while (off + 7 <= len)
    {
        // Find syncword quickly
        if (buf[off] != 0xFF || ((buf[off + 1] & 0xF0) != 0xF0))
        {
            off++;
            continue;
        }

        uint16_t fl = 0;
        if (!avi_parse_adts_frame_length(buf + off, len - off, &fl))
        {
            off++;
            continue;
        }

        frames++;
        off += fl;
    }
    return frames;
}

// Open AVI container
static avdk_err_t avi_parser_open(struct video_player_container_parser_ops_s *ops, const char *file_path)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");

    LOGI("%s: Opening AVI container: %s\n", __func__, file_path);

    // Open AVI file with index (getIndex=1 to enable seeking)
    // Note: We open the file each time init is called, allowing multiple files
    if (ctx->avi_handle != NULL)
    {
        // Close previous file if any
        AVI_close(ctx->avi_handle);
        ctx->avi_handle = NULL;
    }

    ctx->avi_handle = AVI_open_input_file(file_path, 1, AVI_MEM_PSRAM);
    if (ctx->avi_handle == NULL)
    {
        LOGE("%s: Failed to open AVI file: %s\n", __func__, file_path);
        return AVDK_ERR_IO;
    }

    ctx->file_path = file_path;
    ctx->file_size_bytes = 0;
    ctx->duration_ms = 0;
    ctx->video_format = VIDEO_PLAYER_VIDEO_FORMAT_UNKNOWN;
    ctx->jpeg_subsampling = VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE;

    // Cache file size best-effort.
    struct stat st;
    if (stat(file_path, &st) == 0 && st.st_size > 0)
    {
        ctx->file_size_bytes = (uint64_t)st.st_size;
    }

    // Get video info
    ctx->video_width = AVI_video_width(ctx->avi_handle);
    ctx->video_height = AVI_video_height(ctx->avi_handle);
    ctx->video_fps = AVI_video_frame_rate(ctx->avi_handle);
    ctx->total_video_frames = AVI_video_frames(ctx->avi_handle);

    // Get audio info
    ctx->audio_channels = AVI_audio_channels(ctx->avi_handle);
    ctx->audio_rate = AVI_audio_rate(ctx->avi_handle);
    ctx->audio_bits = AVI_audio_bits(ctx->avi_handle);
    ctx->audio_format = AVI_audio_format(ctx->avi_handle);
    ctx->total_audio_chunks = AVI_audio_chunks(ctx->avi_handle);

    // Cache duration best-effort.
    if (ctx->video_fps > 0 && ctx->total_video_frames > 0)
    {
        ctx->duration_ms = (uint64_t)((double)ctx->total_video_frames / ctx->video_fps * 1000.0 + 0.5);
    }

    // Cache video format + MJPEG subsampling best-effort (so get_media_info is just a copy).
    char *compressor = AVI_video_compressor(ctx->avi_handle);
    if (compressor != NULL)
    {
        if (os_strncmp(compressor, "MJPG", 4) == 0 || os_strncmp(compressor, "mjpg", 4) == 0)
        {
            ctx->video_format = VIDEO_PLAYER_VIDEO_FORMAT_MJPEG;

            // Probe first frame subsampling once at open.
            if (ctx->total_video_frames > 0)
            {
                long first_frame_size = 0;
                int seek_ret = AVI_set_video_read_index(ctx->avi_handle, 0, &first_frame_size);
                if (seek_ret == 0 && first_frame_size > 0)
                {
                    // Allocate buffer via upper-layer callback to avoid os_malloc/os_free in parser.
                    if (ctx->video_packet_alloc_cb == NULL || ctx->video_packet_free_cb == NULL)
                    {
                        LOGW("%s: video packet buffer callbacks not set, skip MJPEG subsampling probe\n", __func__);
                    }
                    else
                    {
                        video_player_buffer_t tmp = {0};
                        tmp.length = (uint32_t)first_frame_size;
                        avdk_err_t alloc_ret = ctx->video_packet_alloc_cb(ctx->video_packet_user_data, &tmp);
                        if (alloc_ret == AVDK_ERR_OK && tmp.data != NULL && tmp.length >= (uint32_t)first_frame_size)
                        {
                            long bytes_read = AVI_read_next_video_frame(ctx->avi_handle, (char *)tmp.data, first_frame_size);
                            if (bytes_read == first_frame_size)
                            {
                                video_player_jpeg_subsampling_t subsampling = VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE;
                                avdk_err_t jpeg_ret = bk_video_player_probe_jpeg_subsampling(tmp.data,
                                                                                              (uint32_t)first_frame_size,
                                                                                              &subsampling);
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
                                LOGW("%s: Failed to read first frame, expected %ld bytes, got %ld\n",
                                     __func__, first_frame_size, bytes_read);
                            }

                            ctx->video_packet_free_cb(ctx->video_packet_user_data, &tmp);
                        }
                        else
                        {
                            LOGW("%s: video_packet_alloc_cb failed, ret=%d, size=%ld\n", __func__, alloc_ret, first_frame_size);
                            if (alloc_ret == AVDK_ERR_OK && tmp.data != NULL)
                            {
                                ctx->video_packet_free_cb(ctx->video_packet_user_data, &tmp);
                            }
                        }
                    }
                }
                else
                {
                    LOGW("%s: Failed to seek to first frame or invalid frame size, ret=%d, size=%ld\n",
                         __func__, seek_ret, first_frame_size);
                }

                // Reset read index for normal decoding.
                (void)AVI_set_video_read_index(ctx->avi_handle, 0, NULL);
                if (ctx->total_audio_chunks > 0) {
                    (void)AVI_set_audio_read_chunk(ctx->avi_handle, 0, NULL);
                }
            }
        }
        else if (os_strncmp(compressor, "H264", 4) == 0 || os_strncmp(compressor, "h264", 4) == 0)
        {
            ctx->video_format = VIDEO_PLAYER_VIDEO_FORMAT_H264;
        }
        else
        {
            ctx->video_format = VIDEO_PLAYER_VIDEO_FORMAT_UNKNOWN;
        }
    }

    // Initialize decode state
    ctx->current_video_frame = 0;
    ctx->current_audio_chunk = 0;
    ctx->audio_bytes_read = 0;
    ctx->audio_aac_samples = 0;
    ctx->video_pts_base = 0;
    ctx->audio_pts_base = 0;
    ctx->video_eof = false;
    ctx->audio_eof = false;

    // Check if index is available (segment loader populates indices on demand)
    ctx->has_video_index = (ctx->total_video_frames > 0);
    ctx->has_audio_index = (ctx->total_audio_chunks > 0);

    // Initialize mutex for thread safety
    if (rtos_init_mutex(&ctx->mutex) != BK_OK)
    {
        LOGE("%s: Failed to init mutex\n", __func__);
        AVI_close(ctx->avi_handle);
        ctx->avi_handle = NULL;
        return AVDK_ERR_IO;
    }

    // Reset read index to start
    (void)AVI_set_video_read_index(ctx->avi_handle, 0, NULL);
    if (ctx->total_audio_chunks > 0) {
        (void)AVI_set_audio_read_chunk(ctx->avi_handle, 0, NULL);
    }

    LOGI("%s: Index status - video_index=%s, audio_index=%s\n",
            __func__, ctx->has_video_index ? "yes" : "no", ctx->has_audio_index ? "yes" : "no");

    LOGI("%s: AVI file opened successfully, video: %dx%d@%.2ffps, frames=%ld, audio: %dch@%ldHz@%dbits\n",
            __func__, ctx->video_width, ctx->video_height, ctx->video_fps, ctx->total_video_frames,
            ctx->audio_channels, ctx->audio_rate, ctx->audio_bits);

    return AVDK_ERR_OK;
}

// Close AVI container
static avdk_err_t avi_parser_close(struct video_player_container_parser_ops_s *ops)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;

    if (ctx->avi_handle != NULL)
    {
        LOGI("%s: Closing AVI file\n", __func__);
        AVI_close(ctx->avi_handle);
        ctx->avi_handle = NULL;
    }

    // Deinitialize mutex
    rtos_deinit_mutex(&ctx->mutex);

    return AVDK_ERR_OK;
}

static avdk_err_t avi_parser_get_supported_file_extensions(const struct video_player_container_parser_ops_s *ops,
                                                           const char * const **exts,
                                                           uint32_t *ext_count)
{
    (void)ops;
    if (exts == NULL || ext_count == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    static const char * const s_exts[] = {
        ".avi",
    };

    *exts = s_exts;
    *ext_count = (uint32_t)(sizeof(s_exts) / sizeof(s_exts[0]));
    return AVDK_ERR_OK;
}

// Parse audio info from AVI container
static avdk_err_t avi_parser_parse_audio_info(struct video_player_container_parser_ops_s *ops, video_player_audio_params_t *audio_params)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(audio_params, AVDK_ERR_INVAL, TAG, "audio_params is NULL");
    AVDK_RETURN_ON_FALSE(ctx->avi_handle, AVDK_ERR_IO, TAG, "AVI file not opened");

    // Check if audio stream exists
    if (ctx->audio_channels == 0)
    {
        LOGW("%s: AVI file has no audio stream\n", __func__);
        return AVDK_ERR_NODEV;
    }

    // Fill audio parameters
    audio_params->channels = ctx->audio_channels;
    audio_params->sample_rate = ctx->audio_rate;
    audio_params->bits_per_sample = ctx->audio_bits;

    /*
     * Normalize AVI/WAV format codes into video_player_audio_format_t.
     *
     * IMPORTANT:
     * - WAV format code is NOT the same as video_player_audio_format_t.
     *   Example: WAVE_FORMAT_PCM == 1, but VIDEO_PLAYER_AUDIO_FORMAT_AAC == 1.
     *   Casting directly will cause the player to select a wrong decoder (AAC instead of PCM).
     */
    if (ctx->audio_format == WAVE_FORMAT_PCM)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_PCM;
    }
    else if (ctx->audio_format == WAVE_FORMAT_AAC)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_AAC;
    }
    else if (ctx->audio_format == WAVE_FORMAT_ALAW)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_G711A;
    }
    else if (ctx->audio_format == WAVE_FORMAT_MULAW)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_G711U;
    }
    else if (ctx->audio_format == WAVE_FORMAT_G722)
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_G722;
    }
    else
    {
        audio_params->format = VIDEO_PLAYER_AUDIO_FORMAT_UNKNOWN;
    }

    audio_params->codec_config = NULL;
    audio_params->codec_config_size = 0;

    LOGI("%s: Audio info: channels=%d, sample_rate=%ld, bits=%d, wav_format=0x%x -> format=%d\n",
         __func__, audio_params->channels, audio_params->sample_rate,
         audio_params->bits_per_sample, (unsigned)ctx->audio_format, (int)audio_params->format);

    // Only PCM/AAC/G711A/G711U/G722 are supported for AVI audio.
    if (audio_params->format == VIDEO_PLAYER_AUDIO_FORMAT_UNKNOWN)
    {
        LOGE("%s: Unsupported AVI audio wav_format=0x%x\n", __func__, (unsigned)ctx->audio_format);
        return AVDK_ERR_NODEV;
    }

    // For AAC, determine whether payload is ADTS or raw AAC, and provide ASC for raw mode.
    if (ctx->audio_format == WAVE_FORMAT_AAC)
    {
        int is_adts = 0;
        const uint8_t *asc = NULL;
        uint32_t asc_size = 0;
        if (AVI_get_aac_stream_info(ctx->avi_handle, &is_adts, &asc, &asc_size) != 0)
        {
            LOGE("%s: Failed to get AAC stream info\n", __func__);
            return AVDK_ERR_NODEV;
        }

        ctx->audio_aac_is_adts = (is_adts != 0);
        if (!ctx->audio_aac_is_adts)
        {
            if (asc == NULL || asc_size < 2)
            {
                LOGE("%s: Raw AAC in AVI but ASC is missing\n", __func__);
                return AVDK_ERR_NODEV;
            }
            audio_params->codec_config = asc;
            audio_params->codec_config_size = asc_size;
        }
    }

    return AVDK_ERR_OK;
}

// Parse video info from AVI file
static avdk_err_t avi_parser_parse_video_info(struct video_player_container_parser_ops_s *ops, video_player_video_params_t *video_params)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(video_params, AVDK_ERR_INVAL, TAG, "video_params is NULL");
    AVDK_RETURN_ON_FALSE(ctx->avi_handle, AVDK_ERR_IO, TAG, "AVI file not opened");

    // Check if video stream exists
    if (ctx->video_width == 0 || ctx->video_height == 0)
    {
        LOGW("%s: AVI file has no video stream\n", __func__);
        return AVDK_ERR_NODEV;
    }

    // Fill video parameters
    video_params->width = ctx->video_width;
    video_params->height = ctx->video_height;
    video_params->fps = (uint32_t)(ctx->video_fps + 0.5);
    video_params->format = ctx->video_format;
    video_params->jpeg_subsampling = ctx->jpeg_subsampling;

    LOGI("%s: Video info: width=%d, height=%d, fps=%d, format=%d, jpeg_subsampling=%u\n",
         __func__, video_params->width, video_params->height,
         video_params->fps, video_params->format, video_params->jpeg_subsampling);

    return AVDK_ERR_OK;
}

// Resolve the exact frame index that will be read for the next video packet.
// This must stay consistent between get_video_packet_size() and read_video_packet(),
// otherwise buffer sizes can mismatch after seek.
static long avi_resolve_video_read_frame(avi_parser_ctx_t *ctx,
                                                uint64_t target_pts,
                                                long *target_frame,
                                                bool *is_seek_target)
{
    long frame = ctx->current_video_frame;
    long seek_frame = frame;
    long calc_target = frame;
    bool seek_target = false;

    if (target_pts != VIDEO_PLAYER_PTS_INVALID)
    {
        // Resolve target frame by PTS in parser (avilib does not provide PTS mapping).
        if (avi_calc_video_frame_by_pts_ms(ctx, target_pts, &calc_target) != 0)
        {
            // Classify as EOF only when we have a reliable cached duration.
            if (ctx->duration_ms > 0 && target_pts >= ctx->duration_ms)
            {
                ctx->video_eof = true;
            }
            return -1;
        }

        // For H.264, always seek to the previous keyframe and start playback from it.
        if (ctx->video_format == VIDEO_PLAYER_VIDEO_FORMAT_H264 && ctx->has_video_index)
        {
            long key = 0;
            if (AVI_find_prev_video_keyframe(ctx->avi_handle, calc_target, &key) == 0)
            {
                seek_frame = key;
            }
            else
            {
                // If keyframe lookup fails, seek directly to target frame.
                seek_frame = calc_target;
            }
        }
        else
        {
            seek_frame = calc_target;
        }
    }
    if (target_frame != NULL)
    {
        *target_frame = calc_target;
    }
    if (is_seek_target != NULL)
    {
        *is_seek_target = seek_target;
    }

    return seek_frame;
}

static uint64_t avi_adjust_seek_pts_for_h264_keyframe(const avi_parser_ctx_t *ctx, uint64_t target_pts)
{
    if (ctx == NULL || ctx->avi_handle == NULL) {
        return target_pts;
    }
    if (target_pts == VIDEO_PLAYER_PTS_INVALID) {
        return target_pts;
    }
    if (ctx->video_format != VIDEO_PLAYER_VIDEO_FORMAT_H264 || !ctx->has_video_index) {
        return target_pts;
    }
    if (ctx->video_fps <= 0.0) {
        return target_pts;
    }

    long target_frame = 0;
    if (avi_calc_video_frame_by_pts_ms(ctx, target_pts, &target_frame) != 0) {
        return target_pts;
    }

    long keyframe = 0;
    if (AVI_find_prev_video_keyframe(ctx->avi_handle, target_frame, &keyframe) != 0) {
        return target_pts;
    }

    // keyframe_pts_ms = keyframe / fps * 1000 with rounding.
    double pts_ms_d = ((double)keyframe / ctx->video_fps) * 1000.0 + 0.5;
    if (pts_ms_d < 0.0) {
        pts_ms_d = 0.0;
    }
    if (pts_ms_d > (double)UINT64_MAX) {
        return target_pts;
    }
    return (uint64_t)pts_ms_d;
}

// Get encoded video packet size from video index
// If target_pts is provided (target_pts != VIDEO_PLAYER_PTS_INVALID), get size for the packet/frame at that PTS
// If target_pts is VIDEO_PLAYER_PTS_INVALID, get size for the next packet/frame sequentially
static avdk_err_t avi_parser_get_video_packet_size(struct video_player_container_parser_ops_s *ops, uint32_t *packet_size, uint64_t target_pts)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(packet_size, AVDK_ERR_INVAL, TAG, "packet_size is NULL");
    AVDK_RETURN_ON_FALSE(ctx->avi_handle, AVDK_ERR_IO, TAG, "AVI file not opened");

    *packet_size = 0;

    // Lock mutex for thread-safe access
    rtos_lock_mutex(&ctx->mutex);

    long target_frame = ctx->current_video_frame;
    long query_frame = avi_resolve_video_read_frame(ctx, target_pts, &target_frame, NULL);
    if (query_frame < 0)
    {
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_EOF;
    }

    // Check if video EOF
    if (query_frame >= ctx->total_video_frames)
    {
        ctx->video_eof = true;
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_EOF; // EOF
    }

    // Get video frame size from index if available
    long frame_len = 0;

    // Priority: use index to get exact frame size
    if (ctx->has_video_index)
    {
        long saved_frame = ctx->current_video_frame;
        int set_ret = AVI_set_video_read_index(ctx->avi_handle, query_frame, &frame_len);
        // Restore read index to avoid changing read state in size query.
        (void)AVI_set_video_read_index(ctx->avi_handle, saved_frame, NULL);
        if (set_ret != 0 || frame_len <= 0)
        {
            LOGE("%s: Failed to get frame size from index for frame %ld\n", __func__, query_frame);
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }
    }
    else
    {
        // No index available, cannot proceed without index
        LOGE("%s: No video index available, cannot get frame size\n", __func__);
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_IO;
    }

    rtos_unlock_mutex(&ctx->mutex);

    *packet_size = (uint32_t)frame_len;
    return AVDK_ERR_OK;
}

// Get encoded audio packet size from audio index
static avdk_err_t avi_parser_get_audio_packet_size(struct video_player_container_parser_ops_s *ops, uint32_t *packet_size, uint64_t target_pts)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(packet_size, AVDK_ERR_INVAL, TAG, "packet_size is NULL");
    AVDK_RETURN_ON_FALSE(ctx->avi_handle, AVDK_ERR_IO, TAG, "AVI file not opened");

    *packet_size = 0;

    // Get audio frame size from index if available
    uint32_t audio_size = 0;

    // Priority: use index to get exact audio chunk size
    if (ctx->has_audio_index)
    {
        long query_chunk = ctx->current_audio_chunk;
        long audio_chunk_len = 0;
        if (target_pts != VIDEO_PLAYER_PTS_INVALID)
        {
            uint64_t anchor_pts = avi_adjust_seek_pts_for_h264_keyframe(ctx, target_pts);
            if (avi_calc_audio_chunk_by_pts_ms(ctx, anchor_pts, &query_chunk) != 0)
            {
                if (ctx->duration_ms > 0 && target_pts >= ctx->duration_ms)
                {
                    ctx->audio_eof = true;
                    return AVDK_ERR_EOF;
                }
                return AVDK_ERR_IO;
            }
            // Query chunk length via read-index primitive.
            if (AVI_set_audio_read_chunk(ctx->avi_handle, query_chunk, &audio_chunk_len) != 0 || audio_chunk_len <= 0)
            {
                if (ctx->duration_ms > 0 && target_pts >= ctx->duration_ms)
                {
                    ctx->audio_eof = true;
                    return AVDK_ERR_EOF;
                }
                return AVDK_ERR_IO;
            }
        }
        else
        {
            if (AVI_set_audio_read_chunk(ctx->avi_handle, query_chunk, &audio_chunk_len) != 0 || audio_chunk_len <= 0)
            {
                ctx->audio_eof = true;
                return AVDK_ERR_EOF;
            }
        }

        if (query_chunk < 0 || query_chunk >= AVI_audio_chunks(ctx->avi_handle))
        {
            ctx->audio_eof = true;
            return AVDK_ERR_EOF;
        }

        audio_size = (uint32_t)audio_chunk_len;
    }
    else
    {
        // No index available, cannot proceed without index
        LOGE("%s: No audio index available, cannot get chunk size\n", __func__);
        return AVDK_ERR_IO;
    }

    *packet_size = audio_size;
    return AVDK_ERR_OK;
}

// Read encoded video packet from AVI container
// If target_pts is provided (target_pts != VIDEO_PLAYER_PTS_INVALID), seek to the packet/frame at that PTS before reading
// If target_pts is VIDEO_PLAYER_PTS_INVALID, read the next packet/frame sequentially
// If index is available, use index-based reading for better precision and performance
static avdk_err_t avi_parser_read_video_packet(struct video_player_container_parser_ops_s *ops, video_player_buffer_t *out_buffer, uint64_t target_pts)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(out_buffer, AVDK_ERR_INVAL, TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(ctx->avi_handle, AVDK_ERR_IO, TAG, "AVI file not opened");
    AVDK_RETURN_ON_FALSE(out_buffer->data, AVDK_ERR_INVAL, TAG, "out_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->length > 0, AVDK_ERR_INVAL, TAG, "out_buffer->length is 0");

    // Lock mutex for thread-safe access (important in pipeline mode)
    rtos_lock_mutex(&ctx->mutex);

    // Use index-based reading if available, otherwise use sequential reading
    if (ctx->has_video_index)
    {
        long target_frame = ctx->current_video_frame;
        bool is_seek_target = false;
        long seek_frame = avi_resolve_video_read_frame(ctx, target_pts, &target_frame, &is_seek_target);
        if (seek_frame < 0)
        {
            ctx->video_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_EOF; // EOF
        }

        // Save current frame number for logging before updating.
        long old_frame = ctx->current_video_frame;

        // Update current_video_frame to seek_frame for seeking.
        // This ensures we read the keyframe when target_pts is provided.
        if (seek_frame != ctx->current_video_frame)
        {
            ctx->current_video_frame = seek_frame;
            if (target_pts != VIDEO_PLAYER_PTS_INVALID)
            {
                if (seek_frame > old_frame)
                {
                    LOGD("%s: Seeking forward to frame %ld (keyframe=%ld) based on target_pts=%llu ms (was %ld)\n",
                         __func__, target_frame, seek_frame, target_pts, old_frame);
                }
                else
                {
                    LOGD("%s: Seeking backward to frame %ld (keyframe=%ld) based on target_pts=%llu ms (was %ld)\n",
                         __func__, target_frame, seek_frame, target_pts, old_frame);
                }
            }
        }

        // Use index-based video reading for better precision
        if (ctx->current_video_frame >= ctx->total_video_frames)
        {
            ctx->video_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_EOF; // EOF
        }

        // Set video position using index (this also gets the frame length)
        // Note: frame_len should have been obtained from get_video_packet_size,
        // and buffer should have been allocated accordingly
        long frame_len = 0;
        int ret = AVI_set_video_read_index(ctx->avi_handle, ctx->current_video_frame, &frame_len);
        if (ret != 0)
        {
            LOGE("%s: Failed to set video position to frame %ld\n", __func__, ctx->current_video_frame);
            ctx->video_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }

        // Verify buffer size matches frame size (should be allocated based on get_video_packet_size)
        if (out_buffer->length < (uint32_t)frame_len)
        {
            LOGE("%s: Buffer too small for video frame, need %ld bytes, got %u bytes. "
                 "Buffer should be allocated based on get_video_packet_size result.\n",
                 __func__, frame_len, out_buffer->length);
            // Buffer is insufficient, do not output truncated data (it is not useful for decoder).
            // Keep current_video_frame unchanged so upper layer can allocate a larger buffer and retry.
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_NOMEM;
        }

        // Read video frame directly into the provided buffer using index
        // AVI_read_next_video_frame will use video_index[video_pos] to get file position and read data
        long bytes_read = AVI_read_next_video_frame(ctx->avi_handle, (char *)out_buffer->data, frame_len);
        if (bytes_read <= 0)
        {
            LOGE("%s: Failed to read video frame %ld, bytes_read=%ld\n",
                 __func__, ctx->current_video_frame, bytes_read);
            ctx->video_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }

        // Update state
        out_buffer->length = (uint32_t)bytes_read;
        // Calculate PTS: frame_number / fps * 1000 (milliseconds)
        long frame_num = ctx->current_video_frame;
        if (ctx->video_fps > 0)
        {
            out_buffer->pts = (uint64_t)((double)frame_num / ctx->video_fps * 1000.0);
        }
        else
        {
            out_buffer->pts = frame_num * 33; // Default 30fps
        }

        ctx->current_video_frame++;

        // Check if video EOF
        if (ctx->current_video_frame >= ctx->total_video_frames)
        {
            ctx->video_eof = true;
        }

        rtos_unlock_mutex(&ctx->mutex);
        LOGV("%s: Read video frame %ld (index-based), size=%u, pts=%llu\n",
             __func__, frame_num, out_buffer->length, out_buffer->pts);
        return AVDK_ERR_OK;
    }
    else
    {
        // No index available, cannot proceed without index
        LOGE("%s: No video index available, cannot decode video\n", __func__);
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_IO;
    }
}

// Decode audio data from file (extract encoded audio data from AVI file)
// Used by file audio decode thread in pipeline mode
// If index is available, use index-based reading for better precision and performance
static avdk_err_t avi_parser_read_audio_packet(struct video_player_container_parser_ops_s *ops, video_player_buffer_t *out_buffer, uint64_t target_pts)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(out_buffer, AVDK_ERR_INVAL, TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(ctx->avi_handle, AVDK_ERR_IO, TAG, "AVI file not opened");
    AVDK_RETURN_ON_FALSE(out_buffer->data, AVDK_ERR_INVAL, TAG, "out_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->length > 0, AVDK_ERR_INVAL, TAG, "out_buffer->length is 0");

    // Lock mutex for thread-safe access (important in pipeline mode)
    rtos_lock_mutex(&ctx->mutex);

    // Use index-based reading if available, otherwise use sequential reading
    if (ctx->has_audio_index)
    {
        if (target_pts != VIDEO_PLAYER_PTS_INVALID)
        {
            uint64_t anchor_pts = avi_adjust_seek_pts_for_h264_keyframe(ctx, target_pts);
            long target_chunk = 0;
            if (avi_calc_audio_chunk_by_pts_ms(ctx, anchor_pts, &target_chunk) != 0)
            {
                if (ctx->duration_ms > 0 && target_pts >= ctx->duration_ms)
                {
                    ctx->audio_eof = true;
                    out_buffer->length = 0;
                    rtos_unlock_mutex(&ctx->mutex);
                    return AVDK_ERR_EOF;
                }
                ctx->audio_eof = true;
                out_buffer->length = 0;
                rtos_unlock_mutex(&ctx->mutex);
                return AVDK_ERR_IO;
            }
            if (target_chunk < 0 || target_chunk >= AVI_audio_chunks(ctx->avi_handle))
            {
                ctx->audio_eof = true;
                out_buffer->length = 0;
                rtos_unlock_mutex(&ctx->mutex);
                return AVDK_ERR_EOF;
            }

            ctx->current_audio_chunk = target_chunk;
            ctx->audio_eof = false;

            // Align counters with the selected chunk so out_buffer->pts follows the seek request.
            // Use global byte offset (segment base + local tot) to keep PTS monotonic across segments.
            long seek_chunk_len = 0;
            int seek_ret = AVI_set_audio_read_chunk(ctx->avi_handle, target_chunk, &seek_chunk_len);
            if (seek_ret != 0 || seek_chunk_len <= 0)
            {
                ctx->audio_eof = true;
                out_buffer->length = 0;
                rtos_unlock_mutex(&ctx->mutex);
                return AVDK_ERR_EOF;
            }
            uint64_t chunk_byte_off = 0;
            if (AVI_audio_byte_offset_of_chunk(ctx->avi_handle, target_chunk, &chunk_byte_off) != 0)
            {
                ctx->audio_eof = true;
                out_buffer->length = 0;
                rtos_unlock_mutex(&ctx->mutex);
                return AVDK_ERR_IO;
            }
            if (chunk_byte_off > (uint64_t)LONG_MAX)
            {
                ctx->audio_eof = true;
                out_buffer->length = 0;
                rtos_unlock_mutex(&ctx->mutex);
                return AVDK_ERR_IO;
            }
            ctx->audio_bytes_read = (long)chunk_byte_off;

            // For AAC (ADTS), track timing by AAC samples (1024 per frame). We cannot derive this from bytes reliably.
            // Use target_pts as the base so PTS seeking is effective for A/V sync and future offsets.
            if (ctx->audio_format == WAVE_FORMAT_AAC && ctx->audio_rate > 0)
            {
                uint64_t target_samples = (anchor_pts * (uint64_t)ctx->audio_rate + 500ULL) / 1000ULL;
                // Round down to 1024-aligned boundary to keep monotonicity stable.
                target_samples = (target_samples / 1024ULL) * 1024ULL;
                ctx->audio_aac_samples = target_samples;
            }
        }

        // Use index-based audio reading
        // Set audio position using index to get exact chunk size
        if (ctx->current_audio_chunk >= AVI_audio_chunks(ctx->avi_handle))
        {
            ctx->audio_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_EOF; // EOF
        }

        long audio_chunk_len = 0;
        int set_ret = AVI_set_audio_read_chunk(ctx->avi_handle, ctx->current_audio_chunk, &audio_chunk_len);
        if (set_ret != 0 || audio_chunk_len <= 0)
        {
            ctx->audio_eof = true;
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_EOF;
        }

        // Verify buffer size matches chunk size (should be allocated based on get_audio_packet_size)
        uint32_t bytes_to_read = out_buffer->length;
        if (bytes_to_read < (uint32_t)audio_chunk_len)
        {
            LOGW("%s: Buffer too small for audio chunk, need %ld bytes, got %u bytes. "
                 "Buffer should be allocated based on get_audio_packet_size result.\n",
                 __func__, audio_chunk_len, bytes_to_read);
            bytes_to_read = out_buffer->length; // Use available buffer
        }
        else
        {
            bytes_to_read = (uint32_t)audio_chunk_len; // Use exact chunk size
        }

        // Read audio data directly into the provided buffer (no temporary buffer needed)
        long bytes_read = AVI_read_next_audio_chunk(ctx->avi_handle, (char *)out_buffer->data, bytes_to_read);
        if (bytes_read <= 0)
        {
            // Check if we've read all audio
            if (ctx->audio_bytes_read >= ctx->avi_handle->audio_bytes)
            {
                ctx->audio_eof = true;
                out_buffer->length = 0;
                rtos_unlock_mutex(&ctx->mutex);
                return AVDK_ERR_EOF; // EOF
            }
            LOGE("%s: Failed to read audio data, bytes_read=%ld\n", __func__, bytes_read);
            out_buffer->length = 0;
            rtos_unlock_mutex(&ctx->mutex);
            return AVDK_ERR_IO;
        }

        // Update state
        out_buffer->length = (uint32_t)bytes_read;

        // Calculate PTS BEFORE updating cumulative counters.
        out_buffer->pts = VIDEO_PLAYER_PTS_INVALID;
        if (ctx->audio_format == WAVE_FORMAT_AAC && ctx->audio_rate > 0)
        {
            // AAC: PTS based on decoded samples (1024 samples per access unit).
            out_buffer->pts = (uint64_t)((double)ctx->audio_aac_samples / (double)ctx->audio_rate * 1000.0);
            if (ctx->audio_aac_is_adts)
            {
                uint32_t frames = avi_count_adts_frames((const uint8_t *)out_buffer->data, out_buffer->length);
                if (frames == 0)
                {
                    // Treat as a fatal format mismatch instead of guessing.
                    LOGE("%s: AAC is ADTS but no ADTS frames found in packet\n", __func__);
                    ctx->audio_eof = true;
                    out_buffer->length = 0;
                    rtos_unlock_mutex(&ctx->mutex);
                    return AVDK_ERR_IO;
                }
                ctx->audio_aac_samples += (uint64_t)frames * 1024ULL;
            }
            else
            {
                // Raw AAC: one access unit per chunk.
                ctx->audio_aac_samples += 1024ULL;
            }
        }
        else if (ctx->audio_format == WAVE_FORMAT_G722)
        {
            // Backward compatibility: default to 64 kbps (8000 bytes/sec).
            const uint64_t bytes_per_sec = 8000ULL;
            out_buffer->pts = (ctx->audio_bytes_read * 1000ULL + bytes_per_sec / 2ULL) / bytes_per_sec;
        }
        else if (ctx->audio_rate > 0 && ctx->audio_channels > 0)
        {
            uint32_t bytes_per_sample = (ctx->audio_bits + 7) / 8;
            if (bytes_per_sample > 0)
            {
                uint64_t total_samples = ctx->audio_bytes_read / ((uint64_t)ctx->audio_channels * (uint64_t)bytes_per_sample);
                out_buffer->pts = (uint64_t)((double)total_samples / (double)ctx->audio_rate * 1000.0);
            }
        }
        else
        {
            // Keep VIDEO_PLAYER_PTS_INVALID for unknown timing.
        }

        // Update cumulative bytes AFTER calculating PTS
        ctx->audio_bytes_read += bytes_read;
        ctx->current_audio_chunk++;

        // Check if audio EOF
        if (ctx->current_audio_chunk >= AVI_audio_chunks(ctx->avi_handle))
        {
            ctx->audio_eof = true;
        }

        rtos_unlock_mutex(&ctx->mutex);
        LOGV("%s: Read audio chunk %ld (index-based), size=%u, pts=%llu\n",
             __func__, ctx->current_audio_chunk - 1, out_buffer->length, out_buffer->pts);
        return AVDK_ERR_OK;
    }
    else
    {
        // No index available, cannot proceed without index
        LOGE("%s: No audio index available, cannot read audio packet\n", __func__);
        rtos_unlock_mutex(&ctx->mutex);
        return AVDK_ERR_IO;
    }
}

static avdk_err_t avi_parser_get_media_info(struct video_player_container_parser_ops_s *ops, video_player_media_info_t *media_info)
{
    avi_parser_instance_t *avi_instance = __containerof(ops, avi_parser_instance_t, ops);
    AVDK_RETURN_ON_FALSE(avi_instance, AVDK_ERR_INVAL, TAG, "parser_ctx is NULL");
    avi_parser_ctx_t *ctx = &avi_instance->ctx;
    AVDK_RETURN_ON_FALSE(media_info, AVDK_ERR_INVAL, TAG, "media_info is NULL");
    AVDK_RETURN_ON_FALSE(ctx->avi_handle, AVDK_ERR_IO, TAG, "AVI file not opened");

    os_memset(media_info, 0, sizeof(*media_info));

    // File-level info is cached during open(), get_media_info() only copies.
    media_info->duration_ms = ctx->duration_ms;
    media_info->file_size_bytes = ctx->file_size_bytes;

    // Stream params are also cached during open(); parse_* are copy-only now.
    (void)avi_parser_parse_video_info(ops, &media_info->video);
    (void)avi_parser_parse_audio_info(ops, &media_info->audio);

    if ((media_info->video.width == 0 || media_info->video.height == 0) &&
        (media_info->audio.channels == 0 || media_info->audio.sample_rate == 0))
    {
        return AVDK_ERR_NODEV;
    }

    return AVDK_ERR_OK;
}

static video_player_container_parser_ops_t *avi_parser_create(void *video_packet_user_data,
                                                              video_player_video_packet_buffer_alloc_cb_t video_packet_alloc_cb,
                                                              video_player_video_packet_buffer_free_cb_t video_packet_free_cb)
{
    avi_parser_instance_t *instance = os_malloc(sizeof(avi_parser_instance_t));
    if (instance == NULL)
    {
        LOGE("%s: Failed to allocate AVI parser instance\n", __func__);
        return NULL;
    }
    os_memset(instance, 0, sizeof(avi_parser_instance_t));

    os_memcpy(&instance->ops, &s_avi_parser_ops, sizeof(video_player_container_parser_ops_t));

    // Inject optional packet buffer callbacks at create-time (dependency injection).
    instance->ctx.video_packet_user_data = video_packet_user_data;
    instance->ctx.video_packet_alloc_cb = video_packet_alloc_cb;
    instance->ctx.video_packet_free_cb = video_packet_free_cb;

    return &instance->ops;
}

static void avi_parser_destroy(struct video_player_container_parser_ops_s *ops)
{
    avi_parser_instance_t *instance = __containerof(ops, avi_parser_instance_t, ops);
    if (instance == NULL)
    {
        return;
    }
    os_free(instance);
}

// AVI container parser operations structure
static video_player_container_parser_ops_t s_avi_parser_ops = {
    .create = avi_parser_create,
    .destroy = avi_parser_destroy,
    .open = avi_parser_open,
    .close = avi_parser_close,
    .get_supported_file_extensions = avi_parser_get_supported_file_extensions,
    .get_media_info = avi_parser_get_media_info,
    .get_video_packet_size = avi_parser_get_video_packet_size,
    .get_audio_packet_size = avi_parser_get_audio_packet_size,
    .read_video_packet = avi_parser_read_video_packet,
    .read_audio_packet = avi_parser_read_audio_packet,
};

// Get AVI container parser operations
// Note: Each call creates a new parser context, allowing multiple instances
video_player_container_parser_ops_t *bk_video_player_get_avi_parser_ops(void)
{
    return &s_avi_parser_ops;
}

