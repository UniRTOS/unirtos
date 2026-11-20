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
#include <modules/flac_dec.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"
#include "ring_buffer.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

#define FLAC_DEC_MIN_BLOCK_SAMPLES      4096


enum FALC_PHASE
{
    FALC_PHASE_INIT,
    FLAC_PHASE_PARSE_DONE,
    FLAC_DECODE_STARTING,
    FLAC_DECODE_DONE
};


typedef struct flac_decoder_priv
{
    bk_audio_player_decoder_t *decoder;
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bps;
    uint32_t pcm_sameples;
    enum FALC_PHASE dec_phase;

    uint8_t *dec_buffer;
    uint32_t dec_buffer_offset;
    uint32_t dec_buffer_size;
    bool runing;
    bool decoder_opened;
    bool end_of_stream;
    bool read_timeout;

    flac_dec_setup_t setup;
    flac_dec_data_t dec_data;
    flac_dec_meta_info_t meta_info;

    // Seek related fields
    uint32_t first_frame_offset;
    uint64_t total_samples;
    uint64_t decoded_samples;
    uint64_t pending_seek_samples;
    bool seek_pending;
    uint32_t pending_seek_offset;

} flac_decoder_priv_t;

/* Forward declarations for the bk flac glue callbacks. */
static unsigned int flac_decoder_read_callback(unsigned char *buffer, long unsigned int bytes, void *data);
static unsigned int flac_decoder_write_callback(unsigned char *buffer, long unsigned int bytes, void *data);
static void flac_decoder_metadata_callback(void *data);
static void flac_decoder_error_callback(void *data);
static bool flac_verify_frame_header(bk_audio_player_decoder_t *decoder, uint32_t offset, bool strict_match);
static int flac_find_frame_sync(bk_audio_player_decoder_t *decoder, uint32_t start_offset, uint32_t search_limit, uint32_t *frame_offset, bool strict_match);

static uint8_t flac_crc8_update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (int i = 0; i < 8; i++)
    {
        if (crc & 0x80)
        {
            crc = (uint8_t)((crc << 1) ^ 0x07);
        }
        else
        {
            crc <<= 1;
        }
    }
    return crc & 0xFF;
}

static int flac_parse_utf8_uint(const uint8_t *buffer, int available, uint32_t *value)
{
    if (!buffer || !value || available <= 0)
    {
        return -1;
    }

    uint8_t first = buffer[0];
    int needed = 0;
    uint32_t result = 0;

    if ((first & 0x80) == 0)
    {
        needed = 1;
        result = first & 0x7F;
    }
    else if ((first & 0xE0) == 0xC0)
    {
        needed = 2;
        result = first & 0x1F;
    }
    else if ((first & 0xF0) == 0xE0)
    {
        needed = 3;
        result = first & 0x0F;
    }
    else if ((first & 0xF8) == 0xF0)
    {
        needed = 4;
        result = first & 0x07;
    }
    else if ((first & 0xFC) == 0xF8)
    {
        needed = 5;
        result = first & 0x03;
    }
    else if ((first & 0xFE) == 0xFC)
    {
        needed = 6;
        result = first & 0x01;
    }
    else
    {
        return -1;
    }

    if (needed > available)
    {
        return -1;
    }

    for (int i = 1; i < needed; i++)
    {
        uint8_t byte = buffer[i];
        if ((byte & 0xC0) != 0x80)
        {
            return -1;
        }
        result = (result << 6) | (byte & 0x3F);
    }

    *value = result;
    return needed;
}

static unsigned int flac_decoder_read_callback(unsigned char *buffer, long unsigned int bytes, void *data)
{
    int size = 0;
    flac_dec_data_t *dec_data = (flac_dec_data_t *)data;
    flac_decoder_priv_t *priv = (flac_decoder_priv_t *)dec_data->usr_param;
    bk_audio_player_decoder_t *decoder = priv->decoder;

    /* Guard invalid arguments before attempting any IO. */
    if ((buffer == NULL) || (bytes == 0) || (decoder == NULL))
    {
        priv->runing = false;
        return 0;
    }

    if (priv->total_samples > 0 && priv->decoded_samples >= priv->total_samples)
    {
        priv->end_of_stream = true;
        return 0;
    }

    size = audio_source_read_data(decoder->source, (char *)buffer, bytes);
    if (size == AUDIO_PLAYER_TIMEOUT)
    {
        priv->read_timeout = true;
        return 0;
    }
    else if (size == 0 || size == RB_DONE)
    {
        priv->end_of_stream = true;
        BK_LOGI(AUDIO_PLAYER_TAG, "FLAC read reach end of stream\n");
        return 0;
    }
    else if (size < 0)
    {
        priv->runing = false;
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC read error:%d\n", size);
        return 0;
    }

    priv->read_timeout = false;
    return (unsigned int)size;
}

static unsigned int flac_decoder_write_callback(unsigned char *buffer, long unsigned int bytes, void *data)
{
    flac_dec_data_t *dec_data = (flac_dec_data_t *)data;
    flac_decoder_priv_t *priv = (flac_decoder_priv_t *)dec_data->usr_param;

    /* Decoder notifies us with PCM frames; copy to caller-provided buffer. */
    if ((buffer == NULL) || (bytes == 0))
    {
        return 0;
    }

    if ((priv->dec_buffer == NULL) || (priv->dec_buffer_size == 0))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC write buffer invalid\n");
        priv->runing = false;
        return 0;
    }

    if (priv->dec_buffer_offset >= priv->dec_buffer_size)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "FLAC write overflow detected\n");
        priv->runing = false;
        return 0;
    }

    uint32_t remain = priv->dec_buffer_size - priv->dec_buffer_offset;
    uint32_t copy_size = (bytes > remain) ? remain : bytes;

    os_memcpy(priv->dec_buffer + priv->dec_buffer_offset, buffer, copy_size);
    priv->dec_buffer_offset += copy_size;
    priv->dec_phase = FLAC_DECODE_DONE;

    if (copy_size < bytes)
    {
        // This is normal behavior when decoder produces more data than buffer space
        // Log at DEBUG level to avoid spam during normal playback
        //BK_LOGD(AUDIO_PLAYER_TAG, "FLAC truncated write (%lu -> %u)\n", bytes, copy_size);
    }

    if (priv->channels > 0 && priv->bps > 0)
    {
        uint32_t bytes_per_sample = (priv->bps / 8) * priv->channels;
        if (bytes_per_sample > 0)
        {
            priv->decoded_samples += (copy_size / bytes_per_sample);
            if (priv->total_samples > 0 && priv->decoded_samples >= priv->total_samples)
            {
                priv->end_of_stream = true;
            }
        }
    }

    return copy_size;
}

static void flac_decoder_metadata_callback(void *data)
{
    flac_dec_data_t *dec_data = (flac_dec_data_t *)data;
    flac_decoder_priv_t *priv = (flac_decoder_priv_t *)dec_data->usr_param;

    /* Cache the parsed stream information for later get_info() queries. */
    if ((dec_data->meta_info == NULL) || (priv == NULL))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC metadata callback with invalid param\n");
        priv->runing = false;
        return;
    }

    priv->sample_rate = dec_data->meta_info->sample_rate;
    priv->channels = dec_data->meta_info->channels;
    priv->bps = dec_data->meta_info->bps;

    if (dec_data->meta_info->total_samples > 0)
    {
        priv->total_samples = dec_data->meta_info->total_samples;
    }

    if (priv->sample_rate == 0 || priv->channels == 0 || priv->bps == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC metadata invalid, sr:%d, ch:%d, bps:%d\n", priv->sample_rate, priv->channels, priv->bps);
        priv->runing = false;
        return;
    }

    priv->pcm_sameples = priv->sample_rate / 20;
    if (priv->pcm_sameples < FLAC_DEC_MIN_BLOCK_SAMPLES)
    {
        priv->pcm_sameples = FLAC_DEC_MIN_BLOCK_SAMPLES;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "FLAC meta sr:%d, ch:%d, bits:%d, total_samples:%llu\n", 
               priv->sample_rate, priv->channels, priv->bps, (unsigned long long)priv->total_samples);
    priv->dec_phase = FLAC_PHASE_PARSE_DONE;
}

static void flac_decoder_error_callback(void *data)
{
    flac_dec_data_t *dec_data = (flac_dec_data_t *)data;
    if (!dec_data)
    {
        return;
    }

    flac_decoder_priv_t *priv = (flac_decoder_priv_t *)dec_data->usr_param;
    if (!priv)
    {
        return;
    }

    bool all_samples_decoded = (priv->total_samples > 0) && (priv->decoded_samples >= priv->total_samples);

    if (all_samples_decoded)
    {
        // Decoder already produced the advertised number of samples.
        // Treat late error callbacks as graceful EOS instead of fatal faults.
        priv->end_of_stream = true;
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC decoder reported error after EOS, ignoring\n");
        return;
    }

    /* Fatal decoder errors stop the decode loop immediately. */
    priv->runing = false;
    BK_LOGE(AUDIO_PLAYER_TAG, "FLAC decoder error callback\n");
}


static int flac_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    bk_audio_player_decoder_t *decoder;
    flac_decoder_priv_t *priv;

    if (format != AUDIO_FORMAT_FLAC)
    {
        return AUDIO_PLAYER_INVALID;
    }

    decoder = audio_codec_new(sizeof(flac_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    os_memset(priv, 0x0, sizeof(flac_decoder_priv_t));
    priv->decoder = decoder;

    priv->dec_phase = FALC_PHASE_INIT;
    priv->runing = true;
    priv->dec_data.meta_info = &priv->meta_info;
    priv->dec_data.usr_param = priv;
    priv->setup.read_callback = flac_decoder_read_callback;
    priv->setup.write_callback = flac_decoder_write_callback;
    priv->setup.metadata_callback = flac_decoder_metadata_callback;
    priv->setup.error_callback = flac_decoder_error_callback;
    priv->setup.param = &priv->dec_data;

    // Initialize seek related fields
    priv->first_frame_offset = 0;
    priv->total_samples = 0;
    priv->decoded_samples = 0;
    priv->pending_seek_samples = 0;
    priv->seek_pending = false;
    priv->pending_seek_offset = 0;

    if (bk_aud_flac_dec_init(&priv->setup) != BK_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "bk_aud_flac_dec_init fail\n");
        player_free(decoder);
        return AUDIO_PLAYER_ERR;
    }

    priv->decoder_opened = true;

    *decoder_pp = decoder;

    return AUDIO_PLAYER_OK;
}


static int flac_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    flac_decoder_priv_t *priv;
    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    if (!priv->decoder_opened)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (priv->dec_phase == FALC_PHASE_INIT)
    {
        if (bk_aud_flac_dec_process() != BK_OK)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "FLAC process meta fail\n");
            return AUDIO_PLAYER_ERR;
        }
    }

    uint32_t wait_cnt = 0;
    while (priv->dec_phase != FLAC_PHASE_PARSE_DONE)
    {
        if (!priv->runing)
        {
            return AUDIO_PLAYER_ERR;
        }

        if (++wait_cnt > 50)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "wait meta timeout\n");
            return AUDIO_PLAYER_ERR;
        }

        BK_LOGD(AUDIO_PLAYER_TAG, "wait FLAC metadata ready\n");
        rtos_delay_milliseconds(20);
    }

    info->channel_number = priv->channels;
    info->sample_rate = priv->sample_rate;
    info->sample_bits = priv->bps;
    info->frame_size = priv->pcm_sameples * info->channel_number * (info->sample_bits / 8);
    info->bps = info->sample_rate * info->sample_bits * info->channel_number;

    decoder->info = *info;

    // Note: We don't find first frame here to avoid interfering with decoder's file reading
    // Finding first frame requires changing file position, which may cause decoder to lose sync
    // First frame will be found when needed (during seek operation in calc_position)
    // This ensures normal playback is not affected

    return AUDIO_PLAYER_OK;
}

int flac_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    uint32_t now_time, start_time;
    flac_decoder_priv_t *priv;
    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    if (!priv->decoder_opened)
    {
        return AUDIO_PLAYER_ERR;
    }

    if ((buffer == NULL) || (len <= 0))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC invalid buffer request\n");
        return AUDIO_PLAYER_ERR;
    }

    // Handle pending seek operation
    if (priv->seek_pending && priv->pending_seek_offset > 0)
    {
        uint32_t target_offset = priv->pending_seek_offset;
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: pending seek detected, target offset=%u\n", target_offset);

        // Step 1: rewind to start so decoder can reparse metadata safely
        if (audio_source_seek(decoder->source, 0, SEEK_SET) != AUDIO_PLAYER_OK)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to rewind file before seek\n");
            priv->seek_pending = false;
            priv->pending_seek_offset = 0;
            priv->pending_seek_samples = 0;
            return AUDIO_PLAYER_ERR;
        }

        if (bk_aud_flac_dec_deinit() != BK_OK)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to deinit decoder for seek\n");
            priv->runing = false;
            priv->seek_pending = false;
            priv->pending_seek_offset = 0;
            priv->pending_seek_samples = 0;
            return AUDIO_PLAYER_ERR;
        }

        if (bk_aud_flac_dec_init(&priv->setup) != BK_OK)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to reinit decoder after seek\n");
            priv->runing = false;
            priv->seek_pending = false;
            priv->pending_seek_offset = 0;
            priv->pending_seek_samples = 0;
            return AUDIO_PLAYER_ERR;
        }

        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: decoder reinit complete, reparsing metadata before seek\n");

        priv->dec_phase = FALC_PHASE_INIT;
        priv->end_of_stream = false;
        priv->read_timeout = false;
        priv->runing = true;

        // Reparse metadata (same as initial startup)
        uint32_t parse_wait = 0;
        while (priv->dec_phase != FLAC_PHASE_PARSE_DONE)
        {
            if (!priv->runing)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: decoder stopped while reparsing metadata\n");
                priv->seek_pending = false;
                priv->pending_seek_offset = 0;
                priv->pending_seek_samples = 0;
                return AUDIO_PLAYER_ERR;
            }

            if (bk_aud_flac_dec_process() != BK_OK)
            {
                rtos_delay_milliseconds(5);
            }

            if (++parse_wait > 200)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: metadata reparse timeout during seek\n");
                priv->seek_pending = false;
                priv->pending_seek_offset = 0;
                priv->pending_seek_samples = 0;
                return AUDIO_PLAYER_ERR;
            }
        }

        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: metadata ready, positioning to offset %u\n", target_offset);

        if (audio_source_seek(decoder->source, target_offset, SEEK_SET) != AUDIO_PLAYER_OK)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: seek failed to offset %u after metadata reparse\n", target_offset);
            priv->seek_pending = false;
            priv->pending_seek_offset = 0;
            priv->pending_seek_samples = 0;
            return AUDIO_PLAYER_ERR;
        }

        if (bk_aud_flac_dec_flush() != BK_OK)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "FLAC: decoder flush after seek failed\n");
        }

        priv->seek_pending = false;
        priv->pending_seek_offset = 0;
        priv->decoded_samples = priv->pending_seek_samples;
        priv->pending_seek_samples = 0;
        priv->dec_buffer_offset = 0;
        priv->end_of_stream = false;
        priv->read_timeout = false;
        priv->dec_phase = FLAC_DECODE_STARTING;
        priv->runing = true;
    }

    if (priv->total_samples > 0 && priv->decoded_samples >= priv->total_samples)
    {
        priv->end_of_stream = true;
        BK_LOGI(AUDIO_PLAYER_TAG, "FLAC decode already reached end of stream, returning 0 bytes\n");
        return 0;
    }

    priv->dec_buffer = (uint8_t *)buffer;
    priv->dec_buffer_offset = 0;
    priv->dec_buffer_size = (uint32_t)len;
    priv->dec_phase = FLAC_DECODE_STARTING;
    priv->end_of_stream = false;
    priv->read_timeout = false;

    beken_time_get_time(&start_time);

    /* Pump bk flac core until one PCM chunk is produced or EOS/error occurs. */
    while (priv->dec_phase != FLAC_DECODE_DONE)
    {
        if (priv->end_of_stream)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "FLAC decode reach end of stream, returning %u bytes\n", priv->dec_buffer_offset);
            return priv->dec_buffer_offset > 0 ? priv->dec_buffer_offset : 0;
        }

        if (priv->read_timeout)
        {
            priv->read_timeout = false;
            return priv->dec_buffer_offset > 0 ? priv->dec_buffer_offset : AUDIO_PLAYER_TIMEOUT;
        }

        if (!priv->runing)
        {
            return -1;
        }

        if (bk_aud_flac_dec_process() != BK_OK)
        {
            if (priv->read_timeout)
            {
                priv->read_timeout = false;
                return priv->dec_buffer_offset > 0 ? priv->dec_buffer_offset : AUDIO_PLAYER_TIMEOUT;
            }
            BK_LOGE(AUDIO_PLAYER_TAG, "FLAC process frame fail\n");
            return 0;
        }

        beken_time_get_time(&now_time);
        if ((now_time - start_time) > 1000)
        {
            //BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: decode timeout after 1s, returning %u bytes\n", priv->dec_buffer_offset);
            return priv->dec_buffer_offset > 0 ? priv->dec_buffer_offset : AUDIO_PLAYER_TIMEOUT;
        }

        rtos_delay_milliseconds(20);
    }

    //BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: decode done, returning %u bytes (phase=%d)\n", priv->dec_buffer_offset, priv->dec_phase);
    return priv->dec_buffer_offset > 0 ? priv->dec_buffer_offset : 0;
}

// FLAC frame sync code: 0xFFF8 (14 bits)
// First byte: 0xFF
// Second byte: 0xF8-0xFE (bits 6-7 must be 11, bit 1 must be 0 for reserved bit)
#define FLAC_FRAME_SYNC_BYTE1    0xFF
#define FLAC_FRAME_SYNC_BYTE2_MIN 0xF8
#define FLAC_FRAME_SYNC_BYTE2_MAX 0xFE

// Check if a byte sequence could be a FLAC frame sync
static bool flac_is_sync_code(uint8_t byte1, uint8_t byte2)
{
    if (byte1 != FLAC_FRAME_SYNC_BYTE1)
    {
        return false;
    }

    // Check if byte2 is in valid range and reserved bit (bit 1) is 0
    if (byte2 < FLAC_FRAME_SYNC_BYTE2_MIN || byte2 > FLAC_FRAME_SYNC_BYTE2_MAX)
    {
        return false;
    }

    // Reserved bit (bit 1, 0-indexed) must be 0
    if ((byte2 & 0x02) != 0)
    {
        return false;
    }

    return true;
}

// Map FLAC bits-per-sample code (frame header) to actual bits
static int flac_get_bits_per_sample_from_code(uint8_t code)
{
    switch (code)
    {
        case 0:  // get from STREAMINFO
            return 0;
        case 1:
            return 8;
        case 2:
            return 12;
        case 4:
            return 16;
        case 5:
            return 20;
        case 6:
            return 24;
        default:
            return -1; // reserved / invalid codes (3, 7)
    }
}

// Verify FLAC frame header at given position
// Returns true if frame header appears valid, false otherwise
// Note: This function modifies file position - caller should restore if needed
// If strict_match is false, only basic validation is performed (for first frame finding)
static bool flac_verify_frame_header(bk_audio_player_decoder_t *decoder, uint32_t offset, bool strict_match)
{
    uint8_t header[64];
    int read_size;
    flac_decoder_priv_t *priv;

    if (!decoder || !decoder->source || !decoder->decoder_priv)
    {
        return false;
    }

    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    if (audio_source_seek(decoder->source, offset, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        return false;
    }

    read_size = audio_source_read_data(decoder->source, (char *)header, sizeof(header));
    if (read_size < 5)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: verify_frame_header: insufficient bytes (%d) at %u\n", read_size, offset);
        return false;
    }

    if (!flac_is_sync_code(header[0], header[1]))
    {
        if (strict_match)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: verification failed at offset %u - invalid sync 0x%02X%02X\n",
                       offset, header[0], header[1]);
        }
        return false;
    }

    uint8_t block_size_code = (header[2] >> 4) & 0x0F;
    uint8_t sample_rate_code = header[2] & 0x0F;
    uint8_t channel_code = (header[3] >> 4) & 0x0F;
    uint8_t bits_per_sample_field = (header[3] >> 1) & 0x07;
    int frame_bits_per_sample = flac_get_bits_per_sample_from_code(bits_per_sample_field);

    if (channel_code == 15 || frame_bits_per_sample < 0 || block_size_code == 0 || block_size_code == 15 ||
        sample_rate_code == 15)
    {
        return false;
    }

    uint8_t crc = 0;
    crc = flac_crc8_update(crc, header[0]);
    crc = flac_crc8_update(crc, header[1]);
    crc = flac_crc8_update(crc, header[2]);
    crc = flac_crc8_update(crc, header[3]);
    int idx = 4;

    uint32_t frame_counter = 0;
    int utf_len = flac_parse_utf8_uint(&header[idx], read_size - idx, &frame_counter);
    if (utf_len <= 0)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: invalid UTF-8 frame counter at %u\n", offset);
        return false;
    }
    for (int i = 0; i < utf_len; i++)
    {
        crc = flac_crc8_update(crc, header[idx + i]);
    }
    idx += utf_len;

    if (block_size_code == 6)
    {
        if (idx >= read_size)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: missing blocksize byte at %u\n", offset);
            return false;
        }
        crc = flac_crc8_update(crc, header[idx]);
        idx += 1;
    }
    else if (block_size_code == 7)
    {
        if (idx + 1 >= read_size)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: missing blocksize 16-bit at %u\n", offset);
            return false;
        }
        crc = flac_crc8_update(crc, header[idx]);
        crc = flac_crc8_update(crc, header[idx + 1]);
        idx += 2;
    }

    if (sample_rate_code == 12)
    {
        if (idx >= read_size)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: missing sample-rate byte at %u\n", offset);
            return false;
        }
        crc = flac_crc8_update(crc, header[idx]);
        idx += 1;
    }
    else if (sample_rate_code == 13 || sample_rate_code == 14)
    {
        if (idx + 1 >= read_size)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: missing sample-rate 16-bit at %u\n", offset);
            return false;
        }
        crc = flac_crc8_update(crc, header[idx]);
        crc = flac_crc8_update(crc, header[idx + 1]);
        idx += 2;
    }

    if (idx >= read_size)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: missing CRC byte at %u\n", offset);
        return false;
    }

    uint8_t expected_crc = header[idx];
    if (crc != expected_crc)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: CRC mismatch at %u (calc=0x%02x, header=0x%02x)\n",
                   offset, crc, expected_crc);
        return false;
    }

    if (strict_match && priv->sample_rate > 0 && priv->channels > 0 && priv->bps > 0)
    {
        uint8_t frame_channels = (channel_code <= 7) ? (channel_code + 1) : 2;
        if (frame_channels != priv->channels)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: channel mismatch at %u (frame=%u expected=%u)\n",
                       offset, frame_channels, priv->channels);
            return false;
        }

        int expected_bps = priv->bps;
        int compare_bps = frame_bits_per_sample == 0 ? expected_bps : frame_bits_per_sample;
        if (expected_bps > 0 && compare_bps != expected_bps)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: bits-per-sample mismatch at %u (frame=%u expected=%u)\n",
                       offset, compare_bps, expected_bps);
            return false;
        }
    }

    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: verify_frame_header succeeded at %u (strict=%d)\n", offset, strict_match);
    return true;
}

// Find next FLAC frame sync starting from given offset
// Returns offset of frame sync, or AUDIO_PLAYER_ERR if not found
static int flac_find_frame_sync(bk_audio_player_decoder_t *decoder, uint32_t start_offset, uint32_t search_limit, uint32_t *frame_offset, bool strict_match)
{
    uint8_t buffer[4096];
    uint32_t search_bytes = search_limit;
    uint32_t current_offset = start_offset;
    int read_size;
    int i;
    int ret = AUDIO_PLAYER_ERR;

    if (!decoder || !decoder->source || !frame_offset)
    {
        return AUDIO_PLAYER_ERR;
    }

    // Limit search to reasonable size (max 2MB to prevent excessive searching)
    // Note: This limit is higher than before to support larger metadata blocks
    if (search_bytes > 2 * 1024 * 1024)
    {
        search_bytes = 2 * 1024 * 1024;
    }

    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: find_frame_sync start=%u, limit=%u, strict=%d\n",
               start_offset, search_bytes, strict_match);

    // Seek to start position
    if (audio_source_seek(decoder->source, current_offset, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        return AUDIO_PLAYER_ERR;
    }

    // Search for frame sync in chunks
    uint32_t sync_candidates = 0;
    while (search_bytes > 0)
    {
        uint32_t chunk_size = (search_bytes > sizeof(buffer)) ? sizeof(buffer) : search_bytes;
        read_size = audio_source_read_data(decoder->source, (char *)buffer, chunk_size);
        if (read_size <= 0)
        {
            break;
        }

        // Search for sync code in buffer
        for (i = 0; i < read_size - 1; i++)
        {
            if (flac_is_sync_code(buffer[i], buffer[i + 1]))
            {
                sync_candidates++;
                uint32_t candidate_offset = current_offset + i;
                // Verify this is a real frame header
                // Note: flac_verify_frame_header will seek to candidate_offset
                // Use less strict matching for first frame finding
                if (flac_verify_frame_header(decoder, candidate_offset, strict_match))
                {
                    *frame_offset = candidate_offset;
                    ret = AUDIO_PLAYER_OK;
                    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found verified frame at offset %u (checked %u sync candidates)\n", 
                               candidate_offset, sync_candidates);
                    goto exit;
                }
                else
                {
                    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: candidate at %u rejected during verification (strict=%d)\n",
                               candidate_offset, strict_match);
                }
                // Restore search position after verification
                if (audio_source_seek(decoder->source, current_offset + read_size, SEEK_SET) != AUDIO_PLAYER_OK)
                {
                    goto exit;
                }
            }
        }

        current_offset += read_size;
        search_bytes -= read_size;
    }
    
    if (sync_candidates > 0)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found %u sync candidates but none passed verification (searched from %u, limit=%u, strict=%d)\n", 
                   sync_candidates, start_offset, search_limit, strict_match);
    }
    else
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: no sync candidates found (searched from %u, limit=%u, strict=%d)\n",
                   start_offset, search_limit, strict_match);
    }

exit:
    // Note: File position is left at the last read position
    // Caller should restore if needed
    return ret;
}

// Calculate file position for seek operation
static int flac_decoder_calc_position(bk_audio_player_decoder_t *decoder, int second)
{
    flac_decoder_priv_t *priv;
    uint64_t target_samples;
    uint32_t seek_offset = 0;
    uint32_t search_start;
    uint32_t search_range = 128 * 1024; // Search range: 128KB (increased)
    uint32_t file_size = 0;

    if (!decoder || !decoder->decoder_priv || second < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    if (!priv->decoder_opened || priv->sample_rate == 0)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "FLAC: cannot calculate position, decoder not ready\n");
        return AUDIO_PLAYER_ERR;
    }

    // Get file size
    if (decoder->source)
    {
        file_size = audio_source_get_total_bytes(decoder->source);
    }
    if (file_size == 0 && decoder->info.total_bytes > 0)
    {
        file_size = decoder->info.total_bytes;
    }

    // Calculate target sample number
    target_samples = (uint64_t)second * priv->sample_rate;

    // If we know total samples, clamp target
    if (priv->total_samples > 0 && target_samples >= priv->total_samples)
    {
        target_samples = priv->total_samples - 1;
    }

    // Find first frame if not known
    // Note: This will change file position, but since we're doing a seek operation,
    // the file position will be changed anyway, so it's acceptable
    if (priv->first_frame_offset == 0)
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: first_frame_offset unknown, attempting discovery before seek (file_size=%u)\n", file_size);
        uint32_t first_frame = 0;
        uint32_t start_offset = 0;
        uint32_t flac_marker_offset = 0;

        // Step 1: Check for ID3v2 tag
        if (file_size > 10)
        {
            uint8_t id3_header[10];
            if (audio_source_seek(decoder->source, 0, SEEK_SET) == AUDIO_PLAYER_OK)
            {
                if (audio_source_read_data(decoder->source, (char *)id3_header, 10) == 10)
                {
                    if (id3_header[0] == 'I' && id3_header[1] == 'D' && id3_header[2] == '3')
                    {
                        uint32_t id3_size = ((id3_header[6] & 0x7F) << 21) |
                                            ((id3_header[7] & 0x7F) << 14) |
                                            ((id3_header[8] & 0x7F) << 7) |
                                            (id3_header[9] & 0x7F);
                        if (id3_size > 0 && id3_size < (1024 * 1024))
                        {
                            start_offset = 10 + id3_size;
                        }
                    }
                }
            }
        }

        // Step 2: Find FLAC marker "fLaC" (0x664C6143)
        if (file_size > start_offset + 4)
        {
            uint8_t marker[4];
            if (audio_source_seek(decoder->source, start_offset, SEEK_SET) == AUDIO_PLAYER_OK)
            {
                if (audio_source_read_data(decoder->source, (char *)marker, 4) == 4)
                {
                    uint32_t marker_val = ((uint32_t)marker[0] << 24) | ((uint32_t)marker[1] << 16) |
                                          ((uint32_t)marker[2] << 8) | marker[3];
                    if (marker_val == 0x664C6143) // "fLaC"
                    {
                        flac_marker_offset = start_offset;
                        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found marker at offset %u\n", flac_marker_offset);
                    }
                    else
                    {
                        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: marker not found at offset %u, value=0x%08X\n", start_offset, marker_val);
                    }
                }
            }
        }

        // Step 3: Search for first frame after FLAC marker
        uint32_t search_start = (flac_marker_offset > 0) ? (flac_marker_offset + 4) : 
                                ((start_offset > 0) ? start_offset : 0);
        uint32_t max_search = 2 * 1024 * 1024; // 2MB search range
        if (file_size > search_start + max_search)
        {
            max_search = file_size - search_start;
        }
        else if (file_size <= search_start)
        {
            max_search = 0;
        }

        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: searching for first frame, start=%u, max_search=%u, file_size=%u, marker=%u\n", 
                   search_start, max_search, file_size, flac_marker_offset);
        if (max_search > 0 && flac_find_frame_sync(decoder, search_start, max_search, &first_frame, true) == AUDIO_PLAYER_OK)
        {
            // Verify the found frame with strict matching to ensure it's valid
            if (flac_verify_frame_header(decoder, first_frame, true))
            {
                priv->first_frame_offset = first_frame;
                BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found and verified first frame at offset %u during calc_position\n", first_frame);
            }
            else
            {
                BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found frame at %u but verification failed, trying fallback\n", first_frame);
                // Fallback: try from file start
                uint32_t fallback_search = (file_size > 2 * 1024 * 1024) ? 2 * 1024 * 1024 : file_size;
                if (fallback_search > 0 && flac_find_frame_sync(decoder, 0, fallback_search, &first_frame, true) == AUDIO_PLAYER_OK)
                {
                    if (flac_verify_frame_header(decoder, first_frame, true))
                    {
                        priv->first_frame_offset = first_frame;
                        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found first frame at offset %u (fallback during calc_position)\n", first_frame);
                    }
                    else
                    {
                        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: fallback frame at %u also failed verification\n", first_frame);
                        return AUDIO_PLAYER_ERR;
                    }
                }
                else
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: cannot find first frame, seek operation will fail\n");
                    return AUDIO_PLAYER_ERR;
                }
            }
        }
        else
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: first search failed, trying fallback from file start\n");
            // Fallback: try from file start
            uint32_t fallback_search = (file_size > 2 * 1024 * 1024) ? 2 * 1024 * 1024 : file_size;
            if (fallback_search > 0 && flac_find_frame_sync(decoder, 0, fallback_search, &first_frame, true) == AUDIO_PLAYER_OK)
            {
                if (flac_verify_frame_header(decoder, first_frame, true))
                {
                    priv->first_frame_offset = first_frame;
                    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found first frame at offset %u (fallback during calc_position)\n", first_frame);
                }
                else
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: cannot find verified first frame, seek operation will fail\n");
                    return AUDIO_PLAYER_ERR;
                }
            }
            else
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: cannot find first frame (searched from %u, max=%u, fallback from 0, max=%u), seek operation will fail\n",
                           search_start, max_search, fallback_search);
                return AUDIO_PLAYER_ERR;
            }
        }
    }

    // Estimate file offset based on file size and sample ratio
    uint64_t estimated_bytes = 0;
    if (file_size > 0 && priv->total_samples > 0)
    {
        // Estimate: (file_size - first_frame) * target_samples / total_samples + first_frame
        uint64_t audio_data_size = file_size - priv->first_frame_offset;
        estimated_bytes = priv->first_frame_offset + (audio_data_size * target_samples) / priv->total_samples;
    }
    else if (decoder->info.bps > 0 && priv->sample_rate > 0)
    {
        // Fallback: estimate based on bitrate
        estimated_bytes = priv->first_frame_offset + (target_samples * decoder->info.bps) / (8 * priv->sample_rate);
    }
    else
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "FLAC: insufficient info for position calculation\n");
        return AUDIO_PLAYER_ERR;
    }

    // Clamp estimated position to valid range
    if (estimated_bytes < priv->first_frame_offset)
    {
        estimated_bytes = priv->first_frame_offset;
    }
    if (file_size > 0 && estimated_bytes >= file_size)
    {
        estimated_bytes = file_size - 1;
    }

    search_start = (uint32_t)estimated_bytes;

    // Search backward and forward from estimated position
    // First try backward search (more likely to find correct frame)
    uint32_t backward_start = (search_start > search_range) ? (search_start - search_range) : priv->first_frame_offset;
    uint32_t backward_limit = (search_start - backward_start) + search_range;
    if (flac_find_frame_sync(decoder, backward_start, backward_limit, &seek_offset, true) != AUDIO_PLAYER_OK)
    {
        // If backward search fails, try forward search
        uint32_t forward_limit = (file_size > 0 && search_start + search_range > file_size) ? 
                                 (file_size - search_start) : search_range;
        if (flac_find_frame_sync(decoder, search_start, forward_limit, &seek_offset, true) != AUDIO_PLAYER_OK)
        {
            // Last resort: try from first frame with larger range (slower but more reliable)
            BK_LOGW(AUDIO_PLAYER_TAG, "FLAC: failed to find frame sync near estimated position, trying from first frame\n");
            uint32_t max_search_from_first = (file_size > 0 && file_size > priv->first_frame_offset) ?
                                            (file_size - priv->first_frame_offset) : (512 * 1024);
            if (flac_find_frame_sync(decoder, priv->first_frame_offset, max_search_from_first, &seek_offset, true) != AUDIO_PLAYER_OK)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to find any frame sync, using first frame\n");
                seek_offset = priv->first_frame_offset;
            }
        }
    }

    // Verify the found offset is valid - this is critical to avoid decoder crash
    // If verification fails, we should not proceed with seek as it will cause decoder to lose sync
    // Use strict matching for seek operations to avoid false positives
    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: verifying seek offset %u (first_frame=%u, target_samples=%llu/%llu)\n", 
               seek_offset, priv->first_frame_offset, (unsigned long long)target_samples, (unsigned long long)priv->total_samples);
    bool verified = flac_verify_frame_header(decoder, seek_offset, true);
    if (!verified)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "FLAC: found offset %u failed verification, trying first frame at %u\n", 
                   seek_offset, priv->first_frame_offset);
        // Try to verify first frame as fallback
        if (priv->first_frame_offset > 0)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: verifying first frame at offset %u\n", priv->first_frame_offset);
            if (flac_verify_frame_header(decoder, priv->first_frame_offset, true))
            {
                seek_offset = priv->first_frame_offset;
                verified = true;
                BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: using verified first frame offset %u\n", seek_offset);
            }
            else
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: both offsets failed verification, seek will likely fail\n");
                // Try to find a valid frame near the estimated position with larger search range
                uint32_t extended_range = 256 * 1024; // 256KB extended search
                uint32_t extended_start = (search_start > extended_range) ? (search_start - extended_range) : 0;
                uint32_t extended_limit = (file_size > extended_start + 2 * extended_range) ? 
                                          (2 * extended_range) : (file_size - extended_start);
            if (extended_limit > 0 && flac_find_frame_sync(decoder, extended_start, extended_limit, &seek_offset, true) == AUDIO_PLAYER_OK)
            {
                    if (flac_verify_frame_header(decoder, seek_offset, true))
                    {
                        verified = true;
                        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: found and verified frame at offset %u (extended search)\n", seek_offset);
                    }
                }

                if (!verified)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: cannot find valid frame header for seek, refusing seek operation\n");
                    // If we cannot find a valid frame header, refuse the seek operation
                    // to avoid decoder sync loss
                    // Note: File position may have been changed during search, but we cannot restore it
                    // because audio_source doesn't support tell operation
                    // The decoder may lose sync, but at least we prevented an invalid seek
                    return AUDIO_PLAYER_ERR;
                }
            }
        }
    }
    else
    {
        BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: verified frame header at offset %u\n", seek_offset);
    }

    // Only proceed with seek if we have a verified frame header
    if (!verified)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: seek offset verification failed, refusing seek\n");
        return AUDIO_PLAYER_ERR;
    }

    priv->pending_seek_offset = seek_offset;
    priv->pending_seek_samples = target_samples;
    priv->seek_pending = true;

    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: calc_position second=%d, target_samples=%llu, estimated=%u, seek_offset=%u\n",
               second, (unsigned long long)target_samples, search_start, seek_offset);

    return (int)seek_offset;
}

// Check if seek operation is ready
static int flac_decoder_is_seek_ready(bk_audio_player_decoder_t *decoder)
{
    flac_decoder_priv_t *priv;

    if (!decoder || !decoder->decoder_priv)
    {
        return 1;
    }

    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    // Seek is ready if decoder is opened and not in pending seek state
    if (priv->decoder_opened && !priv->seek_pending)
    {
        return 1;
    }

    return 0;
}

static int flac_decoder_close(bk_audio_player_decoder_t *decoder)
{
    flac_decoder_priv_t *priv;
    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    BK_LOGD(AUDIO_PLAYER_TAG, "%s:%d\n", __FUNCTION__, __LINE__);

    if (priv && priv->decoder_opened)
    {
        if (bk_aud_flac_dec_deinit() != BK_OK)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "bk_aud_flac_dec_deinit fail\n");
        }
        priv->decoder_opened = false;
    }

    return AUDIO_PLAYER_OK;
}


static int flac_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    flac_decoder_priv_t *priv;
    priv = (flac_decoder_priv_t *)decoder->decoder_priv;

    if (priv->channels == 0 || priv->bps == 0)
    {
        return priv->pcm_sameples * 4;
    }

    return priv->pcm_sameples * priv->channels * (priv->bps / 8);
}



const bk_audio_player_decoder_ops_t flac_decoder_ops =
{
    .name = "flac",
    .open = flac_decoder_open,
    .get_info = flac_decoder_get_info,
    .get_chunk_size = flac_decoder_get_chunk_size,
    .get_data = flac_decoder_get_data,
    .close = flac_decoder_close,
    .calc_position = flac_decoder_calc_position,
    .is_seek_ready = flac_decoder_is_seek_ready,
};

/* Get FLAC decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_flac_decoder_ops(void)
{
    return &flac_decoder_ops;
}
