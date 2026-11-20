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


#ifndef __PLAY_SM__
#define __PLAY_SM__

#include "bk_audio_player_private.h"
#include "music_list.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>


int play_sm_init(bk_audio_player_handle_t player);
void play_sm_deinit(bk_audio_player_handle_t player);

int play_sm_play(bk_audio_player_handle_t player, music_info_t *info);
int play_sm_stop(bk_audio_player_handle_t player);

int play_sm_pause(bk_audio_player_handle_t player);
int play_sm_resume(bk_audio_player_handle_t player);

int play_sm_set_volume(bk_audio_player_handle_t player, int volume);

int play_sm_get_time_pos(bk_audio_player_handle_t player);
double play_sm_get_total_time(bk_audio_player_handle_t player);
int play_sm_seek(bk_audio_player_handle_t player, int miliseconds);

enum CHUNK_RESULT
{
    CHUNK_CONTINUE,
    CHUNK_DONE,
    CHUNK_SOURCE_ERR,
    CHUNK_CODEC_ERR,
    CHUNK_SINK_ERR,
};

int play_sm_chunk(bk_audio_player_handle_t player);

#endif
