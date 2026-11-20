// Copyright 2020-2021 Beken
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

#pragma once

#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include "components/avdk_utils/avdk_error.h"
#include "components/bk_video_player/bk_video_player_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Buffer pool node
typedef struct video_player_buffer_node_s
{
    video_player_buffer_t buffer;
    struct video_player_buffer_node_s *next;
    bool in_use;
} video_player_buffer_node_t;

// Buffer pool
typedef struct video_player_buffer_pool_s
{
    video_player_buffer_node_t *nodes;
    uint32_t count;
    uint32_t used_count;
    /*
     * Split semaphores for empty/filled buffers.
     * - empty_sem: counts available empty nodes (buffer.data == NULL)
     * - filled_sem: counts available filled nodes (buffer.data != NULL)
     *
     * This avoids blocking forever in producer threads when only filled buffers exist,
     * and makes the pool behave like a proper producer/consumer queue.
     */
    beken_semaphore_t empty_sem;
    beken_semaphore_t filled_sem;
    beken_mutex_t mutex;
    video_player_buffer_node_t *empty_list;
    video_player_buffer_node_t *filled_list;
} video_player_buffer_pool_t;

/**
 * @brief Initialize buffer pool
 *
 * @param pool Buffer pool pointer
 * @param count Number of buffers in the pool
 * @return avdk_err_t AVDK_ERR_OK on success, error code on failure
 */
avdk_err_t buffer_pool_init(video_player_buffer_pool_t *pool, uint32_t count);

/**
 * @brief Deinitialize buffer pool
 *
 * @param pool Buffer pool pointer
 */
void buffer_pool_deinit(video_player_buffer_pool_t *pool);

/**
 * @brief Get an empty buffer from the pool (blocking)
 * Used by file decode thread: get empty buffer, fill with data, then put back with buffer_pool_put_filled
 *
 * @param pool Buffer pool pointer
 * @return video_player_buffer_node_t* Buffer node pointer with buffer->data == NULL, NULL on error
 */
video_player_buffer_node_t *buffer_pool_get_empty(video_player_buffer_pool_t *pool);

/**
 * @brief Try to get an empty buffer from the pool (non-blocking)
 *
 * @param pool Buffer pool pointer
 * @return video_player_buffer_node_t* Empty buffer node pointer, NULL if none available
 */
video_player_buffer_node_t *buffer_pool_try_get_empty(video_player_buffer_pool_t *pool);

/**
 * @brief Put a filled buffer back to the pool
 * Used by file decode thread: put back buffer with data filled
 *
 * @param pool Buffer pool pointer
 * @param node Buffer node pointer (must have buffer->data != NULL)
 */
void buffer_pool_put_filled(video_player_buffer_pool_t *pool, video_player_buffer_node_t *node);

/**
 * @brief Get a filled buffer from the pool (blocking)
 * Used by decode thread: get buffer with data, decode it, then put back with buffer_pool_put_empty
 *
 * @param pool Buffer pool pointer
 * @return video_player_buffer_node_t* Buffer node pointer with buffer->data != NULL, NULL on error
 */
video_player_buffer_node_t *buffer_pool_get_filled(video_player_buffer_pool_t *pool);

/**
 * @brief Try to get a filled buffer from the pool (non-blocking)
 * Used by control logic (stop/play) to drain stale packets without waiting.
 *
 * @param pool Buffer pool pointer
 * @return video_player_buffer_node_t* Buffer node pointer with buffer->data != NULL, NULL if none available
 */
video_player_buffer_node_t *buffer_pool_try_get_filled(video_player_buffer_pool_t *pool);

/**
 * @brief Put an empty buffer back to the pool
 * Used by decode thread: put back buffer after freeing data (buffer->data should be NULL)
 *
 * @param pool Buffer pool pointer
 * @param node Buffer node pointer (should have buffer->data == NULL)
 */
void buffer_pool_put_empty(video_player_buffer_pool_t *pool, video_player_buffer_node_t *node);

#ifdef __cplusplus
}
#endif

