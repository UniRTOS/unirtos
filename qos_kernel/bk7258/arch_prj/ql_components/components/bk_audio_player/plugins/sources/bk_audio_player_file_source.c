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
#include "ring_buffer.h"

#include <fcntl.h>
#include <unistd.h>
#include <bk_posix.h>

typedef struct file_source_priv_s
{
    int fd;
    uint32_t file_size;
    int codec_type;
} file_source_priv_t;

static int file_source_open(char *url, bk_audio_player_source_t **source_pp)
{
    bk_audio_player_source_t *source;
    file_source_priv_t *priv;
    char *file_name;
    char *ext_name;

    if (strncmp(url, "file://", 7) == 0)
    {
        file_name = url + 7;
    }
    else if (url[0] == '/')
    {
        file_name = url;
    }
    else if (strncmp(url, "http://", 7) == 0)
    {
        return AUDIO_PLAYER_INVALID;
    }
    else if (strncmp(url, "https://", 7) == 0)
    {
        return AUDIO_PLAYER_INVALID;
    }
    else
    {
        file_name = url;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "file_source_open : %s\n", file_name);
    source = audio_source_new(sizeof(file_source_priv_t));
    if (!source)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (file_source_priv_t *)source->source_priv;

    ext_name = strrchr(url, '.');
    priv->codec_type = audio_codec_get_type(ext_name);
    if (priv->codec_type == AUDIO_FORMAT_UNKNOWN)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, media with ext: %s not support, %d\n", __func__, ext_name, __LINE__);
        player_free(source);
        source = NULL;
        return AUDIO_PLAYER_ERR;
    }

    priv->fd = open(file_name, O_RDONLY);
    if (priv->fd < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, can't open: %s, %d\n", __func__, file_name, __LINE__);
        player_free(source);
        source = NULL;
        return AUDIO_PLAYER_ERR;
    }

    priv->file_size = (uint32_t)lseek(priv->fd, 0, SEEK_END);
    BK_LOGI(AUDIO_PLAYER_TAG, "%s file_size: %d\n", file_name, priv->file_size);
    lseek(priv->fd, 0, SEEK_SET);

    *source_pp = source;

    return AUDIO_PLAYER_OK;
}

static int file_source_close(bk_audio_player_source_t *source)
{
    file_source_priv_t *priv;

    priv = (file_source_priv_t *)source->source_priv;
    BK_LOGI(AUDIO_PLAYER_TAG, "file_source_close : fd=%d\n", priv->fd);
    close(priv->fd);

    return AUDIO_PLAYER_OK;
}

static int file_source_get_codec_type(bk_audio_player_source_t *source)
{
    file_source_priv_t *priv;

    priv = (file_source_priv_t *)source->source_priv;

    return priv->codec_type;
}

static uint32_t file_source_get_total_bytes(bk_audio_player_source_t *source)
{
    file_source_priv_t *priv;

    priv = (file_source_priv_t *)source->source_priv;

    return priv->file_size;
}

static int file_source_read(bk_audio_player_source_t *source, char *buffer, int len)
{
    int ret;
    //int offset;
    file_source_priv_t *priv;

    priv = (file_source_priv_t *)source->source_priv;

    //offset = lseek(priv->fd,0,SEEK_CUR);
    ret = read(priv->fd, buffer, len);
    //bk_printf("file read ret:%d offset:%d\r\n",ret,offset);
    //BK_LOGI(AUDIO_PLAYER_TAG, "file_source_read : len=%d, actual=%d\n", len, ret);

    /* read() == 0 is a normal EOF (decoder keeps RB_DONE semantics and decodes the
     * tail). A negative result is an actual read error (e.g. FS/SD failure): surface
     * it as RB_ABORT so the decoder treats the residual as a truncated stream. */
    if (ret < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, read error: %d, %d\n", __func__, ret, __LINE__);
        return RB_ABORT;
    }

    return ret;
}

static int file_source_seek(bk_audio_player_source_t *source, int offset, uint32_t whence)
{
    file_source_priv_t *priv;

    priv = (file_source_priv_t *)source->source_priv;

    lseek(priv->fd, offset, whence);

    return AUDIO_PLAYER_OK;
}

#undef open
#undef read
#undef seek
#undef close

const bk_audio_player_source_ops_t file_source_ops =
{
    .open = file_source_open,
    .get_codec_type = file_source_get_codec_type,
    .get_total_bytes = file_source_get_total_bytes,
    .read = file_source_read,
    .seek = file_source_seek,
    .close = file_source_close,
};

/* Get file source operations structure */
const bk_audio_player_source_ops_t *bk_audio_player_get_file_source_ops(void)
{
    return &file_source_ops;
}
