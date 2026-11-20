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

#include <os/os.h>
#include "frame_info_list.h"
#include "player_mem.h"


#define TAG "frame_info"
//#define str_begin_with(s, t) (strncasecmp(s, t, strlen(t)) == 0)

void debug_frame_info_list(frame_info_list_t *frame_info_list)
{
    frame_info_item_t *frame_info_item_ptr, *frame_info_item_tmp;

    if (frame_info_list == NULL)
    {
        return;
    }

    BK_LOGI(TAG, "============== frame_info_list =============\n");
    STAILQ_FOREACH_SAFE(frame_info_item_ptr, frame_info_list, next, frame_info_item_tmp)
    {
        if (frame_info_item_ptr != NULL)
        {
            BK_LOGI(TAG, "url_item_ptr:%p, num:%d \n", frame_info_item_ptr, frame_info_item_ptr->num);
            BK_LOGI(TAG, "bitRate:%d, nChans:%d, sampRate:%d, bitsPerSample:%d, outputSamps:%d\n",
                    frame_info_item_ptr->frame_info.bitRate,
                    frame_info_item_ptr->frame_info.nChans,
                    frame_info_item_ptr->frame_info.sampRate,
                    frame_info_item_ptr->frame_info.bitsPerSample,
                    frame_info_item_ptr->frame_info.outputSamps);
            BK_LOGI(TAG, "\n");
        }
    }
}

void frame_info_list_init(frame_info_list_t *frame_info_list)
{
    STAILQ_INIT(frame_info_list);
}

static frame_info_item_t *frame_info_is_exist_in_frame_info_list(frame_info_list_t *frame_info_list, frame_info_t frame_info)
{
    frame_info_item_t *frame_info_item_ptr, *frame_info_item_tmp;
    frame_info_item_t *frame_info_item = NULL;

    if (frame_info_list == NULL)
    {
        BK_LOGE(TAG, "%s, frame_info_list is NULL, %d\n", __func__, __LINE__);
        return NULL;
    }

    STAILQ_FOREACH_SAFE(frame_info_item_ptr, frame_info_list, next, frame_info_item_tmp)
    {
        if (frame_info_item_ptr
            && frame_info_item_ptr->frame_info.bitRate == frame_info.bitRate
            && frame_info_item_ptr->frame_info.nChans == frame_info.nChans
            && frame_info_item_ptr->frame_info.sampRate == frame_info.sampRate
            && frame_info_item_ptr->frame_info.bitsPerSample == frame_info.bitsPerSample
            && frame_info_item_ptr->frame_info.outputSamps == frame_info.outputSamps)
        {
            frame_info_item = frame_info_item_ptr;
            break;
        }
    }

    return frame_info_item;
}

static int frame_info_item_num_add(frame_info_item_t *frame_info_item)
{
    if (frame_info_item == NULL)
    {
        BK_LOGE(TAG, "%s, frame_info_item is NULL, %d\n", __func__, __LINE__);
        return -1;
    }

    frame_info_item->num = frame_info_item->num + 1;

    return 0;
}

static int frame_info_list_push(frame_info_list_t *frame_info_list, frame_info_t frame_info)
{
    if (frame_info_list == NULL)
    {
        BK_LOGE(TAG, "%s, frame_info_list is NULL, %d\n", __func__, __LINE__);
        return -1;
    }

    frame_info_item_t *frame_info_item = player_malloc(sizeof(frame_info_item_t));
    if (frame_info_item == NULL)
    {
        BK_LOGE(TAG, "%s, player_malloc frame_info_item fail, %d\n", __func__, __LINE__);
        return -1;
    }

    frame_info_item->num = 1;
    frame_info_item->frame_info.bitRate = frame_info.bitRate;
    frame_info_item->frame_info.nChans = frame_info.nChans;
    frame_info_item->frame_info.sampRate = frame_info.sampRate;
    frame_info_item->frame_info.bitsPerSample = frame_info.bitsPerSample;
    frame_info_item->frame_info.outputSamps = frame_info.outputSamps;

    STAILQ_INSERT_TAIL(frame_info_list, frame_info_item, next);

    return 0;
}

int frame_info_handler(frame_info_list_t *frame_info_list, frame_info_t frame_info)
{
    int ret = 0;

    if (frame_info_list == NULL)
    {
        BK_LOGE(TAG, "%s, frame_info_list is NULL, %d\n", __func__, __LINE__);
        return -1;
    }

    frame_info_item_t *frame_info_item = frame_info_is_exist_in_frame_info_list(frame_info_list, frame_info);
    if (frame_info_item)
    {
        ret = frame_info_item_num_add(frame_info_item);
        if (ret != 0)
        {
            BK_LOGE(TAG, "%s, frame_info_item_num_add fail, %d\n", __func__, __LINE__);
        }
    }
    else
    {
        ret = frame_info_list_push(frame_info_list, frame_info);
        if (ret != 0)
        {
            BK_LOGE(TAG, "%s, frame_info_list_push fail, %d\n", __func__, __LINE__);
        }
    }

    return ret;
}

/* Get the farme info item from frame_info_list that it have the max number */
frame_info_item_t *get_max_num_frame_info(frame_info_list_t *frame_info_list)
{
    if (frame_info_list == NULL)
    {
        BK_LOGE(TAG, "%s, frame_info_list is NULL, %d\n", __func__, __LINE__);
        return NULL;
    }

    frame_info_item_t *frame_info_item_ptr, *frame_info_item_tmp;
    frame_info_item_t *frame_info_item = NULL;

    STAILQ_FOREACH_SAFE(frame_info_item_ptr, frame_info_list, next, frame_info_item_tmp)
    {
        if (frame_info_item_ptr)
        {
            if (frame_info_item == NULL)
            {
                frame_info_item = frame_info_item_ptr;
            }
            else
            {
                if (frame_info_item_ptr->num > frame_info_item->num)
                {
                    frame_info_item = frame_info_item_ptr;
                }
            }
        }
    }

    return frame_info_item;
}

/* free all item in frame_info_list */
int frame_info_list_deinit(frame_info_list_t *frame_info_list)
{
    if (frame_info_list == NULL)
    {
        BK_LOGE(TAG, "%s, frame_info_list is NULL, %d\n", __func__, __LINE__);
        return -1;
    }

    frame_info_item_t *frame_info_item_ptr, *frame_info_item_tmp;

    STAILQ_FOREACH_SAFE(frame_info_item_ptr, frame_info_list, next, frame_info_item_tmp)
    {
        STAILQ_REMOVE(frame_info_list, frame_info_item_ptr, frame_info_item, next);
        if (frame_info_item_ptr)
        {
            player_free(frame_info_item_ptr);
            frame_info_item_ptr = NULL;
        }
    }

    return 0;
}

