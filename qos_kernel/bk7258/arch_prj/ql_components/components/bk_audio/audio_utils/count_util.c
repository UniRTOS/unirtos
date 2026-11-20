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

/* This file is used to debug uac work status by collecting statistics on the uac mic and speaker. */

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_audio/audio_utils/count_util.h>


#define COUNT_UTIL_TAG "count_util"

#define LOGI(...) BK_LOGI(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(COUNT_UTIL_TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(COUNT_UTIL_TAG, ##__VA_ARGS__)



static void count_util_callback(void *param)
{
    count_util_t *count_util = (count_util_t *)param;

    if (!count_util)
    {
        return;
    }

    uint32_t temp = count_util->data_size;

    count_util->data_size = count_util->data_size / 1024 / (count_util->timer_interval / 1000);

    LOGD("[%s] data_size: %d(Bytes), %uKB/s \n", count_util->tag, temp, count_util->data_size);
    count_util->data_size  = 0;
}

bk_err_t count_util_destroy(count_util_t *count_util)
{
    bk_err_t ret = BK_OK;

    if (!count_util)
    {
        LOGE("%s, %d, count_util is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (count_util->timer.handle)
    {
        ret = rtos_stop_timer(&count_util->timer);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, stop count util timer fail \n", __func__, __LINE__);
        }

        ret = rtos_deinit_timer(&count_util->timer);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, deinit count util timer fail \n", __func__, __LINE__);
        }
        count_util->timer.handle = NULL;
    }
    count_util->data_size = 0;
    count_util->timer_interval = 0;
    os_memset(count_util->tag, '\0', 20);
    LOGD("%s, %d, destroy count util timer complete \n", __func__, __LINE__);

    return ret;
}

bk_err_t count_util_create(count_util_t *count_util, uint32_t interval, char *tag)
{
    bk_err_t ret = BK_OK;

    if (!count_util || interval <= 0 || !tag)
    {
        LOGE("%s, %d, count_util: %p, interval: %d, tag: %p \n", __func__, __LINE__, count_util, interval, tag);
        return BK_FAIL;
    }

    if (count_util->timer.handle != NULL)
    {
        ret = rtos_deinit_timer(&count_util->timer);
        if (BK_OK != ret)
        {
            LOGE("%s, %d, deinit count util time fail \n", __func__, __LINE__);
            goto exit;
        }
        count_util->timer.handle = NULL;
    }

    count_util->data_size = 0;
    count_util->timer_interval = interval;
    if (os_strlen(tag) > 19)
    {
        LOGW("%s, %d, tag length: %d > 19 \n", __func__, __LINE__, os_strlen(tag));
        os_memcpy(count_util->tag, tag, 19);
        count_util->tag[19] = '\0';
    }
    else
    {
        os_memcpy(count_util->tag, tag, os_strlen(tag));
        count_util->tag[os_strlen(tag)] = '\0';
    }

    ret = rtos_init_timer(&count_util->timer, count_util->timer_interval, count_util_callback, count_util);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, init %s count util timer fail \n", __func__, __LINE__, count_util->tag);
        goto exit;
    }
    ret = rtos_start_timer(&count_util->timer);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, start %s count util timer fail \n", __func__, __LINE__, count_util->tag);
        goto exit;
    }
    LOGD("%s, %d, create %s count util timer complete \n", __func__, __LINE__, count_util->tag);

    return BK_OK;
exit:

    count_util_destroy(count_util);
    return BK_FAIL;
}

void count_util_add_size(count_util_t *count_util, int32_t size)
{
    if (!count_util)
    {
        LOGV("%s, %d, count_util is NULL \n", __func__, __LINE__);
        return;
    }

    if (size > 0)
    {
        count_util->data_size += size;
    }
}

/**
 * @brief      Timer callback for multi-parameter count util
 *
 * @param[in]  param  Pointer to count_util_multi_t structure
 *
 * @return     None
 */
static void count_util_multi_callback(void *param)
{
    count_util_multi_t *count_util = (count_util_multi_t *)param;

    if (!count_util || !count_util->params)
    {
        return;
    }

    /* Calculate and print statistics for all parameters together */
    char log_buffer[256] = {0};
    uint32_t offset = 0;

    for (uint32_t i = 0; i < count_util->param_count; i++)
    {
        uint32_t temp = count_util->params[i].data_size;
        uint32_t speed = temp / 1024 / (count_util->timer_interval / 1000);

        /* Append each parameter's statistics to the log buffer */
        if (i > 0)
        {
            offset += os_snprintf(log_buffer + offset, sizeof(log_buffer) - offset, " | ");
        }
        offset += os_snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                            "[%s]:%d(Bytes),%uKB/s", count_util->params[i].tag, temp, speed);

        /* Reset data size for next interval */
        count_util->params[i].data_size = 0;

        /* Check buffer overflow protection */
        if (offset >= sizeof(log_buffer) - 1)
        {
            break;
        }
    }

    LOGD("%s \n", log_buffer);
}

bk_err_t count_util_multi_destroy(count_util_multi_t *count_util)
{
    bk_err_t ret = BK_OK;

    if (!count_util)
    {
        LOGE("%s, %d, count_util is NULL \n", __func__, __LINE__);
        return BK_FAIL;
    }

    /* Stop and deinitialize timer */
    if (count_util->timer.handle)
    {
        ret = rtos_stop_timer(&count_util->timer);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, stop count util multi timer fail \n", __func__, __LINE__);
        }

        ret = rtos_deinit_timer(&count_util->timer);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, deinit count util multi timer fail \n", __func__, __LINE__);
        }
        count_util->timer.handle = NULL;
    }

    /* Free allocated memory for parameters */
    if (count_util->params)
    {
        os_free(count_util->params);
        count_util->params = NULL;
    }

    count_util->param_count = 0;
    count_util->timer_interval = 0;
    LOGD("%s, %d, destroy count util multi timer complete \n", __func__, __LINE__);

    return ret;
}

bk_err_t count_util_multi_create(count_util_multi_t *count_util, uint32_t interval, char **tags, uint32_t param_count)
{
    bk_err_t ret = BK_OK;

    /* Validate input parameters */
    if (!count_util || interval <= 0 || !tags || param_count == 0)
    {
        LOGE("%s, %d, count_util: %p, interval: %d, tags: %p, param_count: %d \n",
             __func__, __LINE__, count_util, interval, tags, param_count);
        return BK_FAIL;
    }

    /* Clean up existing timer if present */
    if (count_util->timer.handle != NULL)
    {
        ret = rtos_deinit_timer(&count_util->timer);
        if (BK_OK != ret)
        {
            LOGE("%s, %d, deinit count util multi timer fail \n", __func__, __LINE__);
            goto exit;
        }
        count_util->timer.handle = NULL;
    }

    /* Free existing parameters if present */
    if (count_util->params)
    {
        os_free(count_util->params);
        count_util->params = NULL;
    }

    /* Allocate memory for parameters */
    count_util->params = (count_util_param_t *)os_malloc(sizeof(count_util_param_t) * param_count);
    if (!count_util->params)
    {
        LOGE("%s, %d, malloc count util params fail \n", __func__, __LINE__);
        ret = BK_ERR_NO_MEM;
        goto exit;
    }

    /* Initialize parameters */
    count_util->param_count = param_count;
    count_util->timer_interval = interval;

    for (uint32_t i = 0; i < param_count; i++)
    {
        count_util->params[i].data_size = 0;

        /* Copy tag with length validation */
        if (tags[i])
        {
            uint32_t tag_len = os_strlen(tags[i]);
            if (tag_len > 19)
            {
                LOGW("%s, %d, tag[%d] length: %d > 19 \n", __func__, __LINE__, i, tag_len);
                os_memcpy(count_util->params[i].tag, tags[i], 19);
                count_util->params[i].tag[19] = '\0';
            }
            else
            {
                os_memcpy(count_util->params[i].tag, tags[i], tag_len);
                count_util->params[i].tag[tag_len] = '\0';
            }
        }
        else
        {
            LOGE("%s, %d, tags[%d] is NULL \n", __func__, __LINE__, i);
            ret = BK_FAIL;
            goto exit;
        }
    }

    /* Initialize and start timer */
    ret = rtos_init_timer(&count_util->timer, count_util->timer_interval, count_util_multi_callback, count_util);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, init count util multi timer fail \n", __func__, __LINE__);
        goto exit;
    }

    ret = rtos_start_timer(&count_util->timer);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, start count util multi timer fail \n", __func__, __LINE__);
        goto exit;
    }

    LOGD("%s, %d, create count util multi timer complete, param_count: %d \n", __func__, __LINE__, param_count);

    return BK_OK;

exit:
    count_util_multi_destroy(count_util);
    return ret;
}

void count_util_multi_add_size(count_util_multi_t *count_util, uint32_t param_index, int32_t size)
{
    /* Validate input parameters */
    if (!count_util)
    {
        LOGV("%s, %d, count_util is NULL \n", __func__, __LINE__);
        return;
    }

    if (!count_util->params)
    {
        LOGV("%s, %d, count_util->params is NULL \n", __func__, __LINE__);
        return;
    }

    /* Check parameter index bounds */
    if (param_index >= count_util->param_count)
    {
        LOGV("%s, %d, param_index: %d >= param_count: %d \n", __func__, __LINE__, param_index, count_util->param_count);
        return;
    }

    /* Add size if positive */
    if (size > 0)
    {
        count_util->params[param_index].data_size += size;
    }
}

