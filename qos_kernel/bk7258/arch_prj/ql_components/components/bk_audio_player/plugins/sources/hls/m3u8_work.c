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

#include <components/webclient.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"

#include <components/bk_audio/audio_pipeline/bsd_queue.h>
#include "m3u8_work.h"


#define BIT1     0x00000002
#define BIT0     0x00000001

const static int TASK_CREATED_BIT = BIT0;
const static int TASK_DESTROYED_BIT = BIT1;


static struct webclient_session *m3u8_session = NULL;


static void *_malloc_memory(uint32_t size)
{
    return player_malloc(size);
}

static void _free_memory(void *ptr)
{
    return player_free(ptr);
}

static int _init_mutex(beken_mutex_t *mutex)
{
    return osal_init_mutex(mutex);
}

static int _deinit_mutex(beken_mutex_t *mutex)
{
    return osal_deinit_mutex(mutex);
}

static int _lock_mutex(beken_mutex_t *mutex)
{
    return osal_lock_mutex(mutex);
}

static int _unlock_mutex(beken_mutex_t *mutex)
{
    return osal_unlock_mutex(mutex);
}

static int _read(void *buffer, uint32_t length)
{
    if (!m3u8_session)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, m3u8_session is NULL, %d\n", __func__, __LINE__);
        return -1;
    }
    return webclient_read(m3u8_session, buffer, length);
}

static m3u8_osi_funcs_t m3u8_osi_funcs =
{
    .malloc_memory = _malloc_memory,
    .free_memory = _free_memory,
    .init_mutex = _init_mutex,
    .deinit_mutex = _deinit_mutex,
    .lock_mutex = _lock_mutex,
    .unlock_mutex = _unlock_mutex,
    .read = _read,
};

int hls_m3u8_init(top_m3u8_info_t *top_m3u8, char *top_m3u8_url)
{
    m3u8_osi_funcs_init((void *)&m3u8_osi_funcs);
    return m3u8_list_init(top_m3u8, top_m3u8_url);
}

int hls_m3u8_deinit(top_m3u8_info_t *top_m3u8)
{
    m3u8_list_deinit(top_m3u8);
    m3u8_osi_funcs_init((void *)&m3u8_osi_funcs);

    return 0;
}


int hls_m3u8_fetch(m3u8_session_t *m3u8, const char *url)
{
    int code;
    char *mime = NULL;
    //    int content_length = 0;

    //    BK_LOGE(AUDIO_PLAYER_TAG, "%s \n", __func__);

    m3u8_session = webclient_session_create(2048);
    if (!m3u8_session)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, webclient_session_create() for %s fail, %d\n", __func__, url, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    code = webclient_get(m3u8_session, (const char *)url);
    if (code != 200)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, webclient_get() for %s fail, code=%d, %d\n", __func__, url, code, __LINE__);
        if (m3u8_session)
        {
            webclient_close(m3u8_session);
            m3u8_session = NULL;
        }
        return AUDIO_PLAYER_ERR;
    }
    //  content_length = webclient_content_length_get(session);
    mime = (char *)webclient_header_fields_get(m3u8_session, "Content-Type");

    /*  Specific url
        URL: http://satellitepull.cnr.cn/live/wxgdcszs/playlist.m3u8
        Content-Type: application/x-mpegurl
    */
    if (mime && strcmp(mime, "application/vnd.apple.mpegurl") == 0)
    {
        m3u8_parse(&m3u8->m3u8_info);
    }

#if 1
    if (mime && strcmp(mime, "application/x-mpegurl") == 0)
    {
        m3u8_parse(&m3u8->m3u8_info);
    }
#endif

    webclient_close(m3u8_session);

    return 0;
}

void _hls_m3u8_thread(void *param)
{
    m3u8_session_t *m3u8 = (m3u8_session_t *)param;
    if (!m3u8)
    {
        return;
    }

    m3u8->running = 1;

    xEventGroupSetBits(m3u8->state_event, TASK_CREATED_BIT);
    xEventGroupClearBits(m3u8->state_event, TASK_DESTROYED_BIT);

    while (m3u8->running)
    {
        int timeout;
        double total_duration = 0;

        /* calculate timeout tick */
        /* When the following situations occur, this calculation method according to the max time duration is incorrect.
            (1) The max time duration of m3u8 url is not exist.
            (2) the time duration of some m3u8 urls is not equal to the max time duration, but it is less than the max time duration.

            So we should count the total time duration of the remaining m3u8 urls and calculate timeout tick based on that time.
        */
        total_duration = _get_current_use_second_m3u8_total_time_duration(&m3u8->m3u8_info);
        if (total_duration <= 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s _get_current_use_second_m3u8_total_time_duration fail, use default value: 4.000000 %d \n", __func__, __LINE__);
            total_duration = 4.000000;
        }

        timeout = total_duration * 1000 / 2 / portTICK_RATE_MS;
        BK_LOGE(AUDIO_PLAYER_TAG, "%s timeout: %d ms, total_duration: %f \n", __func__, timeout * portTICK_RATE_MS, total_duration);

        EventBits_t uxBits = xEventGroupWaitBits(m3u8->state_event, TASK_DESTROYED_BIT, false, true, timeout);
        if (uxBits & TASK_DESTROYED_BIT)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s TASK_DESTROYED_BIT \n", __func__);
            break;
        }

        hls_m3u8_fetch(m3u8, m3u8->m3u8_info.top_m3u8_url);

        //        debug_m3u8_lists(&m3u8->m3u8_info);

        /* If top_m3u8_url type is M3U8_TYPE_TOP_M3U8, parse second m3u8 current used and update m3u8 url list */
        if (m3u8->m3u8_info.m3u8_type == M3U8_TYPE_TOP_M3U8)
        {
            hls_m3u8_fetch(m3u8, m3u8->m3u8_info.current_use_m3u8.second_m3u8_url);
        }

        //        debug_m3u8_lists(&m3u8->m3u8_info);
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, thread exit, %d \n", __func__, __LINE__);
    osal_post_sema(&m3u8->thread_sem);
    osal_delete_thread(NULL);
}

int hls_m3u8_thread_create(m3u8_session_t *m3u8)
{
    int ret = 0;

    m3u8->state_event = xEventGroupCreate();
    if (!m3u8->state_event)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, xEventGroupCreate fail, %d \n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    ret = osal_init_sema(&m3u8->thread_sem, 1, 0);
    if (ret != BK_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, init thread_sem fail, %d \n", __func__, __LINE__);
        goto fail;
    }

    xEventGroupClearBits(m3u8->state_event, TASK_CREATED_BIT);
    /* create HLS m3u thread to read m3u8 url, and get url item */
    ret = osal_create_thread(&m3u8->tid, (osal_thread_func)_hls_m3u8_thread, 4096, "hls_m3u8", m3u8, BEKEN_DEFAULT_WORKER_PRIORITY);
    if (ret)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, can't create _hls_m3u8_thread, %d \n", __func__, __LINE__);
        goto fail;
    }
    else
    {
        EventBits_t uxBits = xEventGroupWaitBits(m3u8->state_event, TASK_CREATED_BIT, false, true, (4000 / portTICK_RATE_MS));
        if (uxBits & TASK_CREATED_BIT)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, create _hls_m3u8_thread ok \n", __func__);
        }
        else
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, _hls_m3u8_thread run fail %d \n", __func__, __LINE__);
            goto fail;
        }
    }

    return AUDIO_PLAYER_OK;

fail:
    if (m3u8->state_event)
    {
        vEventGroupDelete(m3u8->state_event);
        m3u8->state_event = NULL;
    }

    if (m3u8->thread_sem)
    {
        osal_deinit_sema(&m3u8->thread_sem);
        m3u8->thread_sem = NULL;
    }

    return AUDIO_PLAYER_ERR;
}

int hls_m3u8_thread_destroy(m3u8_session_t *m3u8)
{
    if (!m3u8)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, m3u8 is NULL, %d \n", __func__, __LINE__);
        return 0;
    }

    if (m3u8->running && m3u8->state_event && m3u8->thread_sem)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "hls_m3u8_thread_destroy \n");
        m3u8->running = 0;
        xEventGroupSetBits(m3u8->state_event, TASK_DESTROYED_BIT);
        osal_wait_sema(&m3u8->thread_sem, BEKEN_WAIT_FOREVER);
    }

    hls_m3u8_deinit(&m3u8->m3u8_info);

    //m3u8_list_deinit(&m3u8->m3u8_info);

    if (m3u8->thread_sem)
    {
        osal_deinit_sema(&m3u8->thread_sem);
        m3u8->thread_sem = NULL;
    }

    if (m3u8->state_event)
    {
        vEventGroupDelete(m3u8->state_event);
        m3u8->state_event = NULL;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "hls_m3u8_thread_destroy complete \n");

    return AUDIO_PLAYER_OK;
}
