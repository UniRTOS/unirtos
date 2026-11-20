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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <os/mem.h>
#include "player_mem.h"
#include <modules/wav_head.h>
#include <modules/adpcm.h>
#include <modules/g711.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"
#include "ring_buffer.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

// 48k per 20ms : 48 * 1000 * 4 / (1000 / 20) = 3840
#define WAV_CHUNK_SIZE 3840

typedef struct wav_decoder_priv
{
    int channels;
    int sample_rate;
    int sample_bits;
    int bps;
    uint32_t total_bytes;
    uint32_t header_bytes;
    double duration;    // in milisecond (ms)

    uint32_t data_start;            /* offset of data chunk in file */
    uint16_t audio_format;          /* WAV format tag */
    uint16_t block_align;           /* block alignment for compressed formats */
    uint16_t samples_per_block;     /* samples per block for ADPCM */

    /* runtime decode buffers for compressed formats */
    uint8_t *block_buffer;          /* temporary buffer for compressed block */
    size_t block_buffer_size;
    int16_t *pcm_buffer;            /* decoded PCM samples */
    size_t pcm_buffer_capacity;     /* in samples */
    size_t pcm_buffer_pos;
    size_t pcm_buffer_filled;

    adpcm_state_t adpcm_state[2];   /* support up to stereo ADPCM */
    bool need_reset;                /* flag to reset decoder state after seek */
    int16_t *adpcm_channel_buf[2];
    size_t adpcm_channel_capacity;
    uint8_t *adpcm_channel_bytes[2];
    size_t adpcm_channel_byte_capacity;
} wav_decoder_priv_t;

static int wav_decode_adpcm(bk_audio_player_decoder_t *decoder, char *buffer, int len);
static int wav_decode_g711(bk_audio_player_decoder_t *decoder, char *buffer, int len);
static int wav_decode_adpcm_block(wav_decoder_priv_t *priv, const uint8_t *block, int block_bytes);

static int calc_wav_position(bk_audio_player_decoder_t *decoder, int second)
{
    wav_decoder_priv_t *priv = (wav_decoder_priv_t *)decoder->decoder_priv;

    if (second < 0)
    {
        second = 0;
    }

    priv->need_reset = true;

    int64_t offset = priv->data_start;

    switch (priv->audio_format)
    {
        case 0x0011: /* IMA ADPCM */
        {
            if (priv->samples_per_block > 0 && priv->block_align > 0)
            {
                int64_t samples = (int64_t)second * priv->sample_rate;
                int64_t block_samples = priv->samples_per_block;
                int64_t block_index = samples / block_samples;
                offset += block_index * priv->block_align;
            }
            break;
        }
        case 0x0006: /* G711 A-law */
        case 0x0007: /* G711 u-law */
        {
            int64_t bytes_per_second = (int64_t)priv->sample_rate * priv->channels;
            offset += bytes_per_second * second;
            break;
        }
        default: /* Linear PCM */
        {
            int64_t bytes_per_second = (int64_t)priv->channels * priv->sample_rate * priv->sample_bits / 8;
            offset += bytes_per_second * second;
            break;
        }
    }

    if (offset < priv->data_start)
    {
        offset = priv->data_start;
    }

    return (int)offset;
}

static int wav_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    bk_audio_player_decoder_t *decoder;
    wav_decoder_priv_t *priv;

    if (format != AUDIO_FORMAT_WAV)
    {
        return AUDIO_PLAYER_INVALID;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    decoder = audio_codec_new(sizeof(wav_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }


    priv = (wav_decoder_priv_t *)decoder->decoder_priv;
    priv->channels = 0;
    priv->sample_rate = 0;
    priv->sample_bits = 0;
    priv->bps = 0;
    priv->total_bytes = 0;
    priv->header_bytes = 44;
    priv->duration = 0.0;

    priv->data_start = 0;
    priv->audio_format = 0x0001;
    priv->block_align = 0;
    priv->samples_per_block = 0;
    priv->block_buffer = NULL;
    priv->block_buffer_size = 0;
    priv->pcm_buffer = NULL;
    priv->pcm_buffer_capacity = 0;
    priv->pcm_buffer_pos = 0;
    priv->pcm_buffer_filled = 0;
    priv->adpcm_state[0].valprev = 0;
    priv->adpcm_state[0].index = 0;
    priv->adpcm_state[1].valprev = 0;
    priv->adpcm_state[1].index = 0;
    priv->need_reset = false;
    priv->adpcm_channel_buf[0] = NULL;
    priv->adpcm_channel_buf[1] = NULL;
    priv->adpcm_channel_capacity = 0;
    priv->adpcm_channel_bytes[0] = NULL;
    priv->adpcm_channel_bytes[1] = NULL;
    priv->adpcm_channel_byte_capacity = 0;

    *decoder_pp = decoder;

    return AUDIO_PLAYER_OK;
}

static int wav_decoder_close(bk_audio_player_decoder_t *decoder)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "%s \n", __func__);

    if (decoder && decoder->decoder_priv)
    {
        wav_decoder_priv_t *priv = (wav_decoder_priv_t *)decoder->decoder_priv;
        if (priv->block_buffer)
        {
            player_free(priv->block_buffer);
            priv->block_buffer = NULL;
        }
        if (priv->pcm_buffer)
        {
            player_free(priv->pcm_buffer);
            priv->pcm_buffer = NULL;
        }
        for (int i = 0; i < 2; i++)
        {
            if (priv->adpcm_channel_buf[i])
            {
                player_free(priv->adpcm_channel_buf[i]);
                priv->adpcm_channel_buf[i] = NULL;
            }
            if (priv->adpcm_channel_bytes[i])
            {
                player_free(priv->adpcm_channel_bytes[i]);
                priv->adpcm_channel_bytes[i] = NULL;
            }
        }
        priv->adpcm_channel_capacity = 0;
        priv->adpcm_channel_byte_capacity = 0;
    }
    return AUDIO_PLAYER_OK;
}

static int wav_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    wav_decoder_priv_t *priv = (wav_decoder_priv_t *)decoder->decoder_priv;
    uint8_t chunk_header[8];
    uint8_t riff_header[12];
    uint32_t current_offset = 0;
    uint32_t data_size = 0;
    bool fmt_found = false;
    bool data_found = false;

    if (audio_source_read_data(decoder->source, (char *)riff_header, sizeof(riff_header)) != sizeof(riff_header))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, read riff header failed\n", __func__);
        return AUDIO_PLAYER_ERR;
    }
    current_offset += sizeof(riff_header);

    if (memcmp(riff_header, "RIFF", 4) != 0 || memcmp(riff_header + 8, "WAVE", 4) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid riff header\n", __func__);
        return AUDIO_PLAYER_ERR;
    }

    while (!fmt_found || !data_found)
    {
        if (audio_source_read_data(decoder->source, (char *)chunk_header, sizeof(chunk_header)) != sizeof(chunk_header))
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, failed to read chunk header\n", __func__);
            return AUDIO_PLAYER_ERR;
        }
        current_offset += sizeof(chunk_header);

        uint32_t chunk_size = chunk_header[4] | (chunk_header[5] << 8) | (chunk_header[6] << 16) | (chunk_header[7] << 24);

        if (memcmp(chunk_header, "fmt ", 4) == 0)
        {
            uint8_t stack_buf[64];
            uint8_t *fmt_buf = stack_buf;
            if (chunk_size > sizeof(stack_buf))
            {
                fmt_buf = (uint8_t *)player_malloc(chunk_size);
                if (!fmt_buf)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc fmt buffer failed\n", __func__);
                    return AUDIO_PLAYER_NO_MEM;
                }
            }

            if (audio_source_read_data(decoder->source, (char *)fmt_buf, chunk_size) != (int)chunk_size)
            {
                if (fmt_buf != stack_buf)
                {
                    player_free(fmt_buf);
                }
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, read fmt chunk failed\n", __func__);
                return AUDIO_PLAYER_ERR;
            }
            current_offset += chunk_size;

            priv->audio_format = fmt_buf[0] | (fmt_buf[1] << 8);
            priv->channels = fmt_buf[2] | (fmt_buf[3] << 8);
            priv->sample_rate = fmt_buf[4] | (fmt_buf[5] << 8) | (fmt_buf[6] << 16) | (fmt_buf[7] << 24);
            priv->bps = fmt_buf[8] | (fmt_buf[9] << 8) | (fmt_buf[10] << 16) | (fmt_buf[11] << 24);
            priv->block_align = fmt_buf[12] | (fmt_buf[13] << 8);
            priv->sample_bits = fmt_buf[14] | (fmt_buf[15] << 8);

            priv->samples_per_block = 0;
            if (chunk_size > 16)
            {
                uint32_t offset = 16;
                uint16_t cbSize = fmt_buf[offset] | (fmt_buf[offset + 1] << 8);
                offset += 2;
                if (priv->audio_format == 0x0011 && chunk_size >= offset + 2)
                {
                    priv->samples_per_block = fmt_buf[offset] | (fmt_buf[offset + 1] << 8);
                }
                (void)cbSize;
            }

            if (fmt_buf != stack_buf)
            {
                player_free(fmt_buf);
            }

            fmt_found = true;
        }
        else if (memcmp(chunk_header, "data", 4) == 0)
        {
            priv->data_start = current_offset;
            data_size = chunk_size;
            data_found = true;
            break;
        }
        else
        {
            /* skip unknown chunk */
            if (audio_source_seek(decoder->source, chunk_size, SEEK_CUR) != AUDIO_PLAYER_OK)
            {
                char tmp[128];
                uint32_t remaining = chunk_size;
                while (remaining > 0)
                {
                    uint32_t to_read = remaining > sizeof(tmp) ? sizeof(tmp) : remaining;
                    if (audio_source_read_data(decoder->source, tmp, to_read) != (int)to_read)
                    {
                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, skip chunk failed\n", __func__);
                        return AUDIO_PLAYER_ERR;
                    }
                    remaining -= to_read;
                }
            }
            current_offset += chunk_size;
        }

        /* chunks are padded to even size */
        if (chunk_size & 1)
        {
            if (audio_source_seek(decoder->source, 1, SEEK_CUR) != AUDIO_PLAYER_OK)
            {
                char pad;
                if (audio_source_read_data(decoder->source, &pad, 1) != 1)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, skip pad byte failed\n", __func__);
                    return AUDIO_PLAYER_ERR;
                }
            }
            current_offset += 1;
        }
    }

    if (!fmt_found || !data_found)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, fmt/data chunk missing\n", __func__);
        return AUDIO_PLAYER_ERR;
    }

    info->channel_number = priv->channels;
    info->sample_rate = priv->sample_rate;
    info->frame_size = WAV_CHUNK_SIZE;

    decoder->info.channel_number = priv->channels;
    decoder->info.sample_rate = priv->sample_rate;
    decoder->info.frame_size = info->frame_size;

    switch (priv->audio_format)
    {
        case 0x0001: /* Linear PCM */
        {
            info->sample_bits = priv->sample_bits;
            info->total_bytes = data_size;
            info->bps = priv->sample_rate * priv->channels * priv->sample_bits;
            info->duration = (double)data_size * 8 * 1000.0 / (double)info->bps;

            decoder->info.sample_bits = info->sample_bits;
            decoder->info.total_bytes = data_size;
            decoder->info.bps = info->bps;
            decoder->info.duration = info->duration;

            priv->total_bytes = data_size;
            priv->duration = info->duration;
            break;
        }
        case 0x0011: /* IMA ADPCM */
        {
            if (priv->channels > 2)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, ADPCM supports up to 2 channels\n", __func__);
                return AUDIO_PLAYER_ERR;
            }
            if (priv->samples_per_block == 0 && priv->block_align > 0 && priv->channels > 0)
            {
                priv->samples_per_block = ((priv->block_align - (4 * priv->channels)) * 2 / priv->channels) + 1;
            }

            if (priv->samples_per_block <= 0 || priv->block_align == 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid ADPCM block parameters\n", __func__);
                return AUDIO_PLAYER_ERR;
            }

            uint64_t total_samples = 0;
            if (priv->block_align > 0 && priv->samples_per_block > 0)
            {
                uint64_t full_blocks = data_size / priv->block_align;
                total_samples = full_blocks * priv->samples_per_block;
                uint32_t remainder = data_size % priv->block_align;
                if (remainder >= (uint32_t)(4 * priv->channels))
                {
                    uint32_t bytes_per_channel = (remainder - 4 * priv->channels) / priv->channels;
                    uint32_t extra_samples = bytes_per_channel * 2 + 1;
                    total_samples += extra_samples;
                }
            }

            info->sample_bits = 16;
            info->bps = priv->sample_rate * priv->channels * 16;
            if (total_samples == 0 && info->bps > 0)
            {
                total_samples = (uint64_t)data_size * 8 / (priv->channels * priv->sample_bits);
            }

            info->total_bytes = total_samples * priv->channels * 2;
            info->duration = (double)total_samples * 1000.0 / (double)priv->sample_rate;

            decoder->info.sample_bits = info->sample_bits;
            decoder->info.total_bytes = info->total_bytes;
            decoder->info.bps = info->bps;
            decoder->info.duration = info->duration;

            priv->sample_bits = 16;
            priv->total_bytes = info->total_bytes;
            priv->duration = info->duration;

            if (priv->block_align > 0)
            {
                priv->block_buffer_size = priv->block_align;
                priv->block_buffer = (uint8_t *)player_malloc(priv->block_buffer_size);
                if (!priv->block_buffer)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc block buffer failed\n", __func__);
                    goto adpcm_alloc_fail;
                }
            }

            priv->pcm_buffer_capacity = (size_t)priv->samples_per_block * priv->channels;
            priv->pcm_buffer = (int16_t *)player_malloc(priv->pcm_buffer_capacity * sizeof(int16_t));
            if (!priv->pcm_buffer)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc pcm buffer failed\n", __func__);
                goto adpcm_alloc_fail;
            }
            priv->pcm_buffer_pos = 0;
            priv->pcm_buffer_filled = 0;

            size_t bytes_per_channel = 0;
            if (priv->block_align > 4 * priv->channels)
            {
                bytes_per_channel = (priv->block_align - 4 * priv->channels) / priv->channels;
            }
            if (bytes_per_channel == 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid ADPCM bytes per channel\n", __func__);
                return AUDIO_PLAYER_ERR;
            }
            priv->adpcm_channel_capacity = priv->samples_per_block;
            priv->adpcm_channel_byte_capacity = bytes_per_channel;

            for (int ch = 0; ch < priv->channels && ch < 2; ch++)
            {
                priv->adpcm_channel_buf[ch] = (int16_t *)player_malloc(priv->adpcm_channel_capacity * sizeof(int16_t));
                if (!priv->adpcm_channel_buf[ch])
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc adpcm channel buf failed\n", __func__);
                    goto adpcm_alloc_fail;
                }

                if (bytes_per_channel > 0)
                {
                    priv->adpcm_channel_bytes[ch] = (uint8_t *)player_malloc(bytes_per_channel);
                    if (!priv->adpcm_channel_bytes[ch])
                    {
                        BK_LOGE(AUDIO_PLAYER_TAG, "%s, malloc adpcm channel bytes failed\n", __func__);
                        goto adpcm_alloc_fail;
                    }
                }
            }
            break;
        }
        case 0x0006: /* G.711 A-law */
        case 0x0007: /* G.711 u-law */
        {
            info->sample_bits = 16;
            info->bps = priv->sample_rate * priv->channels * 16;
            info->total_bytes = (uint64_t)data_size * 2;
            info->duration = (double)data_size * 1000.0 / (double)(priv->sample_rate * priv->channels);

            decoder->info.sample_bits = info->sample_bits;
            decoder->info.total_bytes = info->total_bytes;
            decoder->info.bps = info->bps;
            decoder->info.duration = info->duration;

            priv->sample_bits = 16;
            priv->total_bytes = info->total_bytes;
            priv->duration = info->duration;
            break;
        }
        default:
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, unsupported wav format: 0x%x\n", __func__, priv->audio_format);
            return AUDIO_PLAYER_ERR;
        }
    }

    priv->total_bytes = info->total_bytes;
    priv->duration = info->duration;
    priv->bps = info->bps;
    priv->need_reset = false;
    priv->pcm_buffer_pos = 0;
    priv->pcm_buffer_filled = 0;
    priv->header_bytes = priv->data_start;
    decoder->info.header_bytes = priv->data_start;

    return AUDIO_PLAYER_OK;

adpcm_alloc_fail:
    if (priv->block_buffer)
    {
        player_free(priv->block_buffer);
        priv->block_buffer = NULL;
    }
    if (priv->pcm_buffer)
    {
        player_free(priv->pcm_buffer);
        priv->pcm_buffer = NULL;
    }
    for (int i = 0; i < 2; i++)
    {
        if (priv->adpcm_channel_buf[i])
        {
            player_free(priv->adpcm_channel_buf[i]);
            priv->adpcm_channel_buf[i] = NULL;
        }
        if (priv->adpcm_channel_bytes[i])
        {
            player_free(priv->adpcm_channel_bytes[i]);
            priv->adpcm_channel_bytes[i] = NULL;
        }
    }
    return AUDIO_PLAYER_NO_MEM;
}

static int wav_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    return WAV_CHUNK_SIZE;
}

static int wav_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    wav_decoder_priv_t *priv = (wav_decoder_priv_t *)decoder->decoder_priv;

    if (priv->need_reset)
    {
        priv->need_reset = false;
        priv->pcm_buffer_pos = 0;
        priv->pcm_buffer_filled = 0;
    }

    switch (priv->audio_format)
    {
        case 0x0011:
            return wav_decode_adpcm(decoder, buffer, len);
        case 0x0006:
        case 0x0007:
            return wav_decode_g711(decoder, buffer, len);
        default:
        {
            int ret;

            ret = audio_source_read_data(decoder->source, buffer, len);
            if (ret == AUDIO_PLAYER_TIMEOUT)
            {
                return AUDIO_PLAYER_TIMEOUT;
            }

            if (ret == 0 || ret == RB_DONE || ret == RB_ABORT)
            {
                return 0;
            }
            if (ret < 0)
            {
                return -1;
            }

            if (priv->sample_bits == 32)
            {
                float *buffer_32bit = (float *)buffer;
                int16_t *buffer_16bit = (int16_t *)buffer;

                for (int i = 0; i < ret / 4; i++)
                {
                    buffer_16bit[i] = (int16_t)(buffer_32bit[i] * 32768);
                }

                return ret / 2;
            }

            return ret;
        }
    }
}

static int wav_decode_g711(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    wav_decoder_priv_t *priv = (wav_decoder_priv_t *)decoder->decoder_priv;
    int16_t *out = (int16_t *)buffer;
    int samples_needed = len / 2;
    int samples_out = 0;

    uint8_t temp[256];
    bool source_done = false;

    while (samples_out < samples_needed)
    {
        int to_read = samples_needed - samples_out;
        if (to_read > (int)sizeof(temp))
        {
            to_read = sizeof(temp);
        }

        int ret;
        ret = audio_source_read_data(decoder->source, (char *)temp, to_read);
        if (ret == AUDIO_PLAYER_TIMEOUT)
        {
            return samples_out > 0 ? samples_out * 2 : AUDIO_PLAYER_TIMEOUT;
        }
        if (ret == 0 || ret == RB_DONE || ret == RB_ABORT)
        {
            source_done = true;
            break;
        }
        if (ret < 0)
        {
            return samples_out > 0 ? samples_out * 2 : AUDIO_PLAYER_ERR;
        }

        for (int i = 0; i < ret; i++)
        {
            if (priv->audio_format == 0x0006)
            {
                out[samples_out + i] = (int16_t)alaw2linear(temp[i]);
            }
            else
            {
                out[samples_out + i] = (int16_t)ulaw2linear(temp[i]);
            }
        }

        samples_out += ret;
    }

    if (samples_out == 0)
    {
        return source_done ? 0 : -1;
    }

    return samples_out * 2;
}

static inline uint8_t wav_swap_adpcm_nibbles(uint8_t value)
{
    return (uint8_t)(((value & 0x0F) << 4) | ((value & 0xF0) >> 4));
}

static int wav_decode_adpcm_block(wav_decoder_priv_t *priv, const uint8_t *block, int block_bytes)
{
    int channels = priv->channels;
    if (channels <= 0 || channels > 2)
    {
        return 0;
    }

    if (block_bytes < 4 * channels)
    {
        return 0;
    }

    const uint8_t *encoded = block + 4 * channels;
    size_t payload_total = (block_bytes > 4 * channels) ? (size_t)(block_bytes - 4 * channels) : 0;

    for (int ch = 0; ch < channels; ch++)
    {
        int16_t predictor = (int16_t)(block[4 * ch] | (block[4 * ch + 1] << 8));
        int index = block[4 * ch + 2];
        if (index < 0)
        {
            index = 0;
        }
        else if (index > 88)
        {
            index = 88;
        }

        priv->adpcm_state[ch].valprev = predictor;
        priv->adpcm_state[ch].index = index;

        int16_t *channel_pcm = priv->adpcm_channel_buf[ch];
        channel_pcm[0] = predictor;

        uint8_t *channel_bytes = priv->adpcm_channel_bytes[ch];
        os_memset(channel_bytes, 0, priv->adpcm_channel_byte_capacity);
    }

    size_t channel_bytes_written[2] = {0};
    size_t payload_pos = 0;
    bool overflow_detected = false;
    const size_t bytes_per_subframe = 4; /* 8 samples (2 nibbles per byte) per channel */

    /* IMA ADPCM payload layout:
     * - after the 4-byte header per channel, the encoded bytes are interleaved per channel
     *   at a granularity of 4 bytes (one subframe = 8 samples) [IMA ADPCM spec].
     * - each byte stores the low nibble first, but adpcm_decoder() expects high nibble first,
     *   therefore swap the nibbles when copying into the per-channel buffer.
     */
    while (payload_total >= payload_pos + bytes_per_subframe * (size_t)channels)
    {
        for (int ch = 0; ch < channels; ch++)
        {
            if (channel_bytes_written[ch] + bytes_per_subframe <= priv->adpcm_channel_byte_capacity)
            {
                uint8_t *dest = priv->adpcm_channel_bytes[ch] + channel_bytes_written[ch];
                for (size_t i = 0; i < bytes_per_subframe; i++)
                {
                    dest[i] = wav_swap_adpcm_nibbles(encoded[payload_pos + i]);
                }
                channel_bytes_written[ch] += bytes_per_subframe;
            }
            else
            {
                overflow_detected = true;
            }
            payload_pos += bytes_per_subframe;
        }
    }

    int distribute_channel = 0;
    if (channels > 0)
    {
        distribute_channel = (payload_pos / bytes_per_subframe) % channels;
    }

    while (payload_pos < payload_total)
    {
        int ch = distribute_channel;
        distribute_channel = (distribute_channel + 1) % channels;

        if (channel_bytes_written[ch] < priv->adpcm_channel_byte_capacity)
        {
            priv->adpcm_channel_bytes[ch][channel_bytes_written[ch]] = wav_swap_adpcm_nibbles(encoded[payload_pos]);
            channel_bytes_written[ch]++;
        }
        else
        {
            overflow_detected = true;
        }

        payload_pos++;
    }

    if (overflow_detected)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, ADPCM channel bytes overflow, truncating payload\n", __func__);
    }

    int samples_this_block = 0;
    for (int ch = 0; ch < channels; ch++)
    {
        size_t written = channel_bytes_written[ch];
        if (written > 0)
        {
            int samples_to_decode = (int)(written * 2);
            if (samples_this_block == 0 || (samples_to_decode + 1) < samples_this_block)
            {
                samples_this_block = samples_to_decode + 1;
            }

            if (samples_to_decode > 0)
            {
                adpcm_decoder((char *)priv->adpcm_channel_bytes[ch],
                              priv->adpcm_channel_buf[ch] + 1,
                              samples_to_decode,
                              &priv->adpcm_state[ch]);
            }
        }
    }

    if (samples_this_block == 0)
    {
        return 0;
    }

    if ((size_t)samples_this_block > priv->adpcm_channel_capacity)
    {
        samples_this_block = (int)priv->adpcm_channel_capacity;
    }

    int total_samples = samples_this_block * channels;
    for (int sample = 0; sample < samples_this_block; sample++)
    {
        for (int ch = 0; ch < channels; ch++)
        {
            priv->pcm_buffer[sample * channels + ch] = priv->adpcm_channel_buf[ch][sample];
        }
    }

    return total_samples;
}

static int wav_decode_adpcm(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    wav_decoder_priv_t *priv = (wav_decoder_priv_t *)decoder->decoder_priv;
    int16_t *out = (int16_t *)buffer;
    int samples_needed = len / 2;
    int samples_out = 0;
    bool source_done = false;

    if (!priv->block_buffer || !priv->pcm_buffer)
    {
        return AUDIO_PLAYER_ERR;
    }

    while (samples_out < samples_needed)
    {
        if (priv->pcm_buffer_pos >= priv->pcm_buffer_filled)
        {
            int ret;
            ret = audio_source_read_data(decoder->source, (char *)priv->block_buffer, priv->block_align);
            if (ret == AUDIO_PLAYER_TIMEOUT)
            {
                return samples_out > 0 ? samples_out * 2 : AUDIO_PLAYER_TIMEOUT;
            }
            if (ret == 0 || ret == RB_DONE || ret == RB_ABORT)
            {
                source_done = true;
                break;
            }
            if (ret < 0)
            {
                return samples_out > 0 ? samples_out * 2 : AUDIO_PLAYER_ERR;
            }

            priv->pcm_buffer_filled = wav_decode_adpcm_block(priv, priv->block_buffer, ret);
            priv->pcm_buffer_pos = 0;

            if (priv->pcm_buffer_filled <= 0)
            {
                break;
            }
        }

        int available = priv->pcm_buffer_filled - priv->pcm_buffer_pos;
        int required = samples_needed - samples_out;
        int to_copy = available < required ? available : required;
        os_memcpy(out + samples_out, priv->pcm_buffer + priv->pcm_buffer_pos, to_copy * sizeof(int16_t));

        priv->pcm_buffer_pos += to_copy;
        samples_out += to_copy;
    }

    if (samples_out == 0)
    {
        return source_done ? 0 : -1;
    }

    return samples_out * 2;
}


const bk_audio_player_decoder_ops_t wav_decoder_ops =
{
    .name = "wav",
    .open = wav_decoder_open,
    .get_info = wav_decoder_get_info,
    .get_chunk_size = wav_decoder_get_chunk_size,
    .get_data = wav_decoder_get_data,
    .close = wav_decoder_close,
    .calc_position = calc_wav_position,
    .is_seek_ready = NULL,
};

/* Get WAV decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_wav_decoder_ops(void)
{
    return &wav_decoder_ops;
}
