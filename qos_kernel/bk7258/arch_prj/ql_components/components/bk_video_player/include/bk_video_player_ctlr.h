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
#include "components/bk_video_player/bk_video_player_types.h"
#include "bk_video_player_buffer_pool.h"
#include "bk_video_player_pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bk_video_player *bk_video_player_ctlr_handle_t;
typedef struct bk_video_player bk_video_player_ctlr_t;
typedef struct bk_video_player_config bk_video_player_ctlr_config_t;

// Player status
typedef struct
{
    video_player_status_t status;
} private_video_player_status_t;

// Forward declarations (defined in bk_video_player_buffer_pool.h)
typedef struct video_player_buffer_node_s video_player_buffer_node_t;
typedef struct video_player_buffer_pool_s video_player_buffer_pool_t;

// Decoder list node
typedef struct video_player_audio_decoder_node_s
{
    const video_player_audio_decoder_ops_t *ops;
    struct video_player_audio_decoder_node_s *next;
} video_player_audio_decoder_node_t;

typedef struct video_player_video_decoder_node_s
{
    video_player_video_decoder_ops_t *ops;
    struct video_player_video_decoder_node_s *next;
} video_player_video_decoder_node_t;

typedef struct video_player_container_parser_node_s
{
    video_player_container_parser_ops_t *ops;
    struct video_player_container_parser_node_s *next;
} video_player_container_parser_node_t;

typedef enum
{
    VIDEO_PLAYER_CLOCK_AUDIO = 0,
    VIDEO_PLAYER_CLOCK_VIDEO = 1,
} video_player_clock_source_t;

/**
 * @brief Apply an A/V sync offset to a base clock value (milliseconds).
 *
 * This helper is used by the video side when syncing to the audio-driven clock.
 *
 * Semantics:
 * - offset_ms > 0: treat audio clock as "later" -> video catches up faster (less waiting / more dropping)
 * - offset_ms < 0: treat audio clock as "earlier" -> video is delayed (more waiting)
 *
 * Notes:
 * - base_ms == 0 means "unknown clock", keep it as 0.
 * - The result is saturated to [0, UINT64_MAX].
 */
static inline uint64_t bk_video_player_apply_av_sync_offset_ms(uint64_t base_ms, int32_t offset_ms)
{
    if (base_ms == 0 || offset_ms == 0)
    {
        return base_ms;
    }

    if (offset_ms > 0)
    {
        uint64_t off = (uint64_t)(uint32_t)offset_ms;
        if (UINT64_MAX - base_ms < off)
        {
            return UINT64_MAX;
        }
        return base_ms + off;
    }

    uint64_t off = (uint64_t)(uint32_t)(-offset_ms);
    if (off >= base_ms)
    {
        return 0;
    }
    return base_ms - off;
}

// Private controller structure
typedef struct private_video_player_ctlr_s
{
    bk_video_player_config_t config;
    bk_video_player_t ops;
    private_video_player_status_t module_status;

    // Decoder lists
    video_player_audio_decoder_node_t *audio_decoder_list;
    video_player_video_decoder_node_t *video_decoder_list;
    video_player_container_parser_node_t *container_parser_list;
    beken_mutex_t decoder_list_mutex;

    // Active decoders (selected for current file)
    video_player_audio_decoder_ops_t *active_audio_decoder;
    video_player_video_decoder_ops_t *active_video_decoder;
    video_player_container_parser_ops_t *active_container_parser;
    // Per-track enable flags for current file.
    // These flags decouple "stream exists in container" from "stream is playable".
    // - If a stream exists but no suitable decoder is found, the track is disabled and playback continues with the other track.
    // - If a stream does not exist, the corresponding track is disabled.
    // - For PCM audio, audio track is enabled even when active_audio_decoder is NULL (zero-copy path).
    bool audio_track_enabled;
    bool video_track_enabled;
    // Protect active_* pointers and their underlying contexts (parser/decoder) from concurrent access.
    // This avoids races between stop/close (deinit) and parse/decode threads (use).
    beken_mutex_t active_mutex;
    
    // Pipeline structures
    video_pipeline_t video_pipeline;  // Video pipeline: container parse -> video decode -> output
    audio_pipeline_t audio_pipeline;   // Audio pipeline: container parse -> audio decode -> output

    // Thread management
    // Container parse threads (pipeline stage 1)
    beken_thread_t *video_parse_thread;  // Video parse thread (read video packets from container)
    beken_thread_t *audio_parse_thread;  // Audio parse thread (read audio packets from container)
    // Decode threads (pipeline stage 2)
    beken_thread_t *video_decode_thread;        // Video decode thread
    beken_thread_t *audio_decode_thread;       // Audio decode thread
    
    beken_semaphore_t video_parse_sem;
    beken_semaphore_t audio_parse_sem;
    beken_semaphore_t video_decode_sem;
    beken_semaphore_t audio_decode_sem;
    
    bool video_parse_thread_running;
    bool audio_parse_thread_running;
    bool video_decode_thread_running;
    bool audio_decode_thread_running;
    
    bool video_parse_thread_exit;
    bool audio_parse_thread_exit;
    bool video_decode_thread_exit;
    bool audio_decode_thread_exit;

    // Playback control
    bool is_paused;
    uint64_t pause_start_time_ms;     // System time (ms) when pause started, used to calculate pause duration on resume
    uint32_t eof_notified_session_id;      // The play_session_id that has already posted FINISHED event (0 = none)
    bool video_eof_reached;                // Video EOF flag
    bool audio_eof_reached;                // Audio EOF flag
    beken_semaphore_t stop_sem;

    // Playback finished event notify (optional, only created when playback_finished_cb is provided)
    beken_thread_t *event_thread;
    beken_queue_t event_queue;
    beken_semaphore_t event_sem;
    bool event_thread_running;

    // Current file info (cached from parser during open/play)
    video_player_media_info_t current_media_info;

    // Probe cache (optional):
    // When upper layer calls get_media_info(file_path) before playback (e.g. to init audio output),
    // the controller can keep an OPENED parser instance here and reuse it later in select_decoders().
    // This avoids parsing the same container twice (e.g. moov/stbl in MP4).
    //
    // Lifetime:
    // - Filled by video_player_ctlr_get_media_info() probe path (only when an independent parser
    //   instance is available via create/destroy).
    // - Consumed (moved) by select_decoders() when file_path matches.
    // - Cleared on set_file_path() (when switching to a different file) and on close/delete.
    video_player_container_parser_ops_t *prefetch_container_parser;
    char *prefetch_file_path;
    video_player_media_info_t prefetch_media_info;

    // Current playing file path (single file, not a file list).
    // Used to restart playback on resume after a paused seek.
    char *current_file_path;

    // If seek is requested while paused, controller decodes exactly one video frame for preview,
    // and remembers the target so that resume will restart from the seek position.
    bool paused_seek_pending;
    uint64_t paused_seek_time_ms;

    // Playback timing
    // Single synchronization time point (playback PTS in ms).
    // All A/V sync and seek/restart logic must be based on this value only.
    uint64_t current_time_ms;
    beken_mutex_t time_mutex;
    // A/V sync offset (milliseconds) applied by video side when clock source is AUDIO.
    // See bk_video_player_apply_av_sync_offset_ms() for semantics.
    int32_t av_sync_offset_ms;
    // Last system tick (ms) when current_time_ms was updated (seek or A/V clock update).
    // Used to detect stalled audio clock and fall back to video clock to avoid deadlock.
    uint32_t last_time_update_tick_ms;

    // Seek preroll output control:
    // When seeking to a non-keyframe (e.g. H.264 P-frame), the parser may first output a keyframe packet
    // (and then jump directly to the target packet), so the decoder can establish references.
    // In this mode we must NOT deliver decoded frames whose pts is less than the requested seek pts.
    bool video_seek_drop_enable;
    uint64_t video_seek_drop_until_pts_ms;

    // Clock source:
    // - Prefer audio-driven clock if audio stream exists.
    // - Fall back to video-driven clock if no audio stream exists.
    video_player_clock_source_t clock_source;
    // Monotonic playback session id (incremented on each play start).
    // Used to tag encoded packets and drop stale packets from previous sessions.
    uint32_t play_session_id;

    // Delivered decoded video frame index (1-based), used by decode_complete_cb meta.
    // Updated in video decode thread (sync delivery) and vp_evt thread (async delivery).
    uint64_t delivered_video_frame_index;

    // Video pacing state (used by vp_evt thread when delivering video frames to upper layer).
    uint64_t vp_evt_last_video_pts;
    uint32_t vp_evt_last_video_time_ms;
    uint32_t vp_evt_last_video_session_id;
} private_video_player_ctlr_t;

/**
 * @brief Handle play mode (called from parse/decode threads)
 *
 * This is an internal helper used by pipeline threads when both audio/video reach EOF.
 *
 * @param controller Internal controller pointer
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_handle_play_mode(private_video_player_ctlr_t *controller);

/**
 * @brief Create new video player controller
 *
 * @param handle Output handle pointer
 * @param config Player configuration
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_ctlr_new(bk_video_player_ctlr_handle_t *handle, bk_video_player_ctlr_config_t *config);

#ifdef __cplusplus
}
#endif
