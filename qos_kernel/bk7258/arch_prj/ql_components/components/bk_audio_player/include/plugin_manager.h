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


#ifndef __PLUGIN_MANAGER_H__
#define __PLUGIN_MANAGER_H__
#include "player_osal.h"
#include "play_manager.h"
#include "linked_list.h"
#include "source_api.h"
#include "codec_api.h"
#include "sink_api.h"

/* Per-instance plugin init/deinit; plugin lists are stored in struct bk_audio_player */
int plugin_init(bk_audio_player_handle_t player);
void plugin_deinit(bk_audio_player_handle_t player);

list *audio_sources_get(bk_audio_player_handle_t player);
list *audio_codecs_get(bk_audio_player_handle_t player);
list *audio_sinks_get(bk_audio_player_handle_t player);

/* Metadata parser list and find; per-instance */
list *audio_metadata_parser_list(bk_audio_player_handle_t player);
const bk_audio_player_metadata_parser_ops_t *audio_metadata_parser_find(bk_audio_player_handle_t player, audio_format_t format, const char *filepath);
int audio_metadata_parser_init(bk_audio_player_handle_t player);

#endif
