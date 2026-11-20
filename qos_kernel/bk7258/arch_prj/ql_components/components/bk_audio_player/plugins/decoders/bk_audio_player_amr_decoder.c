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
#include <limits.h>
#include <stdbool.h>
#include "modules/amr/amrnb/interf_dec.h"
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"
#include "ring_buffer.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

static const char *AMR_MAGIC_NUMBER = "#!AMR\n";
static const int amr_frame[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

#define AMR_FRAMES_PER_SECOND        50
#define AMR_MAX_FRAME_PAYLOAD_BYTES  31
#define AMR_RESYNC_BYTES_LIMIT       4096

static int amr_read_exact(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    int total = 0;

    while (total < len)
    {
        int ret = audio_source_read_data(decoder->source, buffer + total, len - total);
        if (ret > 0)
        {
            total += ret;
            continue;
        }
        if (ret == AUDIO_PLAYER_TIMEOUT)
        {
            return total > 0 ? total : AUDIO_PLAYER_TIMEOUT;
        }
        if (ret == 0 || ret == RB_DONE || ret == RB_ABORT)
        {
            return total > 0 ? total : 0;
        }
        return ret;
    }

    return total;
}


typedef struct amr_decoder_priv
{
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t pcm_sameples;

    void *decoder;
    uint32_t stream_offset;
    uint32_t pending_seek_offset;
    bool stream_offset_valid;
    bool pending_seek_valid;

} amr_decoder_priv_t;


/* Validate and decode AMR frame header (ToC byte) to avoid false sync. */
static bool amr_validate_frame_header(uint8_t toc, int *frame_type, int *payload_bytes)
{
    if ((toc & 0x83) != 0)
    {
        return false;
    }

    int type = (toc >> 3) & 0x0f;
    if (type < 0 || type >= (int)(sizeof(amr_frame) / sizeof(amr_frame[0])))
    {
        return false;
    }

    if ((type > 11) && (type != 15))
    {
        return false;
    }

    int payload = amr_frame[type];
    if ((payload <= 0) && (type != 15))
    {
        return false;
    }

    if (frame_type)
    {
        *frame_type = type;
    }

    if (payload_bytes)
    {
        *payload_bytes = payload;
    }

    return true;
}


static int amr_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    bk_audio_player_decoder_t *decoder;
    amr_decoder_priv_t *priv;

    if (format != AUDIO_FORMAT_AMR)
    {
        return AUDIO_PLAYER_INVALID;
    }

    decoder = audio_codec_new(sizeof(amr_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (amr_decoder_priv_t *)decoder->decoder_priv;

    os_memset(priv, 0x0, sizeof(amr_decoder_priv_t));

    priv->decoder = Decoder_Interface_init();
    if (!priv->decoder)
    {
        player_free(decoder);
        return AUDIO_PLAYER_ERR;
    }

    *decoder_pp = decoder;

    return AUDIO_PLAYER_OK;
}



static int amr_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    int bytes_read;
    amr_decoder_priv_t *priv;
    priv = (amr_decoder_priv_t *)decoder->decoder_priv;

    char amr_header[6 + 1];
    int len;

    len  = strlen(AMR_MAGIC_NUMBER);
    amr_header[len] = 0;

__retry:
    bytes_read = amr_read_exact(decoder, amr_header, len);

    if (bytes_read != len)
    {
        if (bytes_read == AUDIO_PLAYER_TIMEOUT)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "read timeout,try again\r\n");
            rtos_delay_milliseconds(20);
            goto __retry;
        }

        BK_LOGW(AUDIO_PLAYER_TAG, "read amr AMR_MAGIC_NUMBER err! %d:%d.", bytes_read, len);
        return AUDIO_PLAYER_ERR;
    }

    if (strcmp(AMR_MAGIC_NUMBER, amr_header) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "amr magic number error:%s.", amr_header);
        return AUDIO_PLAYER_ERR;
    }

    priv->pcm_sameples = 160;
    priv->sample_rate = info->sample_rate = 8000;
    priv->channels = info->channel_number = 1;
    priv->stream_offset = len;
    priv->stream_offset_valid = true;
    priv->pending_seek_valid = false;

    info->sample_bits = 16;
    info->frame_size = priv->pcm_sameples * info->channel_number * (info->sample_bits / 8);
    info->bps = AUDIO_INFO_UNKNOWN;
    info->total_bytes = AUDIO_INFO_UNKNOWN;
    info->header_bytes = len;
    info->duration = AUDIO_INFO_UNKNOWN;

    decoder->info = *info;

    return AUDIO_PLAYER_OK;
}


int amr_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    amr_decoder_priv_t *priv;
    priv = (amr_decoder_priv_t *)decoder->decoder_priv;

    if (priv->pending_seek_valid)
    {
        priv->stream_offset = priv->pending_seek_offset;
        priv->stream_offset_valid = true;
        priv->pending_seek_valid = false;
    }

    char amr_data[32];
    int amr_size;
    int bytes_read;

    /* get amr frame header. */
    bytes_read = amr_read_exact(decoder, &amr_data[0], 1);

    if (bytes_read != 1)
    {
        if (bytes_read == AUDIO_PLAYER_TIMEOUT)
        {
            return AUDIO_PLAYER_TIMEOUT;
        }
        if (bytes_read == 0 || bytes_read == RB_DONE || bytes_read == RB_ABORT)
        {
            return 0;
        }
        BK_LOGE(AUDIO_PLAYER_TAG, "read amr frame header err, break!");
        priv->stream_offset_valid = false;
        return -1;
    }

    int frame_type = (amr_data[0] >> 3) & 0x0f;
    int quality_bit = (amr_data[0] >> 2) & 0x01;
    int bfi = (quality_bit == 0) ? 1 : 0;

    if (frame_type < 0 || frame_type >= (int)(sizeof(amr_frame) / sizeof(amr_frame[0])))
    {
        frame_type = 15; /* treat as no data */
    }

    amr_size = amr_frame[frame_type];

    if (frame_type == 15)
    {
        bfi = 1;
    }

    bytes_read = amr_read_exact(decoder, &amr_data[1], amr_size);

    if (bytes_read != amr_size)
    {
        if (bytes_read == AUDIO_PLAYER_TIMEOUT || bytes_read > 0)
        {
            return AUDIO_PLAYER_TIMEOUT;
        }
        if (bytes_read == 0 || bytes_read == RB_DONE || bytes_read == RB_ABORT)
        {
            return 0;
        }
        BK_LOGE(AUDIO_PLAYER_TAG, "read amr_size err, %d:%d.", amr_size, bytes_read);
        priv->stream_offset_valid = false;
        return -1;
    }

    Decoder_Interface_Decode(priv->decoder, (const unsigned char *)amr_data, (short *)(buffer), bfi);

    if (priv->channels == 2)
    {
        int i;
        int16_t *ptr = (int16_t *)(buffer);

        for (i = priv->pcm_sameples - 1; i >= 0; i--)
        {
            ptr[i * 2]     = ptr[i];
            ptr[i * 2 + 1] = ptr[i];
        }
    }

    priv->stream_offset += (uint32_t)(1 + amr_size);

    return priv->pcm_sameples * priv->channels * sizeof(int16_t);
}

static int amr_decoder_close(bk_audio_player_decoder_t *decoder)
{
    amr_decoder_priv_t *priv;
    priv = (amr_decoder_priv_t *)decoder->decoder_priv;

    BK_LOGD(AUDIO_PLAYER_TAG, "%s:%d\n", __FUNCTION__, __LINE__);

    if (priv && priv->decoder)
    {
        Decoder_Interface_exit(priv->decoder);
        priv->decoder = NULL;
    }

    return AUDIO_PLAYER_OK;
}


static int amr_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    amr_decoder_priv_t *priv;
    priv = (amr_decoder_priv_t *)decoder->decoder_priv;
    return priv->pcm_sameples * priv->channels * sizeof(int16_t);
}

static int calc_amr_position(bk_audio_player_decoder_t *decoder, int second)
{
    amr_decoder_priv_t *priv = (amr_decoder_priv_t *)decoder->decoder_priv;
    if (!decoder || !decoder->source || !priv)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid decoder handle", __func__);
        return AUDIO_PLAYER_ERR;
    }

    if (!priv->stream_offset_valid)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, stream offset unknown, deny seek", __func__);
        return AUDIO_PLAYER_ERR;
    }

    if (second < 0)
    {
        second = 0;
    }

    uint32_t resume_offset = priv->stream_offset;
    uint32_t data_start = decoder->info.header_bytes;
    if (data_start == 0)
    {
        data_start = strlen(AMR_MAGIC_NUMBER);
    }

    if (resume_offset > (uint32_t)INT32_MAX)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, resume offset overflow:%u", __func__, resume_offset);
        return AUDIO_PLAYER_ERR;
    }

    if (audio_source_seek(decoder->source, (int)data_start, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, seek to data start failed", __func__);
        return AUDIO_PLAYER_ERR;
    }

    /* Convert target time to frame count (AMR NB = 20 ms per frame). */
    uint64_t target_frames = (uint64_t)second * AMR_FRAMES_PER_SECOND;
    uint64_t frames_advanced = 0;
    uint32_t last_valid_offset = data_start;
    uint32_t current_offset = data_start;
    uint32_t invalid_guard = 0;
    char payload_buf[AMR_MAX_FRAME_PAYLOAD_BYTES];
    int result = (int)data_start;

    if (target_frames == 0)
    {
        goto __restore;
    }

    uint32_t total_bytes = audio_source_get_total_bytes(decoder->source);
    bool total_known = (total_bytes != 0U);

    while (frames_advanced < target_frames)
    {
        if (total_known && (current_offset >= total_bytes))
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, reach end before target frame:%llu", __func__, (unsigned long long)target_frames);
            break;
        }

        uint32_t frame_offset = current_offset;
        uint8_t toc = 0;
        int read_ret = audio_source_read_data(decoder->source, (char *)&toc, 1);
        if (read_ret != 1)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, read frame header failed @%u", __func__, frame_offset);
            result = AUDIO_PLAYER_ERR;
            break;
        }

        current_offset = frame_offset + 1;
        int frame_type = 0;
        int payload_bytes = 0;
        if (!amr_validate_frame_header(toc, &frame_type, &payload_bytes))
        {
            invalid_guard++;
            if (invalid_guard >= AMR_RESYNC_BYTES_LIMIT)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, too many invalid frame headers", __func__);
                result = AUDIO_PLAYER_ERR;
                break;
            }
            continue;
        }

        invalid_guard = 0;

        /* Drop payload data while keeping decoder state untouched. */
        if (payload_bytes > 0)
        {
            if (audio_source_read_data(decoder->source, payload_buf, payload_bytes) != payload_bytes)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, read payload failed @%u", __func__, frame_offset);
                result = AUDIO_PLAYER_ERR;
                break;
            }
        }

        current_offset = frame_offset + 1 + payload_bytes;
        last_valid_offset = frame_offset;
        frames_advanced++;
    }

    if (result >= 0)
    {
        if ((uint64_t)last_valid_offset > (uint64_t)INT32_MAX)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, offset overflow:%u", __func__, last_valid_offset);
            result = AUDIO_PLAYER_ERR;
        }
        else
        {
            result = (int)last_valid_offset;
            if (frames_advanced < target_frames)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, seek truncated, frames:%llu/%llu", __func__,
                           (unsigned long long)frames_advanced, (unsigned long long)target_frames);
            }
        }
    }

__restore:
    if (audio_source_seek(decoder->source, (int)resume_offset, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, restore offset failed:%u", __func__, resume_offset);
        priv->stream_offset_valid = false;
        priv->pending_seek_valid = false;
        return AUDIO_PLAYER_ERR;
    }

    priv->stream_offset = resume_offset;
    priv->stream_offset_valid = true;

    if (result >= 0)
    {
        priv->pending_seek_offset = (uint32_t)result;
        priv->pending_seek_valid = true;
    }
    else
    {
        priv->pending_seek_valid = false;
    }

    return result;
}

const bk_audio_player_decoder_ops_t amr_decoder_ops =
{
    .name = "amr",
    .open = amr_decoder_open,
    .get_info = amr_decoder_get_info,
    .get_chunk_size = amr_decoder_get_chunk_size,
    .get_data = amr_decoder_get_data,
    .close = amr_decoder_close,
    .calc_position = calc_amr_position,
    .is_seek_ready = NULL,
};

/* Get AMR decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_amr_decoder_ops(void)
{
    return &amr_decoder_ops;
}
