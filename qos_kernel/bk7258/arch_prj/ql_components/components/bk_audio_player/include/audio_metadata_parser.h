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

#ifndef __AUDIO_METADATA_PARSER_H__
#define __AUDIO_METADATA_PARSER_H__

#include <components/bk_audio_player/bk_audio_player_types.h>
#include "linked_list.h"
#include "play_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

int audio_metadata_parser_init(bk_audio_player_handle_t player);
const bk_audio_player_metadata_parser_ops_t *audio_metadata_parser_find(bk_audio_player_handle_t player, audio_format_t format, const char *filepath);
list *audio_metadata_parser_list(bk_audio_player_handle_t player);

#ifdef __cplusplus
}
#endif

#endif

