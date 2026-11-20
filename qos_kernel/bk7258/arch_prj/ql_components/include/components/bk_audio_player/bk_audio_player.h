// Copyright 2024-2025 Beken
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


#ifndef __BK_AUDIO_PLAYER_H__
#define __BK_AUDIO_PLAYER_H__

#include <components/bk_audio_player/bk_audio_player_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Create a new audio player instance.
 *
 * @param[out]     handle  On success, receives the new instance handle.
 * @param[in]      cfg     Audio player configuration, see bk_audio_player_cfg_t.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 *                 - AUDIO_PLAYER_NO_MEM: Out of memory.
 *                 - AUDIO_PLAYER_INVALID: Invalid parameter.
 */
int bk_audio_player_new(bk_audio_player_handle_t *handle, bk_audio_player_cfg_t *cfg);

/**
 * @brief      Destroy an audio player instance and release its resources.
 *
 * @param[in]      handle  Instance handle returned by bk_audio_player_new.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_INVALID: Invalid handle (e.g. NULL).
 */
int bk_audio_player_delete(bk_audio_player_handle_t handle);

/**
 * @brief      Set play mode.
 *
 * @param[in]      handle  Instance handle.
 * @param[in]      mode  Play mode, see audio_player_mode_t.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_INVALID: Invalid parameter.
 */
int bk_audio_player_set_play_mode(bk_audio_player_handle_t handle, audio_player_mode_t mode);

/**
 * @brief      Set volume.
 *
 * @param[in]      handle  Instance handle.
 * @param[in]      volume  Volume level, range 0-100.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_INVALID: Invalid parameter.
 */
int bk_audio_player_set_volume(bk_audio_player_handle_t handle, int volume);

/**
 * @brief      Get current volume.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return     Current volume level (0-100).
 */
int bk_audio_player_get_volume(bk_audio_player_handle_t handle);

/**
 * @brief      Clear music list.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 */
int bk_audio_player_clear_music_list(bk_audio_player_handle_t handle);

/**
 * @brief      Add music to playlist.
 *
 * @param[in]      handle  Instance handle.
 * @param[in]      name  Music name.
 * @param[in]      uri   Music URI (URL or file path).
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 */
int bk_audio_player_add_music(bk_audio_player_handle_t handle, char *name, char *uri);

/**
 * @brief      Delete music from playlist by name.
 *
 * @param[in]      handle  Instance handle.
 * @param[in]      name  Music name.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 */
int bk_audio_player_del_music_by_name(bk_audio_player_handle_t handle, char *name);

/**
 * @brief      Delete music from playlist by URI.
 *
 * @param[in]      handle  Instance handle.
 * @param[in]      uri  Music URI (URL or file path).
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 */
int bk_audio_player_del_music_by_uri(bk_audio_player_handle_t handle, char *uri);

/**
 * @brief      Start playback.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 *                 - PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_start(bk_audio_player_handle_t handle);

/**
 * @brief      Stop playback.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 *                 - PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_stop(bk_audio_player_handle_t handle);

/**
 * @brief      Pause playback.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 *                 - PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_pause(bk_audio_player_handle_t handle);

/**
 * @brief      Resume playback.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 *                 - PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_resume(bk_audio_player_handle_t handle);

/**
 * @brief      Play previous song.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_INVALID: No previous song available.
 *                 - AUDIO_PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_prev(bk_audio_player_handle_t handle);

/**
 * @brief      Play next song.
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_INVALID: No next song available.
 *                 - AUDIO_PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_next(bk_audio_player_handle_t handle);

/**
 * @brief      Jump to specific song in playlist.
 *
 * @param[in]      handle  Instance handle.
 * @param[in]      idx  Song index in playlist.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_INVALID: Invalid index.
 *                 - AUDIO_PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_jumpto(bk_audio_player_handle_t handle, int idx);

/**
 * @brief      Seek current track to the specified playback position.
 *
 * @note       This API only works while playing local audio files.
 *
 * @param[in]      handle  Instance handle.
 * @param[in]      second Playback position in seconds from start of track.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success (seek request accepted).
 *                 - AUDIO_PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_seek(bk_audio_player_handle_t handle, int second);

/**
 * @brief Extract metadata from an audio file.
 *
 * This API opens the specified audio file and fills the metadata structure with
 * available fields such as title, artist, genre, bitrate and duration.
 * Uses the metadata parsers registered with the given instance.
 *
 * @param[in]  handle    Instance handle.
 * @param[in]  filepath  Full path to the audio file.
 * @param[out] metadata  Pointer to metadata structure to be filled.
 *
 * @return
 *      -  0: Success.
 *      - -1: Error opening file.
 *      - -2: Unsupported file format.
 *      - -3: Invalid parameter.
 *      - -4: File read error.
 */
int bk_audio_player_get_metadata_from_file(bk_audio_player_handle_t handle, const char *filepath, audio_metadata_t *metadata);

/**
 * @brief Register a custom metadata parser implementation.
 *
 * This API allows the user to register an external metadata parser for new
 * audio formats. The provided ops structure must remain valid for the lifetime
 * of the audio player instance.
 *
 * @param[in] handle  Instance handle.
 * @param[in] ops  Pointer to metadata parser operations, see bk_audio_player_metadata_parser_ops_t.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Parser registered successfully.
 *                 - AUDIO_PLAYER_ERR: Invalid parameter or internal list not initialized.
 */
int bk_audio_player_register_metadata_parser(bk_audio_player_handle_t handle, const bk_audio_player_metadata_parser_ops_t *ops);

/**
 * @brief Register a custom audio source implementation.
 *
 * This API allows the user to register an external audio source, such as a new
 * network protocol or storage backend. The provided ops structure must remain
 * valid for the lifetime of the audio player instance.
 *
 * @param[in] handle  Instance handle.
 * @param[in] ops  Pointer to audio source operations, see bk_audio_player_source_ops_t.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Source registered successfully.
 *                 - AUDIO_PLAYER_ERR: Invalid parameter or internal list not initialized.
 */
int bk_audio_player_register_source(bk_audio_player_handle_t handle, const bk_audio_player_source_ops_t *ops);

/**
 * @brief Register a custom audio codec implementation.
 *
 * This API allows the user to register an external decoder implementation for
 * additional audio formats. The provided ops structure must remain valid for
 * the lifetime of the audio player instance.
 *
 * @param[in] handle  Instance handle.
 * @param[in] ops  Pointer to decoder operations, see bk_audio_player_decoder_ops_t.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Codec registered successfully.
 *                 - AUDIO_PLAYER_ERR: Invalid parameter or internal list not initialized.
 */
int bk_audio_player_register_decoder(bk_audio_player_handle_t handle, const bk_audio_player_decoder_ops_t *ops);

/**
 * @brief Register a custom audio sink implementation.
 *
 * This API allows the user to register an external audio sink, such as a new
 * output device or file/stream writer. The provided ops structure must remain
 * valid for the lifetime of the audio player instance.
 *
 * @param[in] handle  Instance handle.
 * @param[in] ops  Pointer to audio sink operations, see bk_audio_player_sink_ops_t.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Sink registered successfully.
 *                 - AUDIO_PLAYER_ERR: Invalid parameter or internal list not initialized.
 */
int bk_audio_player_register_sink(bk_audio_player_handle_t handle, const bk_audio_player_sink_ops_t *ops);

#ifdef __cplusplus
}
#endif

#endif
