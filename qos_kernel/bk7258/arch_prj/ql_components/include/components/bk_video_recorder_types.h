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
#include "components/avdk_utils/avdk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define delete_video_recorder delete

typedef struct bk_video_recorder *bk_video_recorder_handle_t;
typedef struct bk_video_recorder bk_video_recorder_t;
typedef struct bk_video_recorder_config bk_video_recorder_config_t;

typedef enum
{
    VIDEO_RECORDER_STATUS_NONE = 0,
    VIDEO_RECORDER_STATUS_OPENED,
    VIDEO_RECORDER_STATUS_CLOSED,
    VIDEO_RECORDER_STATUS_STARTED,
    VIDEO_RECORDER_STATUS_STOPPED,
    VIDEO_RECORDER_STATUS_FINISHED,
} video_recorder_status_t;

typedef enum
{
    BK_VIDEO_RECORDER_IOCTL_CMD_BASE = 0,
} bk_video_recorder_ioctl_cmd_t;

// Video record type definitions
#define VIDEO_RECORDER_TYPE_AVI    0
#define VIDEO_RECORDER_TYPE_MP4    1

// Audio format definitions
// Note: For AVI, audio_format follows WAVE format tags. For MP4, this field is used as a hint
// to select the container audio codec (e.g., AAC 'mp4a').
#define VIDEO_RECORDER_AUDIO_FORMAT_PCM   0x0001u
#define VIDEO_RECORDER_AUDIO_FORMAT_ALAW  0x0006u
#define VIDEO_RECORDER_AUDIO_FORMAT_MULAW 0x0007u
#define VIDEO_RECORDER_AUDIO_FORMAT_MP3   0x0055u
#define VIDEO_RECORDER_AUDIO_FORMAT_G722  0x0065u
#define VIDEO_RECORDER_AUDIO_FORMAT_AAC   0x00FFu

// Video record format definitions
#define VIDEO_RECORDER_FORMAT_H264 0
#define VIDEO_RECORDER_FORMAT_MJPEG 1
#define VIDEO_RECORDER_FORMAT_YUV  2

// Forward declarations (defined in bk_video_record_ctlr.h)
// These are typedefs, not struct tags, so we declare them as incomplete types
typedef struct video_recorder_frame_data_s video_recorder_frame_data_t;
typedef struct video_recorder_audio_data_s video_recorder_audio_data_t;

// Callback function to get video frame buffer
// Return: 0 on success, negative on error
// frame_data: output parameter, should be filled with frame data pointer and length
typedef int (*video_recorder_get_frame_cb_t)(void *user_data, video_recorder_frame_data_t *frame_data);

// Callback function to get audio data buffer
// Return: 0 on success, negative on error
// audio_data: output parameter, should be filled with audio data pointer and length
typedef int (*video_recorder_get_audio_cb_t)(void *user_data, video_recorder_audio_data_t *audio_data);

// Callback function to release video frame buffer after writing
// frame_data: the frame data that was written and should be released
typedef void (*video_recorder_release_frame_cb_t)(void *user_data, video_recorder_frame_data_t *frame_data);

// Callback function to release audio data buffer after writing
// audio_data: the audio data that was written and should be released
typedef void (*video_recorder_release_audio_cb_t)(void *user_data, video_recorder_audio_data_t *audio_data);

typedef struct bk_video_recorder_config
{
    uint32_t record_type;      // VIDEO_RECORDER_TYPE_AVI or VIDEO_RECORDER_TYPE_MP4
    uint32_t record_format;    // VIDEO_RECORDER_FORMAT_H264, VIDEO_RECORDER_FORMAT_MJPEG, etc.
    uint32_t record_quality;
    uint32_t record_bitrate;
    uint32_t record_framerate;
    uint32_t video_width;      // Video width in pixels
    uint32_t video_height;     // Video height in pixels
    uint32_t audio_channels;   // Audio channels (0 = no audio)
    uint32_t audio_rate;       // Audio sample rate in Hz
    uint32_t audio_bits;       // Audio bits per sample
    uint32_t audio_format;     // Audio format (WAVE_FORMAT_PCM, etc.)
    // Callback functions to get buffer data
    video_recorder_get_frame_cb_t get_frame_cb;   // Callback to get video frame (can be NULL)
    video_recorder_get_audio_cb_t get_audio_cb;   // Callback to get audio data (can be NULL)
    // Callback functions to release buffer data after writing
    video_recorder_release_frame_cb_t release_frame_cb;   // Callback to release video frame after writing (can be NULL)
    video_recorder_release_audio_cb_t release_audio_cb;   // Callback to release audio data after writing (can be NULL)
    void *user_data;            // User data passed to callbacks
} bk_video_recorder_config_t;

typedef struct bk_video_recorder
{
    avdk_err_t (*open)(bk_video_recorder_handle_t handle); /*!< Open the hardware JPEG decoder */
    avdk_err_t (*close)(bk_video_recorder_handle_t handle); /*!< Close the hardware JPEG decoder */
    avdk_err_t (*start)(bk_video_recorder_handle_t handle, char *file_path, uint32_t record_type); /*!< Decode JPEG frame */
    avdk_err_t (*stop)(bk_video_recorder_handle_t handle); /*!< Decode JPEG frame */
    avdk_err_t (*delete_video_recorder)(bk_video_recorder_handle_t handle); /*!< Delete the hardware JPEG decoder instance */
    avdk_err_t (*ioctl)(bk_video_recorder_handle_t handle, bk_video_recorder_ioctl_cmd_t cmd, void *param); /*!< Extended interface for additional functionalities */
} bk_video_recorder_t;

#ifdef __cplusplus
}
#endif
