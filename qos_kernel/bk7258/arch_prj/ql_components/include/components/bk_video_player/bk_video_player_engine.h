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

#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct bk_video_player_engine *bk_video_player_engine_handle_t;

/**
 * @brief Create new video player engine instance
 *
 * @param handle Output handle pointer
 * @param config Player configuration
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_new(bk_video_player_engine_handle_t *handle, bk_video_player_config_t *config);

/**
 * @brief Delete video player engine instance
 *
 * @param handle Player engine handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_delete(bk_video_player_engine_handle_t handle);

/**
 * @brief Open video player engine
 *
 * @param handle Player engine handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_open(bk_video_player_engine_handle_t handle);

/**
 * @brief Close video player engine
 *
 * @param handle Player engine handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_close(bk_video_player_engine_handle_t handle);

/**
 * @brief Set media file path (must be called before play)
 *
 * @param handle Player engine handle
 * @param file_path Path to media file
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_set_file_path(bk_video_player_engine_handle_t handle, const char *file_path);

/**
 * @brief Start playback (file path must be set first)
 *
 * @param handle Player engine handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_play(bk_video_player_engine_handle_t handle);

/**
 * @brief Stop playback
 *
 * @param handle Player engine handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_stop(bk_video_player_engine_handle_t handle);

/**
 * @brief Set pause state
 *
 * @param handle Player engine handle
 * @param pause Pause state (true = paused, false = playing)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_set_pause(bk_video_player_engine_handle_t handle, bool pause);

/**
 * @brief Seek to specific time position (in milliseconds)
 *
 * @param handle Player engine handle
 * @param time_ms Target time position in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_seek(bk_video_player_engine_handle_t handle, uint64_t time_ms);

/**
 * @brief Fast forward (skip forward by specified time)
 *
 * @param handle Player engine handle
 * @param time_ms Time to skip forward in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_fast_forward(bk_video_player_engine_handle_t handle, uint32_t time_ms);

/**
 * @brief Fast backward (skip backward by specified time)
 *
 * @param handle Player engine handle
 * @param time_ms Time to skip backward in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_rewind(bk_video_player_engine_handle_t handle, uint32_t time_ms);

/**
 * @brief Set A/V sync offset (milliseconds) to help audio/video synchronization during playback.
 *
 * This API adjusts how video is synchronized against the audio-driven clock:
 * - offset_ms > 0: treat audio clock as "later" -> video catches up faster (less waiting / more dropping)
 * - offset_ms < 0: treat audio clock as "earlier" -> video is delayed (more waiting)
 *
 * Notes:
 * - This does NOT change seek targets (media timeline). It only affects video scheduling vs audio clock.
 * - This does NOT delay audio output; it only changes video-side sync decisions.
 *
 * @param handle Player engine handle
 * @param offset_ms Signed offset in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_set_av_sync_offset_ms(bk_video_player_engine_handle_t handle, int32_t offset_ms);

/**
 * @brief Play (or replay) a file from the beginning
 *
 * This helper API hides the common start/restart sequence from upper layers.
 * It performs:
 * - stop (best-effort, ignore errors)
 * - set_file_path(file_path)
 * - play() (controller will start playback from the beginning)
 *
 * @param handle Player engine handle
 * @param file_path Path to media file (must not be NULL/empty)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_play_file(bk_video_player_engine_handle_t handle, const char *file_path);

/**
 * @brief Set audio volume
 *
 * @param handle Player engine handle
 * @param volume Volume level (0-100, where 0 is mute and 100 is maximum)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 *
 * @note This API requires bk_video_player_config_t.audio.audio_set_volume_cb to be provided
 *       when creating the engine instance. Otherwise it returns AVDK_ERR_UNSUPPORTED.
 */
avdk_err_t bk_video_player_engine_set_volume(bk_video_player_engine_handle_t handle, uint8_t volume);

/**
 * @brief Increase audio volume by step
 *
 * @param handle Player engine handle
 * @param step Volume step to increase (0-100). If step is 0, this API does nothing and returns AVDK_ERR_OK.
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 *
 * @note This API requires bk_video_player_config_t.audio.audio_set_volume_cb to be provided
 *       when creating the engine instance. Otherwise it returns AVDK_ERR_UNSUPPORTED.
 */
avdk_err_t bk_video_player_engine_volume_up(bk_video_player_engine_handle_t handle, uint8_t step);

/**
 * @brief Decrease audio volume by step
 *
 * @param handle Player engine handle
 * @param step Volume step to decrease (0-100). If step is 0, this API does nothing and returns AVDK_ERR_OK.
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 *
 * @note This API requires bk_video_player_config_t.audio.audio_set_volume_cb to be provided
 *       when creating the engine instance. Otherwise it returns AVDK_ERR_UNSUPPORTED.
 */
avdk_err_t bk_video_player_engine_volume_down(bk_video_player_engine_handle_t handle, uint8_t step);

/**
 * @brief Set audio mute state
 *
 * @param handle Player engine handle
 * @param mute Mute state (true = muted, false = unmuted)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 *
 * @note Prefer providing bk_video_player_config_t.audio.audio_set_mute_cb to apply actual
 *       mute control in upper layer (DAC/codec/voice service, etc.).
 */
avdk_err_t bk_video_player_engine_set_mute(bk_video_player_engine_handle_t handle, bool mute);

/**
 * @brief Register audio decoder
 *
 * @param handle Player engine handle
 * @param decoder_ops Audio decoder operations
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_register_audio_decoder(bk_video_player_engine_handle_t handle,
                                                         const video_player_audio_decoder_ops_t *decoder_ops);

/**
 * @brief Register video decoder
 *
 * @param handle Player engine handle
 * @param decoder_ops Video decoder operations
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_register_video_decoder(bk_video_player_engine_handle_t handle,
                                                         video_player_video_decoder_ops_t *decoder_ops);

/**
 * @brief Register container parser
 *
 * @param handle Player engine handle
 * @param parser_ops Container parser operations
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_register_container_parser(bk_video_player_engine_handle_t handle,
                                                            video_player_container_parser_ops_t *parser_ops);

/**
 * @brief Get current playback time position (in milliseconds)
 *
 * @param handle Player engine handle
 * @param time_ms Output current time position
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_get_current_time(bk_video_player_engine_handle_t handle, uint64_t *time_ms);

/**
 * @brief Get playback status
 *
 * @param handle Player engine handle
 * @param status Output playback status
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_get_status(bk_video_player_engine_handle_t handle, video_player_status_t *status);

/**
 * @brief Get media information (video/audio parameters + file info)
 *
 * @param handle Player engine handle
 * @param file_path File path to query. If NULL, query current opened file info (cached).
 * @param media_info Output media information (must not be NULL)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_engine_get_media_info(bk_video_player_engine_handle_t handle,
                                                 const char *file_path,
                                                 video_player_media_info_t *media_info);

#ifdef __cplusplus
}
#endif

