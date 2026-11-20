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


#ifndef __CODEC_API_H__
#define __CODEC_API_H__

#include <components/bk_audio_player/bk_audio_player_types.h>
#include "play_manager.h"

typedef enum
{
    BITERATE_TYPE_UNKNOW = 0,
    BITERATE_TYPE_CBR,
    BITERATE_TYPE_CBR_INFO,
    BITERATE_TYPE_VBR_XING,
    BITERATE_TYPE_VBR_VBRI,
    BITERATE_TYPE_VBR_ADTS,
} biterate_type_t;

#define DEFAULT_CHUNK_SIZE 180

#define AUDIO_INFO_UNKNOWN      (-1)

/* Internal layout of bk_audio_player_decoder, not exposed in public headers. */
struct bk_audio_player_decoder
{
    bk_audio_player_decoder_ops_t *ops;
    bk_audio_player_source_t *source;

    audio_info_t info;

    void *decoder_priv;
};

int audio_codec_get_type(char *ext_name);
int audio_codec_get_mime_type(char *mime);

bk_audio_player_decoder_t *audio_codec_new(int priv_size);

bk_audio_player_decoder_t *audio_codec_open(bk_audio_player_handle_t player, audio_format_t codec_type, void *param, bk_audio_player_source_t *source);
int audio_codec_close(bk_audio_player_decoder_t *decoder);

int audio_codec_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info);
int audio_codec_get_chunk_size(bk_audio_player_decoder_t *decoder);
int audio_codec_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len);
int audio_codec_calc_position(bk_audio_player_decoder_t *decoder, int second);
int audio_codec_is_seek_ready(bk_audio_player_decoder_t *decoder);
#endif
