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
#include "sink_api.h"

#include <fcntl.h>
#include <unistd.h>
#include <bk_posix.h>
#include <stdlib.h>

typedef struct file_sink_priv
{
    int fd;
    int total_size;
} file_sink_priv_t;

static int file_sink_open(audio_sink_type_t sink_type, void *param, bk_audio_player_sink_t **sink_pp)
{
    bk_audio_player_sink_t *sink;
    file_sink_priv_t *priv;

    char *path = (char *)param;

    if (sink_type != AUDIO_SINK_FILE)
    {
        return AUDIO_PLAYER_INVALID;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "file_sink_open : dest path=%s\n", path);

    sink = audio_sink_new(sizeof(file_sink_priv_t));
    if (!sink)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (file_sink_priv_t *)sink->sink_priv;
    priv->fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
    if (priv->fd < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "file_sink_open : can't open %s for write\n", path);
        free(sink);
        return AUDIO_PLAYER_ERR;
    }
    priv->total_size = 0;

    *sink_pp = sink;

    return AUDIO_PLAYER_OK;
}

static int file_sink_close(bk_audio_player_sink_t *sink)
{
    file_sink_priv_t *priv;

    priv = (file_sink_priv_t *)sink->sink_priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "file_sink_close : fd=%d, total_size=%d\n", priv->fd, priv->total_size);

    close(priv->fd);

    return AUDIO_PLAYER_OK;
}

static int file_sink_write(bk_audio_player_sink_t *sink, char *buffer, int len)
{
    file_sink_priv_t *priv;
    int ret;

    priv = (file_sink_priv_t *)sink->sink_priv;

    ret = write(priv->fd, buffer, len);
    if (ret > 0)
    {
        priv->total_size += ret;
    }

    return ret;
}

#undef open
#undef write
#undef close

const bk_audio_player_sink_ops_t file_sink_ops =
{
    .open = file_sink_open,
    .write = file_sink_write,
    .control = NULL,
    .close = file_sink_close,
};

/* Get file sink operations structure */
const bk_audio_player_sink_ops_t *bk_audio_player_get_file_sink_ops(void)
{
    return &file_sink_ops;
}
