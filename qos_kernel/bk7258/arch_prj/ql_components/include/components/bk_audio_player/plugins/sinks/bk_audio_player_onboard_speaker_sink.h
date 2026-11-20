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

#ifndef __BK_AUDIO_PLAYER_ONBOARD_SPEAKER_SINK_H__
#define __BK_AUDIO_PLAYER_ONBOARD_SPEAKER_SINK_H__

#include <components/bk_audio_player/bk_audio_player_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get onboard speaker sink operations table.
 *
 * This function returns a pointer to the statically defined onboard speaker
 * sink operations structure. The returned pointer can be passed to bk_audio_player_register_sink().
 *
 * @return Pointer to constant bk_audio_player_sink_ops_t for onboard speaker sink.
 */
const bk_audio_player_sink_ops_t *bk_audio_player_get_onboard_speaker_sink_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* __BK_AUDIO_PLAYER_ONBOARD_SPEAKER_SINK_H__ */
