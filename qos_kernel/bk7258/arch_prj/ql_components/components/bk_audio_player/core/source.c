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


#include "plugin_manager.h"
#include "player_osal.h"

bk_audio_player_source_t *audio_source_new(int priv_size)
{
    bk_audio_player_source_t *source;

    source = player_malloc(sizeof(bk_audio_player_source_t) + priv_size);
    if (!source)
    {
        return NULL;
    }
    source->source_priv = source + 1;
    return source;
}

bk_audio_player_source_t *audio_source_open_url(bk_audio_player_handle_t player, char *url)
{
    list *llist;
    int count;
    bk_audio_player_source_ops_t *ops;
    bk_audio_player_source_t *source = NULL;
    int i;

    if (!player)
    {
        return NULL;
    }

    llist = audio_sources_get(player);
    if (!llist)
    {
        return NULL;
    }

    count = get_size(llist);
    for (i = 0; i < count; i++)
    {
        ops = get_index(llist, i);
        if (ops && ops->open(url, &source) == AUDIO_PLAYER_OK)
        {
            source->ops = ops;
            break;
        }
    }
    return source;
}

int audio_source_close(bk_audio_player_source_t *source)
{
    int ret;

    if (source && source->ops->close)
    {
        ret = source->ops->close(source);
    }
    else
    {
        ret = AUDIO_PLAYER_OK;
    }

    if (source)
    {
        player_free(source);
    }

    return ret;
}

int audio_source_get_codec_type(bk_audio_player_source_t *source)
{
    if (!source)
    {
        return AUDIO_FORMAT_UNKNOWN;
    }

    return source->ops->get_codec_type(source);
}

uint32_t audio_source_get_total_bytes(bk_audio_player_source_t *source)
{
    if (!source)
    {
        return 0;
    }

    if (source->ops->get_total_bytes)
    {
        return source->ops->get_total_bytes(source);
    }
    else
    {
        return 0;
    }
}

int audio_source_read_data(bk_audio_player_source_t *source, char *buffer, int len)
{
    if (!source || !buffer || !len)
    {
        return AUDIO_PLAYER_ERR;
    }

    return source->ops->read(source, buffer, len);
}

int audio_source_seek(bk_audio_player_source_t *source, int offset, uint32_t whence)
{
    if (!source || !source->ops->seek)
    {
        return AUDIO_PLAYER_ERR;
    }

    return source->ops->seek(source, offset, whence);
}
