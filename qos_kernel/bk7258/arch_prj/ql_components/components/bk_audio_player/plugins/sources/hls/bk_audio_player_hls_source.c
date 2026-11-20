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

#include "player_osal.h"
#include "source_api.h"
#include "codec_api.h"

#include <components/webclient.h>
#include "ring_buffer.h"
#include "sockets.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"

#include "m3u8_work.h"


#ifndef CONFIG_AUDIO_PLAYER_HLS_NET_PIPE_SIZE
#define CONFIG_AUDIO_PLAYER_HLS_NET_PIPE_SIZE   (8 * 1024)
#endif
#define HLS_NET_PIPE_SIZE       CONFIG_AUDIO_PLAYER_HLS_NET_PIPE_SIZE
#define HLS_NET_CHUNK_SIZE      (128)

#ifndef CONFIG_AUDIO_PLAYER_WEB_RETRY_COUNT
#define CONFIG_AUDIO_PLAYER_WEB_RETRY_COUNT (10)
#endif
#define WEB_RETRY_COUNT         CONFIG_AUDIO_PLAYER_WEB_RETRY_COUNT

#define M3U8_ITEM_SZ            (128)
#define M3U8_ITEM_MAX           (5)


typedef struct hls_source_priv_s
{
    struct webclient_session *session;
    int content_length;
    int codec_type;
    int finish_bytes;

    osal_thread_t tid;
    int running;
    osal_sema_t thread_sem;
    int result;
    ringbuf_handle_t pipe;
    char *net_buffer;
    int net_buffer_len;
    char *url;      //net url
    int read_timeout_cnt;   /* consecutive rb_read timeouts in hls_source_read */

    m3u8_session_t m3u8_session;
} hls_source_priv_t;

static void hls_net_set_socket_timeout(struct webclient_session *session, uint32_t timeout_ms)
{
    //webclient_set_timeout(priv->session, 3 * 1000);
    webclient_set_timeout(session, timeout_ms);

    /* add keepalive */
    if (1)
    {
        int res;
        int keepalive = 1;      //Enable keepalive.
        int keepidle = 8;       //idle time is 60s.
        int keepinterval = 3;   //sending interval of detective packet
        int keepcount = 2;      //detective count.
        res = setsockopt(session->socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
        if (res < 0)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "SO_KEEPALIVE %d.", res);
        }
        res = setsockopt(session->socket, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
        if (res < 0)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "TCP_KEEPIDLE %d.", res);
        }
        res = setsockopt(session->socket, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval));
        if (res < 0)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "TCP_KEEPINTVL %d.", res);
        }
        res = setsockopt(session->socket, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount));
        if (res < 0)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "TCP_KEEPCNT %d.", res);
        }
    }
}

int webclient_session_open(bk_audio_player_source_t *source)
{
    int code;
    char *ext_name;
    char *mime;
    //    m3u8_url_info_t *url_item = NULL;
    char *m3u8_url = NULL;

    hls_source_priv_t *priv;

    priv = (hls_source_priv_t *)source->source_priv;
    priv->finish_bytes = 0;

    /* get m3u8_url_item */
    //m3u8_url_list_pop(&priv->m3u8_session.m3u8_info, &url_item);
    m3u8_url = get_used_complete_m3u8_url(&priv->m3u8_session.m3u8_info);
    if (!m3u8_url)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, m3u8_url_list is empty, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }
    else
    {
        //BK_LOGI(AUDIO_PLAYER_TAG, "get m3u8_url_item, sequence: %d, url: %s\n", url_item->url_item->sequence, url_item->url_item->item);
        _url_printf("hls", m3u8_url);
    }

    //    strncpy(priv->url, url_item->url_item->item, M3U8_ITEM_SZ);
    if (priv->url)
    {
        player_free(priv->url);
        priv->url = NULL;
    }
    priv->url = player_strdup(m3u8_url);

    /* free url_item after process */
    //m3u8_url_list_free_item(url_item);
    player_free(m3u8_url);
    m3u8_url = NULL;

    priv->session = webclient_session_create(2048);
    if (!priv->session)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, webclient_session_create() for %s fail, %d\n", __func__, priv->url, __LINE__);
        //      free(source);
        return AUDIO_PLAYER_ERR;
    }

    code = webclient_get(priv->session, (const char *)priv->url);
    if (code != 200)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, webclient_get() for %s fail, code=%d, %d\n", __func__, priv->url, code, __LINE__);
        if (priv->session)
        {
            webclient_close(priv->session);
            priv->session = NULL;
        }
        //      free(source);
        return AUDIO_PLAYER_ERR;
    }
    priv->content_length = webclient_content_length_get(priv->session);
    mime = (char *)webclient_header_fields_get(priv->session, "Content-Type");

    ext_name = strrchr(priv->url, '.');
    priv->codec_type = audio_codec_get_type(ext_name);
    if (priv->codec_type == AUDIO_FORMAT_UNKNOWN)
    {
        priv->codec_type = audio_codec_get_mime_type(mime);
        if (priv->codec_type == AUDIO_FORMAT_UNKNOWN)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "mime type %s not supported\n", mime);
            if (priv->session)
            {
                webclient_close(priv->session);
                priv->session = NULL;
            }
            player_free(source);
            return AUDIO_PLAYER_ERR;
        }
    }

    return AUDIO_PLAYER_OK;
}

static void *_hls_net_source_bg_thread(void *param)
{
    bk_audio_player_source_t *source;
    hls_source_priv_t *priv;
    int ret;
    int len;
    int pos;
    int retry;

    source = (bk_audio_player_source_t *)param;
    priv = (hls_source_priv_t *)source->source_priv;

    priv->running = 1;
    retry = WEB_RETRY_COUNT;

    hls_net_set_socket_timeout(priv->session, 500);

    while (priv->running)
    {
        len = webclient_read(priv->session, priv->net_buffer, priv->net_buffer_len);
        if (len == -WEBCLIENT_TIMEOUT)
        {
            continue;
        }
        else if (len > 0)
        {
            priv->finish_bytes += len;
            pos = 0;

            while (pos < len)
            {
                ret = rb_write(priv->pipe, priv->net_buffer + pos, len - pos, 10 / portTICK_RATE_MS);
                if (ret > 0)
                {
                    pos += ret;
                }
                else if (ret == 0 || ret == RB_TIMEOUT)
                {
                    if (priv->running)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    continue;    // TODO
                }
            }
        }
        else if (len == 0)
        {
            priv->result = 1;

            BK_LOGI(AUDIO_PLAYER_TAG, "%s, socket close , content_length=%d, actual_length=%d\n", __func__, priv->content_length, priv->finish_bytes);

            if (priv->content_length > priv->finish_bytes &&  0 < retry--)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "try again retry:%d\r\n", retry);
                if (priv->session)
                {
                    webclient_close(priv->session);
                    priv->session = webclient_session_create(2048);
                    webclient_get_position(priv->session, priv->url, priv->finish_bytes);
                }
                continue;
            }
            else
            {
                if (priv->session)
                {
                    webclient_close(priv->session);
                    priv->session = NULL;
                }
                ret = webclient_session_open(source);
                if (ret != AUDIO_PLAYER_OK)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "webclient_session_open fail \n", priv->url);
                }
                else
                {
                    continue;
                }
            }
            break;
        }
        else
        {
            priv->result = -1;
            BK_LOGE(AUDIO_PLAYER_TAG, "net_source_read failed, ret = %d\n", len);
            break;
        }
    }

    /* notify app when return because of error */
    //TODO
    BK_LOGD(AUDIO_PLAYER_TAG, "%s %d thread exit \r\n", __func__, __LINE__);
    rb_done_write(priv->pipe);
    osal_post_sema(&priv->thread_sem);
    osal_delete_thread(NULL);
    return NULL;
}


static int hls_net_source_start_worker(bk_audio_player_source_t *source)
{
    int ret;
    hls_source_priv_t *priv;

    priv = (hls_source_priv_t *)source->source_priv;

    priv->net_buffer_len = HLS_NET_CHUNK_SIZE;
    priv->net_buffer = player_malloc(priv->net_buffer_len);
    if (!priv->net_buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, can't malloc net_buffer, %d \n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    priv->pipe = rb_create(HLS_NET_PIPE_SIZE);
    if (!priv->pipe)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, can't create pipe, %d \n", __func__, __LINE__);
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
        return AUDIO_PLAYER_ERR;
    }

    osal_init_sema(&priv->thread_sem, 1, 0);

    ret = osal_create_thread(&priv->tid, (osal_thread_func)_hls_net_source_bg_thread, 4096, "hls_net", source, BEKEN_DEFAULT_WORKER_PRIORITY);
    if (ret)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, can't create thread, %d \n", __func__, __LINE__);
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
        rb_destroy(priv->pipe);
        priv->pipe = NULL;
        return AUDIO_PLAYER_ERR;
    }
    else
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, create thread ok \n", __func__);
        return AUDIO_PLAYER_OK;
    }
}

static int hls_net_source_stop_worker(bk_audio_player_source_t *source)
{
    hls_source_priv_t *priv;

    priv = (hls_source_priv_t *)source->source_priv;

    priv->running = 0;
    osal_wait_sema(&priv->thread_sem, BEKEN_WAIT_FOREVER);
    osal_deinit_sema(&priv->thread_sem);

    player_free(priv->net_buffer);
    priv->net_buffer = NULL;
    rb_destroy(priv->pipe);
    priv->pipe = NULL;
    return AUDIO_PLAYER_OK;
}

static int hls_source_open(char *url, bk_audio_player_source_t **source_pp)
{
    bk_audio_player_source_t *source;
    hls_source_priv_t *priv;
    int ret;

    if ((strncmp(url, "http://", 7) != 0) ||
        (strncmp(&url[os_strlen(url) - 6], ".m3u8", 5) != 0
         && strncmp(&url[os_strlen(url) - 5], ".m3u", 4) != 0
         && strncmp(&url[os_strlen(url) - 5], "m3u8", 4) != 0
         && strncmp(&url[os_strlen(url) - 4], "m3u", 3) != 0))
    {
        //BK_LOGE(AUDIO_PLAYER_TAG, "%s, uri: %s invalid, %d \n", __func__, url, __LINE__);
        return AUDIO_PLAYER_INVALID;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "net_source_open : %s\n", url);
    source = audio_source_new(sizeof(hls_source_priv_t));
    if (!source)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc hls_source_priv_t fail, %d \n", __func__, __LINE__);
        return AUDIO_PLAYER_NO_MEM;
    }
    else
    {
        os_memset(source->source_priv, 0, sizeof(hls_source_priv_t));
    }

    priv = (hls_source_priv_t *)source->source_priv;

    /* init m3u8 list */
    hls_m3u8_init(&priv->m3u8_session.m3u8_info, url);

    //    BK_LOGI(AUDIO_PLAYER_TAG, "&priv->m3u8_session: %p \n", &priv->m3u8_session);

    /* fetch first m3u8 item list */
    /* read url and parse m3u8 url to get the following informations:
        1. m3u8 type of "url", top m3u8 or second m3u8
        2. second m3u8 list if "url" type is top level m3u8
        3. play url list if "url" type is second level m3u8
    note:
        If "url" type is second level m3u8, set "m3u8_type" to M3U8_TYPE_SECOND_M3U8.
        And only one second level m3u8 in second m3u8 list, it is "url".
    */
    hls_m3u8_fetch(&priv->m3u8_session, url);
    //rtos_delay_milliseconds(50);
    debug_m3u8_lists(&priv->m3u8_session.m3u8_info);

    m3u8_type_t m3u8_type = get_m3u8_type(&priv->m3u8_session.m3u8_info);
    switch (m3u8_type)
    {
        case M3U8_TYPE_TOP_M3U8:
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, M3U8_TYPE_TOP_M3U8 \n", __func__);
            /* parse second m3u8 file */
            if (m3u8_list_get_second_m3u8_num(&priv->m3u8_session.m3u8_info) > 0)
            {
                //                BK_LOGI(AUDIO_PLAYER_TAG, "m3u8_list_set_use_second_m3u8_by_id \n");
                /* set the first second m3u8 of second m3u8 list to the used second m3u8. */
                if (0 != m3u8_list_set_use_second_m3u8_by_id(&priv->m3u8_session.m3u8_info, 0))
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, m3u8_list_set_use_second_m3u8_by_id fail, %d \n", __func__, __LINE__);
                    return AUDIO_PLAYER_ERR;
                }
                else
                {
                    /* parse second m3u8 according to current_use_m3u8 to get play url list. */
                    /* get the second m3u8 current used */
                    char *second_m3u8_url = get_used_complete_second_m3u8_url(&priv->m3u8_session.m3u8_info);
                    if (second_m3u8_url == NULL)
                    {
                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, get_used_complete_second_m3u8_url fail, %d \n", __func__, __LINE__);
                        return AUDIO_PLAYER_ERR;
                    }
                    else
                    {
                        /* parse second m3u8 to get play url list. */
                        BK_LOGI(AUDIO_PLAYER_TAG, "%s, parse second_m3u8_url \n", __func__);
                        _url_printf("hls", second_m3u8_url);
                        hls_m3u8_fetch(&priv->m3u8_session, second_m3u8_url);
                        //rtos_delay_milliseconds(50);
                        debug_m3u8_lists(&priv->m3u8_session.m3u8_info);
                        /* free second_m3u8_url after use */
                        free_url_obtained_from_m3u8(second_m3u8_url);
                        second_m3u8_url = NULL;
                    }
                }
            }
            else
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, second_m3u8_num is 0, %d \n", __func__, __LINE__);
                return AUDIO_PLAYER_ERR;
            }
            break;

        case M3U8_TYPE_SECOND_M3U8:
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, M3U8_TYPE_SECOND_M3U8 \n", __func__);
            break;

        case M3U8_TYPE_M3U:
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, M3U8_TYPE_M3U \n", __func__);
            break;

        default:
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, m3u8_type: %d not right, %d \n", __func__, m3u8_type, __LINE__);
            return AUDIO_PLAYER_ERR;
    }

    /* check whether url list of current used second m3u8 is empty  */
    if (m3u8_url_list_is_empty(&priv->m3u8_session.m3u8_info))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, m3u8_url_list is empty, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    /* create hls_m3u8 task to update m3u8 item periodical */
    ret = hls_m3u8_thread_create(&priv->m3u8_session);
    if (ret != AUDIO_PLAYER_OK)
    {
        return AUDIO_PLAYER_ERR;
    }

    ret = webclient_session_open(source);
    if (ret != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, webclient_session_open fail, %d \n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    ret = hls_net_source_start_worker(source);
    if (ret)
    {
        if (priv->session)
        {
            webclient_close(priv->session);
            priv->session = NULL;
        }
        player_free(source);
        source = NULL;
        return AUDIO_PLAYER_ERR;
    }

    *source_pp = source;

    //osal_usleep(1000);

    return AUDIO_PLAYER_OK;
}

static int hls_source_close(bk_audio_player_source_t *source)
{
    hls_source_priv_t *priv;

    priv = (hls_source_priv_t *)source->source_priv;
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    hls_m3u8_thread_destroy(&priv->m3u8_session);

    hls_net_source_stop_worker(source);

    if (priv->session)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, webclient_close \n", __func__);
        webclient_close(priv->session);
        priv->session = NULL;
    }

    if (priv->url)
    {
        player_free(priv->url);
        priv->url = NULL;
    }

    return AUDIO_PLAYER_OK;
}

static int hls_source_get_codec_type(bk_audio_player_source_t *source)
{
    hls_source_priv_t *priv;

    priv = (hls_source_priv_t *)source->source_priv;

    return priv->codec_type;
}

static int hls_source_read(bk_audio_player_source_t *source, char *buffer, int len)
{
    int ret;
    hls_source_priv_t *priv;
    priv = (hls_source_priv_t *)source->source_priv;

    ret = rb_read(priv->pipe, buffer, len, 300 / portTICK_RATE_MS);

    if (ret == RB_TIMEOUT)
    {
        /* Poor network: pipe underran. Treat as a transient timeout so the
         * TS parser keeps waiting, for up to WEB_RETRY_COUNT consecutive tries
         * (~WEB_RETRY_COUNT * 300ms). If still no data, actively end so the
         * player tears down and auto-restarts (same intent as net_source). */
        if (++priv->read_timeout_cnt < WEB_RETRY_COUNT)
        {
            return AUDIO_PLAYER_TIMEOUT;   /* -200: ts_format retries */
        }
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, read timeout %d times, end stream\n", __func__, priv->read_timeout_cnt);
        priv->read_timeout_cnt = 0;
        return ret;                        /* -4: ts_format ends the segment */
    }

    priv->read_timeout_cnt = 0;            /* data (>0) or RB_DONE: reset */
    return ret;
}

static int hls_source_seek(bk_audio_player_source_t *source, int offset, uint32_t whence)
{
    hls_source_priv_t *priv;
    priv = (hls_source_priv_t *)source->source_priv;
    int ret;
    int seek_offset = offset;

    if (whence == SEEK_CUR)
    {
        seek_offset = offset + rb_bytes_filled(priv->pipe);
    }
    else if (whence == SEEK_END)
    {
        if (priv->content_length != -1)
        {
            seek_offset = priv->content_length - offset;
        }
    }

    if (seek_offset > priv->content_length)
    {
        seek_offset = priv->content_length;
    }

    BK_LOGD(AUDIO_PLAYER_TAG, "seek offset:%d,offset:%d,whence:%d\r\n", seek_offset, offset, whence);

    if (!priv->running)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "no runing seek return \r\n");
        return AUDIO_PLAYER_ERR;
    }

    hls_net_source_stop_worker(source);

    if (priv->session)
    {
        webclient_close(priv->session);
        priv->session = webclient_session_create(2048);
        priv->finish_bytes =  seek_offset;
        webclient_get_position(priv->session, priv->url, seek_offset);
    }

    ret = hls_net_source_start_worker(source);
    if (ret)
    {
        if (priv->session)
        {
            webclient_close(priv->session);
            priv->session = NULL;
        }
        player_free(source);
        return AUDIO_PLAYER_ERR;
    }
    return AUDIO_PLAYER_OK;
}

const bk_audio_player_source_ops_t hls_source_ops =
{
    .open = hls_source_open,
    .get_codec_type = hls_source_get_codec_type,
    .get_total_bytes = NULL,
    .read = hls_source_read,
    .seek = hls_source_seek,
    .close = hls_source_close,
};

/* Get HLS source operations structure */
const bk_audio_player_source_ops_t *bk_audio_player_get_hls_source_ops(void)
{
    return &hls_source_ops;
}
