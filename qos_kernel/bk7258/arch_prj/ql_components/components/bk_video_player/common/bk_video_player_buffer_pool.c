#include "os/os.h"
#include "os/mem.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_player_buffer_pool.h"

#define TAG "buffer_pool"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static void buffer_pool_used_inc(video_player_buffer_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    if (pool->used_count < pool->count)
    {
        pool->used_count++;
    }
    else
    {
        // Keep it best-effort: do not wrap-around.
        LOGW("%s: used_count overflow (used=%u,count=%u)\n", __func__, pool->used_count, pool->count);
    }
}

static void buffer_pool_used_dec(video_player_buffer_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    if (pool->used_count > 0)
    {
        pool->used_count--;
    }
    else
    {
        // Keep it best-effort: do not wrap-around.
        LOGW("%s: used_count underflow\n", __func__);
    }
}

static void buffer_pool_list_push(video_player_buffer_node_t **head, video_player_buffer_node_t *node)
{
    if (head == NULL || node == NULL)
    {
        return;
    }

    node->next = *head;
    *head = node;
}

static video_player_buffer_node_t *buffer_pool_list_pop(video_player_buffer_node_t **head)
{
    if (head == NULL || *head == NULL)
    {
        return NULL;
    }

    video_player_buffer_node_t *n = *head;
    *head = n->next;
    n->next = NULL;
    return n;
}

static video_player_buffer_node_t *buffer_pool_list_pop_min_pts(video_player_buffer_node_t **head)
{
    if (head == NULL || *head == NULL)
    {
        return NULL;
    }

    video_player_buffer_node_t *prev_min = NULL;
    video_player_buffer_node_t *min_node = *head;
    uint64_t min_pts = min_node->buffer.pts;

    video_player_buffer_node_t *prev = *head;
    video_player_buffer_node_t *cur = (*head)->next;
    while (cur != NULL)
    {
        if (cur->buffer.pts < min_pts)
        {
            min_pts = cur->buffer.pts;
            prev_min = prev;
            min_node = cur;
        }
        prev = cur;
        cur = cur->next;
    }

    if (prev_min != NULL)
    {
        prev_min->next = min_node->next;
    }
    else
    {
        *head = min_node->next;
    }
    min_node->next = NULL;
    return min_node;
}

avdk_err_t buffer_pool_init(video_player_buffer_pool_t *pool, uint32_t count)
{
    if (pool == NULL || count == 0)
    {
        return AVDK_ERR_INVAL;
    }

    pool->nodes = os_malloc(sizeof(video_player_buffer_node_t) * count);
    if (pool->nodes == NULL)
    {
        LOGE("%s: Failed to allocate buffer pool nodes\n", __func__);
        return AVDK_ERR_NOMEM;
    }

    os_memset(pool->nodes, 0, sizeof(video_player_buffer_node_t) * count);
    pool->count = count;
    pool->used_count = 0;
    pool->empty_list = NULL;
    pool->filled_list = NULL;

    /*
     * rtos_init_semaphore() takes max_count and initializes current count to 0.
     * For buffer pool:
     * - empty_sem should start from 'count' (all nodes are empty at init)
     * - filled_sem should start from 0 (no filled node at init)
     */
    if (rtos_init_semaphore_ex(&pool->empty_sem, count, 1) != BK_OK)
    {
        LOGE("%s: Failed to init empty semaphore\n", __func__);
        os_free(pool->nodes);
        pool->nodes = NULL;
        return AVDK_ERR_IO;
    }

    if (rtos_init_semaphore_ex(&pool->filled_sem, count, 0) != BK_OK)
    {
        LOGE("%s: Failed to init filled semaphore\n", __func__);
        rtos_deinit_semaphore(&pool->empty_sem);
        os_free(pool->nodes);
        pool->nodes = NULL;
        return AVDK_ERR_IO;
    }

    if (rtos_init_mutex(&pool->mutex) != BK_OK)
    {
        LOGE("%s: Failed to init mutex\n", __func__);
        rtos_deinit_semaphore(&pool->filled_sem);
        rtos_deinit_semaphore(&pool->empty_sem);
        os_free(pool->nodes);
        pool->nodes = NULL;
        return AVDK_ERR_IO;
    }

    // Initialize all buffers as empty and available
    rtos_lock_mutex(&pool->mutex);
    for (uint32_t i = 0; i < count; i++)
    {
        pool->nodes[i].in_use = false;
        pool->nodes[i].buffer.data = NULL;
        pool->nodes[i].buffer.length = 0;
        pool->nodes[i].buffer.pts = 0;
        pool->nodes[i].buffer.user_data = NULL;
        buffer_pool_list_push(&pool->empty_list, &pool->nodes[i]);
    }
    rtos_unlock_mutex(&pool->mutex);

    return AVDK_ERR_OK;
}

void buffer_pool_deinit(video_player_buffer_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    if (pool->nodes != NULL)
    {
        os_free(pool->nodes);
        pool->nodes = NULL;
    }

    rtos_deinit_semaphore(&pool->filled_sem);
    rtos_deinit_semaphore(&pool->empty_sem);
    rtos_deinit_mutex(&pool->mutex);
    pool->count = 0;
    pool->used_count = 0;
    pool->empty_list = NULL;
    pool->filled_list = NULL;
}

// Get an empty buffer from the pool (non-blocking)
// Producer side (container parse): get empty node, fill buffer->data, then put back with buffer_pool_put_filled
video_player_buffer_node_t *buffer_pool_get_empty(video_player_buffer_pool_t *pool)
{
    if (pool == NULL || pool->nodes == NULL)
    {
        return NULL;
    }

    if (rtos_get_semaphore(&pool->empty_sem, BEKEN_WAIT_FOREVER) != BK_OK)
    {
        return NULL;
    }

    rtos_lock_mutex(&pool->mutex);
    video_player_buffer_node_t *node = buffer_pool_list_pop(&pool->empty_list);
    if (node == NULL)
    {
        rtos_unlock_mutex(&pool->mutex);
        // This should never happen if semaphore/list are consistent.
        // Do NOT restore the semaphore token here; doing so would keep the inconsistency persistent
        // and may cause a busy loop that repeatedly acquires the semaphore but cannot pop a node.
        LOGW("%s: empty_sem acquired but empty_list is NULL (count=%u,used=%u)\n",
             __func__, pool->count, pool->used_count);
        return NULL;
    }

    node->in_use = true;
    buffer_pool_used_inc(pool);
    rtos_unlock_mutex(&pool->mutex);
    return node;
}

// Put a filled buffer back to the pool
// Producer side: put back node with buffer->data != NULL for consumer to decode
void buffer_pool_put_filled(video_player_buffer_pool_t *pool, video_player_buffer_node_t *node)
{
    if (pool == NULL || node == NULL)
    {
        return;
    }

    rtos_lock_mutex(&pool->mutex);

    if (!node->in_use)
    {
        LOGW("%s: node not in_use when put_filled, ignoring\n", __func__);
        rtos_unlock_mutex(&pool->mutex);
        return;
    }

    // Verify that buffer has data
    if (node->buffer.data == NULL)
    {
        LOGW("%s: putting back filled node with NULL data\n", __func__);
    }

    node->in_use = false;
    buffer_pool_used_dec(pool);
    buffer_pool_list_push(&pool->filled_list, node);
    rtos_unlock_mutex(&pool->mutex);
    rtos_set_semaphore(&pool->filled_sem);
}

// Get a filled buffer from the pool (blocking)
// Used by decode thread: get buffer with data, decode it, then put back with buffer_pool_put_empty
// Returns buffer with the smallest PTS to maintain playback order
video_player_buffer_node_t *buffer_pool_get_filled(video_player_buffer_pool_t *pool)
{
    if (pool == NULL || pool->nodes == NULL)
    {
        return NULL;
    }

    // Loop until we get a filled buffer or timeout/error
    while (1)
    {
        if (rtos_get_semaphore(&pool->filled_sem, 50) != BK_OK)
        {
            return NULL;
        }

        rtos_lock_mutex(&pool->mutex);
        video_player_buffer_node_t *node = buffer_pool_list_pop_min_pts(&pool->filled_list);
        if (node == NULL)
        {
            rtos_unlock_mutex(&pool->mutex);
            // This should never happen if semaphore/list are consistent.
            // Do NOT restore the semaphore token here; doing so would keep the inconsistency persistent
            // and may cause a busy loop that repeatedly acquires the semaphore but cannot pop a node.
            LOGW("%s: filled_sem acquired but filled_list is NULL (count=%u,used=%u)\n",
                 __func__, pool->count, pool->used_count);
            continue;
        }

        node->in_use = true;
        buffer_pool_used_inc(pool);
        rtos_unlock_mutex(&pool->mutex);
        return node;
    }
}

video_player_buffer_node_t *buffer_pool_try_get_filled(video_player_buffer_pool_t *pool)
{
    if (pool == NULL || pool->nodes == NULL)
    {
        return NULL;
    }

    if (rtos_get_semaphore(&pool->filled_sem, 0) != BK_OK)
    {
        return NULL;
    }

    rtos_lock_mutex(&pool->mutex);
    video_player_buffer_node_t *node = buffer_pool_list_pop_min_pts(&pool->filled_list);
    if (node == NULL)
    {
        rtos_unlock_mutex(&pool->mutex);
        // See buffer_pool_get_filled() for rationale: do not restore token here.
        LOGW("%s: filled_sem acquired but filled_list is NULL (count=%u,used=%u)\n",
             __func__, pool->count, pool->used_count);
        return NULL;
    }

    node->in_use = true;
    buffer_pool_used_inc(pool);
    rtos_unlock_mutex(&pool->mutex);
    return node;
}

// Put an empty buffer back to the pool
// Used by decode thread: put back buffer after freeing data (buffer->data should be NULL)
void buffer_pool_put_empty(video_player_buffer_pool_t *pool, video_player_buffer_node_t *node)
{
    if (pool == NULL || node == NULL)
    {
        return;
    }

    rtos_lock_mutex(&pool->mutex);

    if (!node->in_use)
    {
        LOGW("%s: node not in_use when put_empty, ignoring\n", __func__);
        rtos_unlock_mutex(&pool->mutex);
        return;
    }

    // Clear buffer data to make it an empty buffer
    node->buffer.data = NULL;
    node->buffer.length = 0;
    node->buffer.pts = 0;
    node->buffer.user_data = NULL;
    node->in_use = false;
    buffer_pool_used_dec(pool);
    buffer_pool_list_push(&pool->empty_list, node);
    rtos_unlock_mutex(&pool->mutex);
    rtos_set_semaphore(&pool->empty_sem);
}

