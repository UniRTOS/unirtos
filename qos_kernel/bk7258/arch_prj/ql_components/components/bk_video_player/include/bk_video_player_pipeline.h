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
#include "components/bk_video_player/bk_video_player_types.h"
#include "bk_video_player_buffer_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pipeline stage definition
typedef enum
{
    PIPELINE_STAGE_CONTAINER_PARSE,  // Container parse stage (extract encoded packets from container)
    PIPELINE_STAGE_DECODE,       // Decode stage (decode encoded data)
    PIPELINE_STAGE_OUTPUT,       // Output stage (send decoded data to external)
} pipeline_stage_t;

// Pipeline structure for video pipeline
// Container parse (video packets) -> Video decode -> Video output
typedef struct video_pipeline_s
{
    // Stage 1: Container parse -> Stage 2 buffer pool
    video_player_buffer_pool_t parser_to_decode_pool;  // Buffer pool between container parser and video decode
    
    // Stage 2: Video decode -> Stage 3 buffer pool
    video_player_buffer_pool_t decode_to_output_pool; // Buffer pool between video decode and output
    
    // Current video PTS for sync
    uint64_t current_video_pts;
    // Next video frame PTS: the target PTS for the next video packet/frame to be read from container
    // This is used for A/V sync: audio decode thread checks if this is expired and updates it
    uint64_t next_video_frame_pts;
    beken_mutex_t pts_mutex;
} video_pipeline_t;

// Pipeline structure for audio pipeline
// Container parse (audio packets) -> Audio decode -> Audio output
typedef struct audio_pipeline_s
{
    // Stage 1: Container parse -> Stage 2 buffer pool
    video_player_buffer_pool_t parser_to_decode_pool;  // Buffer pool between container parser and audio decode
    
    // Stage 2: Audio decode -> Stage 3 buffer pool
    video_player_buffer_pool_t decode_to_output_pool; // Buffer pool between audio decode and output
    
    // Current audio PTS for sync
    uint64_t current_audio_pts;
    beken_mutex_t pts_mutex;
} audio_pipeline_t;

/**
 * @brief Initialize video pipeline
 *
 * @param pipeline Video pipeline pointer
 * @param file_to_decode_buffer_count Buffer count for file-to-decode pool
 * @param decode_to_output_buffer_count Buffer count for decode-to-output pool
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t video_pipeline_init(video_pipeline_t *pipeline, uint32_t file_to_decode_buffer_count, uint32_t decode_to_output_buffer_count);

/**
 * @brief Deinitialize video pipeline
 *
 * @param pipeline Video pipeline pointer
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t video_pipeline_deinit(video_pipeline_t *pipeline);

/**
 * @brief Initialize audio pipeline
 *
 * @param pipeline Audio pipeline pointer
 * @param file_to_decode_buffer_count Buffer count for file-to-decode pool
 * @param decode_to_output_buffer_count Buffer count for decode-to-output pool
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t video_player_audio_pipeline_init(audio_pipeline_t *pipeline, uint32_t file_to_decode_buffer_count, uint32_t decode_to_output_buffer_count);

/**
 * @brief Deinitialize audio pipeline
 *
 * @param pipeline Audio pipeline pointer
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t video_player_audio_pipeline_deinit(audio_pipeline_t *pipeline);

/**
 * @brief Update video PTS in pipeline
 *
 * @param pipeline Video pipeline pointer
 * @param pts Presentation timestamp
 */
void video_pipeline_update_pts(video_pipeline_t *pipeline, uint64_t pts);

/**
 * @brief Get current video PTS from pipeline
 *
 * @param pipeline Video pipeline pointer
 * @return uint64_t Current video PTS
 */
uint64_t video_pipeline_get_pts(video_pipeline_t *pipeline);

/**
 * @brief Update audio PTS in pipeline
 *
 * @param pipeline Audio pipeline pointer
 * @param pts Presentation timestamp
 */
void audio_pipeline_update_pts(audio_pipeline_t *pipeline, uint64_t pts);

/**
 * @brief Get current audio PTS from pipeline
 *
 * @param pipeline Audio pipeline pointer
 * @return uint64_t Current audio PTS
 */
uint64_t audio_pipeline_get_pts(audio_pipeline_t *pipeline);

/**
 * @brief Set next video frame PTS (target PTS for next frame to be read)
 *
 * @param pipeline Video pipeline pointer
 * @param pts Target PTS for next video frame
 */
void video_pipeline_set_next_frame_pts(video_pipeline_t *pipeline, uint64_t pts);

/**
 * @brief Get next video frame PTS (target PTS for next frame to be read)
 *
 * @param pipeline Video pipeline pointer
 * @return uint64_t Next video frame PTS
 */
uint64_t video_pipeline_get_next_frame_pts(video_pipeline_t *pipeline);

#ifdef __cplusplus
}
#endif

