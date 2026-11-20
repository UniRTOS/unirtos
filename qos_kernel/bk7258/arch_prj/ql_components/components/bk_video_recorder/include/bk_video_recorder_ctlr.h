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

#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include "components/bk_video_recorder_types.h"

typedef struct bk_video_recorder *bk_video_recorder_ctlr_handle_t;
typedef struct bk_video_recorder bk_video_recorder_ctlr_t;
typedef struct bk_video_recorder_config bk_video_recorder_ctlr_config_t;

typedef struct
{
    video_recorder_status_t status;
} private_video_recorder_status_t;

typedef struct
{
    uint8_t *record_data;
    uint32_t record_len;
} private_video_recorder_param_t;

typedef enum {
	VIDEO_RECORDER_START = 0,
    VIDEO_RECORDER_STOP,
} video_recorder_msg_type_t;

typedef enum {
	VIDEO_RECORDER_EVENT_START = 0,
    VIDEO_RECORDER_EVENT_STOP,
} video_recorder_event_type_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
} video_recorder_msg_t;

// Structure for writing video frame data
typedef struct video_recorder_frame_data_s
{
    uint8_t *data;      // Video frame data pointer
    uint32_t length;    // Video frame data length
    uint32_t width;     // Video frame width (optional, can be 0 if already set)
    uint32_t height;    // Video frame height (optional, can be 0 if already set)
    void *frame_buffer; // Pointer to frame_buffer_t structure, used to release the buffer (can be NULL)
    uint32_t is_key_frame; // is key frame or not
} video_recorder_frame_data_t;

// Structure for writing audio data
typedef struct video_recorder_audio_data_s
{
    uint8_t *data;      // Audio data pointer
    uint32_t length;    // Audio data length
} video_recorder_audio_data_t;

typedef struct
{
    bk_video_recorder_config_t config;
    bk_video_recorder_t ops;
    private_video_recorder_status_t module_status;
    char *file_path;
    // AVI recording handle
    void *avi_handle;   // avi_t* pointer for AVI recording
    // Mutex for thread-safe AVI write operations
    // Protect concurrent AVI_write_frame/AVI_write_audio calls on the same avi_handle
    beken_mutex_t avi_write_mutex;
    // MP4 recording handle
    void *mp4_handle;   // mp4_t* pointer for MP4 recording
    beken_mutex_t mp4_write_mutex;   // Mutex to protect MP4_write_frame and MP4_write_audio calls
    uint32_t mp4_video_frame_count;  // Video frame count for MP4
    uint32_t mp4_audio_sample_count; // Audio sample count for MP4
    uint64_t mp4_mdat_offset;        // Offset of mdat box start
    uint64_t mp4_mdat_size;         // Size of mdat box
    // MP4 write-mode index cache (to minimize index file write次数).
    // The cache is provided to mp4lib at MP4_open_output_file_with_index_cache().
    void *mp4_index_cache_buf;
    uint32_t mp4_index_cache_size;
    // Thread management
    beken_thread_t *record_thread;   // Recording thread handle
    beken_semaphore_t thread_sem;    // Semaphore to control thread start/stop
    beken_semaphore_t stop_sem;      // Semaphore to wait for thread to finish current operation
    bool thread_running;             // Flag to indicate if thread should run
    bool thread_exit;                // Flag to indicate if thread should exit (for close)
    // Dynamic frame rate calculation
    uint32_t video_frame_count;      // Total video frames written
    uint32_t recording_start_time_ms; // Recording start time (ms)
    uint32_t last_frame_time_ms;     // Last frame write time (ms)
    uint32_t first_saved_frame_time_ms; // First saved frame time (ms) for fps estimation
    uint32_t last_saved_frame_time_ms; // Last saved frame time (ms) for frame rate limiting
    // H264 start gating: ensure recording begins from the first IDR frame.
    bool wait_for_video_idr;         // True when H264 needs to wait for first IDR frame
    bool video_idr_started;          // True after first IDR frame is written
    bool last_is_key_frame;          // Cached key frame flag from callback
    // MP4 + fixed-frame audio (G.711/G.722) requires fixed 20ms packetization to keep mp4lib STTS consistent.
    // Some audio sources may return 40ms (e.g., 320 bytes) on the first read due to backlog.
    // We split it into fixed 20ms chunks before feeding mp4lib.
    bool mp4_audio_packetize_enable;
    uint32_t mp4_audio_packetize_frame_bytes;   // bytes per 20ms frame for current codec/channels/rate
    uint32_t mp4_audio_packetize_partial_used;
    uint8_t *mp4_audio_packetize_partial_buf;   // allocated on start, size == mp4_audio_packetize_frame_bytes
    uint32_t mp4_audio_packetize_partial_buf_size;
    // Mutex for thread-safe MP4 write operations
} private_video_recorder_ctlr_t;

void *video_recorder_psram_malloc(uint32_t size);
void video_recorder_psram_free(void *ptr);

avdk_err_t bk_video_recorder_ctlr_new(bk_video_recorder_ctlr_handle_t *handle, bk_video_recorder_ctlr_config_t *config);
