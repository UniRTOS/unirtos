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


#ifndef CONFIG_AUDIO_PLAYER_NET_PIPE_SIZE
#define CONFIG_AUDIO_PLAYER_NET_PIPE_SIZE   (8 * 1024)
#endif

#ifndef CONFIG_AUDIO_PLAYER_WEB_RETRY_COUNT
#define CONFIG_AUDIO_PLAYER_WEB_RETRY_COUNT (10)
#endif

#define NET_PIPE_SIZE   CONFIG_AUDIO_PLAYER_NET_PIPE_SIZE
#define NET_CHUNK_SIZE  (128)
#define WEB_RETRY_COUNT CONFIG_AUDIO_PLAYER_WEB_RETRY_COUNT


typedef struct net_source_priv_s
{
    struct webclient_session *session;
    int content_length;
    int codec_type;
    int finish_bytes;

    osal_thread_t tid;
    int runing;
    osal_sema_t thread_sem;
    int result;
    ringbuf_handle_t pipe;
    char *net_buffer;
    int net_buffer_len;
    char *url;
} net_source_priv_t;

static void net_set_socket_timeout(struct webclient_session *session, uint32_t timeout_ms)
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


static void *_net_source_bg_thread(void *param)
{
    bk_audio_player_source_t *source;
    net_source_priv_t *priv;
    int ret;
    int len;
    int pos;
    int retry;

    source = (bk_audio_player_source_t *)param;
    priv = (net_source_priv_t *)source->source_priv;

    retry = WEB_RETRY_COUNT;

    net_set_socket_timeout(priv->session, 500);

    while (priv->runing)
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
                    if (priv->runing)
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
                    if (!priv->session)
                    {
                        BK_LOGE(AUDIO_PLAYER_TAG, "webclient_session_create retry failed\n");
                        priv->result = -1;
                        break;
                    }
                    ret = webclient_get_position(priv->session, priv->url, priv->finish_bytes);
                    if (ret <= 0)
                    {
                        BK_LOGE(AUDIO_PLAYER_TAG, "webclient_get_position retry failed, ret=%d\n", ret);
                        priv->result = -1;
                        break;
                    }
                }
                else
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "retry without valid webclient session\n");
                    priv->result = -1;
                    break;
                }
                continue;
            }
            if (priv->content_length > priv->finish_bytes)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "retry exhausted, content_length=%d, actual_length=%d\n", priv->content_length, priv->finish_bytes);
                priv->result = -1;
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

    /* Distinguish a clean end-of-stream from an aborted/incomplete download so the
     * decoder can decide whether to decode the tail (RB_DONE) or drop a truncated
     * residual frame (RB_ABORT). */
    int stream_complete;
    if (priv->content_length > 0)
    {
        stream_complete = (priv->finish_bytes >= priv->content_length) ? 1 : 0;
    }
    else
    {
        stream_complete = (priv->result != -1) ? 1 : 0;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s thread exit, complete=%d, result=%d, content_length=%d, finish_bytes=%d\n",
            __func__, stream_complete, priv->result, priv->content_length, priv->finish_bytes);

    if (stream_complete)
    {
        rb_done_write(priv->pipe);
    }
    else
    {
        rb_abort(priv->pipe);
    }
    osal_post_sema(&priv->thread_sem);
    osal_delete_thread(NULL);
    return NULL;
}

static int net_source_start_worker(bk_audio_player_source_t *source)
{
    int ret;
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;

    priv->net_buffer_len = NET_CHUNK_SIZE;
    priv->net_buffer = player_malloc(priv->net_buffer_len);
    if (!priv->net_buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "net_source_start_worker : can't malloc net_buffer\n");
        return AUDIO_PLAYER_ERR;
    }

    priv->pipe = rb_create(NET_PIPE_SIZE);
    if (!priv->pipe)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "net_source_start_worker : can't create pipe\n");
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
        return AUDIO_PLAYER_ERR;
    }

    osal_init_sema(&priv->thread_sem, 1, 0);

    priv->runing = 1;

    ret = osal_create_thread(&priv->tid, (osal_thread_func)_net_source_bg_thread, 4096, "net", source, BEKEN_DEFAULT_WORKER_PRIORITY);
    if (ret)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "net_source_start_worker : can't create thread\n");
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
        rb_destroy(priv->pipe);
        priv->pipe = NULL;
        priv->runing = 0;
        return AUDIO_PLAYER_ERR;
    }
    else
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "net_source_start_worker : create thread ok\n");
        return AUDIO_PLAYER_OK;
    }
}

static int net_source_stop_worker(bk_audio_player_source_t *source)
{
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;

    priv->runing = 0;
    osal_wait_sema(&priv->thread_sem, BEKEN_WAIT_FOREVER);
    osal_deinit_sema(&priv->thread_sem);

    if (priv->net_buffer)
    {
        player_free(priv->net_buffer);
        priv->net_buffer = NULL;
    }
    rb_destroy(priv->pipe);
    return AUDIO_PLAYER_OK;
}


static int net_source_open(char *url, bk_audio_player_source_t **source_pp)
{
    bk_audio_player_source_t *source;
    net_source_priv_t *priv;
    int code;
    char *ext_name;
    char *mime;
    int ret;

    if (strncmp(url, "http", 4) != 0
        || strncmp(&url[os_strlen(url) - 6], ".m3u8", 5) == 0
        || strncmp(&url[os_strlen(url) - 5], ".m3u", 4) == 0
        || strncmp(&url[os_strlen(url) - 5], "m3u8", 4) == 0
        || strncmp(&url[os_strlen(url) - 4], "m3u", 3) == 0)
    {
        return AUDIO_PLAYER_INVALID;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "net_source_open : %s\n", url);
    source = audio_source_new(sizeof(net_source_priv_t));
    if (!source)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (net_source_priv_t *)source->source_priv;

    priv->finish_bytes = 0;
    priv->session = webclient_session_create(2048);
    if (!priv->session)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "webclient_session_create() for %s failed\n", url);
        player_free(source);
        source = NULL;
        return AUDIO_PLAYER_ERR;
    }

    priv->url = player_strdup(url);

    code = webclient_get(priv->session, (const char *)url);
    if (code != 200)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "webclient_get() for %s failed, code=%d\n", url, code);
        if (priv->session)
        {
            webclient_close(priv->session);
            priv->session = NULL;
        }
        player_free(source);
        source = NULL;
        return AUDIO_PLAYER_ERR;
    }
    priv->content_length = webclient_content_length_get(priv->session);
    mime = (char *)webclient_header_fields_get(priv->session, "Content-Type");

    ext_name = os_strrchr(url, '.');
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
            source = NULL;
            return AUDIO_PLAYER_ERR;
        }
    }

    ret = net_source_start_worker(source);
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

static int net_source_close(bk_audio_player_source_t *source)
{
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;
    BK_LOGI(AUDIO_PLAYER_TAG, "net_source_close\n");

    net_source_stop_worker(source);

    if (priv->session)
    {
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

static int net_source_get_codec_type(bk_audio_player_source_t *source)
{
    net_source_priv_t *priv;

    priv = (net_source_priv_t *)source->source_priv;

    return priv->codec_type;
}

static uint32_t net_source_get_total_bytes(bk_audio_player_source_t *source)
{
    net_source_priv_t *priv;

    if (!source)
    {
        return 0;
    }

    priv = (net_source_priv_t *)source->source_priv;
    if (!priv || priv->content_length <= 0)
    {
        return 0;
    }

    return (uint32_t)priv->content_length;
}

/*
#define NET_READ_RETRY 3
static int net_source_read_direct(bk_audio_player_source_t *source, char *buffer, int len)
{
    int ret;
    net_source_priv_t *priv;
    int retry = NET_READ_RETRY;

    priv = (net_source_priv_t *)source->source_priv;

    while(retry-- > 0) {
        ret = webclient_read(priv->session, buffer, len);
        if (ret == -WEBCLIENT_TIMEOUT) {
            continue;
        } else if (ret > 0) {
            priv->finish_bytes += ret;
            break;
        } else if (ret == 0) {
            BK_LOGI(AUDIO_PLAYER_TAG, "download finished, content_length=%d, actual_length=%d\n", priv->content_length, priv->finish_bytes);
            break;
        } else {
            BK_LOGE(AUDIO_PLAYER_TAG, "net_source_read failed, ret = %d\n", ret);
            break;
        }
    }

    //BK_LOGI(AUDIO_PLAYER_TAG, "net_source_read : len=%d, actual=%d\n", len, ret);

    return ret;
}
*/

static int net_source_read(bk_audio_player_source_t *source, char *buffer, int len)
{
    int ret;
    net_source_priv_t *priv;
    priv = (net_source_priv_t *)source->source_priv;

    ret = rb_read(priv->pipe, buffer, len, 300 / portTICK_RATE_MS);
    if (ret == RB_TIMEOUT)
    {
        return AUDIO_PLAYER_TIMEOUT;
    }

    return ret;
}

static int net_source_seek(bk_audio_player_source_t *source, int offset, uint32_t whence)
{
    net_source_priv_t *priv;
    priv = (net_source_priv_t *)source->source_priv;
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
            if (offset < 0)
            {
                seek_offset = priv->content_length + offset;
            }
            else
            {
                seek_offset = priv->content_length - offset;
            }
        }
    }

    if (seek_offset > priv->content_length)
    {
        seek_offset = priv->content_length;
    }

    BK_LOGD(AUDIO_PLAYER_TAG, "seek offset:%d,offset:%d,whence:%d\r\n", seek_offset, offset, whence);

    if (!priv->runing)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "no runing seek return \r\n");
        return AUDIO_PLAYER_ERR;
    }

    net_source_stop_worker(source);

    if (priv->session)
    {
        webclient_close(priv->session);
        priv->session = webclient_session_create(2048);
        priv->finish_bytes =  seek_offset;
        webclient_get_position(priv->session, priv->url, seek_offset);
    }

    ret = net_source_start_worker(source);
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

const bk_audio_player_source_ops_t net_source_ops =
{
    .open = net_source_open,
    .get_codec_type = net_source_get_codec_type,
    .get_total_bytes = net_source_get_total_bytes,
    .read = net_source_read,
    .seek = net_source_seek,
    .close = net_source_close,
};

/* Get HTTP network source operations structure */
const bk_audio_player_source_ops_t *bk_audio_player_get_net_source_ops(void)
{
    return &net_source_ops;
}
