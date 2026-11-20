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


#include <os/mem.h>
#include <string.h>
#include <unistd.h>
#include "bk_audio_player_opus_stream_decoder.h"
#include "bk_audio_player_vorbis_stream_decoder.h"
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"


#define OGG_MAX_PAGE_SIZE                  (64 * 1024)
#define OGG_PAGE_HEADER_BYTES              (27)
#define OGG_READ_RETRY                     (5)

typedef enum
{
    OGG_INNER_UNKNOWN = 0,
    OGG_INNER_OPUS,
    OGG_INNER_VORBIS,
} ogg_inner_type_t;

typedef struct
{
    int (*open)(audio_format_t codec_type,
                audio_format_t expected_type,
                void *param,
                bk_audio_player_decoder_t **decoder_pp);
    int (*get_info)(bk_audio_player_decoder_t *codec, audio_info_t *info);
    int (*get_chunk_size)(bk_audio_player_decoder_t *codec);
    int (*get_data)(bk_audio_player_decoder_t *codec, char *buffer, int len);
    int (*close)(bk_audio_player_decoder_t *codec);
    int (*calc_position)(bk_audio_player_decoder_t *codec, int second);
    int (*is_seek_ready)(bk_audio_player_decoder_t *codec);
} ogg_inner_dispatch_t;

typedef struct
{
    bk_audio_player_source_t *orig;
    uint8_t        *prefetch;
    size_t          prefetch_len;
    size_t          prefetch_pos;
} ogg_prefetch_source_priv_t;

static int ogg_prefetch_get_codec_type(bk_audio_player_source_t *source)
{
    ogg_prefetch_source_priv_t *priv = (ogg_prefetch_source_priv_t *)source->source_priv;
    if (priv->orig && priv->orig->ops->get_codec_type)
    {
        return priv->orig->ops->get_codec_type(priv->orig);
    }
    return AUDIO_FORMAT_UNKNOWN;
}

static uint32_t ogg_prefetch_get_total_bytes(bk_audio_player_source_t *source)
{
    ogg_prefetch_source_priv_t *priv = (ogg_prefetch_source_priv_t *)source->source_priv;
    if (priv->orig && priv->orig->ops->get_total_bytes)
    {
        return priv->orig->ops->get_total_bytes(priv->orig);
    }
    return 0;
}

static int ogg_prefetch_read(bk_audio_player_source_t *source, char *buffer, int len)
{
    ogg_prefetch_source_priv_t *priv = (ogg_prefetch_source_priv_t *)source->source_priv;
    int total = 0;

    while (len > 0 && priv->prefetch && priv->prefetch_pos < priv->prefetch_len)
    {
        size_t remain = priv->prefetch_len - priv->prefetch_pos;
        size_t chunk = remain > (size_t)len ? (size_t)len : remain;
        os_memcpy(buffer + total, priv->prefetch + priv->prefetch_pos, chunk);
        priv->prefetch_pos += chunk;
        len -= (int)chunk;
        total += (int)chunk;
        if (priv->prefetch_pos >= priv->prefetch_len)
        {
            player_free(priv->prefetch);
            priv->prefetch = NULL;
            priv->prefetch_len = 0;
            priv->prefetch_pos = 0;
        }
    }

    if (len > 0 && priv->orig)
    {
        int ret = audio_source_read_data(priv->orig, buffer + total, len);
        if (ret > 0)
        {
            total += ret;
        }
        else
        {
            return (total > 0) ? total : ret;
        }
    }

    return total;
}

static int ogg_prefetch_seek(bk_audio_player_source_t *source, int offset, uint32_t whence)
{
    ogg_prefetch_source_priv_t *priv = (ogg_prefetch_source_priv_t *)source->source_priv;
    if (!priv->orig || !priv->orig->ops->seek)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (priv->prefetch)
    {
        player_free(priv->prefetch);
        priv->prefetch = NULL;
        priv->prefetch_len = 0;
        priv->prefetch_pos = 0;
    }

    return priv->orig->ops->seek(priv->orig, offset, whence);
}

static int ogg_prefetch_close(bk_audio_player_source_t *source)
{
    ogg_prefetch_source_priv_t *priv = (ogg_prefetch_source_priv_t *)source->source_priv;
    if (priv->prefetch)
    {
        player_free(priv->prefetch);
        priv->prefetch = NULL;
    }
    priv->prefetch_len = 0;
    priv->prefetch_pos = 0;
    priv->orig = NULL;
    return AUDIO_PLAYER_OK;
}

static const bk_audio_player_source_ops_t ogg_prefetch_source_ops =
{
    .open = NULL,
    .get_codec_type = ogg_prefetch_get_codec_type,
    .get_total_bytes = ogg_prefetch_get_total_bytes,
    .read = ogg_prefetch_read,
    .seek = ogg_prefetch_seek,
    .close = ogg_prefetch_close,
};

typedef struct
{
    ogg_inner_type_t            type;
    bk_audio_player_decoder_t   *inner_decoder;
    const ogg_inner_dispatch_t *inner_ops;
    bk_audio_player_source_t             *wrapped_source;
    uint8_t                    *prefetch_buf;
    size_t                      prefetch_len;
} ogg_decoder_priv_t;

static bk_audio_player_source_t *ogg_create_prefetch_source(bk_audio_player_source_t *orig, uint8_t *buf, size_t len)
{
    bk_audio_player_source_t *wrapper = audio_source_new(sizeof(ogg_prefetch_source_priv_t));
    if (!wrapper)
    {
        return NULL;
    }

    ogg_prefetch_source_priv_t *priv = (ogg_prefetch_source_priv_t *)wrapper->source_priv;
    os_memset(priv, 0, sizeof(*priv));
    priv->orig = orig;
    priv->prefetch = buf;
    priv->prefetch_len = len;
    priv->prefetch_pos = 0;

    wrapper->ops = (bk_audio_player_source_ops_t *)&ogg_prefetch_source_ops;
    return wrapper;
}

static int ogg_read_exact(bk_audio_player_source_t *source, uint8_t *buffer, size_t bytes)
{
    size_t total = 0;

    while (total < bytes)
    {
        int ret = audio_source_read_data(source, (char *)buffer + total, (int)(bytes - total));
        if (ret == AUDIO_PLAYER_TIMEOUT)
        {
            rtos_delay_milliseconds(20);
            continue;
        }
        if (ret <= 0)
        {
            return AUDIO_PLAYER_ERR;
        }
        total += (size_t)ret;
    }

    return AUDIO_PLAYER_OK;
}

static int ogg_capture_first_page(bk_audio_player_decoder_t *decoder,
                                  ogg_decoder_priv_t *priv,
                                  const uint8_t **packet,
                                  size_t *packet_len)
{
    if (!decoder->source)
    {
        return AUDIO_PLAYER_ERR;
    }

    uint8_t header[OGG_PAGE_HEADER_BYTES];
    int ret = ogg_read_exact(decoder->source, header, sizeof(header));
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    if (memcmp(header, "OggS", 4) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ogg: invalid capture pattern\n");
        return AUDIO_PLAYER_ERR;
    }

    uint8_t segment_count = header[26];
    if (segment_count == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ogg: invalid segment count\n");
        return AUDIO_PLAYER_ERR;
    }

    if (segment_count > 255)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ogg: segment count overflow\n");
        return AUDIO_PLAYER_ERR;
    }

    uint8_t laces[255];
    ret = ogg_read_exact(decoder->source, laces, segment_count);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    size_t body_len = 0;
    for (int i = 0; i < segment_count; i++)
    {
        body_len += laces[i];
    }

    size_t total_len = (size_t)OGG_PAGE_HEADER_BYTES + segment_count + body_len;
    if (total_len > OGG_MAX_PAGE_SIZE)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ogg: page too large (%u)\n", (unsigned int)total_len);
        return AUDIO_PLAYER_ERR;
    }

    uint8_t *buffer = player_malloc(total_len);
    if (!buffer)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    os_memcpy(buffer, header, OGG_PAGE_HEADER_BYTES);
    os_memcpy(buffer + OGG_PAGE_HEADER_BYTES, laces, segment_count);

    if (body_len > 0)
    {
        ret = ogg_read_exact(decoder->source, buffer + OGG_PAGE_HEADER_BYTES + segment_count, body_len);
        if (ret != AUDIO_PLAYER_OK)
        {
            player_free(buffer);
            return ret;
        }
    }

    priv->prefetch_buf = buffer;
    priv->prefetch_len = total_len;

    const uint8_t *body = buffer + OGG_PAGE_HEADER_BYTES + segment_count;
    size_t first_packet_len = 0;
    bool packet_complete = false;
    for (int i = 0; i < segment_count; i++)
    {
        first_packet_len += laces[i];
        if (laces[i] < 255)
        {
            packet_complete = true;
            break;
        }
    }

    if (!packet_complete)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ogg: BOS packet spans multiple pages, unsupported\n");
        return AUDIO_PLAYER_ERR;
    }

    *packet = body;
    *packet_len = first_packet_len;
    return AUDIO_PLAYER_OK;
}

static const ogg_inner_dispatch_t ogg_inner_opus_ops =
{
    .open = bk_opus_stream_decoder_open,
    .get_info = bk_opus_stream_decoder_get_info,
    .get_chunk_size = bk_opus_stream_decoder_get_chunk_size,
    .get_data = bk_opus_stream_decoder_get_data,
    .close = bk_opus_stream_decoder_close,
    .calc_position = bk_opus_stream_calc_position,
    .is_seek_ready = bk_opus_stream_is_seek_ready,
};

static const ogg_inner_dispatch_t ogg_inner_vorbis_ops =
{
    .open = bk_vorbis_stream_decoder_open,
    .get_info = bk_vorbis_stream_decoder_get_info,
    .get_chunk_size = bk_vorbis_stream_decoder_get_chunk_size,
    .get_data = bk_vorbis_stream_decoder_get_data,
    .close = bk_vorbis_stream_decoder_close,
    .calc_position = bk_vorbis_stream_calc_position,
    .is_seek_ready = bk_vorbis_stream_is_seek_ready,
};

static int ogg_decoder_prepare_inner(bk_audio_player_decoder_t *decoder, ogg_decoder_priv_t *priv)
{
    if (priv->inner_decoder)
    {
        return AUDIO_PLAYER_OK;
    }

    const uint8_t *packet = NULL;
    size_t packet_len = 0;
    int ret = ogg_capture_first_page(decoder, priv, &packet, &packet_len);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    ogg_inner_type_t type = OGG_INNER_UNKNOWN;
    if (packet_len >= 8 && memcmp(packet, "OpusHead", 8) == 0)
    {
        type = OGG_INNER_OPUS;
    }
    else if (packet_len >= 7 && packet[0] == 0x01 && memcmp(packet + 1, "vorbis", 6) == 0)
    {
        type = OGG_INNER_VORBIS;
    }

    if (type == OGG_INNER_UNKNOWN)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ogg: unsupported BOS packet\n");
        player_free(priv->prefetch_buf);
        priv->prefetch_buf = NULL;
        priv->prefetch_len = 0;
        return AUDIO_PLAYER_ERR;
    }

    const ogg_inner_dispatch_t *dispatch = (type == OGG_INNER_OPUS) ?
                                           &ogg_inner_opus_ops :
                                           &ogg_inner_vorbis_ops;

    bk_audio_player_source_t *wrapper = ogg_create_prefetch_source(decoder->source,
                                                         priv->prefetch_buf,
                                                         priv->prefetch_len);
    if (!wrapper)
    {
        player_free(priv->prefetch_buf);
        priv->prefetch_buf = NULL;
        priv->prefetch_len = 0;
        return AUDIO_PLAYER_NO_MEM;
    }

    bk_audio_player_decoder_t *inner = NULL;
    ret = dispatch->open(AUDIO_FORMAT_OGG, AUDIO_FORMAT_OGG, NULL, &inner);
    if (ret != AUDIO_PLAYER_OK)
    {
        audio_source_close(wrapper);
        player_free(priv->prefetch_buf);
        priv->prefetch_buf = NULL;
        priv->prefetch_len = 0;
        return ret;
    }

    inner->source = wrapper;

    priv->type = type;
    priv->inner_decoder = inner;
    priv->inner_ops = dispatch;
    priv->wrapped_source = wrapper;
    priv->prefetch_buf = NULL;
    priv->prefetch_len = 0;

    BK_LOGI(AUDIO_PLAYER_TAG, "ogg: detected %s stream\n",
               (type == OGG_INNER_OPUS) ? "Opus" : "Vorbis");
    return AUDIO_PLAYER_OK;
}

static int ogg_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    (void)param;

    if (format != AUDIO_FORMAT_OGG)
    {
        return AUDIO_PLAYER_INVALID;
    }

    bk_audio_player_decoder_t *decoder = audio_codec_new(sizeof(ogg_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    ogg_decoder_priv_t *priv = (ogg_decoder_priv_t *)decoder->decoder_priv;
    os_memset(priv, 0, sizeof(*priv));

    *decoder_pp = decoder;
    return AUDIO_PLAYER_OK;
}

static int ogg_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    if (!decoder || !info)
    {
        return AUDIO_PLAYER_ERR;
    }

    ogg_decoder_priv_t *priv = (ogg_decoder_priv_t *)decoder->decoder_priv;
    int ret = ogg_decoder_prepare_inner(decoder, priv);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    ret = priv->inner_ops->get_info(priv->inner_decoder, info);
    if (ret == AUDIO_PLAYER_OK)
    {
        decoder->info = *info;
    }
    return ret;
}

static int ogg_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    ogg_decoder_priv_t *priv = (ogg_decoder_priv_t *)decoder->decoder_priv;
    int ret = ogg_decoder_prepare_inner(decoder, priv);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }
    return priv->inner_ops->get_data(priv->inner_decoder, buffer, len);
}

static int ogg_decoder_close(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return AUDIO_PLAYER_OK;
    }

    ogg_decoder_priv_t *priv = (ogg_decoder_priv_t *)decoder->decoder_priv;

    if (priv->inner_decoder && priv->inner_ops && priv->inner_ops->close)
    {
        priv->inner_ops->close(priv->inner_decoder);
    }
    if (priv->inner_decoder)
    {
        player_free(priv->inner_decoder);
        priv->inner_decoder = NULL;
    }

    if (priv->wrapped_source)
    {
        audio_source_close(priv->wrapped_source);
        priv->wrapped_source = NULL;
    }

    if (priv->prefetch_buf)
    {
        player_free(priv->prefetch_buf);
        priv->prefetch_buf = NULL;
    }
    priv->prefetch_len = 0;
    priv->inner_ops = NULL;
    priv->type = OGG_INNER_UNKNOWN;

    return AUDIO_PLAYER_OK;
}

static int ogg_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    ogg_decoder_priv_t *priv = (ogg_decoder_priv_t *)decoder->decoder_priv;
    int ret = ogg_decoder_prepare_inner(decoder, priv);
    if (ret != AUDIO_PLAYER_OK)
    {
        return DEFAULT_CHUNK_SIZE;
    }
    return priv->inner_ops->get_chunk_size(priv->inner_decoder);
}

static int ogg_decoder_calc_position(bk_audio_player_decoder_t *decoder, int second)
{
    ogg_decoder_priv_t *priv = (ogg_decoder_priv_t *)decoder->decoder_priv;
    int ret = ogg_decoder_prepare_inner(decoder, priv);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }
    if (priv->inner_ops->calc_position)
    {
        int seek_offset = priv->inner_ops->calc_position(priv->inner_decoder, second);
        if (seek_offset >= 0 && priv->wrapped_source && priv->wrapped_source->ops->seek)
        {
            int seek_ret = priv->wrapped_source->ops->seek(priv->wrapped_source, seek_offset, SEEK_SET);
            if (seek_ret != AUDIO_PLAYER_OK)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "ogg: wrapped source seek failed (%d)\n", seek_ret);
            }
        }
        return seek_offset;
    }
    return AUDIO_PLAYER_ERR;
}

static int ogg_decoder_is_seek_ready(bk_audio_player_decoder_t *decoder)
{
    ogg_decoder_priv_t *priv = (ogg_decoder_priv_t *)decoder->decoder_priv;
    int ret = ogg_decoder_prepare_inner(decoder, priv);
    if (ret != AUDIO_PLAYER_OK)
    {
        return 0;
    }
    if (priv->inner_ops->is_seek_ready)
    {
        return priv->inner_ops->is_seek_ready(priv->inner_decoder);
    }
    return 0;
}

const bk_audio_player_decoder_ops_t ogg_decoder_ops =
{
    .name = "ogg",
    .open = ogg_decoder_open,
    .get_info = ogg_decoder_get_info,
    .get_chunk_size = ogg_decoder_get_chunk_size,
    .get_data = ogg_decoder_get_data,
    .close = ogg_decoder_close,
    .calc_position = ogg_decoder_calc_position,
    .is_seek_ready = ogg_decoder_is_seek_ready,
};

/* Get OGG decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_ogg_decoder_ops(void)
{
    return &ogg_decoder_ops;
}
