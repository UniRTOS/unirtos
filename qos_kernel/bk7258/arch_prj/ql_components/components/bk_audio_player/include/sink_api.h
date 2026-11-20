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


#ifndef __SINK_API_H__
#define __SINK_API_H__

#include <components/bk_audio_player/bk_audio_player_types.h>
#include "play_manager.h"

typedef struct
{
    int nChans;
    int sampRate;
    int bitsPerSample;
    int volume;
} sink_info_t;

/* For AUDIO_SINK_DEVICE: param passed to audio_sink_open is device_sink_param_t * */
typedef struct
{
    audio_info_t *info;
    bk_audio_player_handle_t player;
} device_sink_param_t;

/* Internal layout of bk_audio_player_sink, not exposed in public headers. */
struct bk_audio_player_sink
{
    bk_audio_player_sink_ops_t *ops;
    sink_info_t info;
    void *sink_priv;
};

bk_audio_player_sink_t *audio_sink_new(int priv_size);

bk_audio_player_sink_t *audio_sink_open(bk_audio_player_handle_t player, audio_sink_type_t sink_type, void *param);
int audio_sink_close(bk_audio_player_sink_t *sink);

int audio_sink_write_data(bk_audio_player_sink_t *sink, char *buffer, int len);
int audio_sink_control(bk_audio_player_sink_t *sink, audio_sink_control_t control);
int audio_sink_set_info(bk_audio_player_sink_t *sink, int rate, int bits, int ch);
int audio_sink_set_volume(bk_audio_player_sink_t *sink, int volume);
#endif

