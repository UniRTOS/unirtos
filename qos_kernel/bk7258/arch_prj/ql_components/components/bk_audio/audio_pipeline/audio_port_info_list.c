// Copyright 2025-2026 Beken
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

#include <stdio.h>
#include <string.h>
#include <components/bk_audio/audio_pipeline/audio_port_info_list.h>
#include <components/bk_audio/audio_pipeline/audio_mem.h>
#include <components/bk_audio/audio_pipeline/audio_error.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>

#define TAG  "AUDIO_PORT_INFO_LIST"

bk_err_t audio_port_info_list_init(input_audio_port_info_list_t *input_port_list)
{
    /* Check input parameter */
    if (input_port_list == NULL)
    {
        BK_LOGE(TAG, "input_port_list is NULL\n");
        return BK_FAIL;
    }

    /* Initialize the list */
    STAILQ_INIT(input_port_list);

    return BK_OK;
}

void audio_port_info_list_debug_print(input_audio_port_info_list_t *input_port_list, const char *func, int line)
{
    if (input_port_list == NULL)
    {
        BK_LOGD(TAG, "input_port_list is NULL\n");
        return;
    }

    input_audio_port_info_item_t *audio_port_info_item = NULL;

    BK_LOGD(TAG, "----------------- [%s] %d, input_port_list -----------------\n", func, line);
    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item)
        {
            BK_LOGD(TAG, "port_id: %d, priority: %d, port: %p, chl_num: %d, sample_rate: %d, dig_gain: %d, ana_gain: %d, bits: %d\n",
                    audio_port_info_item->port_info.port_id,
                    audio_port_info_item->port_info.priority,
                    audio_port_info_item->port_info.port,
                    audio_port_info_item->port_info.chl_num,
                    audio_port_info_item->port_info.sample_rate,
                    audio_port_info_item->port_info.dig_gain,
                    audio_port_info_item->port_info.ana_gain,
                    audio_port_info_item->port_info.bits);
        }
    }
    BK_LOGD(TAG, "\n");
}

int8_t audio_port_info_list_get_valid_port_id(input_audio_port_info_list_t *input_port_list)
{
    int8_t valid_port_id = -1;
    input_audio_port_info_item_t *audio_port_info_item = NULL;

    /* Check input parameter */
    if (input_port_list == NULL)
    {
        BK_LOGE(TAG, "input_port_list is NULL\n");
        return -1;
    }

    /* Traverse the list according to port priority to obtain the high-priority port id with valid data */
    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.port)
        {
            uint32_t filled_size = audio_port_get_filled_size(audio_port_info_item->port_info.port);
            if (filled_size > 0)
            {
                //BK_LOGD(TAG, "port_id: %d, filled_size: %d\n", audio_port_info_item->port_info.port_id, filled_size);
                valid_port_id = audio_port_info_item->port_info.port_id;
                break;
            }
        }
    }

    return valid_port_id;
}

audio_port_info_t *audio_port_info_list_get_by_port_id(input_audio_port_info_list_t *input_port_list, uint8_t input_port_id)
{
    audio_port_info_t *port_info = NULL;
    input_audio_port_info_item_t *audio_port_info_item = NULL;

    /* Check input parameter */
    if (input_port_list == NULL)
    {
        BK_LOGE(TAG, "input_port_list is NULL\n");
        return NULL;
    }

    /* Search for the port info by port id */
    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.port_id == input_port_id)
        {
            port_info = &audio_port_info_item->port_info;
            break;
        }
    }

    return port_info;
}

bk_err_t audio_port_info_list_add(input_audio_port_info_list_t *input_port_list, audio_port_info_t *port_info)
{
    input_audio_port_info_item_t *audio_port_info_item = NULL;
    input_audio_port_info_item_t *prev_audio_port_info_item = NULL;

    /* Check input parameters */
    if (input_port_list == NULL || port_info == NULL)
    {
        BK_LOGE(TAG, "input_port_list or port_info is NULL\n");
        return BK_FAIL;
    }

    /* Find the insertion position according to priority, the list is sorted in descending order of priority */
    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.priority >= port_info->priority)
        {
            prev_audio_port_info_item = audio_port_info_item;
        }
    }

    /* Allocate memory for new item */
    input_audio_port_info_item_t *new_audio_port_info_item = audio_calloc(1, sizeof(input_audio_port_info_item_t));
    AUDIO_MEM_CHECK(TAG, new_audio_port_info_item, return BK_FAIL);

    /* Copy port info to new item */
    os_memcpy(&new_audio_port_info_item->port_info, port_info, sizeof(audio_port_info_t));

    /* Insert new item into list */
    if (prev_audio_port_info_item)
    {
        STAILQ_INSERT_AFTER(input_port_list, prev_audio_port_info_item, new_audio_port_info_item, next);
    }
    else
    {
        STAILQ_INSERT_HEAD(input_port_list, new_audio_port_info_item, next);
    }

    return BK_OK;
}

bk_err_t audio_port_info_list_update(input_audio_port_info_list_t *input_port_list, audio_port_info_t *port_info)
{
    input_audio_port_info_item_t *audio_port_info_item = NULL;
    input_audio_port_info_item_t *tmp_item = NULL;
    input_audio_port_info_item_t *prev_audio_port_info_item = NULL;
    input_audio_port_info_item_t *audio_port_info_item_bk = NULL;

    /* Check input parameters */
    if (input_port_list == NULL || port_info == NULL)
    {
        BK_LOGE(TAG, "input_port_list or port_info is NULL\n");
        return BK_FAIL;
    }

    /* Find and remove the old item with the same port id */
    STAILQ_FOREACH_SAFE(audio_port_info_item, input_port_list, next, tmp_item)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.port_id == port_info->port_id)
        {
            STAILQ_REMOVE(input_port_list, audio_port_info_item, input_audio_port_info_item, next);
            audio_port_info_item_bk = audio_port_info_item;
            break;
        }
    }

    /* If port_info->port is NULL, remove this port from list and free memory */
    if (port_info->port == NULL)
    {
        audio_free(audio_port_info_item_bk);
        return BK_OK;
    }

    /* If no old item found, return fail */
    if (audio_port_info_item_bk == NULL)
    {
        BK_LOGE(TAG, "port_id %d not found in list\n", port_info->port_id);
        return BK_FAIL;
    }

    /* Update port info */
    os_memcpy(&audio_port_info_item_bk->port_info, port_info, sizeof(audio_port_info_t));

    /* Find the insertion position according to priority */
    audio_port_info_item = NULL;
    STAILQ_FOREACH(audio_port_info_item, input_port_list, next)
    {
        if (audio_port_info_item && audio_port_info_item->port_info.priority >= port_info->priority)
        {
            prev_audio_port_info_item = audio_port_info_item;
        }
    }

    /* Insert updated item back into list */
    if (prev_audio_port_info_item)
    {
        STAILQ_INSERT_AFTER(input_port_list, prev_audio_port_info_item, audio_port_info_item_bk, next);
    }
    else
    {
        STAILQ_INSERT_HEAD(input_port_list, audio_port_info_item_bk, next);
    }

    return BK_OK;
}

bk_err_t audio_port_info_list_clear(input_audio_port_info_list_t *input_port_list)
{
    input_audio_port_info_item_t *audio_port_info_item = NULL;
    input_audio_port_info_item_t *tmp_item = NULL;

    /* Check input parameter */
    if (input_port_list == NULL)
    {
        BK_LOGE(TAG, "input_port_list is NULL\n");
        return BK_FAIL;
    }

    /* Traverse and free all nodes in the list */
    STAILQ_FOREACH_SAFE(audio_port_info_item, input_port_list, next, tmp_item)
    {
        if (audio_port_info_item)
        {
            STAILQ_REMOVE(input_port_list, audio_port_info_item, input_audio_port_info_item, next);
            audio_free(audio_port_info_item);
        }
    }

    return BK_OK;
}

