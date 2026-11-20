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

#ifndef __AUDIO_METADATA_PARSER_COMMON_H__
#define __AUDIO_METADATA_PARSER_COMMON_H__

#include "bk_audio_player_private.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t metadata_parse_synchsafe_int(const uint8_t *data);
void metadata_safe_string_copy(char *dst, const char *src, int max_len);
char *metadata_extract_id3v2_text(const uint8_t *data, uint32_t size);
void metadata_fill_title_from_path(const char *filepath, audio_metadata_t *metadata);
int metadata_parse_id3v2(int fd, audio_metadata_t *metadata);

#ifdef __cplusplus
}
#endif

#endif

