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

#ifndef __BK_AUDIO_PLAYER_VORBIS_STREAM_DECODER_H__
#define __BK_AUDIO_PLAYER_VORBIS_STREAM_DECODER_H__

//#include "plugin_manager.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

int bk_vorbis_stream_decoder_open(audio_format_t format,
                                audio_format_t expected_type,
                                void *param,
                                bk_audio_player_decoder_t **decoder_pp);

int bk_vorbis_stream_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info);
int bk_vorbis_stream_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len);
int bk_vorbis_stream_decoder_close(bk_audio_player_decoder_t *decoder);
int bk_vorbis_stream_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder);
int bk_vorbis_stream_calc_position(bk_audio_player_decoder_t *decoder, int second);
int bk_vorbis_stream_is_seek_ready(bk_audio_player_decoder_t *decoder);

#endif /* __BK_AUDIO_PLAYER_VORBIS_STREAM_DECODER_H__ */
