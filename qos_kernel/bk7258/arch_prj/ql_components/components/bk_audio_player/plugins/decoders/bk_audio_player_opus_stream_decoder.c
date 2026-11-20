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
#include <modules/opus.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"


#define BK_OPUS_READ_CHUNK_BYTES      (4096)
#define BK_OPUS_MAX_FRAME_SAMPLES     (5760) /* 120 ms @ 48 KHz */
#define BK_OPUS_SAMPLE_SIZE_BYTES     ((uint32_t)sizeof(int16_t))
#define BK_OPUS_READ_RETRY            (5)
#define BK_OPUS_TARGET_FRAME_MS       (20)

/* Shared Ogg-Opus bitstream handler that feeds both .ogg and .opus decoders. */

typedef struct bk_opus_head_s
{
    uint8_t version;
    uint8_t channel_count;
    uint16_t pre_skip;
    uint32_t input_sample_rate;
    int16_t output_gain;
    uint8_t mapping_family;
} bk_opus_head_t;

typedef struct opus_decoder_priv
{
    ogg_sync_state    oy;
    ogg_stream_state  os;
    ogg_page          og;
    ogg_packet        op;
    bool              stream_initialized;
    bool              header_parsed;
    OpusDecoder      *decoder;
    uint32_t          sample_rate;
    uint32_t          channels;
    uint32_t          chunk_bytes;
    uint64_t          stream_offset;
    int               stream_serial;
    bool              need_reset;
    uint32_t          pending_seek_offset;
    bool              pending_seek_valid;
} opus_decoder_priv_t;

static uint32_t bk_opus_default_chunk_bytes(uint32_t channels, uint32_t sample_rate)
{
    uint64_t samples;

    if (channels == 0)
    {
        channels = 1;
    }
    if (sample_rate == 0)
    {
        sample_rate = 48000;
    }

    samples = (uint64_t)sample_rate * BK_OPUS_TARGET_FRAME_MS;
    samples = (samples + 999) / 1000; /* round up to whole samples */

    if (samples == 0)
    {
        samples = BK_OPUS_MAX_FRAME_SAMPLES / 3;
    }

    return (uint32_t)(samples * channels * BK_OPUS_SAMPLE_SIZE_BYTES);
}

/* Read helper with timeout retry to keep decoding resilient against slow sources. */
static int bk_opus_stream_read(bk_audio_player_decoder_t *decoder, char *buffer, size_t request_bytes)
{
    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;
    int bytes = audio_source_read_data(decoder->source, buffer, request_bytes);
    if (bytes > 0 && priv)
    {
        priv->stream_offset += (uint32_t)bytes;
    }

    return bytes;
}

/* Feed additional ogg bytes into the sync buffer. */
static int bk_opus_stream_fill(bk_audio_player_decoder_t *decoder, opus_decoder_priv_t *priv)
{
    char *buffer = ogg_sync_buffer(&priv->oy, BK_OPUS_READ_CHUNK_BYTES);
    if (!buffer)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus: unable to obtain ogg buffer\n");
        return AUDIO_PLAYER_ERR;
    }

    int bytes = bk_opus_stream_read(decoder, buffer, BK_OPUS_READ_CHUNK_BYTES);
    if (bytes > 0)
    {
        ogg_sync_wrote(&priv->oy, bytes);
    }

    return bytes;
}

/* Initialises ogg logical stream once the first BOS page is seen. */
static int bk_opus_stream_init_chain(opus_decoder_priv_t *priv)
{
    if (priv->stream_initialized)
    {
        return AUDIO_PLAYER_OK;
    }

    int serial = ogg_page_serialno(&priv->og);
    if (ogg_stream_init(&priv->os, serial) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus: ogg_stream_init failed\n");
        return AUDIO_PLAYER_ERR;
    }

    priv->stream_initialized = true;
    priv->stream_serial = serial;
    return AUDIO_PLAYER_OK;
}

/* Release ogg resources; used during close. */
static void bk_opus_stream_reset(opus_decoder_priv_t *priv)
{
    if (priv->stream_initialized)
    {
        ogg_stream_clear(&priv->os);
        priv->stream_initialized = false;
    }
    ogg_sync_clear(&priv->oy);
}

static void bk_opus_stream_restart(opus_decoder_priv_t *priv)
{
    bk_opus_stream_reset(priv);
    ogg_sync_init(&priv->oy);
    priv->stream_initialized = false;
}

/* Configure opus decoder once OpusHead has been parsed. */
static int bk_opus_prepare_decoder(opus_decoder_priv_t *priv, const bk_opus_head_t *head)
{
    int error = OPUS_OK;

    if (priv->decoder)
    {
        opus_decoder_destroy(priv->decoder);
        priv->decoder = NULL;
    }

    priv->sample_rate = head->input_sample_rate ? head->input_sample_rate : 48000;
    priv->channels = head->channel_count ? head->channel_count : 1;
    priv->chunk_bytes = bk_opus_default_chunk_bytes(priv->channels, priv->sample_rate);

    priv->decoder = opus_decoder_create((opus_int32)priv->sample_rate, (int)priv->channels, &error);
    if (!priv->decoder || error != OPUS_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus: opus_decoder_create failed, err=%d\n", error);
        if (priv->decoder)
        {
            opus_decoder_destroy(priv->decoder);
            priv->decoder = NULL;
        }
        return AUDIO_PLAYER_ERR;
    }

    return AUDIO_PLAYER_OK;
}

static int bk_opus_parse_head_packet(const uint8_t *packet, uint32_t bytes, bk_opus_head_t *head)
{
    if (!packet || bytes < 19 || os_memcmp(packet, "OpusHead", 8) != 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    head->version = packet[8];
    head->channel_count = packet[9];
    head->pre_skip = (uint16_t)packet[10] | ((uint16_t)packet[11] << 8);
    head->input_sample_rate = ((uint32_t)packet[12]) |
                              ((uint32_t)packet[13] << 8) |
                              ((uint32_t)packet[14] << 16) |
                              ((uint32_t)packet[15] << 24);
    head->output_gain = (int16_t)((uint16_t)packet[16] | ((uint16_t)packet[17] << 8));
    head->mapping_family = packet[18];
    return AUDIO_PLAYER_OK;
}

static int bk_opus_parse_head(bk_audio_player_decoder_t *decoder, opus_decoder_priv_t *priv)
{
    while (!priv->header_parsed)
    {
        int page_status = ogg_sync_pageout(&priv->oy, &priv->og);
        if (page_status == 1)
        {
            if (bk_opus_stream_init_chain(priv) != AUDIO_PLAYER_OK)
            {
                return AUDIO_PLAYER_ERR;
            }

            if (ogg_stream_pagein(&priv->os, &priv->og) != 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "opus: ogg_stream_pagein failed\n");
                continue;
            }

            while (ogg_stream_packetout(&priv->os, &priv->op) > 0)
            {
                if (priv->op.packetno == 0)
                {
                    // Check if this is a Vorbis file (not supported)
                    if (priv->op.bytes >= 7 && priv->op.packet[0] == 0x01)
                    {
                        if (os_memcmp(priv->op.packet + 1, "vorbis", 6) == 0)
                        {
                            BK_LOGE(AUDIO_PLAYER_TAG, "opus: Ogg file contains Vorbis stream (not supported, only Opus is supported)\n");
                            return AUDIO_PLAYER_ERR;
                        }
                    }

                    bk_opus_head_t head;
                    if (bk_opus_parse_head_packet(priv->op.packet, (uint32_t)priv->op.bytes, &head) != AUDIO_PLAYER_OK)
                    {
                        BK_LOGE(AUDIO_PLAYER_TAG, "opus: invalid OpusHead (expected Opus stream in Ogg container)\n");
                        return AUDIO_PLAYER_ERR;
                    }

                    if (bk_opus_prepare_decoder(priv, &head) != AUDIO_PLAYER_OK)
                    {
                        return AUDIO_PLAYER_ERR;
                    }

                    priv->header_parsed = true;
                    return AUDIO_PLAYER_OK;
                }
            }
        }
        else if (page_status < 0)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "opus: skipped corrupt ogg page\n");
        }
        else
        {
            int bytes = bk_opus_stream_fill(decoder, priv);
            if (bytes == AUDIO_PLAYER_TIMEOUT)
            {
                rtos_delay_milliseconds(5);
                continue;
            }
            if (bytes <= 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "opus: unexpected end of stream while parsing header\n");
                return AUDIO_PLAYER_ERR;
            }
        }
    }

    return AUDIO_PLAYER_OK;
}

int bk_opus_stream_decoder_open(audio_format_t format,
                                audio_format_t expected_type,
                                void *param,
                                bk_audio_player_decoder_t **decoder_pp)
{
    (void)param;

    if (format != expected_type)
    {
        return AUDIO_PLAYER_INVALID;
    }

    bk_audio_player_decoder_t *decoder = audio_codec_new(sizeof(opus_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;
    os_memset(priv, 0, sizeof(opus_decoder_priv_t));

    ogg_sync_init(&priv->oy);
    priv->sample_rate = 48000;
    priv->channels = 2;
    priv->chunk_bytes = bk_opus_default_chunk_bytes(priv->channels, priv->sample_rate);
    priv->stream_offset = 0;
    priv->stream_serial = 0;
    priv->need_reset = false;
    priv->pending_seek_offset = 0;
    priv->pending_seek_valid = false;

    *decoder_pp = decoder;
    return AUDIO_PLAYER_OK;
}

int bk_opus_stream_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    if (!decoder || !info)
    {
        return AUDIO_PLAYER_ERR;
    }

    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;

    int ret = bk_opus_parse_head(decoder, priv);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    info->channel_number = priv->channels;
    info->sample_rate = priv->sample_rate;
    info->sample_bits = 16;
    info->frame_size = priv->chunk_bytes;
    info->bps = AUDIO_INFO_UNKNOWN;
    info->total_bytes = AUDIO_INFO_UNKNOWN;
    info->header_bytes = AUDIO_INFO_UNKNOWN;
    info->duration = AUDIO_INFO_UNKNOWN;

    decoder->info = *info;

    return AUDIO_PLAYER_OK;
}

static int bk_opus_decode_next_packet(bk_audio_player_decoder_t *decoder,
                                      opus_decoder_priv_t *priv,
                                      char *buffer,
                                      int len)
{
    int max_samples = len / (int)(priv->channels * BK_OPUS_SAMPLE_SIZE_BYTES);
    if (max_samples <= 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus: insufficient buffer for decode\n");
        return AUDIO_PLAYER_ERR;
    }

    while (priv->stream_initialized && ogg_stream_packetout(&priv->os, &priv->op) > 0)
    {
        if (priv->op.packetno < 2)
        {
            continue;
        }

        int samples = opus_decode(priv->decoder,
                                  priv->op.packet,
                                  priv->op.bytes,
                                  (opus_int16 *)buffer,
                                  max_samples,
                                  0);
        if (samples < 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "opus: opus_decode failed, err=%d\n", samples);
            return AUDIO_PLAYER_ERR;
        }

        if (samples == 0)
        {
            continue;
        }

        return samples * (int)(priv->channels * BK_OPUS_SAMPLE_SIZE_BYTES);
    }

    return 0;
}

int bk_opus_stream_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    if (!decoder || !buffer || len <= 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;

    if (priv->need_reset)
    {
        bk_opus_stream_restart(priv);
        if (priv->decoder)
        {
            opus_decoder_ctl(priv->decoder, OPUS_RESET_STATE);
        }
        if (priv->pending_seek_valid)
        {
            priv->stream_offset = priv->pending_seek_offset;
            priv->pending_seek_valid = false;
        }
        priv->need_reset = false;
    }

    if (!priv->header_parsed)
    {
        int ret = bk_opus_parse_head(decoder, priv);
        if (ret != AUDIO_PLAYER_OK)
        {
            return ret;
        }
    }

    if (!priv->decoder)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus: decoder not initialized\n");
        return AUDIO_PLAYER_ERR;
    }

    while (1)
    {
        int decoded_bytes = bk_opus_decode_next_packet(decoder, priv, buffer, len);
        if (decoded_bytes > 0)
        {
            return decoded_bytes;
        }
        else if (decoded_bytes < 0)
        {
            return AUDIO_PLAYER_ERR;
        }

        int page_status = ogg_sync_pageout(&priv->oy, &priv->og);
        if (page_status == 1)
        {
            if (!priv->stream_initialized && bk_opus_stream_init_chain(priv) != AUDIO_PLAYER_OK)
            {
                return AUDIO_PLAYER_ERR;
            }

            if (ogg_stream_pagein(&priv->os, &priv->og) != 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "opus: ogg_stream_pagein failed\n");
            }
            continue;
        }
        else if (page_status < 0)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "opus: skipped corrupt ogg page during decode\n");
            continue;
        }

        int bytes = bk_opus_stream_fill(decoder, priv);
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

static int bk_opus_scan_to_granule(bk_audio_player_decoder_t *decoder,
                                   opus_decoder_priv_t *priv,
                                   int64_t target_granule,
                                   uint32_t *offset_out)
{
    if (!priv || !offset_out)
    {
        return AUDIO_PLAYER_ERR;
    }

    uint32_t total_bytes = audio_source_get_total_bytes(decoder->source);
    if (total_bytes == 0U)
    {
        return AUDIO_PLAYER_ERR;
    }

    uint64_t resume_offset = priv->stream_offset;
    if (audio_source_seek(decoder->source, 0, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        return AUDIO_PLAYER_ERR;
    }
    priv->stream_offset = 0;

    ogg_sync_state scanner;
    ogg_sync_init(&scanner);
    ogg_page page;
    uint64_t consumed = 0;
    bool found = false;
    uint32_t candidate_offset = 0;
    int64_t candidate_granule = 0;

    while (consumed < total_bytes)
    {
        char *buf = ogg_sync_buffer(&scanner, BK_OPUS_READ_CHUNK_BYTES);
        int bytes = audio_source_read_data(decoder->source, buf, BK_OPUS_READ_CHUNK_BYTES);
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
            if (priv->stream_serial && serial != priv->stream_serial)
            {
                continue;
            }
            else if (!priv->stream_serial)
            {
                priv->stream_serial = serial;
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
    audio_source_seek(decoder->source, (int)resume_offset, SEEK_SET);
    priv->stream_offset = resume_offset;

    if (!found && candidate_granule == 0)
    {
        *offset_out = 0;
        return AUDIO_PLAYER_ERR;
    }

    *offset_out = candidate_offset;
    return AUDIO_PLAYER_OK;
}

int bk_opus_stream_calc_position(bk_audio_player_decoder_t *decoder, int second)
{
    if (!decoder || second < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;
    if (!priv)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (!priv->header_parsed)
    {
        if (bk_opus_parse_head(decoder, priv) != AUDIO_PLAYER_OK)
        {
            return AUDIO_PLAYER_ERR;
        }
    }

    int64_t target_granule = (int64_t)second * 48000;
    uint32_t seek_offset = 0;
    int ret = bk_opus_scan_to_granule(decoder, priv, target_granule, &seek_offset);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    priv->pending_seek_offset = seek_offset;
    priv->pending_seek_valid = true;
    priv->need_reset = true;
    return (int)seek_offset;
}

int bk_opus_stream_is_seek_ready(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return 0;
    }

    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;
    if (!priv)
    {
        return 0;
    }

    return (priv->decoder != NULL && !priv->need_reset) ? 1 : 0;
}

int bk_opus_stream_decoder_close(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return AUDIO_PLAYER_OK;
    }

    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;

    if (priv->decoder)
    {
        opus_decoder_destroy(priv->decoder);
        priv->decoder = NULL;
    }

    bk_opus_stream_reset(priv);
    os_memset(&priv->oy, 0, sizeof(priv->oy));
    os_memset(&priv->os, 0, sizeof(priv->os));

    return AUDIO_PLAYER_OK;
}

int bk_opus_stream_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return DEFAULT_CHUNK_SIZE;
    }

    opus_decoder_priv_t *priv = (opus_decoder_priv_t *)decoder->decoder_priv;
    if (priv->chunk_bytes == 0)
    {
        priv->chunk_bytes = bk_opus_default_chunk_bytes(priv->channels ? priv->channels : 2,
                                                        priv->sample_rate);
    }
    return (int)priv->chunk_bytes;
}
