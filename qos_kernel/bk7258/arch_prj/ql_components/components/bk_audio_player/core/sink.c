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

#include "linked_list.h"
#include <components/bk_audio_player/bk_audio_player_types.h>
#include "sink_api.h"
#include "player_osal.h"


bk_audio_player_sink_t *audio_sink_new(int priv_size)
{
    bk_audio_player_sink_t *sink;

    sink = player_malloc(sizeof(bk_audio_player_sink_t) + priv_size);
    if (!sink)
    {
        return NULL;
    }
    sink->sink_priv = sink + 1;
    return sink;
}

bk_audio_player_sink_t *audio_sink_open(bk_audio_player_handle_t player, audio_sink_type_t sink_type, void *param)
{
    list *llist;
    int count;
    bk_audio_player_sink_ops_t *ops;
    bk_audio_player_sink_t *sink = NULL;
    int i;

    if (!player)
    {
        return NULL;
    }

    llist = audio_sinks_get(player);
    if (!llist)
    {
        return NULL;
    }

    count = get_size(llist);
    for (i = 0; i < count; i++)
    {
        ops = get_index(llist, i);
        if (ops && ops->open(sink_type, param, &sink) == AUDIO_PLAYER_OK)
        {
            sink->ops = ops;
            break;
        }
    }

    if (sink && sink->ops == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, sink is valid, but sink->ops is NULL, %d \n", __func__, __LINE__);
    }

    return sink;
}

int audio_sink_close(bk_audio_player_sink_t *sink)
{
    int ret = AUDIO_PLAYER_OK;

    if (sink && sink->ops->close)
    {
        ret = sink->ops->close(sink);
    }

    if (ret != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, sink->ops->close fail, ret: %d, %d \n", __func__, ret, __LINE__);
    }

    if (sink)
    {
        player_free(sink);
    }

    return ret;
}

int audio_sink_write_data(bk_audio_player_sink_t *sink, char *buffer, int len)
{
    if (!sink || !buffer || !len)
    {
        return AUDIO_PLAYER_ERR;
    }

    return sink->ops->write(sink, buffer, len);
}

int audio_sink_control(bk_audio_player_sink_t *sink, audio_sink_control_t control)
{
    int ret;

    if (sink && sink->ops->control)
    {
        ret = sink->ops->control(sink, control);
    }
    else
    {
        ret = AUDIO_PLAYER_OK;
    }

    return ret;
}

int audio_sink_set_info(bk_audio_player_sink_t *sink, int rate, int bits, int ch)
{
    if (!sink)
    {
        return AUDIO_PLAYER_ERR;
    }

    sink->info.sampRate = rate;
    sink->info.bitsPerSample = bits;
    sink->info.nChans = ch;

    return AUDIO_PLAYER_OK;
}

int audio_sink_set_volume(bk_audio_player_sink_t *sink, int volume)
{
    if (!sink)
    {
        return AUDIO_PLAYER_ERR;
    }

    sink->info.volume = volume;

    return AUDIO_PLAYER_OK;
}

