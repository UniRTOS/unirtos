#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <bk_vfs.h>
#include <bk_posix.h>
#include <errno.h>

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "bk_video_player_file_list.h"

#define TAG "file_list"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

avdk_err_t file_list_init(video_player_file_list_t *list)
{
    if (list == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->current = NULL;

    if (rtos_init_mutex(&list->mutex) != BK_OK)
    {
        LOGE("%s: Failed to init mutex\n", __func__);
        return AVDK_ERR_IO;
    }

    return AVDK_ERR_OK;
}

void file_list_deinit(video_player_file_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    file_list_clear(list);
    rtos_deinit_mutex(&list->mutex);
}

avdk_err_t file_list_add(video_player_file_list_t *list, const char *file_path)
{
    if (list == NULL || file_path == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&list->mutex);

    // Check if file already exists in list
    video_player_file_node_t *node = list->head;
    while (node != NULL)
    {
        if (os_strcmp(node->file_path, file_path) == 0)
        {
            rtos_unlock_mutex(&list->mutex);
            return AVDK_ERR_OK; // Already exists
        }
        node = node->next;
    }

    // Create new node
    video_player_file_node_t *new_node = os_malloc(sizeof(video_player_file_node_t));
    if (new_node == NULL)
    {
        rtos_unlock_mutex(&list->mutex);
        return AVDK_ERR_NOMEM;
    }

    // Use os_strdup to keep string allocation consistent.
    new_node->file_path = os_strdup(file_path);
    if (new_node->file_path == NULL)
    {
        os_free(new_node);
        rtos_unlock_mutex(&list->mutex);
        return AVDK_ERR_NOMEM;
    }

    new_node->next = NULL;
    new_node->prev = NULL;

    // Add to tail
    if (list->tail == NULL)
    {
        list->head = new_node;
        list->tail = new_node;
    }
    else
    {
        new_node->prev = list->tail;
        list->tail->next = new_node;
        list->tail = new_node;
    }

    rtos_unlock_mutex(&list->mutex);
    return AVDK_ERR_OK;
}

avdk_err_t file_list_remove(video_player_file_list_t *list, const char *file_path)
{
    if (list == NULL || file_path == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&list->mutex);

    video_player_file_node_t *node = list->head;
    while (node != NULL)
    {
        if (os_strcmp(node->file_path, file_path) == 0)
        {
            // Remove node
            if (node->prev != NULL)
            {
                node->prev->next = node->next;
            }
            else
            {
                list->head = node->next;
            }

            if (node->next != NULL)
            {
                node->next->prev = node->prev;
            }
            else
            {
                list->tail = node->prev;
            }

            // Update current file if needed
            if (list->current == node)
            {
                list->current = node->next;
                if (list->current == NULL)
                {
                    list->current = list->head;
                }
            }

            os_free(node->file_path);
            os_free(node);
            rtos_unlock_mutex(&list->mutex);
            return AVDK_ERR_OK;
        }
        node = node->next;
    }

    rtos_unlock_mutex(&list->mutex);
    return AVDK_ERR_NODEV;
}

void file_list_clear(video_player_file_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    rtos_lock_mutex(&list->mutex);

    video_player_file_node_t *node = list->head;
    while (node != NULL)
    {
        video_player_file_node_t *next = node->next;
        os_free(node->file_path);
        os_free(node);
        node = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->current = NULL;

    rtos_unlock_mutex(&list->mutex);
}

video_player_file_node_t *file_list_find_next(video_player_file_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }

    rtos_lock_mutex(&list->mutex);

    if (list->current == NULL)
    {
        list->current = list->head;
    }
    else
    {
        list->current = list->current->next;
        if (list->current == NULL)
        {
            // Loop to head
            list->current = list->head;
        }
    }

    video_player_file_node_t *result = list->current;
    rtos_unlock_mutex(&list->mutex);
    return result;
}

video_player_file_node_t *file_list_find_prev(video_player_file_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }

    rtos_lock_mutex(&list->mutex);

    if (list->current == NULL)
    {
        list->current = list->tail;
    }
    else
    {
        list->current = list->current->prev;
        if (list->current == NULL)
        {
            // Loop to tail
            list->current = list->tail;
        }
    }

    video_player_file_node_t *result = list->current;
    rtos_unlock_mutex(&list->mutex);
    return result;
}

avdk_err_t file_list_set_current(video_player_file_list_t *list, const char *file_path)
{
    if (list == NULL || file_path == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    rtos_lock_mutex(&list->mutex);

    video_player_file_node_t *node = list->head;
    while (node != NULL)
    {
        if (os_strcmp(node->file_path, file_path) == 0)
        {
            list->current = node;
            rtos_unlock_mutex(&list->mutex);
            return AVDK_ERR_OK;
        }
        node = node->next;
    }

    rtos_unlock_mutex(&list->mutex);
    return AVDK_ERR_NODEV;
}

video_player_file_node_t *file_list_get_current(video_player_file_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }

    rtos_lock_mutex(&list->mutex);
    video_player_file_node_t *result = list->current;
    rtos_unlock_mutex(&list->mutex);
    return result;
}

avdk_err_t file_list_check_file_valid(const char *file_path)
{
    if (file_path == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    struct stat statbuf;
    int ret = stat(file_path, &statbuf);
    if (ret != 0)
    {
        // Get error code for debugging
        int err = errno;
        LOGE("%s: File does not exist or cannot access: %s, stat ret=%d, errno=%d\n", 
             __func__, file_path, ret, err);
        return AVDK_ERR_NODEV;
    }

    LOGD("%s: File exists: %s, size=%lld bytes\n", __func__, file_path, (long long)statbuf.st_size);

    if (statbuf.st_size == 0)
    {
        LOGE("%s: File size is zero: %s\n", __func__, file_path);
        return AVDK_ERR_INVAL;
    }

    return AVDK_ERR_OK;
}

