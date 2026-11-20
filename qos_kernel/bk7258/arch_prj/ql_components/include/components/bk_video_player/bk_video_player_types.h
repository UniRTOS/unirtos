// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>
#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include "components/avdk_utils/avdk_error.h"
#include "components/media_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define delete_video_player delete

// -----------------------------------------------------------------------------
// Forward declarations / opaque handles
// -----------------------------------------------------------------------------

typedef struct bk_video_player *bk_video_player_handle_t;
typedef struct bk_video_player bk_video_player_t;
typedef struct bk_video_player_config bk_video_player_config_t;

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

/*
 * Special PTS value used by container parser APIs to indicate "sequential read" (do not seek).
 *
 * IMPORTANT:
 * - 0 is a valid PTS (seek to beginning).
 * - Upper layer must use VIDEO_PLAYER_PTS_INVALID to request sequential read.
 */
#define VIDEO_PLAYER_PTS_INVALID    ((uint64_t)(~0ULL))

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------

// Player status
typedef enum
{
    VIDEO_PLAYER_STATUS_NONE = 0,
    VIDEO_PLAYER_STATUS_OPENED,
    VIDEO_PLAYER_STATUS_CLOSED,
    VIDEO_PLAYER_STATUS_PLAYING,
    VIDEO_PLAYER_STATUS_PAUSED,
    VIDEO_PLAYER_STATUS_STOPPED,
    VIDEO_PLAYER_STATUS_FINISHED,
} video_player_status_t;

// IOCTL commands
typedef enum
{
    BK_VIDEO_PLAYER_IOCTL_CMD_BASE = 0,
    // Set A/V sync offset (milliseconds). Param type: bk_video_player_av_sync_offset_param_t*.
    BK_VIDEO_PLAYER_IOCTL_CMD_SET_AV_SYNC_OFFSET_MS = 1,
} bk_video_player_ioctl_cmd_t;

/**
 * @brief A/V sync offset parameter for BK_VIDEO_PLAYER_IOCTL_CMD_SET_AV_SYNC_OFFSET_MS.
 *
 * The offset is applied by the player when syncing video to the audio-driven clock.
 * See bk_video_player_apply_av_sync_offset_ms() (internal) for detailed semantics.
 */
typedef struct
{
    int32_t offset_ms;
} bk_video_player_av_sync_offset_param_t;

// Audio format enumeration (shared by all container parsers)
typedef enum
{
    VIDEO_PLAYER_AUDIO_FORMAT_UNKNOWN = 0,
    VIDEO_PLAYER_AUDIO_FORMAT_AAC = 1,
    VIDEO_PLAYER_AUDIO_FORMAT_MP3 = 2,
    VIDEO_PLAYER_AUDIO_FORMAT_PCM = 3,
    VIDEO_PLAYER_AUDIO_FORMAT_G711A = 4,
    VIDEO_PLAYER_AUDIO_FORMAT_G711U = 5,
    VIDEO_PLAYER_AUDIO_FORMAT_ADPCM = 6, // IMA/DVI ADPCM
    VIDEO_PLAYER_AUDIO_FORMAT_G722 = 7,
    VIDEO_PLAYER_AUDIO_FORMAT_OPUS = 8,
    VIDEO_PLAYER_AUDIO_FORMAT_FLAC = 9,
    VIDEO_PLAYER_AUDIO_FORMAT_AMR = 10,  // AMR-NB/WB
    VIDEO_PLAYER_AUDIO_FORMAT_SBC = 11,
} video_player_audio_format_t;

// Video format enumeration (shared by all container parsers)
typedef enum
{
    VIDEO_PLAYER_VIDEO_FORMAT_UNKNOWN = 0,
    VIDEO_PLAYER_VIDEO_FORMAT_MJPEG = 1,
    VIDEO_PLAYER_VIDEO_FORMAT_H264 = 2,
} video_player_video_format_t;

// JPEG subsampling format enumeration (shared by all container parsers)
// These values match jpeg_img_fmt_t enum from bk_jpeg_decode_types.h:
// - JPEG_FMT_ERR (0) maps to VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE (0)
// - JPEG_FMT_YUV444 (1), JPEG_FMT_YUV422 (2), JPEG_FMT_YUV420 (3), JPEG_FMT_YUV400 (4)
typedef enum
{
    VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE = 0,  // Not applicable (not MJPEG or parsing failed)
    VIDEO_PLAYER_JPEG_SUBSAMPLING_YUV444 = 1, // JPEG_FMT_YUV444 - YUV 4:4:4 format
    VIDEO_PLAYER_JPEG_SUBSAMPLING_YUV422 = 2, // JPEG_FMT_YUV422 - YUV 4:2:2 format
    VIDEO_PLAYER_JPEG_SUBSAMPLING_YUV420 = 3, // JPEG_FMT_YUV420 - YUV 4:2:0 format
    VIDEO_PLAYER_JPEG_SUBSAMPLING_YUV400 = 4, // JPEG_FMT_YUV400 - YUV 4:0:0 format (grayscale)
} video_player_jpeg_subsampling_t;

// -----------------------------------------------------------------------------
// Core data structures
// -----------------------------------------------------------------------------

// Buffer structure (used for both encoded packets and decoded frames).
typedef struct video_player_buffer_s
{
    uint8_t *data;           // Buffer data pointer
    uint32_t length;         // Buffer data length
    uint64_t pts;            // Presentation timestamp (in milliseconds)
    void *frame_buffer;      // External allocated structure pointer (e.g., frame_buffer_t)
    void *user_data;         // User data for buffer management (reserved for future use)
} video_player_buffer_t;

// Audio decoder parameters
typedef struct
{
    uint32_t channels;        // Audio channels
    uint32_t sample_rate;    // Sample rate in Hz
    uint32_t bits_per_sample; // Bits per sample
    video_player_audio_format_t format; // Audio format
    const uint8_t *codec_config;      // Codec configuration (e.g., AAC AudioSpecificConfig), may be NULL
    uint32_t codec_config_size;       // Codec configuration size in bytes
} video_player_audio_params_t;

// Video decoder parameters
typedef struct
{
    uint32_t width;          // Video width in pixels
    uint32_t height;         // Video height in pixels
    uint32_t fps;           // Frames per second
    video_player_video_format_t format; // Video format
    video_player_jpeg_subsampling_t jpeg_subsampling; // JPEG subsampling format, VIDEO_PLAYER_JPEG_SUBSAMPLING_NONE if not applicable
} video_player_video_params_t;

// Media information for current file (container-level info + stream params)
typedef struct
{
    // Stream parameters (may remain zero/UNKNOWN if stream does not exist or not parsed yet)
    video_player_video_params_t video;
    video_player_audio_params_t audio;

    // File/container information
    uint64_t duration_ms;      // Total duration in milliseconds (0 if unknown/unsupported)
    uint64_t file_size_bytes;  // File size in bytes (0 if unknown)
} video_player_media_info_t;

// Decode complete callback meta information
// These structs are passed to upper layer together with decoded buffers.
// They provide stream parameters and a monotonically increasing index for delivered data.

typedef struct
{
    video_player_video_params_t video;  // Video stream parameters for current file
    uint64_t frame_index;               // 1-based index of delivered decoded frame
    uint64_t pts_ms;                    // Frame PTS in milliseconds (copied from buffer->pts)
} video_player_video_frame_meta_t;

typedef struct
{
    video_player_audio_params_t audio;  // Audio stream parameters for current file
    uint64_t packet_index;              // 1-based index of delivered decoded audio packet
    uint64_t pts_ms;                    // Packet PTS in milliseconds (copied from buffer->pts)
} video_player_audio_packet_meta_t;

// -----------------------------------------------------------------------------
// Callback types
// -----------------------------------------------------------------------------

// Playback finished callback
// NOTE:
// - This callback is invoked asynchronously by the player (not from the CLI thread).
// - file_path points to an internal copy of current file path (may be NULL/empty).
// - Do not block for a long time inside this callback.
typedef void (*video_player_playback_finished_cb_t)(void *user_data, const char *file_path);

// Audio output reset callback (optional)
// Used by app layer when switching tracks (next/prev/play) to clear stale audio buffers.
// Implementations should be best-effort and return quickly.
typedef avdk_err_t (*video_player_audio_output_reset_cb_t)(void *user_data);

// Audio output (re)configuration callback (optional)
// Used by upper layer to apply actual audio output parameters (sample rate/bits/channels) when:
// - starting a new file, or
// - switching to another file in playlist.
// If audio_output_config_user_data is NULL, user_data will be used.
typedef avdk_err_t (*video_player_audio_output_config_cb_t)(void *user_data, const video_player_audio_params_t *params);

// Encoded (packet) buffer allocation callback for video (from container parser)
typedef avdk_err_t (*video_player_video_packet_buffer_alloc_cb_t)(void *user_data, video_player_buffer_t *buffer);
// Encoded (packet) buffer free callback for video (from container parser)
typedef void (*video_player_video_packet_buffer_free_cb_t)(void *user_data, video_player_buffer_t *buffer);

// Decoded (post-decode) buffer allocation callback for audio
typedef avdk_err_t (*video_player_audio_buffer_alloc_cb_t)(void *user_data, video_player_buffer_t *buffer);
// Decoded (post-decode) buffer free callback for audio
typedef void (*video_player_audio_buffer_free_cb_t)(void *user_data, video_player_buffer_t *buffer);

// Decoded (post-decode) buffer allocation callback for video
typedef avdk_err_t (*video_player_video_buffer_alloc_cb_t)(void *user_data, video_player_buffer_t *buffer);
// Decoded (post-decode) buffer free callback for video
typedef void (*video_player_video_buffer_free_cb_t)(void *user_data, video_player_buffer_t *buffer);

// Audio decode complete callback
typedef void (*video_player_audio_decode_complete_cb_t)(void *user_data,
                                                        const video_player_audio_packet_meta_t *meta,
                                                        video_player_buffer_t *buffer);
// Video decode complete callback
typedef void (*video_player_video_decode_complete_cb_t)(void *user_data,
                                                        const video_player_video_frame_meta_t *meta,
                                                        video_player_buffer_t *buffer);

// Audio output control callbacks
// These callbacks are used by controller to notify the upper layer to apply actual audio output control.
typedef avdk_err_t (*video_player_audio_set_volume_cb_t)(void *user_data, uint8_t volume);
typedef avdk_err_t (*video_player_audio_set_mute_cb_t)(void *user_data, bool mute);

// -----------------------------------------------------------------------------
// Decoder / parser ops interfaces
// -----------------------------------------------------------------------------

// Audio decoder operations
typedef struct video_player_audio_decoder_ops_s
{
    // Optional: create/destroy an independent decoder instance.
    // Controller keeps registered decoder ops pointers for the lifetime of the player, so the default
    // getter should return a static template ops to avoid leaks. create() can be used when an
    // independent instance (separate decoder state) is required.
    struct video_player_audio_decoder_ops_s *(*create)(void);
    void (*destroy)(struct video_player_audio_decoder_ops_s *ops);

    // Optional: query supported audio formats before create().
    // This helps controller select a decoder without repeatedly creating instances and trying init().
    //
    // - formats: output pointer to a static const array owned by the decoder (do NOT free).
    // - format_count: number of elements in formats[].
    // - Return AVDK_ERR_OK on success. If not implemented, keep NULL and controller will fallback
    //   to the legacy "create + init" probing behavior.
    avdk_err_t (*get_supported_formats)(const struct video_player_audio_decoder_ops_s *ops,
                                        const video_player_audio_format_t **formats,
                                        uint32_t *format_count);

    // Initialize audio decoder
    avdk_err_t (*init)(struct video_player_audio_decoder_ops_s *ops, video_player_audio_params_t *params);
    // Deinitialize audio decoder
    avdk_err_t (*deinit)(struct video_player_audio_decoder_ops_s *ops);
    // Decode audio data
    avdk_err_t (*decode)(struct video_player_audio_decoder_ops_s *ops, video_player_buffer_t *in_buffer, video_player_buffer_t *out_buffer);
} video_player_audio_decoder_ops_t;

// Video decoder operations
typedef struct video_player_video_decoder_ops_s
{
    // Optional: create/destroy an independent decoder instance.
    struct video_player_video_decoder_ops_s *(*create)(void);
    void (*destroy)(struct video_player_video_decoder_ops_s *ops);

    // Optional: query supported video formats before create().
    // This helps controller select a decoder without repeatedly creating instances and trying init().
    //
    // - formats: output pointer to a static const array owned by the decoder (do NOT free).
    // - format_count: number of elements in formats[].
    // - Return AVDK_ERR_OK on success. If not implemented, keep NULL and controller will fallback
    //   to the legacy "create + init" probing behavior.
    avdk_err_t (*get_supported_formats)(struct video_player_video_decoder_ops_s *ops,
                                        const video_player_video_format_t **formats,
                                        uint32_t *format_count);

    // Initialize video decoder
    avdk_err_t (*init)(struct video_player_video_decoder_ops_s *ops, video_player_video_params_t *params);
    // Deinitialize video decoder
    avdk_err_t (*deinit)(struct video_player_video_decoder_ops_s *ops);
    // Decode video data
    // out_fmt: expected output pixel format for this decode call (e.g., PIXEL_FMT_YUYV/RGB565/RGB888).
    avdk_err_t (*decode)(struct video_player_video_decoder_ops_s *ops, video_player_buffer_t *in_buffer, video_player_buffer_t *out_buffer, pixel_format_t out_fmt);
} video_player_video_decoder_ops_t;

// Container parser operations (AVI/MP4/etc.)
// The parser is responsible for:
// - Parsing container headers and stream metadata
// - Reading encoded audio/video packets (bitstream) from container
typedef struct video_player_container_parser_ops_s
{
    // Optional: create/destroy an independent parser instance.
    // Some parser implementations use a singleton parser_ctx, which is not safe for concurrent open/parse
    // while playback is running. When create() is provided, controller can probe other files by using an
    // independent parser instance (separate parser_ctx) without interfering with playback.
    //
    // create(): returns a newly allocated ops instance (including its own parser_ctx), or NULL on failure.
    // The upper layer can inject optional encoded video packet buffer callbacks here.
    // Some parsers need temporary packet buffers during parse_video_info (e.g., MJPEG first-frame probe).
    // destroy(): releases the ops instance returned by create().
    struct video_player_container_parser_ops_s *(*create)(void *video_packet_user_data,
                                                          video_player_video_packet_buffer_alloc_cb_t video_packet_alloc_cb,
                                                          video_player_video_packet_buffer_free_cb_t video_packet_free_cb);
    void (*destroy)(struct video_player_container_parser_ops_s *ops);

    // Open container and initialize parser context
    avdk_err_t (*open)(struct video_player_container_parser_ops_s *ops, const char *file_path);
    // Close container and deinitialize parser context
    avdk_err_t (*close)(struct video_player_container_parser_ops_s *ops);

    // Get media information (video/audio parameters + container/file info).
    // This is a unified interface that replaces:
    // - parse_audio_info()
    // - parse_video_info()
    // - get_duration()
    //
    // Implementations should fill:
    // - media_info->video (if video stream exists)
    // - media_info->audio (if audio stream exists)
    // - media_info->duration_ms (best-effort)
    // - media_info->file_size_bytes (optional; controller may also fill via stat())
    avdk_err_t (*get_media_info)(struct video_player_container_parser_ops_s *ops, video_player_media_info_t *media_info);
    // Get encoded video packet size (required, must be implemented)
    // If target_pts is provided (target_pts != VIDEO_PLAYER_PTS_INVALID), seek to the packet/frame at that PTS before reading
    // If target_pts is VIDEO_PLAYER_PTS_INVALID, read the next packet/frame sequentially
    avdk_err_t (*get_video_packet_size)(struct video_player_container_parser_ops_s *ops, uint32_t *packet_size, uint64_t target_pts);
    // Get encoded audio packet size (required, must be implemented)
    // If target_pts is provided (target_pts != VIDEO_PLAYER_PTS_INVALID), seek to the packet/chunk at that PTS before reading
    // If target_pts is VIDEO_PLAYER_PTS_INVALID, read the next packet/chunk sequentially
    avdk_err_t (*get_audio_packet_size)(struct video_player_container_parser_ops_s *ops, uint32_t *packet_size, uint64_t target_pts);
    // Read encoded video packet from container into out_buffer->data
    avdk_err_t (*read_video_packet)(struct video_player_container_parser_ops_s *ops, video_player_buffer_t *out_buffer, uint64_t target_pts);
    // Read encoded audio packet from container into out_buffer->data
    // If target_pts is provided (target_pts != VIDEO_PLAYER_PTS_INVALID), seek to the packet/chunk at that PTS before reading
    // If target_pts is VIDEO_PLAYER_PTS_INVALID, read the next packet/chunk sequentially
    avdk_err_t (*read_audio_packet)(struct video_player_container_parser_ops_s *ops, video_player_buffer_t *out_buffer, uint64_t target_pts);

    // Optional: query supported file extensions before create().
    // This helps controller pick parsers without repeatedly creating instances and trying open().
    //
    // - exts: output pointer to a static const array owned by the parser (do NOT free).
    //   Each entry should start with '.' (e.g. ".mp4") and be lowercase; controller will match case-insensitively.
    // - ext_count: number of elements in exts[].
    // - Return AVDK_ERR_OK on success. If not implemented, controller will fallback to legacy heuristics.
    avdk_err_t (*get_supported_file_extensions)(const struct video_player_container_parser_ops_s *ops,
                                                const char * const **exts,
                                                uint32_t *ext_count);
} video_player_container_parser_ops_t;

// -----------------------------------------------------------------------------
// Player configuration
// -----------------------------------------------------------------------------

// Audio configuration
typedef struct bk_video_player_audio_config
{
    // Pipeline buffer pool configuration
    // Audio pipeline: container parser -> audio decode -> output
    uint32_t parser_to_decode_buffer_count;        // Buffer pool between container parser and audio decode
    uint32_t decode_to_output_buffer_count;        // Buffer pool between audio decode and output

    // Unified audio buffer allocation callbacks
    // Used for both:
    // - Encoded packets (container parser -> audio decode)
    // - Decoded audio frames/blocks (audio decode -> output)
    video_player_audio_buffer_alloc_cb_t buffer_alloc_cb;
    video_player_audio_buffer_free_cb_t buffer_free_cb;

    // Decode complete callback
    video_player_audio_decode_complete_cb_t decode_complete_cb;

    // Optional audio output control callbacks for volume/mute.
    // If audio_ctrl_user_data is NULL, user_data will be used.
    video_player_audio_set_volume_cb_t audio_set_volume_cb;
    video_player_audio_set_mute_cb_t audio_set_mute_cb;
    void *audio_ctrl_user_data;

    // Optional audio output reset callback (mainly for app layer track switching).
    // If audio_output_reset_user_data is NULL, user_data will be used.
    video_player_audio_output_reset_cb_t audio_output_reset_cb;
    void *audio_output_reset_user_data;

    // Optional audio output (re)configuration callback.
    // If audio_output_config_user_data is NULL, user_data will be used.
    // This callback is triggered when a new media file is started and audio stream parameters are known.
    video_player_audio_output_config_cb_t audio_output_config_cb;
    void *audio_output_config_user_data;
} bk_video_player_audio_config_t;

// Video configuration
typedef struct bk_video_player_video_config
{
    // Pipeline buffer pool configuration
    // Video pipeline: container parser -> video decode -> output
    uint32_t parser_to_decode_buffer_count;        // Buffer pool between container parser and video decode
    uint32_t decode_to_output_buffer_count;        // Buffer pool between video decode and output

    // Encoded (packet) buffer allocation callbacks (from container parser)
    video_player_video_packet_buffer_alloc_cb_t packet_buffer_alloc_cb;
    video_player_video_packet_buffer_free_cb_t packet_buffer_free_cb;

    // Decoded (post-decode) buffer allocation callbacks
    video_player_video_buffer_alloc_cb_t buffer_alloc_cb;
    video_player_video_buffer_free_cb_t buffer_free_cb;

    // Decode complete callback
    video_player_video_decode_complete_cb_t decode_complete_cb;

    // Expected decoded video output format (e.g., PIXEL_FMT_YUYV, PIXEL_FMT_RGB565)
    // Video decode thread will decode video to this format and then pass it to upper layer.
    pixel_format_t output_format;
} bk_video_player_video_config_t;

// Player configuration
typedef struct bk_video_player_config
{
    bk_video_player_audio_config_t audio;
    bk_video_player_video_config_t video;

    // User data for callbacks
    void *user_data;

    // Playback finished notify callback (optional).
    // This callback is intended for app layer to decide what to do after internal playback completes.
    video_player_playback_finished_cb_t playback_finished_cb;
    void *playback_finished_user_data;
} bk_video_player_config_t;

// -----------------------------------------------------------------------------
// Public player interface (ops table)
// -----------------------------------------------------------------------------

// Player operations structure
typedef struct bk_video_player
{
    avdk_err_t (*open)(bk_video_player_handle_t handle);
    avdk_err_t (*close)(bk_video_player_handle_t handle);

    //play control
    avdk_err_t (*set_file_path)(bk_video_player_handle_t handle, const char *file_path);
    avdk_err_t (*play)(bk_video_player_handle_t handle, const char *file_path);
    avdk_err_t (*stop)(bk_video_player_handle_t handle);
    avdk_err_t (*set_pause)(bk_video_player_handle_t handle, bool pause);
    // Seek to a specific time (in milliseconds)
    avdk_err_t (*seek)(bk_video_player_handle_t handle, uint64_t time_ms);
    // Fast forward (skip forward by specified time)
    avdk_err_t (*fast_forward)(bk_video_player_handle_t handle, uint32_t time_ms);
    // Rewind (skip backward by specified time)
    avdk_err_t (*rewind)(bk_video_player_handle_t handle, uint32_t time_ms);

    // register decoder and parser
    avdk_err_t (*register_audio_decoder)(bk_video_player_handle_t handle, const video_player_audio_decoder_ops_t *decoder_ops);
    avdk_err_t (*register_video_decoder)(bk_video_player_handle_t handle, video_player_video_decoder_ops_t *decoder_ops);
    avdk_err_t (*register_container_parser)(bk_video_player_handle_t handle, video_player_container_parser_ops_t *parser_ops);

    // info query
    avdk_err_t (*get_media_info)(bk_video_player_handle_t handle,
        const char *file_path,
        video_player_media_info_t *media_info);
    avdk_err_t (*get_status)(bk_video_player_handle_t handle, video_player_status_t *status);
    avdk_err_t (*get_current_time)(bk_video_player_handle_t handle, uint64_t *time_ms);

    // Delete the video player instance
    avdk_err_t (*delete_video_player)(bk_video_player_handle_t handle);
    // Extended interface for additional functionalities
    avdk_err_t (*ioctl)(bk_video_player_handle_t handle, bk_video_player_ioctl_cmd_t cmd, void *param);
} bk_video_player_t;

#ifdef __cplusplus
}
#endif

