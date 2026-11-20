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


#ifndef _FRAME_INFO_LIST_H_
#define _FRAME_INFO_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bsd_queue.h"

typedef struct frame_info
{
    int bitRate;
    int nChans;
    int sampRate;
    int bitsPerSample;
    int outputSamps;
} frame_info_t;

typedef struct frame_info_item
{
    STAILQ_ENTRY(frame_info_item)    next;
    frame_info_t                     frame_info;
    int                              num;
} frame_info_item_t;

typedef STAILQ_HEAD(frame_info_list, frame_info_item) frame_info_list_t;


/**
 * @brief   Malloc memory in player
 *
 * @param[in]  size   memory size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void debug_frame_info_list(frame_info_list_t *frame_info_list);

void frame_info_list_init(frame_info_list_t *frame_info_list);

int frame_info_handler(frame_info_list_t *frame_info_list, frame_info_t frame_info);

frame_info_item_t *get_max_num_frame_info(frame_info_list_t *frame_info_list);

int frame_info_list_deinit(frame_info_list_t *frame_info_list);

#ifdef __cplusplus
}
#endif

#endif /*_FRAME_INFO_LIST_H_*/
