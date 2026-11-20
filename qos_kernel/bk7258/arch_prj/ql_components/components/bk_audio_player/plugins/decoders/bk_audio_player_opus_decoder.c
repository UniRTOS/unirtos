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

#include "bk_audio_player_opus_stream_decoder.h"

static int opus_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    return bk_opus_stream_decoder_open(format, AUDIO_FORMAT_OPUS, param, decoder_pp);
}

static int opus_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    return bk_opus_stream_decoder_get_info(decoder, info);
}

static int opus_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    return bk_opus_stream_decoder_get_data(decoder, buffer, len);
}

static int opus_decoder_close(bk_audio_player_decoder_t *decoder)
{
    return bk_opus_stream_decoder_close(decoder);
}

static int opus_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    return bk_opus_stream_decoder_get_chunk_size(decoder);
}

static int opus_decoder_calc_position(bk_audio_player_decoder_t *decoder, int second)
{
    return bk_opus_stream_calc_position(decoder, second);
}

const bk_audio_player_decoder_ops_t opus_decoder_ops =
{
    .name = "opus",
    .open = opus_decoder_open,
    .get_info = opus_decoder_get_info,
    .get_chunk_size = opus_decoder_get_chunk_size,
    .get_data = opus_decoder_get_data,
    .close = opus_decoder_close,
    .calc_position = opus_decoder_calc_position,
    .is_seek_ready = bk_opus_stream_is_seek_ready,
};

/* Get OPUS decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_opus_decoder_ops(void)
{
    return &opus_decoder_ops;
}
