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
#include <os/mem.h>
#include <driver/int.h>
#include <components/log.h>
#include "psram_mem_slab.h"
#include "avdk_crc.h"
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
#endif

#include "mlist.h"

#define TAG "frame_buffer"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define FB_ALLOCATED_PATTERN      (0x8338)
#define FB_FREE_PATTERN           (0xF00F)
#define FB_OPTIMIZER_ENABLE       (1)

#define ALIGN_BITS  (5)

frame_buffer_t *frame_buffer_display_malloc(uint32_t size)
{
    frame_buffer_t *frame = bk_psram_frame_buffer_malloc(PSRAM_HEAP_YUV, size + sizeof(frame_buffer_t) + (1 << ALIGN_BITS));

    if (frame == NULL)
    {
        //dump the heap blocks, default close for performance reason
        //bk_psram_frame_buffer_dump_blocks(PSRAM_HEAP_YUV);
        return NULL;
    }

    os_memset(frame, 0, sizeof(frame_buffer_t));
    frame->frame = (uint8_t *)((((uint32_t)(frame + 1) >> ALIGN_BITS) + 1) << ALIGN_BITS);
    frame->size = size;
    frame->flag = FB_ALLOCATED_PATTERN;
    frame->frame_crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);

    return frame;
}

void frame_buffer_display_free(frame_buffer_t *frame)
{
    if (frame == NULL)
    {
        LOGE("%s %d buffer is NULL\n", __func__, __LINE__);
        return;
    }
    if (frame->flag == FB_ALLOCATED_PATTERN)
    {
        uint8_t crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);
        if (crc == frame->frame_crc)
        {
            frame->flag = FB_FREE_PATTERN;
            bk_psram_frame_buffer_free(frame);
        }
        else
        {
            LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
        }
    }
    else
    {
        LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
    }
}

frame_buffer_t *frame_buffer_encode_malloc(uint32_t size)
{
    frame_buffer_t *frame = bk_psram_frame_buffer_malloc(PSRAM_HEAP_ENCODE, size + sizeof(frame_buffer_t) + (1 << ALIGN_BITS));

    if (frame == NULL)
    {
        return NULL;
    }

    os_memset(frame, 0, sizeof(frame_buffer_t));
    frame->frame = (uint8_t *)((((uint32_t)(frame + 1) >> ALIGN_BITS) + 1) << ALIGN_BITS);
    frame->size = size;
    frame->flag = FB_ALLOCATED_PATTERN;
    frame->frame_crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);

    return frame;
}

void frame_buffer_encode_free(frame_buffer_t *frame)
{
    if (frame == NULL)
    {
        LOGE("%s %d buffer is NULL\n", __func__, __LINE__);
        return;
    }

    if (frame->flag == FB_ALLOCATED_PATTERN)
    {
        uint8_t crc = hnd_crc8((uint8_t *)frame, 6, 0xFF);
        if (crc == frame->frame_crc)
        {
            frame->flag = FB_FREE_PATTERN;
            bk_psram_frame_buffer_free(frame);
        }
        else
        {
            LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
        }
    }
    else
    {
        LOGE("%s %d buffer %p may be released twice %x %p %d\n", __func__, __LINE__, frame, frame->flag, frame->frame, frame->size);
    }
}
