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
#include <os/str.h>
#include <string.h>
#include <modules/mp3dec.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "codec_api.h"
#include "source_api.h"
#include "player_osal.h"
#include "ring_buffer.h"
#include <components/bk_audio_player/bk_audio_player_types.h>

#define MP3_ID3V2_HEADER_SIZE 10
#define MP3_ID3V2_FOOTER_SIZE 10

//#define WIFI_READ_DATA_TIME_DEBUG

#ifdef WIFI_READ_DATA_TIME_DEBUG
#include <driver/aon_rtc.h>

#define WIFI_READ_DATA_TIMEOUT_NUM    (10 * 1000)      //unit us
#endif

//#define FRAME_INFO_FILTER_EN
#ifdef FRAME_INFO_FILTER_EN
#include "frame_info_list.h"

#define FRAME_INFO_TOTAL_NUM    (100)
#define FRAME_INFO_PASS_NUM     (100)
#endif

#define MP3_AUDIO_BUF_SZ            (12 * 1024) /* feel free to change this, but keep big enough for >= one frame at high bitrates */
#define MP3_SEEK_SCAN_BACK_DEFAULT  (4096U)
#define MP3_SEEK_SCAN_SIZE_DEFAULT  (8192U)
#define MP3_SEEK_RETRY_STEP_BYTES   (1024U)
#define MP3_SEEK_RETRY_MAX          (6U)
#define MP3_SYNC_MAX_ATTEMPTS       (48U)
#define MP3_SEEK_PREROLL_FRAMES     (6U)
#define MP3_HEADER_BITRATE_TOLERANCE (1000U)
#define MP3_FRAME_SPACING_TOLERANCE_MIN (2U)
#define MP3_FRAME_SPACING_TOLERANCE_DIV (64U)
#define MP3_FRAME_CHAIN_MIN_FRAMES     (3U)
#define MP3_SEEK_BIAS_LIMIT            (4096)
#define MP3_FRAMECRC_POLY              (0x8005U)

static const uint16_t mp3_bitrate_table_mpeg1_l3[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
static const uint16_t mp3_bitrate_table_mpeg2_l3[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
static const uint32_t mp3_samplerate_table[3][3] =
{
    {44100U, 48000U, 32000U},  /* MPEG1   */
    {22050U, 24000U, 16000U},  /* MPEG2   */
    {11025U, 12000U, 8000U},   /* MPEG2.5 */
};

typedef struct
{
    char identifier[3];         // 必须是 "TAG"
    char title[30];             // 歌曲标题
    char artist[30];            // 艺术家
    char album[30];             // 专辑
    char year[4];               // 年份
    char comment[30];           // 注释
    unsigned char zeroByte;     // 必须是0
    unsigned char trackNum;     // 音轨号
    unsigned char genre;        // 流派代码
} id3v1_info_t;

typedef struct mp3_decoder_priv
{
    /* mp3 information */
    HMP3Decoder decoder;
    MP3FrameInfo frame_info;
    uint32_t frames;
    uint32_t frames_decoded;
    uint32_t stream_offset;
    uint32_t samples_per_frame;
    uint32_t base_samples_per_frame;
    uint32_t frame_bytes_estimate;
    uint64_t frame_bytes_total;
    uint32_t frame_bytes_count;
    int32_t seek_payload_bias;
    uint32_t seek_reference_offset;
    bool seek_reference_valid;
    uint32_t last_invalid_header_offset;
    bool last_invalid_header_valid;
    bool last_alignment_from_invalid;
    uint32_t invalid_guard_span;
    uint32_t pending_seek_offset;
    bool pending_seek_valid;
    uint32_t pending_header_offset;
    bool pending_header_valid;
    bool xing_toc_valid;
    uint8_t xing_toc[100];
    uint32_t last_seek_aligned_offset;
    uint8_t seek_retry;
    bool seek_pending_check;
    bool force_realign;

    /* mp3 read session */
    uint8_t *read_buffer, *read_ptr;
    uint32_t bytes_left;
    bool stream_done;
    bool stream_aborted;

    int current_sample_rate;
    uint32_t base_sample_rate;
    uint32_t base_bit_rate;

    biterate_type_t biterate_type;

    uint32_t id3v2_bytes;
    uint32_t id3v1_bytes;

    uint32_t total_bytes;
    uint32_t total_frames;
    double total_duration;     // play duration of total bytes in milisecond (ms)

    id3v1_info_t id3v1_info;
    bool need_reset;
} mp3_decoder_priv_t;


static int mp3_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder);
static uint32_t mp3_parse_synchsafe32(const uint8_t *data);
static uint32_t mp3_get_invalid_skip_range(uint32_t frame_bytes_hint);
static uint32_t mp3_get_frame_bytes_hint(mp3_decoder_priv_t *priv);
static bool mp3_candidate_has_info_marker(const uint8_t *data, int available);
static bool mp3_check_frame_crc(const uint8_t *frame, uint32_t available_bytes, uint32_t frame_bytes);
static bool mp3_probe_candidate_sync(bk_audio_player_decoder_t *decoder, mp3_decoder_priv_t *priv,
                                     uint8_t *candidate_ptr, int available_bytes, uint32_t candidate_offset);
static int mp3_check_sync_internal(bk_audio_player_decoder_t *decoder, bool probe_only);
static int mp3_decoder_is_seek_ready(bk_audio_player_decoder_t *decoder);
static uint32_t mp3_get_invalid_skip_range(uint32_t frame_bytes_hint)
{
    uint32_t tolerance = (frame_bytes_hint > 0U) ? (frame_bytes_hint * 4U) : 2048U;
    if (tolerance < 1024U)
    {
        tolerance = 1024U;
    }
    if (tolerance > MP3_AUDIO_BUF_SZ)
    {
        tolerance = MP3_AUDIO_BUF_SZ;
    }
    return tolerance;
}

static uint16_t mp3_crc16_update(uint16_t crc, uint8_t data)
{
    crc ^= (uint16_t)data << 8;
    for (int i = 0; i < 8; ++i)
    {
        if ((crc & 0x8000U) != 0U)
        {
            crc = (uint16_t)((crc << 1) ^ MP3_FRAMECRC_POLY);
        }
        else
        {
            crc <<= 1;
        }
    }
    return crc;
}

static bool mp3_check_frame_crc(const uint8_t *frame, uint32_t available_bytes, uint32_t frame_bytes)
{
    if (!frame || frame_bytes == 0U)
    {
        return true;
    }

    if (frame_bytes > available_bytes)
    {
        return true;
    }

    bool crc_enabled = ((frame[1] & 0x01U) == 0U);
    if (!crc_enabled)
    {
        return true;
    }

    if (frame_bytes < 6U)
    {
        return false;
    }

    uint16_t expected = ((uint16_t)frame[4] << 8) | (uint16_t)frame[5];
    uint16_t crc = 0xFFFFU;
    for (uint32_t i = 6U; i < frame_bytes; ++i)
    {
        crc = mp3_crc16_update(crc, frame[i]);
    }
    return crc == expected;
}

static bool mp3_candidate_has_info_marker(const uint8_t *data, int available)
{
    static const char *markers[] = {"Info", "info", "Xing", "xing", "LAME", "Lavc"};
    const size_t marker_count = sizeof(markers) / sizeof(markers[0]);

    if (!data || available <= 16)
    {
        return false;
    }

    int scan = available;
    if (scan > 128)
    {
        scan = 128;
    }

    for (int i = 4; i <= scan - 4; ++i)
    {
        for (size_t j = 0; j < marker_count; ++j)
        {
            const char *marker = markers[j];
            size_t len = os_strlen(marker);
            if (i + (int)len > scan)
            {
                continue;
            }
            if (os_memcmp(data + i, marker, len) == 0)
            {
                return true;
            }
        }
    }

    return false;
}


static inline uint32_t mp3_get_default_samples_per_frame(const mp3_decoder_priv_t *priv)
{
    if (priv->base_samples_per_frame)
    {
        return priv->base_samples_per_frame;
    }

    if (priv->samples_per_frame)
    {
        return priv->samples_per_frame;
    }

    if (priv->frame_info.nChans > 0 && priv->frame_info.outputSamps > 0)
    {
        return priv->frame_info.outputSamps / priv->frame_info.nChans;
    }

    if (priv->frame_info.version == MPEG1)
    {
        return 1152U;
    }

    return 576U;
}

static inline uint32_t mp3_get_effective_sample_rate(const mp3_decoder_priv_t *priv)
{
    if (priv->base_sample_rate > 0)
    {
        return priv->base_sample_rate;
    }
    if (priv->frame_info.samprate > 0)
    {
        return (uint32_t)priv->frame_info.samprate;
    }
    if (priv->current_sample_rate > 0)
    {
        return (uint32_t)priv->current_sample_rate;
    }
    return 44100U;
}

static uint32_t mp3_parse_synchsafe32(const uint8_t *data)
{
    if (!data)
    {
        return 0U;
    }

    return ((uint32_t)data[0] << 21) |
           ((uint32_t)data[1] << 14) |
           ((uint32_t)data[2] << 7) |
           ((uint32_t)data[3]);
}

static uint32_t mp3_estimate_frame_bytes(const mp3_decoder_priv_t *priv)
{
    bool prefer_average = true;
    if (priv->biterate_type == BITERATE_TYPE_CBR ||
        priv->biterate_type == BITERATE_TYPE_CBR_INFO ||
        priv->base_bit_rate > 0U)
    {
        prefer_average = false;
    }

    if (prefer_average && priv->frame_bytes_estimate > 0U)
    {
        return priv->frame_bytes_estimate;
    }

    int bit_rate = priv->frame_info.bitrate;
    if (bit_rate <= 0 && priv->base_bit_rate > 0)
    {
        bit_rate = (int)priv->base_bit_rate;
    }

    if (bit_rate <= 0)
    {
        if (priv->frame_bytes_estimate > 0U)
        {
            return priv->frame_bytes_estimate;
        }
        return 0;
    }

    uint32_t sample_rate = mp3_get_effective_sample_rate(priv);
    if (sample_rate == 0)
    {
        return 0;
    }

    uint32_t factor = 144U;
    if (priv->frame_info.version == MPEG2 || priv->frame_info.version == MPEG25)
    {
        factor = 72U;
    }
    else if (sample_rate < 32000U)
    {
        factor = 72U;
    }

    uint64_t bytes = (uint64_t)factor * (uint64_t)bit_rate + (uint64_t)sample_rate - 1ULL;
    bytes /= (uint64_t)sample_rate;
    if (bytes == 0)
    {
        bytes = 1;
    }
    return (uint32_t)bytes;
}

static uint32_t mp3_get_frame_bytes_hint(mp3_decoder_priv_t *priv)
{
    uint32_t hint = priv->frame_bytes_estimate;
    if (hint == 0U)
    {
        hint = mp3_estimate_frame_bytes(priv);
    }
    if (hint == 0U)
    {
        hint = 417U;
    }
    return hint;
}

static uint32_t mp3_get_average_frame_bytes(const mp3_decoder_priv_t *priv)
{
    if (priv->frame_bytes_total == 0ULL || priv->frame_bytes_count < 32U)
    {
        return 0U;
    }

    uint64_t avg = priv->frame_bytes_total / (uint64_t)priv->frame_bytes_count;
    if (avg == 0ULL)
    {
        return 0U;
    }
    if (avg > (uint64_t)UINT32_MAX)
    {
        return UINT32_MAX;
    }
    return (uint32_t)avg;
}

static bool mp3_frame_spacing_within_tolerance(const mp3_decoder_priv_t *priv, uint32_t frame_bytes)
{
    if (frame_bytes == 0U)
    {
        return false;
    }

    uint32_t avg = mp3_get_average_frame_bytes(priv);
    if (avg == 0U)
    {
        return true;
    }

    uint32_t diff = (frame_bytes > avg) ? (frame_bytes - avg) : (avg - frame_bytes);
    uint32_t margin = avg / MP3_FRAME_SPACING_TOLERANCE_DIV;
    if (margin < MP3_FRAME_SPACING_TOLERANCE_MIN)
    {
        margin = MP3_FRAME_SPACING_TOLERANCE_MIN;
    }

    return diff <= margin;
}

static uint64_t mp3_get_available_payload_bytes(mp3_decoder_priv_t *priv, bk_audio_player_decoder_t *decoder)
{
    if (priv->total_bytes > 0)
    {
        return priv->total_bytes;
    }

    uint32_t source_total = audio_source_get_total_bytes(decoder->source);
    if (source_total <= priv->stream_offset)
    {
        return 0;
    }

    uint64_t available = (uint64_t)source_total - (uint64_t)priv->stream_offset;
    if (priv->id3v1_bytes < available)
    {
        available -= priv->id3v1_bytes;
    }
    else
    {
        available = 0;
    }

    return available;
}

static uint64_t mp3_calc_vbr_payload_offset(mp3_decoder_priv_t *priv, int second, uint64_t available_bytes)
{
    if (!priv->xing_toc_valid || available_bytes == 0 || priv->total_duration <= 0.0)
    {
        return 0;
    }

    double target_ms = (double)second * 1000.0;
    double fraction = target_ms / priv->total_duration;
    if (fraction < 0.0)
    {
        fraction = 0.0;
    }
    if (fraction > 1.0)
    {
        fraction = 1.0;
    }

    double toc_position = fraction * 99.0;
    int index = (int)toc_position;
    if (index < 0)
    {
        index = 0;
    }
    if (index > 99)
    {
        index = 99;
    }
    double frac = toc_position - (double)index;

    uint32_t a = priv->xing_toc[index];
    uint32_t b = (index < 99) ? priv->xing_toc[index + 1] : 256U;
    double interp = (double)a + ((double)b - (double)a) * frac;
    if (interp < 0.0)
    {
        interp = 0.0;
    }
    if (interp > 256.0)
    {
        interp = 256.0;
    }

    return (uint64_t)((interp / 256.0) * (double)available_bytes);
}

static uint32_t mp3_get_preroll_bytes(mp3_decoder_priv_t *priv)
{
    if (MP3_SEEK_PREROLL_FRAMES == 0)
    {
        return 0;
    }

    uint32_t frame_bytes = mp3_estimate_frame_bytes(priv);
    if (frame_bytes == 0)
    {
        frame_bytes = 417U;
    }
    return frame_bytes * MP3_SEEK_PREROLL_FRAMES;
}

static void mp3_commit_seek_bias(mp3_decoder_priv_t *priv)
{
    if (!priv)
    {
        return;
    }

    if (!priv->seek_reference_valid)
    {
        priv->last_alignment_from_invalid = false;
        return;
    }

    int32_t delta = (int32_t)priv->last_seek_aligned_offset - (int32_t)priv->seek_reference_offset;
    if (delta > MP3_SEEK_BIAS_LIMIT)
    {
        delta = MP3_SEEK_BIAS_LIMIT;
    }
    else if (delta < -MP3_SEEK_BIAS_LIMIT)
    {
        delta = -MP3_SEEK_BIAS_LIMIT;
    }

    if (priv->last_alignment_from_invalid)
    {
        priv->seek_payload_bias = (priv->seek_payload_bias * 15 + delta) / 16;
    }
    else
    {
        priv->seek_payload_bias = (priv->seek_payload_bias * 3 + delta) / 4;
    }
    priv->seek_reference_valid = false;
    priv->last_invalid_header_valid = false;
    priv->last_alignment_from_invalid = false;

    BK_LOGI(AUDIO_PLAYER_TAG, "%s, delta:%d, bias:%d\n", __func__, delta, priv->seek_payload_bias);
}

static void mp3_schedule_alignment_retry(bk_audio_player_decoder_t *decoder, mp3_decoder_priv_t *priv, bool forward)
{
    if (!priv)
    {
        return;
    }

    uint32_t attempt = priv->seek_retry + 1U;
    if (attempt > MP3_SEEK_RETRY_MAX)
    {
        attempt = MP3_SEEK_RETRY_MAX;
    }
    priv->seek_retry = attempt;

    uint64_t stream_start = priv->stream_offset;
    uint64_t stream_end = stream_start + mp3_get_available_payload_bytes(priv, decoder);

    uint32_t reference = priv->pending_header_valid ? priv->pending_header_offset : priv->last_seek_aligned_offset;
    uint64_t base_header = reference;
    if (base_header < stream_start)
    {
        base_header = stream_start;
    }

    if (!priv->seek_reference_valid)
    {
        priv->seek_reference_offset = (uint32_t)base_header;
        priv->seek_reference_valid = true;
    }

    bool advance_forward = forward;
    if (priv->seek_retry > 1U)
    {
        advance_forward = ((priv->seek_retry & 0x1U) != 0U);
    }

    uint32_t span_count = (priv->seek_retry + 1U) / 2U;
    uint64_t delta = (uint64_t)span_count * (uint64_t)MP3_SEEK_RETRY_STEP_BYTES;
    if (delta == 0ULL)
    {
        delta = MP3_SEEK_RETRY_STEP_BYTES;
    }

    uint64_t new_header = base_header;
    if (advance_forward)
    {
        new_header = base_header + delta;
        if (stream_end > stream_start && new_header >= stream_end)
        {
            new_header = (stream_end > 0ULL) ? (stream_end - 1ULL) : stream_start;
        }
    }
    else
    {
        if (base_header > stream_start + delta)
        {
            new_header = base_header - delta;
        }
        else
        {
            new_header = stream_start;
        }
    }

    uint32_t preroll = mp3_get_preroll_bytes(priv);
    uint64_t new_start = new_header;
    if (preroll > 0U && new_start > stream_start)
    {
        uint64_t delta = new_start - stream_start;
        if (delta > (uint64_t)preroll)
        {
            new_start -= preroll;
        }
        else
        {
            new_start = stream_start;
        }
    }

    priv->pending_header_offset = (uint32_t)new_header;
    priv->pending_header_valid = true;
    priv->pending_seek_offset = (uint32_t)new_start;
    priv->pending_seek_valid = true;
    priv->seek_pending_check = true;
    priv->need_reset = true;
    priv->force_realign = true;
    priv->seek_reference_offset = (uint32_t)new_header;
    priv->seek_reference_valid = true;

    BK_LOGW(AUDIO_PLAYER_TAG, "%s, schedule realign offset:%u direction:%s\n",
               __func__, priv->pending_header_offset, advance_forward ? "forward" : "backward");
}

static bool mp3_parse_frame_header_detail(uint32_t header, uint32_t *sample_rate_out, uint32_t *frame_bytes_out, uint32_t *bit_rate_out)
{
    if ((header & 0xffe00000U) != 0xffe00000U)
    {
        return false;
    }

    uint32_t version_id = (header >> 19) & 0x3U;
    uint32_t layer = (header >> 17) & 0x3U;
    if (version_id == 1U || layer != 1U)
    {
        return false;
    }

    uint32_t bitrate_index = (header >> 12) & 0xFU;
    uint32_t samplerate_index = (header >> 10) & 0x3U;
    if (bitrate_index == 0U || bitrate_index == 0xFU || samplerate_index == 3U)
    {
        return false;
    }

    const uint16_t *bitrate_table = (version_id == 3U) ? mp3_bitrate_table_mpeg1_l3 : mp3_bitrate_table_mpeg2_l3;
    uint32_t bitrate_kbps = bitrate_table[bitrate_index];
    if (bitrate_kbps == 0U)
    {
        return false;
    }
    uint32_t bitrate_bps = bitrate_kbps * 1000U;

    uint32_t sample_rate = 0U;
    if (version_id == 3U)
    {
        sample_rate = mp3_samplerate_table[0][samplerate_index];
    }
    else if (version_id == 2U)
    {
        sample_rate = mp3_samplerate_table[1][samplerate_index];
    }
    else
    {
        sample_rate = mp3_samplerate_table[2][samplerate_index];
    }

    if (sample_rate == 0U)
    {
        return false;
    }

    uint32_t factor = (version_id == 3U) ? 144U : 72U;
    uint32_t padding = (header >> 9) & 0x1U;
    uint64_t frame_size = (uint64_t)factor * (uint64_t)bitrate_bps;
    frame_size /= (uint64_t)sample_rate;
    frame_size += (uint64_t)padding;

    if (frame_size < 24ULL || frame_size > 2048ULL)
    {
        return false;
    }

    if (sample_rate_out)
    {
        *sample_rate_out = sample_rate;
    }
    if (frame_bytes_out)
    {
        *frame_bytes_out = (uint32_t)frame_size;
    }

    if (bit_rate_out)
    {
        *bit_rate_out = bitrate_bps;
    }

    return true;
}

static bool mp3_validate_frame_candidate(mp3_decoder_priv_t *priv, const uint8_t *data, int available, uint32_t *frame_bytes_out)
{
    if (!data || available < 4)
    {
        return false;
    }

    uint32_t sample_rate = 0U;
    uint32_t frame_bytes = 0U;
    uint32_t bit_rate = 0U;
    uint32_t header = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
    if (!mp3_parse_frame_header_detail(header, &sample_rate, &frame_bytes, &bit_rate))
    {
        return false;
    }

    uint32_t expected_rate = 0U;
    if (priv->frame_info.samprate > 0)
    {
        expected_rate = (uint32_t)priv->frame_info.samprate;
    }
    else if (priv->current_sample_rate > 0)
    {
        expected_rate = (uint32_t)priv->current_sample_rate;
    }

    if (expected_rate > 0U && sample_rate > 0U && expected_rate != sample_rate)
    {
        return false;
    }

    uint32_t expected_bitrate = priv->base_bit_rate ? priv->base_bit_rate : (uint32_t)priv->frame_info.bitrate;
    if (expected_bitrate > 0U && bit_rate > 0U)
    {
        uint32_t diff = (expected_bitrate > bit_rate) ? (expected_bitrate - bit_rate) : (bit_rate - expected_bitrate);
        if (diff > MP3_HEADER_BITRATE_TOLERANCE)
        {
            return false;
        }
    }

    if (!mp3_frame_spacing_within_tolerance(priv, frame_bytes))
    {
        return false;
    }

    const uint8_t *cursor = data;
    int remain = available;
    uint32_t validated_frames = 1U;
    uint32_t base_sample_rate = sample_rate;
    uint32_t base_bit_rate = bit_rate;
    uint32_t first_frame_bytes = frame_bytes;

    while (validated_frames < MP3_FRAME_CHAIN_MIN_FRAMES)
    {
        if (remain < (int)(frame_bytes + 4U))
        {
            return false;
        }

        int rel = MP3FindSyncWord((unsigned char *)(cursor + frame_bytes), remain - (int)frame_bytes);
        if (rel != 0)
        {
            return false;
        }

        cursor += frame_bytes;
        remain -= frame_bytes;

        uint32_t next_sample_rate = 0U;
        uint32_t next_frame_bytes = 0U;
        uint32_t next_bit_rate = 0U;
        uint32_t next_header = ((uint32_t)cursor[0] << 24) |
                               ((uint32_t)cursor[1] << 16) |
                               ((uint32_t)cursor[2] << 8) |
                               (uint32_t)cursor[3];
        if (!mp3_parse_frame_header_detail(next_header, &next_sample_rate, &next_frame_bytes, &next_bit_rate))
        {
            return false;
        }

        if (next_sample_rate != base_sample_rate)
        {
            return false;
        }

        if (expected_rate > 0U && next_sample_rate > 0U && next_sample_rate != expected_rate)
        {
            return false;
        }

        if (base_bit_rate > 0U && next_bit_rate > 0U)
        {
            uint32_t diff = (base_bit_rate > next_bit_rate) ? (base_bit_rate - next_bit_rate) : (next_bit_rate - base_bit_rate);
            if (diff > MP3_HEADER_BITRATE_TOLERANCE)
            {
                return false;
            }
        }

        if (!mp3_frame_spacing_within_tolerance(priv, next_frame_bytes))
        {
            return false;
        }

        frame_bytes = next_frame_bytes;
        base_bit_rate = next_bit_rate;
        validated_frames++;
    }

    if (frame_bytes_out)
    {
        *frame_bytes_out = first_frame_bytes;
    }

    return true;
}

static uint64_t mp3_calc_cbr_payload_offset(mp3_decoder_priv_t *priv, bk_audio_player_decoder_t *decoder, int second, uint64_t available_bytes)
{
    int bit_rate = priv->frame_info.bitrate;
    uint32_t sample_rate = mp3_get_effective_sample_rate(priv);
    if (bit_rate <= 0 || sample_rate == 0)
    {
        return 0;
    }

    uint32_t samples_per_frame = mp3_get_default_samples_per_frame(priv);
    uint64_t frame_index = ((uint64_t)second * (uint64_t)sample_rate) / (uint64_t)samples_per_frame;

    uint64_t payload = 0;
    if (priv->frame_bytes_total > 0ULL && priv->frame_bytes_count >= 32U)
    {
        payload = ((uint64_t)frame_index * priv->frame_bytes_total) / (uint64_t)priv->frame_bytes_count;
    }
    else
    {
        uint32_t factor = (priv->frame_info.version == MPEG1) ? 144U : 72U;
        payload = frame_index * (uint64_t)factor * (uint64_t)bit_rate;
        payload /= (uint64_t)sample_rate;
    }

    if (priv->seek_payload_bias != 0)
    {
        int64_t adjusted = (int64_t)payload + (int64_t)priv->seek_payload_bias;
        if (adjusted < 0)
        {
            adjusted = 0;
        }
        payload = (uint64_t)adjusted;
    }

    uint32_t frame_bytes_hint = mp3_estimate_frame_bytes(priv);
    if (frame_bytes_hint == 0U)
    {
        frame_bytes_hint = (uint32_t)((((uint64_t)bit_rate / 8ULL) + 37ULL) / 38ULL);
        if (frame_bytes_hint == 0U)
        {
            frame_bytes_hint = 417U;
        }
    }

    if (available_bytes > 0 && payload >= available_bytes)
    {
        if (available_bytes >= frame_bytes_hint)
        {
            payload = available_bytes - frame_bytes_hint;
            payload -= payload % frame_bytes_hint;
        }
        else
        {
            payload = 0;
        }
    }

    return payload;
}

static void mp3_align_after_seek(bk_audio_player_decoder_t *decoder)
{
    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;
    if (!priv || !priv->pending_seek_valid)
    {
        return;
    }

    uint32_t target = priv->pending_header_valid ? priv->pending_header_offset : priv->pending_seek_offset;
    uint32_t stream_start = priv->stream_offset;
    if (target < stream_start)
    {
        target = stream_start;
    }

    uint32_t scan_back_limit = target - stream_start;
    uint32_t scan_back = MP3_SEEK_SCAN_BACK_DEFAULT;
    if (priv->seek_retry > 0 && target > stream_start)
    {
        uint32_t bias_abs = (priv->seek_payload_bias >= 0) ? (uint32_t)priv->seek_payload_bias : (uint32_t)(-priv->seek_payload_bias);
        uint32_t adaptive = bias_abs + (uint32_t)priv->seek_retry * MP3_SEEK_RETRY_STEP_BYTES;
        if (adaptive > MP3_AUDIO_BUF_SZ / 2)
        {
            adaptive = MP3_AUDIO_BUF_SZ / 2;
        }
        scan_back = adaptive;
    }
    if (scan_back > scan_back_limit)
    {
        scan_back = scan_back_limit;
    }

    uint32_t scan_start = target - scan_back;
    uint32_t scan_size = MP3_SEEK_SCAN_SIZE_DEFAULT;
    if (scan_back + scan_size > MP3_AUDIO_BUF_SZ)
    {
        if (MP3_AUDIO_BUF_SZ > scan_back)
        {
            scan_size = MP3_AUDIO_BUF_SZ - scan_back;
        }
        else
        {
            scan_size = MP3_AUDIO_BUF_SZ / 2;
        }
    }
    if (scan_size < MP3_SEEK_RETRY_STEP_BYTES)
    {
        scan_size = MP3_SEEK_RETRY_STEP_BYTES;
    }

    if (priv->total_bytes > 0)
    {
        uint64_t max_available = (uint64_t)priv->stream_offset + (uint64_t)priv->total_bytes;
        if ((uint64_t)scan_start + (uint64_t)scan_size > max_available)
        {
            uint64_t remain = (max_available > scan_start) ? (max_available - scan_start) : 0;
            if (remain < 1)
            {
                priv->pending_seek_valid = false;
                return;
            }
            scan_size = (uint32_t)remain;
        }
    }

    if (scan_size < 1)
    {
        priv->pending_seek_valid = false;
        return;
    }

    if (audio_source_seek(decoder->source, scan_start, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, scan seek fail, start=%u\n", __func__, scan_start);
        priv->pending_seek_valid = false;
        return;
    }
    int read_len = audio_source_read_data(decoder->source, (char *)priv->read_buffer, scan_size);
    uint32_t aligned_offset = target;

    int rel = -1;
    bool found_header = false;
    bool aligned_from_invalid = false;
    uint32_t frame_bytes_hint = mp3_get_frame_bytes_hint(priv);
    if (read_len > 0)
    {
        uint32_t target_pos = 0U;
        if (target > scan_start)
        {
            target_pos = target - scan_start;
            if ((int)target_pos >= read_len)
            {
                target_pos = (read_len > 0) ? (uint32_t)(read_len - 1) : 0U;
            }
        }

        uint32_t prefer_margin = mp3_get_preroll_bytes(priv);
        uint32_t forward_margin = frame_bytes_hint * 2U;
        if (prefer_margin < forward_margin)
        {
            prefer_margin = forward_margin;
        }

        int pass_count = (target_pos > 0U) ? 2 : 1;
        int best_rel = -1;
        uint32_t best_distance = UINT32_MAX;
        bool best_before = false;
        int best_after_rel = -1;
        uint32_t best_after_distance = UINT32_MAX;
        int best_before_rel = -1;
        uint32_t best_before_distance = UINT32_MAX;
        for (int pass = 0; pass < pass_count; ++pass)
        {
    uint32_t pass_start = (pass == 0) ? target_pos : 0U;
    uint32_t pass_limit = (pass == 0) ? (uint32_t)read_len : target_pos;
            if (pass_start >= pass_limit)
            {
                continue;
            }

            uint32_t search_pos = pass_start;
            while (search_pos < pass_limit)
            {
                int chunk = (int)pass_limit - (int)search_pos;
                if (chunk <= 0)
                {
                    break;
                }

                int found = MP3FindSyncWord(priv->read_buffer + search_pos, chunk);
                if (found < 0)
                {
                    break;
                }

                found += (int)search_pos;
                uint32_t candidate_offset = scan_start + (uint32_t)found;
                bool candidate_near_invalid = false;
                bool candidate_exact_invalid = false;
                if (priv->last_invalid_header_valid)
                {
                    uint32_t invalid_tolerance = priv->invalid_guard_span;
                    if (invalid_tolerance == 0U)
                    {
                        invalid_tolerance = mp3_get_invalid_skip_range(frame_bytes_hint);
                    }
                    uint32_t invalid_delta = (candidate_offset > priv->last_invalid_header_offset)
                                                 ? (candidate_offset - priv->last_invalid_header_offset)
                                                 : (priv->last_invalid_header_offset - candidate_offset);
                    if (invalid_delta <= invalid_tolerance)
                    {
                        candidate_near_invalid = true;
                        candidate_exact_invalid = (candidate_offset == priv->last_invalid_header_offset);
                    }
                }

                if (candidate_exact_invalid)
                {
                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, skip exact invalid header:%u\n", __func__, candidate_offset);
                    search_pos = (uint32_t)(found + 1);
                    continue;
                }

                if (mp3_candidate_has_info_marker(priv->read_buffer + found, read_len - found))
                {
                    BK_LOGI(AUDIO_PLAYER_TAG, "%s, skip info marker frame:%u\n", __func__, candidate_offset);
                    search_pos = (uint32_t)(found + 1);
                    continue;
                }

                uint32_t candidate_frame_bytes = 0U;
                if (mp3_validate_frame_candidate(priv, priv->read_buffer + found, read_len - found, &candidate_frame_bytes))
                {
                    if (!mp3_check_frame_crc(priv->read_buffer + found, (uint32_t)(read_len - found), candidate_frame_bytes))
                    {
                        BK_LOGI(AUDIO_PLAYER_TAG, "%s, crc mismatch candidate:%u\n", __func__, candidate_offset);
                        search_pos = (uint32_t)(found + 1);
                        continue;
                    }

                    if (!mp3_probe_candidate_sync(decoder, priv, priv->read_buffer + found, read_len - found, candidate_offset))
                    {
                        BK_LOGI(AUDIO_PLAYER_TAG, "%s, sync probe reject candidate:%u\n", __func__, candidate_offset);
                        search_pos = (uint32_t)(found + 1);
                        continue;
                    }

                    uint32_t distance = (candidate_offset > target) ? (candidate_offset - target) : (target - candidate_offset);
                    bool candidate_before = (candidate_offset <= target);
                    if (candidate_before)
                    {
                        if (distance < best_before_distance)
                        {
                            best_before_distance = distance;
                            best_before_rel = found;
                        }
                    }
                    else
                    {
                        if (distance < best_after_distance)
                        {
                            best_after_distance = distance;
                            best_after_rel = found;
                        }
                    }
                }
                else
                {
                    if (candidate_near_invalid)
                    {
                        uint32_t invalid_tolerance = priv->invalid_guard_span;
                        if (invalid_tolerance == 0U)
                        {
                            invalid_tolerance = mp3_get_invalid_skip_range(frame_bytes_hint);
                        }
                        BK_LOGI(AUDIO_PLAYER_TAG, "%s, reject invalid header candidate:%u span:%u\n",
                                   __func__, candidate_offset, invalid_tolerance);
                    }
                }

                search_pos = (uint32_t)(found + 1);
            }
        }

        if (best_after_rel >= 0)
        {
            best_rel = best_after_rel;
            best_distance = best_after_distance;
            best_before = false;
        }

        if (best_before_rel >= 0)
        {
            bool prefer_before = false;
            if (best_rel < 0)
            {
                prefer_before = true;
            }
            else if (!best_before)
            {
                bool after_is_close = (best_distance <= prefer_margin / 2U);
                bool before_is_much_better = (best_before_distance + frame_bytes_hint) < best_distance;
                bool after_far = (best_distance > prefer_margin);
                bool before_not_far = (best_before_distance <= (frame_bytes_hint * 3U) / 2U);
                bool after_late = (best_distance > frame_bytes_hint);
                if (!after_is_close && (before_is_much_better || after_far))
                {
                    prefer_before = true;
                }
                else if (before_not_far && after_late)
                {
                    prefer_before = true;
                }
                else if (before_not_far && best_before_distance + (frame_bytes_hint / 4U) <= best_distance)
                {
                    prefer_before = true;
                }
            }

            if (prefer_before)
            {
                best_rel = best_before_rel;
                best_distance = best_before_distance;
                best_before = true;
            }
        }

        if (best_rel >= 0)
        {
            rel = best_rel;
            aligned_offset = scan_start + (uint32_t)rel;
            found_header = true;
            if (priv->last_invalid_header_valid)
            {
                uint32_t invalid_tolerance = priv->invalid_guard_span;
                if (invalid_tolerance == 0U)
                {
                    invalid_tolerance = mp3_get_invalid_skip_range(frame_bytes_hint);
                }
                uint32_t invalid_delta = (aligned_offset > priv->last_invalid_header_offset)
                                             ? (aligned_offset - priv->last_invalid_header_offset)
                                             : (priv->last_invalid_header_offset - aligned_offset);
                aligned_from_invalid = (invalid_delta <= invalid_tolerance);
            }
        }
    }

    if (!found_header)
    {
        if (priv->seek_retry < MP3_SEEK_RETRY_MAX)
        {
            priv->seek_retry++;
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, no frame header found, retry:%u target:%u scan_start:%u scan_size:%u\n",
                       __func__, priv->seek_retry, target, scan_start, scan_size);
            priv->pending_seek_valid = true;
            return;
        }

        BK_LOGE(AUDIO_PLAYER_TAG, "%s, failed to align after max retry, fallback target:%u scan_start:%u\n",
                   __func__, target, scan_start);
        aligned_offset = scan_start;
        priv->seek_retry = 0;
        aligned_from_invalid = false;
        priv->last_alignment_from_invalid = false;
        if (priv->invalid_guard_span == 0U)
        {
            priv->invalid_guard_span = mp3_get_invalid_skip_range(frame_bytes_hint);
        }
    }
    else
    {
        priv->seek_retry = 0;
        priv->last_alignment_from_invalid = aligned_from_invalid;
        if (!aligned_from_invalid && priv->invalid_guard_span > 0U)
        {
            priv->invalid_guard_span /= 2U;
            if (priv->invalid_guard_span < 1024U)
            {
                priv->invalid_guard_span = 0U;
            }
        }
    }

    uint32_t header_offset = aligned_offset;
    if (!found_header)
    {
        priv->seek_payload_bias = priv->seek_payload_bias / 2;
    }

    uint32_t preroll_bytes = mp3_get_preroll_bytes(priv);
    uint32_t start_offset = header_offset;
    if (preroll_bytes > 0 && start_offset > stream_start)
    {
        uint64_t delta = (uint64_t)start_offset - (uint64_t)stream_start;
        if (delta > (uint64_t)preroll_bytes)
        {
            start_offset -= preroll_bytes;
        }
        else
        {
            start_offset = stream_start;
        }
    }

    if (audio_source_seek(decoder->source, start_offset, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek fail when aligning, target=%u\n", __func__, start_offset);
        priv->pending_seek_valid = false;
        return;
    }
    priv->pending_header_offset = header_offset;
    priv->pending_header_valid = true;
    priv->pending_seek_offset = start_offset;
    priv->pending_seek_valid = false;
    priv->last_seek_aligned_offset = header_offset;
    priv->seek_pending_check = true;
    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;
    priv->stream_done = false;
    BK_LOGI(AUDIO_PLAYER_TAG, "%s, aligned header:%u, start:%u\n", __func__, header_offset, start_offset);
}

static int mp3_reset_decoder_state(mp3_decoder_priv_t *priv)
{
    if (!priv)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (priv->decoder)
    {
        MP3FreeDecoder(priv->decoder);
    }

    priv->decoder = MP3InitDecoder();
    if (!priv->decoder)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, MP3InitDecoder fail when resetting decoder\n", __func__);
        return AUDIO_PLAYER_ERR;
    }

    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;
    priv->stream_done = false;
    priv->stream_aborted = false;
    priv->frames = 0;
    priv->frames_decoded = 0;
    priv->samples_per_frame = 0;
    priv->pending_seek_offset = 0;
    priv->pending_seek_valid = false;
    priv->pending_header_offset = 0;
    priv->pending_header_valid = false;
    priv->seek_reference_offset = 0;
    priv->seek_reference_valid = false;
    priv->last_invalid_header_offset = 0;
    priv->last_invalid_header_valid = false;
    priv->last_alignment_from_invalid = false;
    priv->invalid_guard_span = 0;
    priv->xing_toc_valid = false;
    priv->need_reset = false;
    priv->last_seek_aligned_offset = 0;
    priv->seek_retry = 0;
    priv->seek_pending_check = false;
    return AUDIO_PLAYER_OK;
}

static int parse_id3v1_header(bk_audio_player_decoder_t *decoder)
{
    int bytes_read;
    int ret = -1;

    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;

    /* reset read_ptr */
    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;

    if (0 != audio_source_seek(decoder->source, -128, SEEK_END))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek fail, %d\n", __func__, __LINE__);
        return -1;
    }


    bytes_read = audio_source_read_data(decoder->source, (char *)(priv->read_buffer), 128);
    if (bytes_read != 128)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, read 'id3v1' fail, bytes_read: %d, %d\n", __func__, bytes_read, __LINE__);
        goto out;
    }

    /* check identifier "TAG" */
    if (os_strncasecmp((char *)priv->read_buffer, "TAG", os_strlen("TAG")) != 0)
    {
        /* not have ID3V1 */
        BK_LOGI(AUDIO_PLAYER_TAG, "not have ID3V1 \n");
        goto out;
    }
    else
    {
        priv->id3v1_bytes = 128;
        os_memcpy(&priv->id3v1_info, priv->read_buffer, 128);
    }

    ret = 0;
out:
    /* reset read_ptr */
    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;

    if (0 != audio_source_seek(decoder->source, 0, SEEK_SET))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek fail, %d\n", __func__, __LINE__);
        return -1;
    }

    return ret;
}

/* must sure the buf_size is enough */
static int parse_vbr_xing_header(mp3_decoder_priv_t *codec_mp3, char *xing_header_buf, int buf_size)
{
    /* check whether buffer size is enough */
    if (buf_size < 156)
    {
        return -1;
    }

    /* check header id of "XING" */
    if ((xing_header_buf[0] == 'x' || xing_header_buf[0] == 'X')
        && (xing_header_buf[1] == 'i' || xing_header_buf[1] == 'I')
        && (xing_header_buf[2] == 'n' || xing_header_buf[2] == 'N')
        && (xing_header_buf[3] == 'g' || xing_header_buf[3] == 'G')
       )
    {
        codec_mp3->biterate_type = BITERATE_TYPE_VBR_XING;
    }
    else if ((xing_header_buf[0] == 'i' || xing_header_buf[0] == 'I')
             && (xing_header_buf[1] == 'n' || xing_header_buf[1] == 'N')
             && (xing_header_buf[2] == 'f' || xing_header_buf[2] == 'F')
             && (xing_header_buf[3] == 'o' || xing_header_buf[3] == 'O')
            )
    {
        codec_mp3->biterate_type = BITERATE_TYPE_CBR_INFO;
    }
    else
    {
        return -1;
    }

    int offset = 8;

    unsigned char flags = xing_header_buf[7];
    if (flags & 0x01)   //Frames field is present
    {
        codec_mp3->total_frames = *((int *)(xing_header_buf + offset));
        offset += 4;
    }

    if (flags & 0x02)   //Bytes field is present
    {
        codec_mp3->total_bytes = *((int *)(xing_header_buf + offset));
        offset += 4;
    }

    codec_mp3->xing_toc_valid = false;
    if (flags & 0x04)   //TOC field is present
    {
        os_memcpy(codec_mp3->xing_toc, xing_header_buf + offset, 100);
        offset += 100;
        codec_mp3->xing_toc_valid = true;
    }

    return 0;
}

/* must sure the buf_size is enough */
static int parse_vbr_vbri_header(mp3_decoder_priv_t *codec_mp3, char *vbri_header_buf, int buf_size)
{
    /* check whether buffer size is enough */
    if (buf_size < 156)
    {
        return -1;
    }

    /* check header id of "VBRI" */
    if ((vbri_header_buf[0] == 'v' || vbri_header_buf[0] == 'V')
        && (vbri_header_buf[1] == 'b' || vbri_header_buf[1] == 'B')
        && (vbri_header_buf[2] == 'r' || vbri_header_buf[2] == 'R')
        && (vbri_header_buf[3] == 'i' || vbri_header_buf[3] == 'I')
       )
    {
        codec_mp3->biterate_type = BITERATE_TYPE_VBR_VBRI;
    }
    else
    {
        return -1;
    }

    unsigned char *offset = (unsigned char *)(vbri_header_buf + 10);

    codec_mp3->total_bytes = *((int *)(offset));
    offset += 4;

    codec_mp3->total_frames = *((int *)(offset));

    return 0;
}

/* skip id3 tag and return bytes consumed */
static int codec_mp3_skip_idtag(bk_audio_player_decoder_t *decoder)
{
    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;
    uint8_t header[MP3_ID3V2_HEADER_SIZE];
    uint8_t version = 0U;
    uint8_t revision = 0U;
    uint8_t flags = 0U;
    uint32_t tag_size = 0U;
    uint32_t total_skip = 0U;
    int read_len = 0;

    priv->read_ptr = priv->read_buffer;
    priv->bytes_left = 0;

    if (audio_source_seek(decoder->source, 0, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, failed to seek to start\n", __func__);
        return 0;
    }

    read_len = audio_source_read_data(decoder->source, (char *)header, MP3_ID3V2_HEADER_SIZE);
    if (read_len != MP3_ID3V2_HEADER_SIZE)
    {
        (void)audio_source_seek(decoder->source, 0, SEEK_SET);
        return 0;
    }

    if (header[0] != 'I' || header[1] != 'D' || header[2] != '3')
    {
        (void)audio_source_seek(decoder->source, 0, SEEK_SET);
        return 0;
    }

    version = header[3];
    revision = header[4];
    flags = header[5];

    if (version < 2U || version > 4U)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "ID3v2: unsupported version %u.%u\n", version, revision);
        (void)audio_source_seek(decoder->source, 0, SEEK_SET);
        return 0;
    }

    tag_size = mp3_parse_synchsafe32(&header[6]);
    total_skip = MP3_ID3V2_HEADER_SIZE + tag_size;

    if ((flags & 0x10U) != 0U && version == 4U)
    {
        total_skip += MP3_ID3V2_FOOTER_SIZE;
    }

    if (audio_source_seek(decoder->source, (int32_t)total_skip, SEEK_SET) != AUDIO_PLAYER_OK)
    {
        uint32_t remaining = total_skip - MP3_ID3V2_HEADER_SIZE;
        while (remaining > 0U)
        {
            uint32_t chunk = (remaining > MP3_AUDIO_BUF_SZ) ? MP3_AUDIO_BUF_SZ : remaining;
            int consumed = audio_source_read_data(decoder->source, (char *)priv->read_buffer, (int)chunk);
            if (consumed <= 0)
            {
                break;
            }
            remaining -= (uint32_t)consumed;
        }
    }

    priv->bytes_left = 0;
    BK_LOGI(AUDIO_PLAYER_TAG, "ID3v2.%u.%u found, flags=0x%02X, size=%u bytes\n",
               version, revision, flags, total_skip);
    return (int)total_skip;
}

static int check_mp3_sync_word(bk_audio_player_decoder_t *decoder)
{
    return mp3_check_sync_internal(decoder, false);
}

static int mp3_check_sync_internal(bk_audio_player_decoder_t *decoder, bool probe_only)
{
    int err;

    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;

    MP3FrameInfo probe_info;
    MP3FrameInfo *frame_info_ptr = probe_only ? &probe_info : &priv->frame_info;

    os_memset(frame_info_ptr, 0, sizeof(MP3FrameInfo));

    err = MP3GetNextFrameInfo(priv->decoder, frame_info_ptr, priv->read_ptr);
    if (err == ERR_MP3_INVALID_FRAMEHEADER)
    {
        if (!probe_only)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "%s, ERR_MP3_INVALID_FRAMEHEADER, %d\n", __func__, __LINE__);
            uint32_t bad_offset = priv->pending_header_valid ? priv->pending_header_offset : priv->last_seek_aligned_offset;
            uint32_t frame_bytes_hint = mp3_get_frame_bytes_hint(priv);
            uint32_t guard_span = mp3_get_invalid_skip_range(frame_bytes_hint);
            if (priv->invalid_guard_span > guard_span)
            {
                guard_span = priv->invalid_guard_span;
            }
            if (priv->last_invalid_header_valid)
            {
                uint32_t invalid_delta = (bad_offset > priv->last_invalid_header_offset)
                                             ? (bad_offset - priv->last_invalid_header_offset)
                                             : (priv->last_invalid_header_offset - bad_offset);
                if (invalid_delta < frame_bytes_hint || invalid_delta < (guard_span / 2U))
                {
                    uint32_t expanded = guard_span * 2U;
                    if (expanded < guard_span)
                    {
                        expanded = MP3_AUDIO_BUF_SZ;
                    }
                    if (expanded > MP3_AUDIO_BUF_SZ)
                    {
                        expanded = MP3_AUDIO_BUF_SZ;
                    }
                    guard_span = expanded;
                }
            }
            priv->last_invalid_header_offset = bad_offset;
            priv->last_invalid_header_valid = true;
            priv->invalid_guard_span = guard_span;
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, guard span:%u around bad offset:%u\n", __func__, guard_span, bad_offset);
            mp3_schedule_alignment_retry(decoder, priv, false);
        }
        goto __err;
    }
    else if (err != ERR_MP3_NONE)
    {
        if (!probe_only)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, MP3GetNextFrameInfo fail, err=%d, %d\n", __func__, err, __LINE__);
        }
        goto __err;
    }
    else if (frame_info_ptr->nChans != 1 && frame_info_ptr->nChans != 2)
    {
        if (!probe_only)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, nChans is not 1 or 2, nChans=%d, %d\n", __func__, frame_info_ptr->nChans, __LINE__);
        }
        goto __err;
    }
    else if (frame_info_ptr->bitsPerSample != 16 && frame_info_ptr->bitsPerSample != 8)
    {
        if (!probe_only)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, bitsPerSample is not 16 or 8, bitsPerSample=%d, %d\n", __func__, frame_info_ptr->bitsPerSample, __LINE__);
        }
        goto __err;
    }
    else
    {
        if (!probe_only)
        {
            if (frame_info_ptr != &priv->frame_info)
            {
                os_memcpy(&priv->frame_info, frame_info_ptr, sizeof(MP3FrameInfo));
            }
            priv->last_invalid_header_valid = false;
            if (priv->invalid_guard_span > 0U)
            {
                priv->invalid_guard_span /= 2U;
                if (priv->invalid_guard_span < 1024U)
                {
                    priv->invalid_guard_span = 0U;
                }
            }
        }
    }

    return 0;

__err:
    return -1;
}

static bool mp3_probe_candidate_sync(bk_audio_player_decoder_t *decoder, mp3_decoder_priv_t *priv,
                                     uint8_t *candidate_ptr, int available_bytes, uint32_t candidate_offset)
{
    if (!decoder || !priv || !candidate_ptr || available_bytes <= 0)
    {
        return false;
    }

    uint8_t *saved_read_ptr = priv->read_ptr;
    uint32_t saved_bytes_left = priv->bytes_left;
    uint32_t saved_pending_header = priv->pending_header_offset;
    bool saved_pending_valid = priv->pending_header_valid;
    uint32_t saved_last_seek = priv->last_seek_aligned_offset;

    priv->read_ptr = candidate_ptr;
    priv->bytes_left = (uint32_t)available_bytes;
    priv->pending_header_offset = candidate_offset;
    priv->pending_header_valid = true;
    priv->last_seek_aligned_offset = candidate_offset;

    int ret = mp3_check_sync_internal(decoder, true);

    priv->read_ptr = saved_read_ptr;
    priv->bytes_left = saved_bytes_left;
    priv->pending_header_offset = saved_pending_header;
    priv->pending_header_valid = saved_pending_valid;
    priv->last_seek_aligned_offset = saved_last_seek;

    return (ret == 0);
}

static int mp3_decoder_is_seek_ready(bk_audio_player_decoder_t *decoder)
{
    if (!decoder || !decoder->decoder_priv)
    {
        return 1;
    }

    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;
    return priv->seek_pending_check ? 0 : 1;
}




static int32_t codec_mp3_fill_buffer(bk_audio_player_decoder_t *decoder)
{
    int bytes_read;
    size_t bytes_to_read;

    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;

    if (priv->stream_done)
    {
        return RB_DONE;
    }

    /* adjust read ptr */
    if (priv->bytes_left > 0xffff0000)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "read_ptr: %p, bytes_left: 0x%x \n", priv->read_ptr, priv->bytes_left);
        return -1;
    }

    if (priv->bytes_left > 0)
    {
        memmove(priv->read_buffer, priv->read_ptr, priv->bytes_left);
    }
    priv->read_ptr = priv->read_buffer;

    bytes_to_read = (MP3_AUDIO_BUF_SZ - priv->bytes_left) & ~(512 - 1);
    if (bytes_to_read == 0)
    {
        return 0;
    }

    bytes_read = audio_source_read_data(decoder->source, (char *)(priv->read_buffer + priv->bytes_left), bytes_to_read);
    if (bytes_read > 0)
    {
        priv->bytes_left = priv->bytes_left + bytes_read;
        priv->stream_done = false;
        priv->stream_aborted = false;
        return 0;
    }
    else if (bytes_read == AUDIO_PLAYER_TIMEOUT)
    {
        return AUDIO_PLAYER_TIMEOUT;
    }
    else if (bytes_read == 0 || bytes_read == RB_DONE)
    {
        /* normal end of stream: tail data is complete and should be decoded */
        priv->stream_done = true;
        priv->stream_aborted = false;
        if (priv->bytes_left != 0)
        {
            return 0;
        }

        BK_LOGW(AUDIO_PLAYER_TAG, "can't read more data, end of stream. left=%d\n", priv->bytes_left);
        return RB_DONE;
    }
    else if (bytes_read == RB_ABORT)
    {
        /* aborted stream (network drop / read error): residual tail may be a truncated
         * frame and must be dropped instead of decoded */
        priv->stream_done = true;
        priv->stream_aborted = true;
        if (priv->bytes_left != 0)
        {
            return 0;
        }

        BK_LOGW(AUDIO_PLAYER_TAG, "stream aborted, end of stream. left=%d\n", priv->bytes_left);
        return RB_DONE;
    }
    else if (priv->bytes_left != 0)
    {
        return 0;
    }

    BK_LOGW(AUDIO_PLAYER_TAG, "can't read more data, ret=%d, left=%d\n", bytes_read, priv->bytes_left);
    return -1;
}



static int mp3_decoder_open(audio_format_t format, void *param, bk_audio_player_decoder_t **decoder_pp)
{
    bk_audio_player_decoder_t *decoder;
    mp3_decoder_priv_t *priv;

    if (format != AUDIO_FORMAT_MP3)
    {
        return AUDIO_PLAYER_INVALID;
    }

    decoder = audio_codec_new(sizeof(mp3_decoder_priv_t));
    if (!decoder)
    {
        return AUDIO_PLAYER_NO_MEM;
    }

    priv = (mp3_decoder_priv_t *)decoder->decoder_priv;

    os_memset(priv, 0x0, sizeof(mp3_decoder_priv_t));

    /* init read session */
    priv->read_ptr = NULL;
    priv->bytes_left = 0;
    priv->frames = 0;
    priv->need_reset = false;

    priv->read_buffer = player_malloc(MP3_AUDIO_BUF_SZ);
    if (priv->read_buffer == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, priv->read_buffer malloc fail, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    priv->decoder = MP3InitDecoder();
    if (!priv->decoder)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, MP3InitDecoder create fail, %d\n", __func__, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    *decoder_pp = decoder;

    return AUDIO_PLAYER_OK;
}

static int mp3_decoder_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    mp3_decoder_priv_t *priv;
    priv = (mp3_decoder_priv_t *)decoder->decoder_priv;
    int read_offset;
    //    int read_retry_cnt = 5;
    int ret = 0;

#ifdef FRAME_INFO_FILTER_EN
    frame_info_list_t frame_info_list = {0};
    int frame_info_cnt = 0;
    frame_info_item_t *frame_info_item = NULL;
    frame_info_list_init(&frame_info_list);
#endif

    /* check and parse ID3V1 field */
    parse_id3v1_header(decoder);

    if (!priv->current_sample_rate)
    {
        priv->stream_offset = codec_mp3_skip_idtag(decoder);
        priv->id3v2_bytes = (uint32_t)priv->stream_offset;
    }

__retry:
    if ((priv->read_ptr == NULL) || priv->bytes_left < 2 * MAINBUF_SIZE)
    {
        int fill_ret = codec_mp3_fill_buffer(decoder);
        if (fill_ret == AUDIO_PLAYER_TIMEOUT)
        {
            goto __retry;
        }
        if (fill_ret != 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, codec_mp3_fill_buffer fail, %d \n", __func__, __LINE__);
            return AUDIO_PLAYER_ERR;
        }
    }

#if 0
    /* Protect mp3 decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < MAINBUF_SIZE)
    {
        if ((read_retry_cnt--) > 0)
        {
            goto __retry;
        }
        else
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, connot read enough data, read: %d < %d, %d \n", __func__, priv->bytes_left, MAINBUF_SIZE);
            return AUDIO_PLAYER_ERR;
        }
    }
#endif

    read_offset = MP3FindSyncWord(priv->read_ptr, priv->bytes_left);
    if (read_offset < 0)
    {
        /* discard this data */
        BK_LOGI(AUDIO_PLAYER_TAG, "%s, MP3FindSyncWord fail, outof sync, byte left: %d, %d\n", __func__, priv->bytes_left, __LINE__);
        /* maybe bytes_left is not enough, need fill buffer again */
        //TODO
        priv->bytes_left = 0;
        return AUDIO_PLAYER_ERR;
    }

    if (read_offset > priv->bytes_left)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, find sync exception, read_offset:%d > bytes_left:%d, %d\n", __func__, read_offset, priv->bytes_left, __LINE__);
        priv->read_ptr += priv->bytes_left;
        priv->bytes_left = 0;
    }
    else
    {
        priv->read_ptr += read_offset;
        priv->bytes_left -= read_offset;
    }

    if (priv->id3v2_bytes == 0U)
    {
        priv->id3v2_bytes = (uint32_t)read_offset;
    }
    BK_LOGI(AUDIO_PLAYER_TAG, "id3v2_bytes: %u \n", priv->id3v2_bytes);

    ret = check_mp3_sync_word(decoder);
    if (ret == -1)
    {
        if (priv->bytes_left > 0)
        {
            priv->bytes_left --;
            priv->read_ptr ++;
        }

        goto __retry;
    }

    priv->read_ptr -= read_offset;
    priv->bytes_left += read_offset;

    BK_LOGI(AUDIO_PLAYER_TAG, "bitrate:%d, nChans:%d, samprate:%d, bitsPerSample:%d, outputSamps:%d\n", priv->frame_info.bitrate, priv->frame_info.nChans, priv->frame_info.samprate, priv->frame_info.bitsPerSample, priv->frame_info.outputSamps);
    /* update audio frame info */
    decoder->info.channel_number = priv->frame_info.nChans;
    decoder->info.sample_rate = priv->frame_info.samprate;
    decoder->info.sample_bits = priv->frame_info.bitsPerSample;
    decoder->info.frame_size = 2 * priv->frame_info.outputSamps;
    decoder->info.bps = priv->frame_info.bitrate;

    if (priv->frame_info.nChans > 0)
    {
        uint32_t samp = priv->frame_info.outputSamps / priv->frame_info.nChans;
        if (samp > 0)
        {
            priv->samples_per_frame = samp;
            if (priv->base_samples_per_frame == 0)
            {
                priv->base_samples_per_frame = samp;
            }
        }
    }
    if (priv->base_bit_rate == 0 && priv->frame_info.bitrate > 0)
    {
        priv->base_bit_rate = (uint32_t)priv->frame_info.bitrate;
    }
    if (priv->base_bit_rate == 0 && priv->frame_info.bitrate > 0)
    {
        priv->base_bit_rate = (uint32_t)priv->frame_info.bitrate;
    }

    if (priv->total_bytes == 0)
    {
        uint32_t file_total_bytes = audio_source_get_total_bytes(decoder->source);
        if (file_total_bytes > 0)
        {
            uint64_t data_bytes = file_total_bytes;
            if (data_bytes > priv->stream_offset)
            {
                data_bytes -= priv->stream_offset;
            }
            else
            {
                data_bytes = 0;
            }

            if (priv->id3v1_bytes < data_bytes)
            {
                data_bytes -= priv->id3v1_bytes;
            }
            else
            {
                data_bytes = 0;
            }

            priv->total_bytes = (uint32_t)data_bytes;
            decoder->info.total_bytes = (int)data_bytes;

            if (priv->frame_info.bitrate > 0 && data_bytes > 0)
            {
                priv->total_duration = ((double)data_bytes * 8.0 / (double)priv->frame_info.bitrate) * 1000.0;
                decoder->info.duration = priv->total_duration;
            }
        }
    }

    if (priv->frame_info.nChans > 0)
    {
        uint32_t samp = priv->frame_info.outputSamps / priv->frame_info.nChans;
        if (samp > 0)
        {
            priv->samples_per_frame = samp;
            if (priv->base_samples_per_frame == 0)
            {
                priv->base_samples_per_frame = samp;
            }
        }
    }

    /* check whether mp3 file is VBR when the frame is first frame */
    if (priv->biterate_type == BITERATE_TYPE_UNKNOW)
    {
        if (0 == parse_vbr_xing_header(priv, (char *)(priv->read_ptr + read_offset), (int)(priv->bytes_left - read_offset)))
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "[XING] biterate_type:%d, total_bytes:%d, total_frames:%d \n", priv->biterate_type, priv->total_bytes, priv->total_frames);
        }
        else if (0 == parse_vbr_vbri_header(priv, (char *)(priv->read_ptr + read_offset), (int)(priv->bytes_left - read_offset)))
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "[VBRI] biterate_type:%d, total_bytes:%d, total_frames:%d \n", priv->biterate_type, priv->total_bytes, priv->total_frames);
        }
        else
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "[CBR] biterate_type:%d \n", priv->biterate_type);
            priv->biterate_type = BITERATE_TYPE_CBR;
        }
    }

    /* calculate total duration of mp3 */
    switch (priv->biterate_type)
    {
        case BITERATE_TYPE_CBR:
        case BITERATE_TYPE_CBR_INFO:
            /* CBR: total_duration = total_bytes / bitrate * 8000 ms */
            //priv->total_duration = priv->total_bytes / priv->frame_info.bitrate * 8000;
            /* set codec info */
            decoder->info.total_bytes = priv->total_bytes;
            decoder->info.header_bytes = priv->id3v2_bytes + priv->id3v1_bytes;
            /* need user calculate */
            decoder->info.duration = 0.0;
            BK_LOGI(AUDIO_PLAYER_TAG, "[CBR] total_bytes:%d \n", priv->total_bytes);
            break;

        case BITERATE_TYPE_VBR_XING:
        case BITERATE_TYPE_VBR_VBRI:
            /* VBR: total_duration = total_bytes / bitrate * 8000 ms */
            priv->total_duration = (double)priv->total_frames * 1152.0 * (1.0 / (double)priv->frame_info.samprate) * 1000.0;
            /* set codec info */
            decoder->info.duration = priv->total_duration;
            BK_LOGI(AUDIO_PLAYER_TAG, "[VBR] total_duration:%f \n", priv->total_duration);
            break;

        default:
            break;
    }

#ifdef FRAME_INFO_FILTER_EN
    /* add frame_info to frame_info_list */
    frame_info_t frame_info = {0};
    frame_info.bitRate = priv->frame_info.bitrate;
    frame_info.nChans = priv->frame_info.nChans;
    frame_info.sampRate = priv->frame_info.samprate;
    frame_info.bitsPerSample = priv->frame_info.bitsPerSample;
    frame_info.outputSamps = priv->frame_info.outputSamps;
    ret = frame_info_handler(&frame_info_list, frame_info);
    if (ret == 0)
    {
        frame_info_cnt ++;
    }

    if (frame_info_cnt == FRAME_INFO_TOTAL_NUM)
    {
        frame_info_item = get_max_num_frame_info(&frame_info_list);
        if (frame_info_item && frame_info_item->num >= FRAME_INFO_PASS_NUM)
        {
            //TODO
            debug_frame_info_list(&frame_info_list);
            BK_LOGI(AUDIO_PLAYER_TAG, "bitRate:%d, nChans:%d, sampRate:%d, bitsPerSample:%d, outputSamps:%d\n",
                       frame_info_item->frame_info.bitRate,
                       frame_info_item->frame_info.nChans,
                       frame_info_item->frame_info.sampRate,
                       frame_info_item->frame_info.bitsPerSample,
                       frame_info_item->frame_info.outputSamps);
        }
        else
        {
            /* restart */
            frame_info_cnt = 0;
            frame_info_list_deinit(&frame_info_list);
            frame_info_list_init(&frame_info_list);
            goto __retry;
        }
    }
    else
    {
        goto __retry;
    }
#endif

#ifdef FRAME_INFO_FILTER_EN
    priv->current_sample_rate = frame_info_item->frame_info.sampRate;
    if (priv->base_sample_rate == 0 && priv->current_sample_rate > 0)
    {
        priv->base_sample_rate = (uint32_t)priv->current_sample_rate;
    }

    info->channel_number = frame_info_item->frame_info.nChans;
    info->sample_rate = frame_info_item->frame_info.sampRate;
    info->sample_bits = frame_info_item->frame_info.bitsPerSample;
    info->frame_size = 2 * frame_info_item->frame_info.outputSamps;
#else
    priv->current_sample_rate = priv->frame_info.samprate;
    if (priv->base_sample_rate == 0 && priv->current_sample_rate > 0)
    {
        priv->base_sample_rate = (uint32_t)priv->current_sample_rate;
    }

    info->channel_number = priv->frame_info.nChans;
    info->sample_rate = priv->frame_info.samprate;
    info->sample_bits = priv->frame_info.bitsPerSample;
    info->frame_size = 2 * priv->frame_info.outputSamps;

    /* update audio frame info */
    decoder->info.channel_number = priv->frame_info.nChans;
    decoder->info.sample_rate = priv->frame_info.samprate;
    decoder->info.sample_bits = priv->frame_info.bitsPerSample;
    decoder->info.frame_size = 2 * priv->frame_info.outputSamps;
#endif

    return AUDIO_PLAYER_OK;
}

int mp3_decoder_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    int err;
    int read_offset;
    int fill_ret;

    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;

    bool had_pending_seek = priv->pending_seek_valid;
    uint32_t pending_seek_offset = priv->pending_seek_offset;
    bool had_pending_header = priv->pending_header_valid;
    uint32_t pending_header_offset = priv->pending_header_offset;
    bool had_seek_pending_check = priv->seek_pending_check;
    uint8_t pending_seek_retry = priv->seek_retry;
    uint32_t pending_last_seek_offset = priv->last_seek_aligned_offset;
    bool had_seek_reference = priv->seek_reference_valid;
    uint32_t seek_reference_offset = priv->seek_reference_offset;
    bool had_invalid_header = priv->last_invalid_header_valid;
    uint32_t invalid_header_offset = priv->last_invalid_header_offset;
    uint32_t invalid_guard_span = priv->invalid_guard_span;

    if (priv->need_reset)
    {
        if (mp3_reset_decoder_state(priv) != AUDIO_PLAYER_OK)
        {
            return AUDIO_PLAYER_ERR;
        }

        if (had_pending_seek)
        {
            priv->pending_seek_offset = pending_seek_offset;
            priv->pending_seek_valid = true;
        }
        if (had_pending_header)
        {
            priv->pending_header_offset = pending_header_offset;
            priv->pending_header_valid = true;
        }
        if (had_seek_pending_check)
        {
            priv->seek_pending_check = true;
            priv->seek_retry = pending_seek_retry;
            priv->last_seek_aligned_offset = pending_last_seek_offset;
        }
        if (had_seek_reference)
        {
            priv->seek_reference_offset = seek_reference_offset;
            priv->seek_reference_valid = true;
        }
        if (had_invalid_header)
        {
            priv->last_invalid_header_offset = invalid_header_offset;
            priv->last_invalid_header_valid = true;
        }
        if (invalid_guard_span > 0U)
        {
            priv->invalid_guard_span = invalid_guard_span;
        }
    }

    if (priv->pending_seek_valid)
    {
        mp3_align_after_seek(decoder);
    }

    while (((priv->read_ptr == NULL) || priv->bytes_left < 2 * MAINBUF_SIZE) && !priv->stream_done)
    {

#ifdef WIFI_READ_DATA_TIME_DEBUG
        uint64_t start_time = 0;
        uint32_t start_bytes = priv->bytes_left;
#if CONFIG_ARCH_RISCV
        extern u64 riscv_get_mtimer(void);
        start_time = riscv_get_mtimer();
#elif CONFIG_ARCH_CM33

#if CONFIG_AON_RTC
        start_time = bk_aon_rtc_get_us();
#endif      //#if CONFIG_AON_RTC
#endif      //#if CONFIG_ARCH_RISCV
#endif      //#ifdef WIFI_READ_DATA_TIME_DEBUG

        //GPIO_UP(44);
        fill_ret = codec_mp3_fill_buffer(decoder);
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
        //GPIO_DOWN(44);

#ifdef WIFI_READ_DATA_TIME_DEBUG
        uint64_t stop_time = 0;
        uint32_t stop_bytes = priv->bytes_left;
#if CONFIG_ARCH_RISCV
        extern u64 riscv_get_mtimer(void);
        stop_time = riscv_get_mtimer();
#elif CONFIG_ARCH_CM33

#if CONFIG_AON_RTC
        stop_time = bk_aon_rtc_get_us();
#endif      //#if CONFIG_AON_RTC
#endif      //#if CONFIG_ARCH_RISCV

        /* check wifi read data speed */
        if ((stop_time - start_time) > WIFI_READ_DATA_TIMEOUT_NUM)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "================== Notion =================\n");
            //BK_LOGE(AUDIO_PLAYER_TAG, "stop_time: 0x%04x%04x us, start_time: 0x%04x%04x us\n", (uint32_t)(stop_time>>32 & 0x00000000ffffffff), (uint32_t)(stop_time & 0x00000000ffffffff), (uint32_t)(start_time>>32 & 0x00000000ffffffff), (uint32_t)(start_time & 0x00000000ffffffff));
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, wifi read %d bytes data use 0x%04x%04x us, %d\n", __FUNCTION__, stop_bytes - start_bytes, (uint32_t)((stop_time - start_time) >> 32), (uint32_t)((stop_time - start_time) & 0x00000000ffffffff), __LINE__);
            BK_LOGE(AUDIO_PLAYER_TAG, "\n");
        }
#endif      //#ifdef WIFI_READ_DATA_TIME_DEBUG
    }

    /* Protect mp3 decoder to avoid decoding assert when data is insufficient. */
    if (priv->bytes_left < MAINBUF_SIZE)
    {
        if (priv->stream_done)
        {
            if (priv->stream_aborted || priv->bytes_left == 0)
            {
                /* Aborted stream: drop the (likely truncated) residual frame. Empty
                 * buffer: nothing left to decode. */
                if (priv->bytes_left != 0)
                {
                    BK_LOGW(AUDIO_PLAYER_TAG, "%s, aborted, drop tail: %d < %d, %d\n", __func__, priv->bytes_left, MAINBUF_SIZE, __LINE__);
                }
                priv->bytes_left = 0;
                return 0;
            }

            /* Clean end of stream with residual data: fall through and let the remaining
             * whole frame(s) be decoded below. MP3Decode returns ERR_MP3_INDATA_UNDERFLOW
             * once no full frame is left, which ends playback without truncating the tail. */
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, end of stream, decode tail: %d < %d, %d\n", __func__, priv->bytes_left, MAINBUF_SIZE, __LINE__);
        }
        else
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, wait for more data, read: %d < %d, %d\n", __func__, priv->bytes_left, MAINBUF_SIZE, __LINE__);
            return AUDIO_PLAYER_TIMEOUT;
        }
    }

    int sync_attempts = 0;
    while (1)
    {
        read_offset = MP3FindSyncWord(priv->read_ptr, priv->bytes_left);
        if (read_offset < 0)
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, MP3FindSyncWord fail, outof sync, byte left: %d, %d\n", __func__, priv->bytes_left, __LINE__);
            priv->bytes_left = 0;
                goto __sync_retry;
        }

        if ((uint32_t)read_offset > priv->bytes_left)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "%s, find sync exception, read_offset:%d > bytes_left:%d, %d\n", __func__, read_offset, priv->bytes_left, __LINE__);
            priv->read_ptr += priv->bytes_left;
            priv->bytes_left = 0;
                goto __sync_retry;
        }

        priv->read_ptr += read_offset;
        priv->bytes_left -= read_offset;

        if (check_mp3_sync_word(decoder) == 0)
        {
            mp3_commit_seek_bias(priv);
            priv->seek_pending_check = false;
            priv->seek_retry = 0;
            goto __sync_done;
        }

        if (priv->force_realign)
        {
            priv->force_realign = false;
            priv->bytes_left = 0;
            return 0;
        }

        sync_attempts++;
        if (sync_attempts >= MP3_SYNC_MAX_ATTEMPTS || priv->bytes_left == 0)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, sync retry limit reached (%d)\n", __func__, sync_attempts);
            goto __sync_retry;
        }

        priv->read_ptr++;
        priv->bytes_left--;
        if (priv->bytes_left == 0)
        {
            goto __sync_retry;
        }
    }

__sync_retry:
    if (priv->seek_pending_check && priv->seek_retry < MP3_SEEK_RETRY_MAX)
    {
        uint32_t step = ((uint32_t)priv->seek_retry + 1U) * MP3_SEEK_RETRY_STEP_BYTES;
        uint64_t new_offset = priv->last_seek_aligned_offset;
        uint64_t stream_start = (uint64_t)priv->stream_offset;
        if (new_offset > stream_start)
        {
            uint64_t distance = new_offset - stream_start;
            if (distance > step)
            {
                new_offset -= step;
            }
            else
            {
                new_offset = stream_start;
            }
        }
        else
        {
            new_offset = stream_start;
        }
        priv->pending_seek_offset = (uint32_t)new_offset;
        priv->pending_seek_valid = true;
        priv->pending_header_offset = (uint32_t)new_offset;
        priv->pending_header_valid = true;
        priv->seek_retry++;
        priv->need_reset = true;
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, seek alignment retry %u, offset=%u\n", __func__, priv->seek_retry, priv->pending_seek_offset);
        return 0;
    }

    if (priv->seek_pending_check)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, seek alignment exhausted, flush buffer\n", __func__);
        priv->seek_pending_check = false;
    }

    priv->bytes_left = 0;
    return 0;

__sync_done:
    /* check whether sample rate change */
    if (priv->frame_info.samprate != priv->current_sample_rate)
    {
        bool allow_update = (priv->base_sample_rate == 0U);
        if (!allow_update && priv->frame_info.samprate > 0)
        {
            int diff = (int)priv->frame_info.samprate - (int)priv->base_sample_rate;
            if (diff < 0)
            {
                diff = -diff;
            }
            if (diff <= 50)
            {
                allow_update = true;
            }
        }

        if (allow_update)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "================== Frame_info Change Notion =================\n");
        BK_LOGI(AUDIO_PLAYER_TAG, "new frame_info=>> bitrate:%d, nChans:%d, samprate:%d, bitsPerSample:%d, outputSamps:%d\n", priv->frame_info.bitrate, priv->frame_info.nChans, priv->frame_info.samprate, priv->frame_info.bitsPerSample, priv->frame_info.outputSamps);
        BK_LOGI(AUDIO_PLAYER_TAG, "new samprate:%d, current_sample_rate:%d\n", priv->frame_info.samprate, priv->current_sample_rate);
        BK_LOGI(AUDIO_PLAYER_TAG, "\n");

        /* update audio frame info */
        decoder->info.channel_number = priv->frame_info.nChans;
        decoder->info.sample_rate = priv->frame_info.samprate;
        decoder->info.sample_bits = priv->frame_info.bitsPerSample;
        decoder->info.frame_size = 2 * priv->frame_info.outputSamps;

        priv->current_sample_rate = priv->frame_info.samprate;
            if (priv->base_sample_rate == 0 && priv->current_sample_rate > 0)
            {
                priv->base_sample_rate = (uint32_t)priv->current_sample_rate;
            }
        }
        else
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, ignore sample rate jump %d -> %d when base=%u\n",
                       __func__, priv->current_sample_rate, priv->frame_info.samprate, priv->base_sample_rate);
        }
    }

#if 0
    if (priv->bytes_left < 1024)
    {
        /* fill more data */
        if (codec_mp3_fill_buffer(decoder) != 0)
        {
            return -1;
        }
    }
#endif

    uint8_t *frame_start_ptr = priv->read_ptr;
    int bytes_before_decode = priv->bytes_left;

    err = MP3Decode(priv->decoder, &priv->read_ptr, (int *)&priv->bytes_left, (short *)buffer, 0);
    priv->frames++;
    if (err != ERR_MP3_NONE)
    {
        switch (err)
        {
            case ERR_MP3_INDATA_UNDERFLOW:
                // LOG_E("ERR_MP3_INDATA_UNDERFLOW.");
                // codec_mp3->bytes_left = 0;
                fill_ret = codec_mp3_fill_buffer(decoder);
                if (fill_ret == AUDIO_PLAYER_TIMEOUT)
                {
                    return AUDIO_PLAYER_TIMEOUT;
                }
                if (fill_ret == RB_DONE)
                {
                    return 0;
                }
                if (fill_ret != 0)
                {
                    /* release this memory block */
                    return AUDIO_PLAYER_ERR;
                }
                break;

            case ERR_MP3_MAINDATA_UNDERFLOW:
                /* do nothing - next call to decode will provide more mainData */
                // LOG_E("ERR_MP3_MAINDATA_UNDERFLOW.");
                break;

            case ERR_MP3_INVALID_HUFFCODES:
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, invalid huffman codes, skip frame, left:%d, %d\n", __func__, priv->bytes_left, __LINE__);
                /* fall through */

            default:
                // LOG_E("unknown error: %d, left: %d.", err, codec_mp3->bytes_left);
                // LOG_D("stream position: %d.", codec_mp3->parent.stream->position);
                // stream_buffer(0, NULL);

                // skip this frame
                if (priv->bytes_left > 0)
                {
                    priv->bytes_left --;
                    priv->read_ptr ++;
                }
                else
                {
                /* decoder ran out of bytes while handling an unexpected error */
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, decode error:%d with empty buffer, %d\n", __func__, err, __LINE__);
//                if (codec_mp3_fill_buffer(decoder) != 0)
//                {
//                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, refill after decode error failed, %d\n", __func__, __LINE__);
//                    return -1;
//                }
                }
                break;
        }
    }
    else
    {
        int outputSamps;
        /* no error */
        MP3GetLastFrameInfo(priv->decoder, &priv->frame_info);
        priv->frames_decoded++;
        if (priv->frame_info.nChans > 0)
        {
            priv->samples_per_frame = priv->frame_info.outputSamps / priv->frame_info.nChans;
        }

        int consumed_bytes = bytes_before_decode - priv->bytes_left;
        if (consumed_bytes <= 0)
        {
            consumed_bytes = (int)(priv->read_ptr - frame_start_ptr);
        }
        if (consumed_bytes > 0)
        {
            priv->frame_bytes_total += (uint64_t)consumed_bytes;
            priv->frame_bytes_count++;
            if (priv->frame_bytes_count > 0)
            {
                uint32_t avg = (uint32_t)(priv->frame_bytes_total / (uint64_t)priv->frame_bytes_count);
                if (avg > 0)
                {
                    priv->frame_bytes_estimate = avg;
                }
            }
        }

        /* write to sound device */
        outputSamps = priv->frame_info.outputSamps;
        if (outputSamps > 0)
        {
#if 0
            if (priv->frame_info.nChans == 1)
            {
                int i;
                for (i = outputSamps - 1; i >= 0; i--)
                {
                    buffer[i * 2] = buffer[i];
                    buffer[i * 2 + 1] = buffer[i];
                }
                outputSamps *= 2;
            }
#endif
            return outputSamps * sizeof(uint16_t);
        }
    }

    return 0;
}

static int mp3_decoder_close(bk_audio_player_decoder_t *decoder)
{
    mp3_decoder_priv_t *priv;
    priv = (mp3_decoder_priv_t *)decoder->decoder_priv;

    BK_LOGD(AUDIO_PLAYER_TAG, "%s, %d\n", __FUNCTION__, __LINE__);

    if (priv && priv->decoder)
    {
        MP3FreeDecoder(priv->decoder);
        priv->decoder = NULL;
    }


    if (priv->read_buffer)
    {
        player_free(priv->read_buffer);
        priv->read_buffer = NULL;
    }

    return AUDIO_PLAYER_OK;
}

static int mp3_decoder_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
#if 0
    mp3_decoder_priv_t *priv;
    priv = (mp3_decoder_priv_t *)decoder->decoder_priv;

    return 2 * priv->frame_info.outputSamps;
#endif

    (void)decoder;
    /* set max value to avoid memory overflow when sample rate change */
    //TODO
    /* bitRate:64000, nChans:2, sampRate:48000, bitsPerSample:16, outputSamps:2304 */
    return 4608;
}

static int calc_mp3_position(bk_audio_player_decoder_t *decoder, int second)
{
    mp3_decoder_priv_t *priv = (mp3_decoder_priv_t *)decoder->decoder_priv;
    if (!priv)
    {
        return 0;
    }

    if (second < 0)
    {
        second = 0;
    }

    uint64_t stream_start = (uint64_t)priv->stream_offset;
    uint64_t available_bytes = mp3_get_available_payload_bytes(priv, decoder);
    uint64_t payload_offset = 0;

    if (priv->xing_toc_valid && available_bytes > 0 && priv->total_duration > 0.0)
    {
        payload_offset = mp3_calc_vbr_payload_offset(priv, second, available_bytes);
    }
    else
    {
        payload_offset = mp3_calc_cbr_payload_offset(priv, decoder, second, available_bytes);
        if (priv->frame_bytes_estimate == 0)
        {
            priv->frame_bytes_estimate = mp3_estimate_frame_bytes(priv);
        }
    }

    uint64_t header_offset = stream_start + payload_offset;
    if (available_bytes > 0)
    {
        uint64_t stream_end = stream_start + available_bytes;
        if (header_offset >= stream_end)
        {
            header_offset = (stream_end > 0) ? (stream_end - 1ULL) : stream_start;
        }
    }

    uint32_t preroll_bytes = mp3_get_preroll_bytes(priv);
    uint64_t start_offset = header_offset;
    if (preroll_bytes > 0 && start_offset > stream_start)
    {
        uint64_t delta = start_offset - stream_start;
        if (delta > (uint64_t)preroll_bytes)
        {
            start_offset -= preroll_bytes;
        }
        else
        {
            start_offset = stream_start;
        }
    }

    if (start_offset > (uint64_t)INT32_MAX)
    {
        start_offset = INT32_MAX;
    }

    priv->pending_header_offset = (uint32_t)header_offset;
    priv->pending_header_valid = true;
    priv->pending_seek_offset = (uint32_t)start_offset;
    priv->pending_seek_valid = true;
    priv->last_seek_aligned_offset = (uint32_t)header_offset;
    priv->seek_reference_offset = (uint32_t)header_offset;
    priv->seek_reference_valid = true;
    priv->last_invalid_header_valid = false;
    priv->last_alignment_from_invalid = false;
    priv->invalid_guard_span = 0;
    priv->seek_retry = 0;
    priv->seek_pending_check = true;
    priv->need_reset = true;
    BK_LOGD(AUDIO_PLAYER_TAG, "%s, second:%d, header:%u, start:%u\n", __func__, second, priv->pending_header_offset, priv->pending_seek_offset);

    return (int)start_offset;
}

const bk_audio_player_decoder_ops_t mp3_decoder_ops =
{
    .name = "mp3",
    .open = mp3_decoder_open,
    .get_info = mp3_decoder_get_info,
    .get_chunk_size = mp3_decoder_get_chunk_size,
    .get_data = mp3_decoder_get_data,
    .close = mp3_decoder_close,
    .calc_position = calc_mp3_position,
    .is_seek_ready = mp3_decoder_is_seek_ready,
};

/* Get MP3 decoder operations structure */
const bk_audio_player_decoder_ops_t *bk_audio_player_get_mp3_decoder_ops(void)
{
    return &mp3_decoder_ops;
}
