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


#ifndef __SOURCE_API_H__
#define __SOURCE_API_H__

#include <components/bk_audio_player/bk_audio_player_types.h>
#include "play_manager.h"

/* Internal layout of bk_audio_player_source, not exposed in public headers. */
struct bk_audio_player_source
{
    bk_audio_player_source_ops_t *ops;
    void *source_priv;
};

bk_audio_player_source_t *audio_source_new(int priv_size);

bk_audio_player_source_t *audio_source_open_url(bk_audio_player_handle_t player, char *url);
int audio_source_close(bk_audio_player_source_t *source);

int audio_source_get_codec_type(bk_audio_player_source_t *source);
uint32_t audio_source_get_total_bytes(bk_audio_player_source_t *source);
int audio_source_read_data(bk_audio_player_source_t *source, char *buffer, int len);
int audio_source_seek(bk_audio_player_source_t *source, int offset, uint32_t whence);

#endif
