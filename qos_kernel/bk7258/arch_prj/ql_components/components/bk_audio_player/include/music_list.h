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


#ifndef __MUSIC_LIST_H__
#define __MUSIC_LIST_H__

#include "bk_audio_player_private.h"

typedef struct music_info_s
{
    char *name;
    char *url;
} music_info_t;

typedef struct music_list_dummy_s music_list_t;

music_list_t *music_list_new();
void music_list_free(music_list_t *list);

int music_list_clear(music_list_t *list);
int music_list_add(music_list_t *list, char *name, char *url);
int music_list_rm_by_name(music_list_t *list, char *name);
int music_list_rm_by_url(music_list_t *list, char *url);

int music_list_get_count(music_list_t *list);
music_info_t *music_list_get_by_index(music_list_t *list, int index);

int music_list_dump(music_list_t *list);

#endif
