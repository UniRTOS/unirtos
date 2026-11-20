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


#include "music_list.h"
#include "linked_list.h"
#include "play_manager.h"
#include "player_osal.h"

static music_info_t *create_music_info(char *name, char *url)
{
    music_info_t *info = NULL;

    info = player_malloc(sizeof(music_info_t));
    if (!info)
    {
        return NULL;
    }

    info->name = player_strdup(name);
    info->url = player_strdup(url);

    return info;
}

static void free_music_info(void *data)
{
    music_info_t *info = (music_info_t *)data;
    if (info->name)
    {
        player_free(info->name);
        info->name = NULL;
    }
    if (info->url)
    {
        player_free(info->url);
        info->url = NULL;
    }
    player_free(info);
    info = NULL;
}

music_list_t *music_list_new()
{
    list *llist = create_list();
    return (music_list_t *)llist;
}

void music_list_free(music_list_t *_list)
{
    list *llist = (list *)_list;
    free_list(llist, free_music_info);
}

int music_list_clear(music_list_t *_list)
{
    list *llist = (list *)_list;
    empty_list(llist, free_music_info);
    return AUDIO_PLAYER_OK;
}

int music_list_add(music_list_t *_list, char *name, char *url)
{
    list *llist = (list *)_list;
    music_info_t *info;

    info = create_music_info(name, url);
    if (!info)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    push_back(llist, info);
    return AUDIO_PLAYER_OK;
}

static int _compare_music_name(void *pred_data, void *cur_data)
{
    char *name = (char *)pred_data;
    music_info_t *info = (music_info_t *)cur_data;

    if (strcmp(name, info->name) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int music_list_rm_by_name(music_list_t *_list, char *name)
{
    list *llist = (list *)_list;
    int count = remove_if(llist, _compare_music_name, name, free_music_info);

    return count > 0 ? AUDIO_PLAYER_OK : AUDIO_PLAYER_INVALID;
}

static int _compare_music_url(void *pred_data, void *cur_data)
{
    char *url = (char *)pred_data;
    music_info_t *info = (music_info_t *)cur_data;

    if (strcmp(url, info->url) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int music_list_rm_by_url(music_list_t *_list, char *url)
{
    list *llist = (list *)_list;
    int count = remove_if(llist, _compare_music_url, url, free_music_info);

    return count > 0 ? AUDIO_PLAYER_OK : AUDIO_PLAYER_INVALID;
}

int music_list_get_count(music_list_t *_list)
{
    list *llist = (list *)_list;
    return get_size(llist);
}

music_info_t *music_list_get_by_index(music_list_t *_list, int index)
{
    list *llist = (list *)_list;
    music_info_t *info;

    info = (music_info_t *)get_index(llist, index);
    return info;
}

static void dump_music_info(void *cur_data)
{
    music_info_t *info = (music_info_t *)cur_data;
    BK_LOGI(AUDIO_PLAYER_TAG, "name=%s, url=%s\n", info->name, info->url);
}

int music_list_dump(music_list_t *_list)
{
    list *llist = (list *)_list;

    BK_LOGI(AUDIO_PLAYER_TAG, "music list dump : count=%d\n", get_size(llist));
    traverse(llist, dump_music_info);
    return AUDIO_PLAYER_OK;
}

