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

#include "audio_metadata_parser_common.h"
#include "player_mem.h"
#include "player_osal.h"

#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <bk_posix.h>

#define MP3_META_SCAN_BUFFER_SIZE      4096
#define MP3_META_FRAME_PROBE_SIZE      512
#define MP3_META_FRAME_CHAIN_COUNT     2
#define MP3_ID3V1_TAG_SIZE             128
#define MP3_ID3V2_HEADER_SIZE          10
#define MP3_ID3V2_FOOTER_SIZE          10

static const uint16_t mp3_bitrate_table_mpeg1_l3[16] = {
    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};
static const uint16_t mp3_bitrate_table_mpeg2_l3[16] = {
    0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0
};
static const uint32_t mp3_samplerate_table[3][3] = {
    {44100U, 48000U, 32000U},
    {22050U, 24000U, 16000U},
    {11025U, 12000U, 8000U},
};

typedef enum
{
    MP3_META_BITRATE_UNKNOWN = 0,
    MP3_META_BITRATE_CBR,
    MP3_META_BITRATE_CBR_INFO,
    MP3_META_BITRATE_VBR_XING,
    MP3_META_BITRATE_VBR_VBRI,
} mp3_meta_bitrate_type_t;

typedef struct mp3_parser_context_s
{
    audio_metadata_t *metadata;
    int fd;

    uint32_t file_size;
    uint32_t id3v2_bytes;
    uint32_t id3v1_bytes;
    uint32_t audio_data_offset;
    uint32_t audio_data_size;

    uint32_t first_frame_offset;
    uint32_t first_frame_header;
    uint32_t first_frame_size;

    uint32_t sample_rate;
    uint32_t samples_per_frame;
    uint32_t bitrate;
    uint8_t version_id;
    uint8_t channel_mode;
    uint8_t protection_absent;

    uint32_t total_frames;
    uint32_t total_bytes;

    mp3_meta_bitrate_type_t bitrate_type;
} mp3_parser_context_t;

static uint32_t mp3_read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

static int mp3_read_at(int fd, uint32_t offset, uint8_t *buffer, uint32_t size)
{
    int read_size;

    if (buffer == NULL || size == 0)
    {
        return -1;
    }

    if (lseek(fd, (off_t)offset, SEEK_SET) < 0)
    {
        return -1;
    }

    read_size = read(fd, buffer, size);
    if (read_size != (int)size)
    {
        return -1;
    }

    return 0;
}

static uint32_t mp3_get_id3v2_total_size(int fd)
{
    uint8_t header[MP3_ID3V2_HEADER_SIZE];
    uint8_t version;
    uint8_t flags;
    uint32_t tag_size;
    uint32_t total_size;

    if (mp3_read_at(fd, 0, header, sizeof(header)) != 0)
    {
        return 0;
    }

    if (header[0] != 'I' || header[1] != 'D' || header[2] != '3')
    {
        return 0;
    }

    version = header[3];
    flags = header[5];
    if (version < 2U || version > 4U)
    {
        return 0;
    }

    tag_size = metadata_parse_synchsafe_int(&header[6]);
    total_size = MP3_ID3V2_HEADER_SIZE + tag_size;
    if ((flags & 0x10U) != 0U && version == 4U)
    {
        total_size += MP3_ID3V2_FOOTER_SIZE;
    }

    return total_size;
}

static void mp3_update_audio_data_size(mp3_parser_context_t *ctx)
{
    uint32_t trailing_bytes = ctx->id3v1_bytes;

    if (ctx->file_size <= ctx->audio_data_offset)
    {
        ctx->audio_data_size = 0;
        return;
    }

    ctx->audio_data_size = ctx->file_size - ctx->audio_data_offset;
    if (ctx->audio_data_size > trailing_bytes)
    {
        ctx->audio_data_size -= trailing_bytes;
    }
    else
    {
        ctx->audio_data_size = 0;
    }
}

static int mp3_probe_file_layout(mp3_parser_context_t *ctx)
{
    int file_size;

    file_size = lseek(ctx->fd, 0, SEEK_END);
    if (file_size <= 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "MP3 metadata: invalid file size %d\n", file_size);
        return -1;
    }

    ctx->file_size = (uint32_t)file_size;
    ctx->id3v2_bytes = mp3_get_id3v2_total_size(ctx->fd);
    if (ctx->id3v2_bytes > ctx->file_size)
    {
        ctx->id3v2_bytes = 0;
    }

    ctx->audio_data_offset = ctx->id3v2_bytes;
    ctx->id3v1_bytes = 0;
    mp3_update_audio_data_size(ctx);

    BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: file_size=%u, id3v2_bytes=%u\n",
            ctx->file_size, ctx->id3v2_bytes);
    return 0;
}

static int mp3_parse_id3v1(mp3_parser_context_t *ctx)
{
    uint8_t tag_data[MP3_ID3V1_TAG_SIZE];

    if (ctx->file_size < sizeof(tag_data))
    {
        return -1;
    }

    if (mp3_read_at(ctx->fd, ctx->file_size - sizeof(tag_data), tag_data, sizeof(tag_data)) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "MP3 metadata: read ID3v1 failed\n");
        return -1;
    }

    if (tag_data[0] != 'T' || tag_data[1] != 'A' || tag_data[2] != 'G')
    {
        return -1;
    }

    ctx->id3v1_bytes = sizeof(tag_data);
    mp3_update_audio_data_size(ctx);

    if (ctx->metadata->title[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->title, (char *)&tag_data[3], 30);
    }

    if (ctx->metadata->artist[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->artist, (char *)&tag_data[33], 30);
    }

    if (ctx->metadata->album[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->album, (char *)&tag_data[63], 30);
    }

    if (ctx->metadata->year[0] == '\0')
    {
        metadata_safe_string_copy(ctx->metadata->year, (char *)&tag_data[93], 4);
    }

    if (ctx->metadata->genre[0] == '\0' && tag_data[127] < 255)
    {
        os_snprintf(ctx->metadata->genre, AUDIO_METADATA_MAX_STRING_LEN, "%u", tag_data[127]);
    }

    if (ctx->metadata->track_number[0] == '\0' && tag_data[125] == 0 && tag_data[126] != 0)
    {
        os_snprintf(ctx->metadata->track_number, AUDIO_METADATA_MAX_STRING_LEN, "%u", tag_data[126]);
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: ID3v1 parsed\n");
    return 0;
}

static int mp3_parse_frame_header(uint32_t header, mp3_parser_context_t *ctx)
{
    uint8_t version_id;
    uint8_t layer;
    uint8_t bitrate_index;
    uint8_t sample_rate_index;
    uint8_t padding;
    uint32_t sample_rate;
    uint32_t bitrate;
    uint32_t frame_size;
    uint32_t samples_per_frame;
    const uint16_t *bitrate_table;

    if ((header & 0xFFE00000U) != 0xFFE00000U)
    {
        return -1;
    }

    version_id = (uint8_t)((header >> 19) & 0x03U);
    layer = (uint8_t)((header >> 17) & 0x03U);
    bitrate_index = (uint8_t)((header >> 12) & 0x0FU);
    sample_rate_index = (uint8_t)((header >> 10) & 0x03U);
    padding = (uint8_t)((header >> 9) & 0x01U);

    if (version_id == 1U || layer != 1U || bitrate_index == 0U || bitrate_index == 0x0FU || sample_rate_index == 0x03U)
    {
        return -1;
    }

    if (version_id == 3U)
    {
        bitrate_table = mp3_bitrate_table_mpeg1_l3;
        sample_rate = mp3_samplerate_table[0][sample_rate_index];
        samples_per_frame = 1152U;
        frame_size = ((144U * (uint32_t)bitrate_table[bitrate_index] * 1000U) / sample_rate) + padding;
    }
    else
    {
        bitrate_table = mp3_bitrate_table_mpeg2_l3;
        sample_rate = (version_id == 2U) ? mp3_samplerate_table[1][sample_rate_index] :
                                            mp3_samplerate_table[2][sample_rate_index];
        samples_per_frame = 576U;
        frame_size = ((72U * (uint32_t)bitrate_table[bitrate_index] * 1000U) / sample_rate) + padding;
    }

    bitrate = (uint32_t)bitrate_table[bitrate_index] * 1000U;
    if (bitrate == 0U || sample_rate == 0U || frame_size < 4U)
    {
        return -1;
    }

    ctx->version_id = version_id;
    ctx->channel_mode = (uint8_t)((header >> 6) & 0x03U);
    ctx->protection_absent = (uint8_t)((header >> 16) & 0x01U);
    ctx->sample_rate = sample_rate;
    ctx->samples_per_frame = samples_per_frame;
    ctx->bitrate = bitrate;
    ctx->first_frame_size = frame_size;
    return 0;
}

static int mp3_validate_frame_chain(mp3_parser_context_t *ctx, uint32_t frame_offset)
{
    uint8_t probe[MP3_META_FRAME_PROBE_SIZE];
    uint32_t current_offset = frame_offset;
    uint32_t current_frame_size;
    uint32_t audio_end = ctx->audio_data_offset + ctx->audio_data_size;
    uint32_t expected_sample_rate;
    uint32_t validated;

    if (mp3_read_at(ctx->fd, frame_offset, probe, sizeof(probe)) != 0)
    {
        return -1;
    }

    ctx->first_frame_header = mp3_read_be32(probe);
    if (mp3_parse_frame_header(ctx->first_frame_header, ctx) != 0)
    {
        return -1;
    }

    expected_sample_rate = ctx->sample_rate;
    current_frame_size = ctx->first_frame_size;
    validated = 0;
    while (validated < MP3_META_FRAME_CHAIN_COUNT)
    {
        mp3_parser_context_t next = *ctx;
        uint8_t next_header_buf[4];
        uint32_t next_offset;
        uint32_t next_header;

        if (current_frame_size == 0U)
        {
            return -1;
        }

        next_offset = current_offset + current_frame_size;
        if (next_offset + sizeof(next_header_buf) > audio_end)
        {
            break;
        }

        if (mp3_read_at(ctx->fd, next_offset, next_header_buf, sizeof(next_header_buf)) != 0)
        {
            return -1;
        }

        next_header = mp3_read_be32(next_header_buf);
        if (mp3_parse_frame_header(next_header, &next) != 0)
        {
            return -1;
        }

        if (next.sample_rate != expected_sample_rate)
        {
            return -1;
        }

        current_offset = next_offset;
        current_frame_size = next.first_frame_size;
        validated++;
    }

    return 0;
}

static int mp3_find_first_frame(mp3_parser_context_t *ctx)
{
    uint8_t *buffer;
    uint32_t scan_offset;
    uint32_t audio_end;
    int ret = -1;

    if (ctx->audio_data_size < 4U)
    {
        return -1;
    }

    buffer = (uint8_t *)os_malloc(MP3_META_SCAN_BUFFER_SIZE);
    if (buffer == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "MP3 metadata: frame scan buffer alloc failed\n");
        return -1;
    }

    scan_offset = ctx->audio_data_offset;
    audio_end = ctx->audio_data_offset + ctx->audio_data_size;

    while (scan_offset < audio_end)
    {
        uint32_t remaining = audio_end - scan_offset;
        uint32_t chunk_size = (remaining > MP3_META_SCAN_BUFFER_SIZE) ? MP3_META_SCAN_BUFFER_SIZE : remaining;
        uint32_t i;

        if (chunk_size < 4U)
        {
            break;
        }

        if (mp3_read_at(ctx->fd, scan_offset, buffer, chunk_size) != 0)
        {
            break;
        }

        for (i = 0; i + 4U <= chunk_size; ++i)
        {
            if (buffer[i] == 0xFFU && (buffer[i + 1] & 0xE0U) == 0xE0U)
            {
                uint32_t frame_offset = scan_offset + i;
                if (mp3_validate_frame_chain(ctx, frame_offset) == 0)
                {
                    ctx->first_frame_offset = frame_offset;
                    BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: first frame offset %u\n", frame_offset);
                    ret = 0;
                    goto exit;
                }
            }
        }

        if (chunk_size <= 4U)
        {
            break;
        }
        scan_offset += chunk_size - 3U;
    }

exit:
    if (ret != 0)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "MP3 metadata: unable to detect bitrate from frame\n");
    }
    os_free(buffer);
    return ret;
}

static int mp3_parse_xing_info_header(mp3_parser_context_t *ctx)
{
    uint8_t frame_buf[156];
    uint32_t side_info_size;
    uint32_t xing_offset;
    const uint8_t *marker;
    uint32_t flags;
    uint32_t offset;

    if (ctx->first_frame_offset == 0U && ctx->audio_data_offset != 0U)
    {
        return -1;
    }

    if (mp3_read_at(ctx->fd, ctx->first_frame_offset, frame_buf, sizeof(frame_buf)) != 0)
    {
        return -1;
    }

    if (ctx->version_id == 3U)
    {
        side_info_size = (ctx->channel_mode == 3U) ? 17U : 32U;
    }
    else
    {
        side_info_size = (ctx->channel_mode == 3U) ? 9U : 17U;
    }

    xing_offset = 4U + (ctx->protection_absent ? 0U : 2U) + side_info_size;
    if (xing_offset + 8U > sizeof(frame_buf))
    {
        return -1;
    }

    marker = frame_buf + xing_offset;
    if (os_memcmp(marker, "Xing", 4) == 0 || os_memcmp(marker, "xing", 4) == 0)
    {
        ctx->bitrate_type = MP3_META_BITRATE_VBR_XING;
    }
    else if (os_memcmp(marker, "Info", 4) == 0 || os_memcmp(marker, "info", 4) == 0)
    {
        ctx->bitrate_type = MP3_META_BITRATE_CBR_INFO;
    }
    else
    {
        return -1;
    }

    flags = mp3_read_be32(marker + 4);
    offset = xing_offset + 8U;

    if ((flags & 0x01U) != 0U)
    {
        if (offset + 4U > sizeof(frame_buf))
        {
            return -1;
        }
        ctx->total_frames = mp3_read_be32(frame_buf + offset);
        offset += 4U;
    }

    if ((flags & 0x02U) != 0U)
    {
        if (offset + 4U > sizeof(frame_buf))
        {
            return -1;
        }
        ctx->total_bytes = mp3_read_be32(frame_buf + offset);
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: detected %s, frames=%u, bytes=%u\n",
            (ctx->bitrate_type == MP3_META_BITRATE_CBR_INFO) ? "Info" : "Xing",
            ctx->total_frames, ctx->total_bytes);
    return 0;
}

static int mp3_parse_vbri_header(mp3_parser_context_t *ctx)
{
    uint8_t frame_buf[64];
    const uint8_t *vbri;

    if (mp3_read_at(ctx->fd, ctx->first_frame_offset, frame_buf, sizeof(frame_buf)) != 0)
    {
        return -1;
    }

    if (sizeof(frame_buf) < 36U + 18U)
    {
        return -1;
    }

    vbri = frame_buf + 36U;
    if (os_memcmp(vbri, "VBRI", 4) != 0 && os_memcmp(vbri, "vbri", 4) != 0)
    {
        return -1;
    }

    ctx->bitrate_type = MP3_META_BITRATE_VBR_VBRI;
    ctx->total_bytes = mp3_read_be32(vbri + 10U);
    ctx->total_frames = mp3_read_be32(vbri + 14U);

    BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: detected VBRI, frames=%u, bytes=%u\n",
            ctx->total_frames, ctx->total_bytes);
    return 0;
}

/* Report the average bitrate of a VBR stream; fall back to the first-frame value. */
static void mp3_set_vbr_average_bitrate(mp3_parser_context_t *ctx, uint64_t duration_ms)
{
    uint32_t stream_bytes = (ctx->total_bytes > 0U) ? ctx->total_bytes : ctx->audio_data_size;

    if (duration_ms > 0ULL && stream_bytes > 0U)
    {
        ctx->metadata->bitrate = (int)((uint64_t)stream_bytes * 8ULL * 1000ULL / duration_ms);
    }
    else
    {
        ctx->metadata->bitrate = (int)ctx->bitrate;
    }
}

static int mp3_calculate_duration_from_headers(mp3_parser_context_t *ctx)
{
    uint64_t duration_ms;

    if ((ctx->bitrate_type == MP3_META_BITRATE_VBR_XING ||
         ctx->bitrate_type == MP3_META_BITRATE_VBR_VBRI) &&
        ctx->total_frames > 0U &&
        ctx->samples_per_frame > 0U &&
        ctx->sample_rate > 0U)
    {
        duration_ms = (uint64_t)ctx->total_frames * (uint64_t)ctx->samples_per_frame * 1000ULL;
        duration_ms /= (uint64_t)ctx->sample_rate;
        ctx->metadata->duration = (double)duration_ms;
        mp3_set_vbr_average_bitrate(ctx, duration_ms);
        return 0;
    }

    if ((ctx->bitrate_type == MP3_META_BITRATE_CBR ||
         ctx->bitrate_type == MP3_META_BITRATE_CBR_INFO) &&
        ctx->audio_data_size > 0U &&
        ctx->bitrate > 0U)
    {
        duration_ms = (uint64_t)ctx->audio_data_size * 8ULL * 1000ULL;
        duration_ms /= (uint64_t)ctx->bitrate;
        ctx->metadata->duration = (double)duration_ms;
        ctx->metadata->bitrate = (int)ctx->bitrate;
        return 0;
    }

    return -1;
}

static int mp3_calculate_duration_from_bitrate(mp3_parser_context_t *ctx)
{
    uint64_t duration_ms;

    if (ctx->bitrate == 0U)
    {
        ctx->bitrate = 128000U;
    }

    if (ctx->audio_data_size == 0U)
    {
        return -1;
    }

    duration_ms = (uint64_t)ctx->audio_data_size * 8ULL * 1000ULL;
    duration_ms /= (uint64_t)ctx->bitrate;
    ctx->metadata->duration = (double)duration_ms;
    ctx->metadata->bitrate = (int)ctx->bitrate;
    return 0;
}

/* Fill in the bitrate field without overriding an already-known duration
 * (e.g. when duration was provided by an ID3v2 TLEN frame). */
static void mp3_fill_bitrate_only(mp3_parser_context_t *ctx)
{
    if (ctx->metadata->bitrate > 0)
    {
        return;
    }

    if ((ctx->bitrate_type == MP3_META_BITRATE_VBR_XING ||
         ctx->bitrate_type == MP3_META_BITRATE_VBR_VBRI) &&
        ctx->metadata->duration > 0.0)
    {
        mp3_set_vbr_average_bitrate(ctx, (uint64_t)ctx->metadata->duration);
        return;
    }

    if (ctx->bitrate > 0U)
    {
        ctx->metadata->bitrate = (int)ctx->bitrate;
    }
}

static int mp3_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    mp3_parser_context_t ctx;

    os_memset(&ctx, 0, sizeof(ctx));
    ctx.metadata = metadata;
    ctx.fd = fd;

    if (metadata_parse_id3v2(fd, metadata) == AUDIO_PLAYER_OK)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: ID3v2 duration %u ms\n", (uint32_t)metadata->duration);
    }

    if (mp3_probe_file_layout(&ctx) != 0)
    {
        metadata_fill_title_from_path(filepath, metadata);
        return AUDIO_PLAYER_OK;
    }

    if (mp3_parse_id3v1(&ctx) == 0)
    {
        metadata->has_id3v1 = 1;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "MP3 metadata: audio_data_offset=%u, audio_data_size=%u, id3v1_bytes=%u\n",
            ctx.audio_data_offset, ctx.audio_data_size, ctx.id3v1_bytes);

    if (mp3_find_first_frame(&ctx) == 0)
    {
        ctx.bitrate_type = MP3_META_BITRATE_CBR;
        if (mp3_parse_xing_info_header(&ctx) != 0)
        {
            (void)mp3_parse_vbri_header(&ctx);
        }

        if (metadata->duration == 0.0)
        {
            if (mp3_calculate_duration_from_headers(&ctx) != 0)
            {
                (void)mp3_calculate_duration_from_bitrate(&ctx);
            }
        }
        else
        {
            /* Duration already known (ID3v2 TLEN); only fill the bitrate field. */
            mp3_fill_bitrate_only(&ctx);
        }
    }
    else if (metadata->duration == 0.0)
    {
        (void)mp3_calculate_duration_from_bitrate(&ctx);
    }

    if (metadata->duration > 0.0)
    {
        BK_LOGI(AUDIO_PLAYER_TAG,
                "MP3 metadata: bitrate_type=%d, bitrate=%d, sample_rate=%u, samples_per_frame=%u, duration=%u ms\n",
                ctx.bitrate_type, metadata->bitrate, ctx.sample_rate,
                ctx.samples_per_frame, (uint32_t)metadata->duration);
    }

    metadata_fill_title_from_path(filepath, metadata);
    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t mp3_metadata_parser_ops = {
    .name = "mp3",
    .format = AUDIO_FORMAT_MP3,
    .probe = NULL,
    .parse = mp3_metadata_parse,
};

const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_mp3_metadata_parser_ops(void)
{
    return &mp3_metadata_parser_ops;
}
