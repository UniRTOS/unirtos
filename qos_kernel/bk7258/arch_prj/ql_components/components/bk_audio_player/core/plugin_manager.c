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
#include "audio_metadata_parser.h"

static void free_dummy(void *data)
{
    (void)data;
}

int plugin_init(bk_audio_player_handle_t player)
{
    int ret;

    if (!player)
    {
        return AUDIO_PLAYER_INVALID;
    }

    player->source_list = create_list();
    if (!player->source_list)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    player->codec_list = create_list();
    if (!player->codec_list)
    {
        free_list(player->source_list, NULL);
        player->source_list = NULL;
        return AUDIO_PLAYER_NO_MEM;
    }

    player->sink_list = create_list();
    if (!player->sink_list)
    {
        free_list(player->source_list, NULL);
        player->source_list = NULL;
        free_list(player->codec_list, NULL);
        player->codec_list = NULL;
        return AUDIO_PLAYER_NO_MEM;
    }

    ret = audio_metadata_parser_init(player);
    if (ret != AUDIO_PLAYER_OK)
    {
        free_list(player->source_list, NULL);
        player->source_list = NULL;
        free_list(player->codec_list, NULL);
        player->codec_list = NULL;
        free_list(player->sink_list, NULL);
        player->sink_list = NULL;
        return ret;
    }

    return AUDIO_PLAYER_OK;
}

void plugin_deinit(bk_audio_player_handle_t player)
{
    if (!player)
    {
        return;
    }

    if (player->source_list)
    {
        free_list(player->source_list, free_dummy);
        player->source_list = NULL;
    }
    if (player->codec_list)
    {
        free_list(player->codec_list, free_dummy);
        player->codec_list = NULL;
    }
    if (player->sink_list)
    {
        free_list(player->sink_list, free_dummy);
        player->sink_list = NULL;
    }
    if (player->parser_list)
    {
        free_list(player->parser_list, free_dummy);
        player->parser_list = NULL;
        player->parser_initialized = 0;
    }
}

int bk_audio_player_register_source(bk_audio_player_handle_t handle, const bk_audio_player_source_ops_t *ops)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t )handle;

    if (!player || !player->source_list || !ops || !ops->open || !ops->get_codec_type || !ops->read)
    {
        return AUDIO_PLAYER_ERR;
    }

    push_back(player->source_list, (void *)ops);
    return AUDIO_PLAYER_OK;
}

list *audio_sources_get(bk_audio_player_handle_t player)
{
    return player ? player->source_list : NULL;
}

int bk_audio_player_register_decoder(bk_audio_player_handle_t handle, const bk_audio_player_decoder_ops_t *ops)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t )handle;

    if (!player || !player->codec_list || !ops || !ops->open || !ops->get_info || !ops->get_data)
    {
        return AUDIO_PLAYER_ERR;
    }

    push_back(player->codec_list, (void *)ops);
    return AUDIO_PLAYER_OK;
}

list *audio_codecs_get(bk_audio_player_handle_t player)
{
    return player ? player->codec_list : NULL;
}

int bk_audio_player_register_sink(bk_audio_player_handle_t handle, const bk_audio_player_sink_ops_t *ops)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t )handle;

    if (!player || !player->sink_list || !ops || !ops->open || !ops->write)
    {
        return AUDIO_PLAYER_ERR;
    }

    push_back(player->sink_list, (void *)ops);
    return AUDIO_PLAYER_OK;
}

list *audio_sinks_get(bk_audio_player_handle_t player)
{
    return player ? player->sink_list : NULL;
}

int bk_audio_player_register_metadata_parser(bk_audio_player_handle_t handle, const bk_audio_player_metadata_parser_ops_t *ops)
{
    int i;
    const bk_audio_player_metadata_parser_ops_t *registered;
    bk_audio_player_handle_t player = (bk_audio_player_handle_t )handle;

    if (!player || !player->parser_list || ops == NULL || ops->parse == NULL)
    {
        return AUDIO_PLAYER_ERR;
    }

    for (i = 0; i < get_size(player->parser_list); i++)
    {
        registered = (const bk_audio_player_metadata_parser_ops_t *)get_index(player->parser_list, i);
        if (registered == NULL)
        {
            continue;
        }

        if (registered->format == ops->format)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "metadata parser for format %d already registered, skip\n", ops->format);
            return AUDIO_PLAYER_ERR;
        }
    }

    push_back(player->parser_list, (void *)ops);
    return AUDIO_PLAYER_OK;
}

int audio_metadata_parser_init(bk_audio_player_handle_t player)
{
    if (!player)
    {
        return AUDIO_PLAYER_INVALID;
    }

    if (player->parser_initialized)
    {
        return AUDIO_PLAYER_OK;
    }

    player->parser_list = create_list();
    if (!player->parser_list)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    player->parser_initialized = 1;
    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t *audio_metadata_parser_find(bk_audio_player_handle_t player, audio_format_t format, const char *filepath)
{
    int i;
    const bk_audio_player_metadata_parser_ops_t *ops;

    if (!player)
    {
        return NULL;
    }

    if (!player->parser_initialized)
    {
        if (audio_metadata_parser_init(player) != AUDIO_PLAYER_OK)
        {
            return NULL;
        }
    }

    if (!player->parser_list)
    {
        return NULL;
    }

    for (i = 0; i < get_size(player->parser_list); i++)
    {
        ops = (const bk_audio_player_metadata_parser_ops_t *)get_index(player->parser_list, i);
        if (!ops)
        {
            continue;
        }

        if (ops->format != format)
        {
            continue;
        }

        if (ops->probe && ops->probe(filepath) != 0)
        {
            continue;
        }

        return ops;
    }

    return NULL;
}

list *audio_metadata_parser_list(bk_audio_player_handle_t player)
{
    return player ? player->parser_list : NULL;
}
