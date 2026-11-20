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
#include <stdbool.h>
#include <stdint.h>
#include <modules/ogg.h>
#include <modules/vorbis/codec.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"


#define BK_VORBIS_READ_CHUNK_BYTES   (4096)
#define BK_VORBIS_SAMPLE_SIZE_BYTES  ((uint32_t)sizeof(int16_t))
#define BK_VORBIS_READ_RETRY         (5)
#define BK_VORBIS_TARGET_FRAME_MS    (20)

typedef struct vorbis_decoder_priv_s
{
    ogg_sync_state   oy;
    ogg_stream_state os;
    ogg_page         og;
    ogg_packet       op;

    bool             stream_initialized;
    bool             headers_ready;
    bool             decoder_ready;
    bool             end_of_stream;

    vorbis_info      vi;
    vorbis_comment   vc;
    vorbis_dsp_state vd;
    vorbis_block     vb;

    uint32_t         sample_rate;
    uint32_t         channels;
    uint32_t         chunk_bytes;
    uint32_t         stream_serial;
    uint64_t         stream_offset;
    uint32_t         pending_seek_offset;
    bool             pending_seek_valid;
    bool             need_reset;
} vorbis_decoder_priv_t;

static uint32_t bk_vorbis_default_chunk_bytes(uint32_t channels, uint32_t sample_rate)
{
    if (channels == 0)
    {
        channels = 1;
    }
    if (sample_rate == 0)
    {
        sample_rate = 44100;
    }

    uint64_t samples = (uint64_t)sample_rate * BK_VORBIS_TARGET_FRAME_MS;
    samples = (samples + 999) / 1000;
    if (samples == 0)
    {
        samples = 1024;
    }

    return (uint32_t)(samples * channels * BK_VORBIS_SAMPLE_SIZE_BYTES);
}

static int bk_vorbis_scan_to_granule(bk_audio_player_decoder_t *decoder,
                                     vorbis_decoder_priv_t *priv,
                                     int64_t target_granule,
                                     uint32_t *offset_out)
{
    if (!decoder || !priv || !offset_out)
    {
        return AUDIO_PLAYER_ERR;
    }

    uint32_t total_bytes = audio_source_get_total_bytes(decoder->source);
    if (total_bytes == 0U)
    {
        return AUDIO_PLAYER_ERR;
    }

    uint64_t resume_offset = priv->stream_offset;
    ogg_sync_state scanner;
    ogg_sync_init(&scanner);
    if (audio_source_seek(decoder->source, 0, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        ogg_sync_clear(&scanner);
        return AUDIO_PLAYER_ERR;
    }
    priv->stream_offset = 0;

    ogg_page page;
    uint64_t consumed = 0;
    bool found = false;
    uint32_t candidate_offset = 0;
    int64_t candidate_granule = 0;

    while (consumed < total_bytes)
    {
        char *buf = ogg_sync_buffer(&scanner, BK_VORBIS_READ_CHUNK_BYTES);
        int bytes = audio_source_read_data(decoder->source, buf, BK_VORBIS_READ_CHUNK_BYTES);
        if (bytes <= 0)
        {
            break;
        }
        priv->stream_offset += (uint32_t)bytes;
        ogg_sync_wrote(&scanner, bytes);

        long page_bytes;
        while ((page_bytes = ogg_sync_pageseek(&scanner, &page)) != 0)
        {
            if (page_bytes < 0)
            {
                consumed += (uint64_t)(-page_bytes);
                continue;
            }

            uint64_t page_start = consumed;
            consumed += (uint64_t)page_bytes;

            int serial = ogg_page_serialno(&page);
            if (priv->stream_serial != 0 && serial != (int)priv->stream_serial)
            {
                continue;
            }

            int64_t granule = ogg_page_granulepos(&page);
            if (granule < 0)
            {
                continue;
            }

            candidate_offset = (uint32_t)page_start;
            candidate_granule = granule;

            if (granule >= target_granule)
            {
                found = true;
                goto __scan_done;
            }
        }
    }

__scan_done:
    ogg_sync_clear(&scanner);
    if (audio_source_seek(decoder->source, (int)resume_offset, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        return AUDIO_PLAYER_ERR;
    }
    priv->stream_offset = resume_offset;

    if (!found && candidate_granule == 0)
    {
        *offset_out = 0;
        return AUDIO_PLAYER_ERR;
    }

    *offset_out = candidate_offset;
    return AUDIO_PLAYER_OK;
}

static int bk_vorbis_stream_read(bk_audio_player_decoder_t *decoder, char *buffer, size_t request_bytes)
{
    int bytes = audio_source_read_data(decoder->source, buffer, request_bytes);
    if (bytes > 0)
    {
        vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;
        if (priv)
        {
            priv->stream_offset += (uint32_t)bytes;
        }
    }

    return bytes;
}

static int bk_vorbis_stream_fill(bk_audio_player_decoder_t *decoder, vorbis_decoder_priv_t *priv)
{
    char *buffer = ogg_sync_buffer(&priv->oy, BK_VORBIS_READ_CHUNK_BYTES);
    if (!buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: unable to obtain ogg buffer\n");
        return AUDIO_PLAYER_ERR;
    }

    int bytes = bk_vorbis_stream_read(decoder, buffer, BK_VORBIS_READ_CHUNK_BYTES);
    if (bytes == AUDIO_PLAYER_TIMEOUT)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "vorbis: read timeout while filling ogg buffer\n");
        return AUDIO_PLAYER_TIMEOUT;
    }

    if (bytes == 0)
    {
        priv->end_of_stream = true;
        BK_LOGI(AUDIO_PLAYER_TAG, "vorbis: source reached end while filling ogg buffer\n");
        return 0;
    }

    if (bytes > 0)
    {
        ogg_sync_wrote(&priv->oy, bytes);
    }

    return bytes;
}

static int bk_vorbis_stream_init_chain(vorbis_decoder_priv_t *priv)
{
    if (priv->stream_initialized)
    {
        return AUDIO_PLAYER_OK;
    }

    int serial = ogg_page_serialno(&priv->og);
    if (ogg_stream_init(&priv->os, serial) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: ogg_stream_init failed\n");
        return AUDIO_PLAYER_ERR;
    }

    priv->stream_initialized = true;
    priv->stream_serial = (uint32_t)serial;
    return AUDIO_PLAYER_OK;
}

static void bk_vorbis_stream_reset(vorbis_decoder_priv_t *priv)
{
    if (priv->stream_initialized)
    {
        ogg_stream_clear(&priv->os);
        priv->stream_initialized = false;
    }
    ogg_sync_clear(&priv->oy);
}

static void bk_vorbis_stream_destroy_decoder(vorbis_decoder_priv_t *priv)
{
    if (priv->decoder_ready)
    {
        vorbis_block_clear(&priv->vb);
        vorbis_dsp_clear(&priv->vd);
        priv->decoder_ready = false;
    }
    vorbis_comment_clear(&priv->vc);
    vorbis_info_clear(&priv->vi);
}

static int bk_vorbis_parse_headers(bk_audio_player_decoder_t *decoder, vorbis_decoder_priv_t *priv)
{
    while (!priv->headers_ready)
    {
        int page_status = ogg_sync_pageout(&priv->oy, &priv->og);
        if (page_status == 1)
        {
            if (bk_vorbis_stream_init_chain(priv) != AUDIO_PLAYER_OK)
            {
                return AUDIO_PLAYER_ERR;
            }

            if (ogg_stream_pagein(&priv->os, &priv->og) != 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "vorbis: ogg_stream_pagein failed\n");
                continue;
            }

            while (ogg_stream_packetout(&priv->os, &priv->op) > 0)
            {
                if (vorbis_synthesis_headerin(&priv->vi, &priv->vc, &priv->op) != 0)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: invalid header packet\n");
                    return AUDIO_PLAYER_ERR;
                }

                if (priv->op.packetno >= 2)
                {
                    priv->headers_ready = true;
                    break;
                }
            }
        }
        else if (page_status < 0)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "vorbis: skipped corrupt ogg page\n");
        }
        else
        {
            int bytes = bk_vorbis_stream_fill(decoder, priv);
            if (bytes == AUDIO_PLAYER_TIMEOUT)
            {
                rtos_delay_milliseconds(5);
                continue;
            }
            if (bytes <= 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: unexpected end of stream while parsing header\n");
                return AUDIO_PLAYER_ERR;
            }
        }
    }

    if (!priv->decoder_ready)
    {
        if (vorbis_synthesis_init(&priv->vd, &priv->vi) != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: synthesis init failed\n");
            return AUDIO_PLAYER_ERR;
        }
        if (vorbis_block_init(&priv->vd, &priv->vb) != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: block init failed\n");
            return AUDIO_PLAYER_ERR;
        }
        priv->decoder_ready = true;
    }

    priv->sample_rate = priv->vi.rate;
    priv->channels = (uint32_t)priv->vi.channels;
    priv->chunk_bytes = bk_vorbis_default_chunk_bytes(priv->channels, priv->sample_rate);
    return AUDIO_PLAYER_OK;
}

static int bk_vorbis_decode_next_packet(bk_audio_player_decoder_t *decoder, vorbis_decoder_priv_t *priv)
{
    while (1)
    {
        int packet = ogg_stream_packetout(&priv->os, &priv->op);
        if (packet > 0)
        {
            if (vorbis_synthesis(&priv->vb, &priv->op) == 0)
            {
                vorbis_synthesis_blockin(&priv->vd, &priv->vb);
                return AUDIO_PLAYER_OK;
            }
            BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: synthesis failed\n");
            return AUDIO_PLAYER_ERR;
        }

        if (packet == 0)
        {
            int page_status = ogg_sync_pageout(&priv->oy, &priv->og);
            if (page_status == 1)
            {
                if (!priv->stream_initialized)
                {
                    if (bk_vorbis_stream_init_chain(priv) != AUDIO_PLAYER_OK)
                    {
                        return AUDIO_PLAYER_ERR;
                    }
                }

                if (ogg_stream_pagein(&priv->os, &priv->og) != 0)
                {
                    continue;
                }

                if (ogg_page_eos(&priv->og))
                {
                    priv->end_of_stream = true;
                }
            }
            else if (page_status < 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "vorbis: corrupt ogg page skipped\n");
            }
            else
            {
                int bytes = bk_vorbis_stream_fill(decoder, priv);
                if (bytes == AUDIO_PLAYER_TIMEOUT)
                {
                    return AUDIO_PLAYER_TIMEOUT;
                }
                if (bytes <= 0)
                {
                    return 0;
                }
            }
        }
        else
        {
            return AUDIO_PLAYER_ERR;
        }
    }
}

static int bk_vorbis_convert_pcm(vorbis_decoder_priv_t *priv, float **pcm, int samples, char *buffer, int len)
{
    int channels = (int)priv->channels;
    int16_t *out = (int16_t *)buffer;
    int frames_capacity = len / (channels * (int)BK_VORBIS_SAMPLE_SIZE_BYTES);
    int frames_to_copy = samples;

    if (frames_capacity < frames_to_copy)
    {
        frames_to_copy = frames_capacity;
    }

    for (int i = 0; i < frames_to_copy; i++)
    {
        for (int c = 0; c < channels; c++)
        {
            float sample = pcm[c][i];
            if (sample > 1.0f)
            {
                sample = 1.0f;
            }
            else if (sample < -1.0f)
            {
                sample = -1.0f;
            }
            int16_t value = (int16_t)(sample * 32767.0f);
            out[i * channels + c] = value;
        }
    }

    vorbis_synthesis_read(&priv->vd, frames_to_copy);
    return frames_to_copy * channels * (int)BK_VORBIS_SAMPLE_SIZE_BYTES;
}

int bk_vorbis_stream_decoder_open(audio_format_t format,
                                audio_format_t expected_type,
                                void *param,
                                bk_audio_player_decoder_t **decoder_pp)
{
    (void)param;

    if (format != expected_type)
    {
        return AUDIO_PLAYER_INVALID;
    }

    bk_audio_player_decoder_t *decoder = audio_codec_new(sizeof(vorbis_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;
    os_memset(priv, 0, sizeof(*priv));

    ogg_sync_init(&priv->oy);
    vorbis_info_init(&priv->vi);
    vorbis_comment_init(&priv->vc);

    *decoder_pp = decoder;
    return AUDIO_PLAYER_OK;
}

int bk_vorbis_stream_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    if (!decoder || !info)
    {
        return AUDIO_PLAYER_ERR;
    }

    vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;
    int ret = bk_vorbis_parse_headers(decoder, priv);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    info->channel_number = (int)priv->channels;
    info->sample_rate = (int)priv->sample_rate;
    info->sample_bits = 16;
    info->frame_size = (int)priv->chunk_bytes;
    info->bps = AUDIO_INFO_UNKNOWN;
    info->total_bytes = AUDIO_INFO_UNKNOWN;
    info->header_bytes = AUDIO_INFO_UNKNOWN;
    info->duration = AUDIO_INFO_UNKNOWN;

    decoder->info = *info;
    return AUDIO_PLAYER_OK;
}

int bk_vorbis_stream_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    if (!decoder || !buffer || len <= 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;
    if (!priv->decoder_ready)
    {
        if (bk_vorbis_parse_headers(decoder, priv) != AUDIO_PLAYER_OK)
        {
            return AUDIO_PLAYER_ERR;
        }
    }

    if (priv->need_reset)
    {
        ogg_sync_reset(&priv->oy);
        if (priv->stream_initialized)
        {
            ogg_stream_reset_serialno(&priv->os, (int)priv->stream_serial);
        }
        priv->end_of_stream = false;
        if (priv->decoder_ready)
        {
            vorbis_synthesis_restart(&priv->vd);
            vorbis_block_clear(&priv->vb);
            if (vorbis_block_init(&priv->vd, &priv->vb) != 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "vorbis: block init failed while resetting\n");
                return AUDIO_PLAYER_ERR;
            }
        }
        if (priv->pending_seek_valid)
        {
            priv->stream_offset = priv->pending_seek_offset;
            priv->pending_seek_valid = false;
        }
        priv->need_reset = false;
    }

    int total_written = 0;

    while (total_written < len)
    {
        float **pcm = NULL;
        int samples = vorbis_synthesis_pcmout(&priv->vd, &pcm);
        if (samples > 0)
        {
            int bytes = bk_vorbis_convert_pcm(priv, pcm, samples, buffer + total_written, len - total_written);
            if (bytes <= 0)
            {
                break;
            }
            total_written += bytes;
            continue;
        }

        if (priv->end_of_stream)
        {
            break;
        }

        int ret = bk_vorbis_decode_next_packet(decoder, priv);
        if (ret == AUDIO_PLAYER_OK)
        {
            continue;
        }
        else if (ret == 0)
        {
            if (priv->end_of_stream)
            {
                break;
            }
            rtos_delay_milliseconds(5);
            continue;
        }
        else if (ret == AUDIO_PLAYER_TIMEOUT)
        {
            return total_written > 0 ? total_written : AUDIO_PLAYER_TIMEOUT;
        }

        return ret;
    }

    return total_written;
}

int bk_vorbis_stream_decoder_close(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return AUDIO_PLAYER_OK;
    }

    vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;

    bk_vorbis_stream_destroy_decoder(priv);
    bk_vorbis_stream_reset(priv);
    os_memset(priv, 0, sizeof(*priv));

    return AUDIO_PLAYER_OK;
}

int bk_vorbis_stream_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return DEFAULT_CHUNK_SIZE;
    }

    vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;
    if (priv->chunk_bytes == 0)
    {
        priv->chunk_bytes = bk_vorbis_default_chunk_bytes(priv->channels ? priv->channels : 2,
                                                          priv->sample_rate ? priv->sample_rate : 44100);
    }
    return (int)priv->chunk_bytes;
}

int bk_vorbis_stream_calc_position(bk_audio_player_decoder_t *decoder, int second)
{
    if (!decoder || second < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;
    if (!priv)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (!priv->decoder_ready)
    {
        if (bk_vorbis_parse_headers(decoder, priv) != AUDIO_PLAYER_OK)
        {
            return AUDIO_PLAYER_ERR;
        }
    }

    if (priv->sample_rate == 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    int64_t target_granule = (int64_t)second * (int64_t)priv->sample_rate;
    if (target_granule < 0)
    {
        target_granule = 0;
    }

    uint32_t seek_offset = 0;
    int ret = bk_vorbis_scan_to_granule(decoder, priv, target_granule, &seek_offset);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    priv->pending_seek_offset = seek_offset;
    priv->pending_seek_valid = true;
    priv->need_reset = true;
    return (int)seek_offset;
}

int bk_vorbis_stream_is_seek_ready(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return 0;
    }

    vorbis_decoder_priv_t *priv = (vorbis_decoder_priv_t *)decoder->decoder_priv;
    if (!priv)
    {
        return 0;
    }

    return (priv->decoder_ready && !priv->need_reset) ? 1 : 0;
}
