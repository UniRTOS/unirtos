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


#ifndef __BK_AUDIO_PLAYER_PRIVATE_H__
#define __BK_AUDIO_PLAYER_PRIVATE_H__

#include <components/bk_audio_player/bk_audio_player.h>
#include <components/bk_audio_player/bk_audio_player_types.h>
#include <components/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for internal notify; full type in play_manager.h */
struct bk_audio_player;

/* Private APIs - for internal use only */

/**
 * @brief      Notify player event (internal use).
 *
 * @param[in]      player       Player instance (struct bk_audio_player *).
 * @param[in]      event        Event type, see audio_player_event_type_t.
 * @param[in]      extra_info   Extra information for the event.
 *
 * @return     None.
 */
void bk_audio_player_notify(struct bk_audio_player *player, audio_player_event_type_t event, void *extra_info);

/**
 * @brief      Dump music list for debugging (internal use).
 *
 * @param[in]      handle  Instance handle.
 *
 * @return         Error code.
 *                 - AUDIO_PLAYER_OK: Success.
 *                 - AUDIO_PLAYER_ERR: Failed.
 *                 - AUDIO_PLAYER_NOT_INIT: Player not initialized.
 */
int bk_audio_player_dump_music_list(bk_audio_player_handle_t handle);

/**
 * @brief Metadata helper utilities (internal, not exposed in public API).
 */
typedef enum
{
    AUDIO_METADATA_TITLE = 0,
    AUDIO_METADATA_ARTIST,
    AUDIO_METADATA_ALBUM,
    AUDIO_METADATA_ALBUM_ARTIST,
    AUDIO_METADATA_GENRE,
    AUDIO_METADATA_YEAR,
    AUDIO_METADATA_COMPOSER,
    AUDIO_METADATA_TRACK_NUMBER,
    AUDIO_METADATA_DURATION,
    AUDIO_METADATA_BITRATE,
    AUDIO_METADATA_SAMPLE_RATE,
    AUDIO_METADATA_CHANNELS,
} audio_metadata_field_t;

audio_format_t bk_audio_metadata_get_format(const char *filepath);
void bk_audio_metadata_init(audio_metadata_t *metadata);
int bk_audio_metadata_get_field_string(const audio_metadata_t *metadata,
                                       audio_metadata_field_t field,
                                       char *buffer,
                                       int buf_size);

#ifdef __cplusplus
}
#endif

#endif
