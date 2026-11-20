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

#include <modules/ts_format.h>
#include <modules/aacdec.h>
#include <stdbool.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"
#include "ring_buffer.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

#define AAC_AUDIO_BUF_SZ    (AAC_MAINBUF_SIZE) /* feel free to change this, but keep big enough for >= one frame(AAC_MAINBUF_SIZE) at high bitrates */

typedef struct ts_decoder_priv
{
    HAACDecoder decoder;
    AACFrameInfo aacFrameInfo;

    uint32_t frames;
    uint32_t stream_offset;

    ts_stream_t ts_stream;

    /* aac read session */
    uint8_t *read_buffer;
    uint8_t *read_ptr;
    uint32_t bytes_left;
    bool stream_done;

    int current_sample_rate;

    float T1;
} ts_decoder_priv_t;

/* Thread-local source for _read callback; avoids conflict when multiple instances use TS decoder */
static __thread bk_audio_player_source_t *ts_decoder_tl_source = NULL;

static void *_malloc_memory(uint32_t size)
{
    return player_malloc(size);
}

static void _free_memory(void *ptr)
{
    return player_free(ptr);
}

static int _read(void *buffer, uint32_t length)
{
    if (ts_decoder_tl_source == NULL || buffer == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, ts_decoder_tl_source or buffer is NULL, %d \n", __func__, __LINE__);
        return -1;
    }

    return audio_source_read_data(ts_decoder_tl_source, buffer, length);
}

static ts_format_osi_funcs_t ts_format_osi_funcs =
{
    .malloc_memory = _malloc_memory,
    .free_memory = _free_memory,
    .read = _read,
};


static int32_t ts_fill_buffer(bk_audio_player_decoder_t *decoder)
{
    int bytes_read;
    size_t bytes_to_read;

    ts_decoder_priv_t *priv = (ts_decoder_priv_t *)decoder->decoder_priv;

    if (priv->stream_done)
    {
        return RB_DONE;
    }

    /* Set thread-local source so _read callback uses this instance's source */
    ts_decoder_tl_source = decoder->source;

    /* adjust read ptr */
    if (priv->bytes_left > 0xffff0000)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "c: 0x%x, buf: 0x%x.\n", priv, priv->read_buffer);
        BK_LOGW(AUDIO_PLAYER_TAG, "rd: 0x%x, left: 0x%x.\n", priv->read_ptr, priv->bytes_left);
        return (-1);
    }

    if (priv->bytes_left > 0)
    {
        memmove(priv->read_buffer, priv->read_ptr, priv->bytes_left);
    }
    priv->read_ptr = priv->read_buffer;

    //    bytes_to_read = (AAC_AUDIO_BUF_SZ - priv->bytes_left) & ~(512 - 1);
    bytes_to_read = AAC_AUDIO_BUF_SZ - priv->bytes_left;
    //    BK_LOGI(AUDIO_PLAYER_TAG,"need size: %d \n", bytes_to_read);

    bytes_read = ts_format_stream_read_aac_data(&priv->ts_stream, priv->read_buffer + priv->bytes_left, bytes_to_read);
    if (bytes_read > 0)
    {
        priv->bytes_left = priv->bytes_left + bytes_read;
        priv->stream_done = false;
        return 0;
    }
    else if (bytes_read == AUDIO_PLAYER_TIMEOUT)
    {
        return AUDIO_PLAYER_TIMEOUT;
    }
    else if (bytes_read == 0 || bytes_read == RB_DONE)
    {
        priv->stream_done = true;
        if (priv->bytes_left != 0)
        {
            return 0;
        }

        BK_LOGW(AUDIO_PLAYER_TAG, "can't read more data, end of stream. left=%d \n", priv->bytes_left);
        return RB_DONE;
    }
    else if (priv->bytes_left != 0)
    {
        return 0;
    }

    BK_LOGW(AUDIO_PLAYER_TAG, "can't read more data, ret=%d, left=%d \n", bytes_read, priv->bytes_left);
    return -1;
}

#if 0
static uint32_t data_seek_aac(void *handle, uint32_t pos, uint32_t whence)
{
    bk_audio_player_decoder_t *decoder = (bk_audio_player_decoder_t *)handle;

    audio_source_seek(decoder->source, pos, whence);

    return 0;
}
#endif

/* not support */
//TODO
static int calc_ts_position(bk_audio_player_decoder_t *decoder, int second)
{
    //TODO
    ts_decoder_priv_t *priv;
    priv = (ts_decoder_priv_t *)decoder->decoder_priv;

    AACFlushCodec(priv->decoder);

    //    uint32_t offset = aac_decoder_position_to_offset(priv->decoder, second*1000);

    return -1;
}


static int ts_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    bk_audio_player_decoder_t *decoder;
    ts_decoder_priv_t *priv;

    if (format != AUDIO_FORMAT_TS)
    {
        return AUDIO_PLAYER_INVALID;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "open ts decoder \n");

    decoder = audio_codec_new(sizeof(ts_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (ts_decoder_priv_t *)decoder->decoder_priv;

    os_memset(priv, 0x0, sizeof(ts_decoder_priv_t));

    /* init read session */
    priv->read_buffer = NULL;
    priv->read_ptr = NULL;
    priv->bytes_left = 0;
    priv->frames = 0;
    priv->stream_offset = 0;
    priv->current_sample_rate = 0;

    priv->read_buffer = player_malloc(AAC_AUDIO_BUF_SZ);
    if (priv->read_buffer == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "priv->read_buffer malloc fail \n");
        return AUDIO_PLAYER_ERR;
    }

    /* init ts_format_osi_funcs and ts_format parse */
    ts_format_osi_funcs_init(&ts_format_osi_funcs);
    if (0 != ts_format_stream_init(&priv->ts_stream))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, ts_format_stream_init fail, %d \n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    /* set aac decoder use psram memory */
    //AACSetMemType(AAC_MEM_TYPE_PSRAM);
    priv->decoder = AACInitDecoder();
    if (!priv->decoder)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "priv->decoder create fail \n");
        return AUDIO_PLAYER_ERR;
    }

    //BK_LOGI(AUDIO_PLAYER_TAG,"aac decoder ptr: %p \n", priv->decoder);

    *decoder_pp = decoder;

    return AUDIO_PLAYER_OK;
}


static int ts_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    int ec;
    ts_decoder_priv_t *priv;
    priv = (ts_decoder_priv_t *)decoder->decoder_priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (decoder->source == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, decoder->source is NULL, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    /* ts_fill_buffer uses ts_decoder_tl_source (set inside ts_fill_buffer) */
    AACFrameInfo aacFrameInfo = {0};
    os_memset(&aacFrameInfo, 0, sizeof(AACFrameInfo));

    /* number of output samples = 1024 per channel (2048 if SBR enabled) * 16bit * channels */
    short *sample_buffer = player_malloc(AAC_MAX_NSAMPS * 2 * 2 * AAC_MAX_NCHANS);
    if (!sample_buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc sample_buffer: %d fail, %d\n", __func__, AAC_MAX_NSAMPS * AAC_MAX_NCHANS * 2 * 2, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

__retry:
    if ((priv->read_ptr == NULL) || priv->bytes_left < AAC_MAINBUF_SIZE)
    {
        int fill_ret = ts_fill_buffer(decoder);
        if (fill_ret == AUDIO_PLAYER_TIMEOUT)
        {
            rtos_delay_milliseconds(5);
            goto __retry;
        }
        if (fill_ret == RB_DONE)
        {
            /* fall through to the short-buffer EOF check below */
        }
        else if (fill_ret != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, ts_fill_buffer fail, %d\n", __func__, __LINE__);
            return AUDIO_PLAYER_ERR;
        }
    }

    /* Protect aac decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < AAC_MAINBUF_SIZE)
    {
        if (priv->stream_done)
        {
            return AUDIO_PLAYER_ERR;
        }
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, bytes_left: %d < %d \n", __func__, priv->bytes_left, AAC_MAINBUF_SIZE);
        rtos_delay_milliseconds(5);
        goto __retry;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, aacdec \n", __func__);

    /* not used */
    int byte_left = priv->bytes_left;
    /* first decoder frame to get aacFrameInfo */
    ec = AACDecode(priv->decoder, &(priv->read_ptr), &byte_left, sample_buffer);
    if (ec == 0)
    {
        /* no error */
        AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);

        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.bitRate: %d \n", priv->aacFrameInfo.bitRate);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.nChans: %d \n", priv->aacFrameInfo.nChans);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.sampRateCore: %d \n", priv->aacFrameInfo.sampRateCore);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.sampRateOut: %d \n", priv->aacFrameInfo.sampRateOut);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.bitsPerSample: %d \n", priv->aacFrameInfo.bitsPerSample);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.outputSamps: %d \n", priv->aacFrameInfo.outputSamps);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.profile: %d \n", priv->aacFrameInfo.profile);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.tnsUsed: %d \n", priv->aacFrameInfo.tnsUsed);
        BK_LOGI(AUDIO_PLAYER_TAG, "aacFrameInfo.pnsUsed: %d \n", priv->aacFrameInfo.pnsUsed);
    }
    else
    {
        if (ec == ERR_AAC_INDATA_UNDERFLOW)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode finish, ec: %d, %d\n", __func__, ec, __LINE__);
            goto __retry;
        }
        else
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, ec: %d, %d\n", __func__, ec, __LINE__);
            return -1;
        }
    }

    priv->current_sample_rate = priv->aacFrameInfo.sampRateOut;

    info->channel_number = priv->aacFrameInfo.nChans;
    info->sample_rate = priv->aacFrameInfo.sampRateOut;
    info->sample_bits = priv->aacFrameInfo.bitsPerSample;
    info->frame_size = 2 * priv->aacFrameInfo.outputSamps;

    /* update audio frame info */
    decoder->info.channel_number = priv->aacFrameInfo.nChans;
    decoder->info.sample_rate = priv->aacFrameInfo.sampRateOut;
    decoder->info.sample_bits = priv->aacFrameInfo.bitsPerSample;
    decoder->info.frame_size = 2 * priv->aacFrameInfo.outputSamps;

    /* reset read_ptr to read_buffer, and decode first frame again */
    priv->read_ptr = priv->read_buffer;

    if (sample_buffer)
    {
        player_free(sample_buffer);
        sample_buffer = NULL;
    }

    return AUDIO_PLAYER_OK;
}


int ts_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    int32_t ec;

    ts_decoder_priv_t *priv;
    priv = (ts_decoder_priv_t *)decoder->decoder_priv;

__retry:

    /* The read buffer holds at most AAC_AUDIO_BUF_SZ == AAC_MAINBUF_SIZE bytes.
     * The fill threshold must not exceed the buffer capacity, otherwise a full
     * buffer keeps re-entering ts_fill_buffer with bytes_to_read == 0, which the
     * stream reader reports as 0 bytes and is mis-detected as end-of-stream. */
    while (((priv->read_ptr == NULL) || priv->bytes_left < AAC_MAINBUF_SIZE) && !priv->stream_done)
    {
        int fill_ret = ts_fill_buffer(decoder);
        if (fill_ret == AUDIO_PLAYER_TIMEOUT)
        {
            return AUDIO_PLAYER_TIMEOUT;
        }
        if (fill_ret == RB_DONE)
        {
            break;
        }
        if (fill_ret != 0)
        {
            return AUDIO_PLAYER_ERR;
        }
    }

    /* Protect aac decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < AAC_MAINBUF_SIZE)
    {
        if (priv->stream_done)
        {
            priv->bytes_left = 0;
            return 0;
        }
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, wait for more data, bytes_left: %d < %d , %d\n", __func__, priv->bytes_left, AAC_MAINBUF_SIZE, __LINE__);
        return AUDIO_PLAYER_TIMEOUT;
    }

    ec = AACDecode(priv->decoder, &priv->read_ptr, (int *)&priv->bytes_left, (short *)buffer);
    if (ec == 0)
    {
        AACGetLastFrameInfo(priv->decoder, &priv->aacFrameInfo);
#if 0
        if (priv->aac_decoder_num_channels == 2)
        {
            os_memcpy((uint8_t *)buffer, priv->aac_decoder_pcm_buffer, priv->aac_decoder_pcm_samples * 2 * sizeof(uint16_t));
        }
        else
        {
            int i;
            int16_t *src, *dst;

            os_memcpy(buffer + priv->aac_decoder_pcm_samples * sizeof(uint16_t),
                      priv->aac_decoder_pcm_buffer,
                      priv->aac_decoder_pcm_samples * sizeof(uint16_t));

            // convert to two channel
            src = (int16_t *)(buffer + priv->aac_decoder_pcm_samples * sizeof(uint16_t));
            dst = (int16_t *)(buffer);
            for (i = 0; i < priv->aac_decoder_pcm_samples; i++)
            {
                dst[2 * i] = src[i];
                dst[2 * i + 1] = src[i];
            }
        }
#endif

        return priv->aacFrameInfo.outputSamps * 2;
    }
    else
    {
        if (ec == ERR_AAC_INDATA_UNDERFLOW)
        {
            if (priv->stream_done)
            {
                return 0;
            }
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode need more data, ec: %d, %d\n", __func__, ec, __LINE__);
            goto __retry;
        }

        /* aac decode error, stop play */
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, aac_decoder_decode fail, ec: %d, %d\n", __func__, ec, __LINE__);
        return -1;
    }

    return 0;
}

static int ts_decoder_close(bk_audio_player_decoder_t *decoder)
{
    ts_decoder_priv_t *priv;
    priv = (ts_decoder_priv_t *)decoder->decoder_priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, %d\n", __func__, __LINE__);

    if (priv && priv->decoder)
    {
        AACFreeDecoder(priv->decoder);
        priv->decoder = NULL;
    }

    if (priv->read_buffer)
    {
        player_free(priv->read_buffer);
        priv->read_buffer = NULL;
    }

    /* deinit ts_format_osi_funcs and ts_format parse */
    ts_format_stream_deinit(&priv->ts_stream);
    ts_format_osi_funcs_deinit();

    return AUDIO_PLAYER_OK;
}


static int ts_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    ts_decoder_priv_t *priv;
    priv = (ts_decoder_priv_t *)decoder->decoder_priv;
    return 2 * priv->aacFrameInfo.outputSamps;
}


const bk_audio_player_decoder_ops_t ts_decoder_ops =
{
    .name = "ts",
    .open = ts_decoder_open,
    .get_info = ts_decoder_get_info,
    .get_chunk_size = ts_decoder_get_chunk_size,
    .get_data = ts_decoder_get_data,
    .close = ts_decoder_close,
    .calc_position = calc_ts_position,
    .is_seek_ready = NULL,
};

/* Get TS decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_ts_decoder_ops(void)
{
    return &ts_decoder_ops;
}
