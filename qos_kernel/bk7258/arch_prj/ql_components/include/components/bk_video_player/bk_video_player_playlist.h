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
#include "components/bk_video_player/bk_video_player_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

// Playback mode when reaching end of file (what to do when playback finishes)
typedef enum
{
    BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_STOP = 0, // Stop playback (default)
    BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_REPEAT,   // Repeat current file
    BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_LOOP,     // Loop playlist (wrap to first when reaching the end)
} bk_video_player_playlist_play_mode_t;

// Forward declaration
typedef struct bk_video_player_playlist *bk_video_player_playlist_handle_t;

/**
 * @brief Create new playlist player instance
 *
 * @param handle Output handle pointer
 * @param config Player configuration
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_new(bk_video_player_playlist_handle_t *handle, bk_video_player_config_t *config);

/**
 * @brief Delete playlist player instance
 *
 * @param handle Playlist player handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_delete(bk_video_player_playlist_handle_t handle);

/**
 * @brief Open playlist player
 *
 * @param handle Playlist player handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_open(bk_video_player_playlist_handle_t handle);

/**
 * @brief Close playlist player
 *
 * @param handle Playlist player handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_close(bk_video_player_playlist_handle_t handle);

// ========== Playlist Management ==========

/**
 * @brief Add file to playlist
 *
 * @param handle Playlist player handle
 * @param file_path Path to media file
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_add_file(bk_video_player_playlist_handle_t handle, const char *file_path);

/**
 * @brief Remove file from playlist
 *
 * @param handle Playlist player handle
 * @param file_path Path to media file
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_remove_file(bk_video_player_playlist_handle_t handle, const char *file_path);

/**
 * @brief Clear all files from playlist
 *
 * @param handle Playlist player handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_clear(bk_video_player_playlist_handle_t handle);

/**
 * @brief Get current file count in playlist
 *
 * @param handle Playlist player handle
 * @param count Output file count
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_get_file_count(bk_video_player_playlist_handle_t handle, uint32_t *count);

/**
 * @brief Get current file path and its index in playlist
 *
 * index is 0-based. If index is NULL, only file_path will be returned.
 *
 * @param handle Playlist player handle
 * @param file_path Output buffer for file path (must be large enough)
 * @param file_path_size Size of file_path buffer
 * @param index Output current index (0-based), optional
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_get_current_file(bk_video_player_playlist_handle_t handle,
                                                     char *file_path,
                                                     uint32_t file_path_size,
                                                     uint32_t *index);

// ========== Playback Control ==========

/**
 * @brief Play (or replay) a file from the beginning
 *
 * This helper API hides the common start/restart sequence from upper layers.
 * It performs:
 * - stop (best-effort, ignore errors)
 * - set current file (if file_path provided)
 * - start playback from the beginning
 *
 * @param handle Playlist player handle
 * @param file_path Path to media file (NULL to play current file in playlist)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_play_file(bk_video_player_playlist_handle_t handle, const char *file_path);

/**
 * @brief Stop playback
 *
 * @param handle Playlist player handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_stop(bk_video_player_playlist_handle_t handle);

/**
 * @brief Set pause state
 *
 * @param handle Playlist player handle
 * @param pause Pause state (true = paused, false = playing)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_set_pause(bk_video_player_playlist_handle_t handle, bool pause);

/**
 * @brief Seek to specific time position (in milliseconds)
 *
 * @param handle Playlist player handle
 * @param time_ms Target time position in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_seek(bk_video_player_playlist_handle_t handle, uint64_t time_ms);

/**
 * @brief Fast forward (skip forward by specified time)
 *
 * @param handle Playlist player handle
 * @param time_ms Time to skip forward in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_fast_forward(bk_video_player_playlist_handle_t handle, uint32_t time_ms);

/**
 * @brief Fast backward (skip backward by specified time)
 *
 * @param handle Playlist player handle
 * @param time_ms Time to skip backward in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_rewind(bk_video_player_playlist_handle_t handle, uint32_t time_ms);

/**
 * @brief Set audio volume
 *
 * @param handle Playlist player handle
 * @param volume Volume level (0-100, where 0 is mute and 100 is maximum)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_set_volume(bk_video_player_playlist_handle_t handle, uint8_t volume);

/**
 * @brief Increase audio volume by step
 *
 * @param handle Playlist player handle
 * @param step Volume step to increase (0-100). If step is 0, this API does nothing and returns AVDK_ERR_OK.
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_volume_up(bk_video_player_playlist_handle_t handle, uint8_t step);

/**
 * @brief Decrease audio volume by step
 *
 * @param handle Playlist player handle
 * @param step Volume step to decrease (0-100). If step is 0, this API does nothing and returns AVDK_ERR_OK.
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_volume_down(bk_video_player_playlist_handle_t handle, uint8_t step);

/**
 * @brief Set audio mute state
 *
 * @param handle Playlist player handle
 * @param mute Mute state (true = muted, false = unmuted)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_set_mute(bk_video_player_playlist_handle_t handle, bool mute);

/**
 * @brief Set A/V sync offset (milliseconds)
 *
 * This is a thin wrapper over bk_video_player_engine_set_av_sync_offset_ms().
 *
 * @param handle Playlist player handle
 * @param offset_ms A/V sync offset in milliseconds
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_set_av_sync_offset_ms(bk_video_player_playlist_handle_t handle, int32_t offset_ms);

/**
 * @brief Get media information (video/audio parameters + file info)
 *
 * @param handle Playlist player handle
 * @param file_path File path to query. If NULL, query current opened file info (cached).
 * @param media_info Output media information (must not be NULL)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_get_media_info(bk_video_player_playlist_handle_t handle,
                                                   const char *file_path,
                                                   video_player_media_info_t *media_info);

// ========== File Navigation ==========

/**
 * @brief Play next file in playlist
 *
 * @param handle Playlist player handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_play_next(bk_video_player_playlist_handle_t handle);

/**
 * @brief Play previous file in playlist
 *
 * @param handle Playlist player handle
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_play_prev(bk_video_player_playlist_handle_t handle);

/**
 * @brief Play file at specific index in playlist
 *
 * @param handle Playlist player handle
 * @param index File index (0-based)
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_play_at_index(bk_video_player_playlist_handle_t handle, uint32_t index);

// ========== Configuration ==========

/**
 * @brief Set play mode (what to do when playback finishes)
 *
 * @param handle Playlist player handle
 * @param mode Play mode
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_set_play_mode(bk_video_player_playlist_handle_t handle,
                                                  bk_video_player_playlist_play_mode_t mode);

// ========== Decoder/Parser Registration ==========

/**
 * @brief Register audio decoder
 *
 * @param handle Playlist player handle
 * @param decoder_ops Audio decoder operations
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_register_audio_decoder(bk_video_player_playlist_handle_t handle,
                                                           const video_player_audio_decoder_ops_t *decoder_ops);

/**
 * @brief Register video decoder
 *
 * @param handle Playlist player handle
 * @param decoder_ops Video decoder operations
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_register_video_decoder(bk_video_player_playlist_handle_t handle,
                                                           video_player_video_decoder_ops_t *decoder_ops);

/**
 * @brief Register container parser
 *
 * @param handle Playlist player handle
 * @param parser_ops Container parser operations
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_register_container_parser(bk_video_player_playlist_handle_t handle,
                                                              video_player_container_parser_ops_t *parser_ops);

// ========== Status Query ==========

/**
 * @brief Get current playback time position (in milliseconds)
 *
 * @param handle Playlist player handle
 * @param time_ms Output current time position
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_get_current_time(bk_video_player_playlist_handle_t handle, uint64_t *time_ms);

/**
 * @brief Get playback status
 *
 * @param handle Playlist player handle
 * @param status Output playback status
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t bk_video_player_playlist_get_status(bk_video_player_playlist_handle_t handle, video_player_status_t *status);

#ifdef __cplusplus
}
#endif

