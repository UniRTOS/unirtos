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

#include <bk_posix.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define M4A_MKTAG(a, b, c, d)  (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))

#define ATOM_FTYP   M4A_MKTAG('f','t','y','p')
#define ATOM_MOOV   M4A_MKTAG('m','o','o','v')
#define ATOM_MVHD   M4A_MKTAG('m','v','h','d')
#define ATOM_TRAK   M4A_MKTAG('t','r','a','k')
#define ATOM_MDIA   M4A_MKTAG('m','d','i','a')
#define ATOM_MDHD   M4A_MKTAG('m','d','h','d')
#define ATOM_HDLR   M4A_MKTAG('h','d','l','r')
#define ATOM_MINF   M4A_MKTAG('m','i','n','f')
#define ATOM_STBL   M4A_MKTAG('s','t','b','l')
#define ATOM_STSD   M4A_MKTAG('s','t','s','d')
#define ATOM_UDTA   M4A_MKTAG('u','d','t','a')
#define ATOM_META   M4A_MKTAG('m','e','t','a')
#define ATOM_ILST   M4A_MKTAG('i','l','s','t')
#define ATOM_DATA   M4A_MKTAG('d','a','t','a')

#define ATOM_TITLE      M4A_MKTAG(0xA9,'n','a','m')
#define ATOM_ARTIST     M4A_MKTAG(0xA9,'A','R','T')
#define ATOM_ALBUM      M4A_MKTAG(0xA9,'a','l','b')
#define ATOM_ALBUM_ART  M4A_MKTAG('a','A','R','T')
#define ATOM_YEAR       M4A_MKTAG(0xA9,'d','a','y')
#define ATOM_COMPOSER   M4A_MKTAG(0xA9,'w','r','t')
#define ATOM_GENRE_STR  M4A_MKTAG(0xA9,'g','e','n')
#define ATOM_GENRE_NUM  M4A_MKTAG('g','n','r','e')
#define ATOM_TRACK      M4A_MKTAG('t','r','k','n')

#define M4A_MAX_METADATA_PAYLOAD   (256 * 1024)

typedef struct
{
    uint32_t type;
    uint64_t size;
    uint64_t start;
    uint64_t data_offset;
    uint64_t end;
} mp4_atom_t;

typedef struct
{
    int fd;
    audio_metadata_t *metadata;
    uint64_t file_size;

    uint32_t movie_time_scale;
    uint64_t movie_duration;

    uint32_t audio_time_scale;
    uint64_t audio_duration;
    uint32_t audio_sample_rate;
    uint16_t audio_channels;
} m4a_parser_ctx_t;

static uint32_t read_be32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
            (uint32_t)buf[3];
}

static uint16_t read_be16(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) |
            (uint16_t)buf[1];
}

static uint64_t read_be64(const uint8_t *buf)
{
    uint64_t high = read_be32(buf);
    uint64_t low = read_be32(buf + 4);
    return (high << 32) | low;
}

static int read_full(int fd, void *buffer, size_t size)
{
    uint8_t *ptr = (uint8_t *)buffer;
    size_t read_total = 0;

    while (read_total < size)
    {
        ssize_t bytes = read(fd, ptr + read_total, size - read_total);
        if (bytes <= 0)
        {
            return -1;
        }
        read_total += (size_t)bytes;
    }
    return 0;
}

static int mp4_read_atom(m4a_parser_ctx_t *ctx, mp4_atom_t *atom)
{
    uint8_t header[8];
    uint64_t start = lseek(ctx->fd, 0, SEEK_CUR);
    uint32_t size32;
    uint32_t type;
    uint64_t size = 0;
    uint32_t header_size = 8;

    if (start == (uint64_t)-1)
    {
        return -1;
    }

    if (read_full(ctx->fd, header, sizeof(header)) != 0)
    {
        return -1;
    }

    size32 = read_be32(header);
    type = read_be32(header + 4);

    if (size32 == 1)
    {
        uint8_t size_buf[8];
        if (read_full(ctx->fd, size_buf, sizeof(size_buf)) != 0)
        {
            return -1;
        }
        size = read_be64(size_buf);
        header_size += 8;
    }
    else if (size32 == 0)
    {
        if (ctx->file_size <= start)
        {
            return -1;
        }
        size = ctx->file_size - start;
    }
    else
    {
        size = size32;
    }

    if (size < header_size)
    {
        return -1;
    }

    atom->type = type;
    atom->size = size;
    atom->start = start;
    atom->data_offset = start + header_size;
    atom->end = start + size;

    if (lseek(ctx->fd, atom->data_offset, SEEK_SET) < 0)
    {
        return -1;
    }

    return 0;
}

static int mp4_skip_to_atom_end(m4a_parser_ctx_t *ctx, const mp4_atom_t *atom)
{
    if (!atom)
    {
        return -1;
    }
    if (lseek(ctx->fd, (off_t)atom->end, SEEK_SET) < 0)
    {
        return -1;
    }
    return 0;
}

static double calc_duration_ms(uint64_t duration, uint32_t time_scale)
{
    if (duration == 0 || time_scale == 0)
    {
        return 0.0;
    }
    return ((double)duration * 1000.0) / (double)time_scale;
}

static void save_string_field(char *dst, const uint8_t *payload, uint32_t size)
{
    char temp[AUDIO_METADATA_MAX_STRING_LEN];

    if (!dst || !payload || size == 0)
    {
        return;
    }

    if (size >= sizeof(temp))
    {
        size = sizeof(temp) - 1;
    }

    os_memcpy(temp, payload, size);
    temp[size] = '\0';
    metadata_safe_string_copy(dst, temp, AUDIO_METADATA_MAX_STRING_LEN);
}

static void save_utf16_string(char *dst, const uint8_t *payload, uint32_t size)
{
    char temp[AUDIO_METADATA_MAX_STRING_LEN];
    uint32_t out = 0;
    uint32_t i;

    if (!dst || !payload || size < 2)
    {
        return;
    }

    for (i = 0; i + 1 < size && out < (sizeof(temp) - 1); i += 2)
    {
        temp[out++] = payload[i + 1];
    }
    temp[out] = '\0';
    metadata_safe_string_copy(dst, temp, AUDIO_METADATA_MAX_STRING_LEN);
}

static void parse_track_number(audio_metadata_t *metadata, const uint8_t *payload, uint32_t size)
{
    uint16_t track = 0;
    uint16_t total = 0;

    if (!payload || size < 2)
    {
        return;
    }

    if (size >= 6)
    {
        track = (payload[2] << 8) | payload[3];
        total = (payload[4] << 8) | payload[5];
    }
    else if (size >= 2)
    {
        track = (payload[size - 2] << 8) | payload[size - 1];
    }

    if (track == 0)
    {
        return;
    }

    if (total > 0)
    {
        os_snprintf(metadata->track_number, AUDIO_METADATA_MAX_STRING_LEN, "%u/%u", track, total);
    }
    else
    {
        os_snprintf(metadata->track_number, AUDIO_METADATA_MAX_STRING_LEN, "%u", track);
    }
}

static void parse_genre(audio_metadata_t *metadata, uint32_t type, const uint8_t *payload, uint32_t size)
{
    if (!payload || size == 0)
    {
        return;
    }

    if (type == ATOM_GENRE_NUM && size >= 2)
    {
        uint16_t genre_idx = (payload[size - 2] << 8) | payload[size - 1];
        if (genre_idx > 0)
        {
            os_snprintf(metadata->genre, AUDIO_METADATA_MAX_STRING_LEN, "%u", genre_idx);
        }
    }
    else
    {
        save_string_field(metadata->genre, payload, size);
    }
}

static void process_metadata_item(audio_metadata_t *metadata, uint32_t tag, const uint8_t *payload, uint32_t size, uint32_t data_type)
{
    if (!metadata || !payload || size == 0)
    {
        return;
    }

    switch (tag)
    {
        case ATOM_TITLE:
            if (metadata->title[0] == '\0')
            {
                if (data_type == 1)
                {
                    save_string_field(metadata->title, payload, size);
                }
                else if (data_type == 2)
                {
                    save_utf16_string(metadata->title, payload, size);
                }
            }
            break;

        case ATOM_ARTIST:
            if (metadata->artist[0] == '\0')
            {
                if (data_type == 1)
                {
                    save_string_field(metadata->artist, payload, size);
                }
                else if (data_type == 2)
                {
                    save_utf16_string(metadata->artist, payload, size);
                }
            }
            break;

        case ATOM_ALBUM:
            if (metadata->album[0] == '\0')
            {
                if (data_type == 1)
                {
                    save_string_field(metadata->album, payload, size);
                }
                else if (data_type == 2)
                {
                    save_utf16_string(metadata->album, payload, size);
                }
            }
            break;

        case ATOM_ALBUM_ART:
            if (metadata->album_artist[0] == '\0')
            {
                if (data_type == 1)
                {
                    save_string_field(metadata->album_artist, payload, size);
                }
                else if (data_type == 2)
                {
                    save_utf16_string(metadata->album_artist, payload, size);
                }
            }
            break;

        case ATOM_YEAR:
            if (metadata->year[0] == '\0')
            {
                if (data_type == 1)
                {
                    save_string_field(metadata->year, payload, size);
                }
                else if (data_type == 2)
                {
                    save_utf16_string(metadata->year, payload, size);
                }
            }
            break;

        case ATOM_COMPOSER:
            if (metadata->composer[0] == '\0')
            {
                if (data_type == 1)
                {
                    save_string_field(metadata->composer, payload, size);
                }
                else if (data_type == 2)
                {
                    save_utf16_string(metadata->composer, payload, size);
                }
            }
            break;

        case ATOM_TRACK:
            if (metadata->track_number[0] == '\0')
            {
                parse_track_number(metadata, payload, size);
            }
            break;

        case ATOM_GENRE_STR:
        case ATOM_GENRE_NUM:
            if (metadata->genre[0] == '\0')
            {
                parse_genre(metadata, tag, payload, size);
            }
            break;

        default:
            break;
    }
}

static void parse_ilst(m4a_parser_ctx_t *ctx, const mp4_atom_t *ilst_atom)
{
    mp4_atom_t item;

    if (!ilst_atom)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)ilst_atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)ilst_atom->end)
    {
        if (mp4_read_atom(ctx, &item) != 0 || item.size < 8)
        {
            break;
        }

        uint64_t item_end = item.end;

        while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)item_end)
        {
            mp4_atom_t child;
            if (mp4_read_atom(ctx, &child) != 0)
            {
                break;
            }

            if (child.type == ATOM_DATA && child.size > 16)
            {
                uint32_t payload_size = (uint32_t)(child.size - (child.data_offset - child.start) - 8);
                uint8_t header[8];
                uint8_t *payload = NULL;
                uint32_t data_type;

                if (payload_size > M4A_MAX_METADATA_PAYLOAD)
                {
                    mp4_skip_to_atom_end(ctx, &child);
                    break;
                }

                if (read_full(ctx->fd, header, sizeof(header)) != 0)
                {
                    mp4_skip_to_atom_end(ctx, &child);
                    break;
                }

                data_type = read_be32(header);
                payload_size = (uint32_t)(child.end - lseek(ctx->fd, 0, SEEK_CUR));

                if (payload_size == 0 || payload_size > M4A_MAX_METADATA_PAYLOAD)
                {
                    mp4_skip_to_atom_end(ctx, &child);
                    break;
                }

                payload = (uint8_t *)player_malloc(payload_size);
                if (!payload)
                {
                    mp4_skip_to_atom_end(ctx, &child);
                    break;
                }

                if (read_full(ctx->fd, payload, payload_size) != 0)
                {
                    player_free(payload);
                    mp4_skip_to_atom_end(ctx, &child);
                    break;
                }

                process_metadata_item(ctx->metadata, item.type, payload, payload_size, data_type);
                player_free(payload);
            }

            mp4_skip_to_atom_end(ctx, &child);
        }

        mp4_skip_to_atom_end(ctx, &item);
    }
}

static void parse_meta(m4a_parser_ctx_t *ctx, const mp4_atom_t *meta_atom)
{
    mp4_atom_t child;
    uint8_t version_flags[4];

    if (!meta_atom || meta_atom->size <= 4)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)meta_atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    if (read_full(ctx->fd, version_flags, sizeof(version_flags)) != 0)
    {
        return;
    }

    while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)meta_atom->end)
    {
        if (mp4_read_atom(ctx, &child) != 0)
        {
            break;
        }

        if (child.type == ATOM_ILST)
        {
            parse_ilst(ctx, &child);
        }

        mp4_skip_to_atom_end(ctx, &child);
    }
}

static void parse_udta(m4a_parser_ctx_t *ctx, const mp4_atom_t *udta_atom)
{
    mp4_atom_t child;

    if (!udta_atom)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)udta_atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)udta_atom->end)
    {
        if (mp4_read_atom(ctx, &child) != 0)
        {
            break;
        }

        if (child.type == ATOM_META)
        {
            parse_meta(ctx, &child);
        }

        mp4_skip_to_atom_end(ctx, &child);
    }
}

static void parse_mvhd(m4a_parser_ctx_t *ctx, const mp4_atom_t *atom)
{
    uint8_t header[12];
    uint8_t version;

    if (!atom)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    if (read_full(ctx->fd, &version, 1) != 0)
    {
        return;
    }

    if (read_full(ctx->fd, header, 3) != 0)
    {
        return;
    }

    if (version == 1)
    {
        /* version 1: creation(8) + modification(8) + timescale(4) + duration(8) */
        uint8_t buffer[28];
        if (read_full(ctx->fd, buffer, sizeof(buffer)) != 0)
        {
            return;
        }
        ctx->movie_time_scale = read_be32(buffer + 16);
        ctx->movie_duration = read_be64(buffer + 20);
    }
    else
    {
        /* version 0: creation(4) + modification(4) + timescale(4) + duration(4) */
        uint8_t buffer[16];
        if (read_full(ctx->fd, buffer, sizeof(buffer)) != 0)
        {
            return;
        }
        ctx->movie_time_scale = read_be32(buffer + 8);
        ctx->movie_duration = read_be32(buffer + 12);
    }
}

static void parse_mdhd(m4a_parser_ctx_t *ctx, const mp4_atom_t *atom, bool *has_audio_time)
{
    uint8_t header[12];
    uint8_t version;

    if (!atom || !has_audio_time)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    if (read_full(ctx->fd, &version, 1) != 0)
    {
        return;
    }

    if (read_full(ctx->fd, header, 3) != 0)
    {
        return;
    }

    if (version == 1)
    {
        /* version 1: creation(8) + modification(8) + timescale(4) + duration(8) */
        uint8_t buffer[28];
        if (read_full(ctx->fd, buffer, sizeof(buffer)) != 0)
        {
            return;
        }
        ctx->audio_time_scale = read_be32(buffer + 16);
        ctx->audio_duration = read_be64(buffer + 20);
    }
    else
    {
        /* version 0: creation(4) + modification(4) + timescale(4) + duration(4) */
        uint8_t buffer[16];
        if (read_full(ctx->fd, buffer, sizeof(buffer)) != 0)
        {
            return;
        }
        ctx->audio_time_scale = read_be32(buffer + 8);
        ctx->audio_duration = read_be32(buffer + 12);
    }

    *has_audio_time = true;
}

static bool parse_hdlr_is_audio(m4a_parser_ctx_t *ctx, const mp4_atom_t *atom)
{
    uint8_t buffer[20];

    if (!atom)
    {
        return false;
    }

    if (atom->size < 24)
    {
        return false;
    }

    if (lseek(ctx->fd, (off_t)atom->data_offset, SEEK_SET) < 0)
    {
        return false;
    }

    if (read_full(ctx->fd, buffer, sizeof(buffer)) != 0)
    {
        return false;
    }

    uint32_t handler_type = read_be32(buffer + 8);
    return (handler_type == M4A_MKTAG('s','o','u','n'));
}

static void parse_stsd(m4a_parser_ctx_t *ctx, const mp4_atom_t *atom)
{
    uint8_t header[8];
    uint32_t entry_count;

    if (!atom || atom->size < 16)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    if (read_full(ctx->fd, header, sizeof(header)) != 0)
    {
        return;
    }

    entry_count = read_be32(header + 4);
    if (entry_count == 0)
    {
        return;
    }

    mp4_atom_t entry;
    if (mp4_read_atom(ctx, &entry) != 0)
    {
        return;
    }

    if (entry.size < 36)
    {
        mp4_skip_to_atom_end(ctx, &entry);
        return;
    }

    uint8_t reserved[6];
    uint8_t sample_entry[20];

    if (read_full(ctx->fd, reserved, sizeof(reserved)) != 0)
    {
        mp4_skip_to_atom_end(ctx, &entry);
        return;
    }

    if (read_full(ctx->fd, sample_entry, sizeof(sample_entry)) != 0)
    {
        mp4_skip_to_atom_end(ctx, &entry);
        return;
    }

    ctx->audio_channels = read_be16(sample_entry + 8);
    ctx->audio_sample_rate = read_be32(sample_entry + 16) / 65536;

    mp4_skip_to_atom_end(ctx, &entry);
}

static void parse_minf(m4a_parser_ctx_t *ctx, const mp4_atom_t *minf_atom)
{
    mp4_atom_t child;

    if (!minf_atom)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)minf_atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)minf_atom->end)
    {
        if (mp4_read_atom(ctx, &child) != 0)
        {
            break;
        }

        if (child.type == ATOM_STBL)
        {
            mp4_atom_t stbl_child;
            if (lseek(ctx->fd, (off_t)child.data_offset, SEEK_SET) < 0)
            {
                mp4_skip_to_atom_end(ctx, &child);
                break;
            }

            while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)child.end)
            {
                if (mp4_read_atom(ctx, &stbl_child) != 0)
                {
                    break;
                }

                if (stbl_child.type == ATOM_STSD)
                {
                    parse_stsd(ctx, &stbl_child);
                }

                mp4_skip_to_atom_end(ctx, &stbl_child);
            }
        }

        mp4_skip_to_atom_end(ctx, &child);
    }
}

static void parse_mdia(m4a_parser_ctx_t *ctx, const mp4_atom_t *mdia_atom)
{
    mp4_atom_t child;
    bool is_audio_track = false;
    bool has_audio_time = false;

    if (!mdia_atom)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)mdia_atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)mdia_atom->end)
    {
        if (mp4_read_atom(ctx, &child) != 0)
        {
            break;
        }

        switch (child.type)
        {
            case ATOM_HDLR:
                is_audio_track = parse_hdlr_is_audio(ctx, &child);
                break;
            case ATOM_MDHD:
                if (!has_audio_time)
                {
                    parse_mdhd(ctx, &child, &has_audio_time);
                }
                break;
            case ATOM_MINF:
                if (is_audio_track)
                {
                    parse_minf(ctx, &child);
                }
                break;
            default:
                break;
        }

        mp4_skip_to_atom_end(ctx, &child);
    }
}

static void parse_trak(m4a_parser_ctx_t *ctx, const mp4_atom_t *trak_atom)
{
    mp4_atom_t child;

    if (!trak_atom)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)trak_atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)trak_atom->end)
    {
        if (mp4_read_atom(ctx, &child) != 0)
        {
            break;
        }

        if (child.type == ATOM_MDIA)
        {
            parse_mdia(ctx, &child);
        }

        mp4_skip_to_atom_end(ctx, &child);
    }
}

static void parse_moov(m4a_parser_ctx_t *ctx, const mp4_atom_t *moov_atom)
{
    mp4_atom_t child;

    if (!moov_atom)
    {
        return;
    }

    if (lseek(ctx->fd, (off_t)moov_atom->data_offset, SEEK_SET) < 0)
    {
        return;
    }

    while (lseek(ctx->fd, 0, SEEK_CUR) < (off_t)moov_atom->end)
    {
        if (mp4_read_atom(ctx, &child) != 0)
        {
            break;
        }

        switch (child.type)
        {
            case ATOM_MVHD:
                parse_mvhd(ctx, &child);
                break;
            case ATOM_TRAK:
                parse_trak(ctx, &child);
                break;
            case ATOM_UDTA:
                parse_udta(ctx, &child);
                break;
            default:
                break;
        }

        mp4_skip_to_atom_end(ctx, &child);
    }
}

static void finalize_metadata(m4a_parser_ctx_t *ctx, const char *filepath)
{
    if (ctx->audio_time_scale > 0 && ctx->audio_duration > 0)
    {
        ctx->metadata->duration = calc_duration_ms(ctx->audio_duration, ctx->audio_time_scale);
    }
    else if (ctx->movie_time_scale > 0 && ctx->movie_duration > 0)
    {
        ctx->metadata->duration = calc_duration_ms(ctx->movie_duration, ctx->movie_time_scale);
    }

    if (ctx->audio_sample_rate > 0)
    {
        ctx->metadata->sample_rate = (int)ctx->audio_sample_rate;
    }
    if (ctx->audio_channels > 0)
    {
        ctx->metadata->channels = ctx->audio_channels;
    }

    if (ctx->metadata->bitrate == 0 && ctx->metadata->duration > 0.0 && ctx->file_size > 0)
    {
        double duration_sec = ctx->metadata->duration / 1000.0;
        if (duration_sec > 0.0)
        {
            ctx->metadata->bitrate = (int)((double)ctx->file_size * 8.0 / duration_sec);
        }
    }

    metadata_fill_title_from_path(filepath, ctx->metadata);
}

static int m4a_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    mp4_atom_t atom;
    m4a_parser_ctx_t ctx = {0};

    ctx.fd = fd;
    ctx.metadata = metadata;
    ctx.file_size = lseek(fd, 0, SEEK_END);

    if (ctx.file_size == (uint64_t)-1 || ctx.file_size < 16)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    while (lseek(fd, 0, SEEK_CUR) < (off_t)ctx.file_size)
    {
        if (mp4_read_atom(&ctx, &atom) != 0)
        {
            break;
        }

        if (atom.type == ATOM_MOOV)
        {
            parse_moov(&ctx, &atom);
        }

        mp4_skip_to_atom_end(&ctx, &atom);
    }

    finalize_metadata(&ctx, filepath);
    return AUDIO_PLAYER_OK;
}

static int m4a_metadata_probe(const char *filepath)
{
    const char *ext;

    if (!filepath)
    {
        return -1;
    }

    ext = os_strrchr(filepath, '.');
    if (!ext)
    {
        return -1;
    }

    if (os_strcasecmp(ext, ".m4a") == 0 || os_strcasecmp(ext, ".mp4") == 0 || os_strcasecmp(ext, ".m4b") == 0)
    {
        return 0;
    }

    return -1;
}

const bk_audio_player_metadata_parser_ops_t m4a_metadata_parser_ops =
{
    .name = "m4a",
    .format = AUDIO_FORMAT_M4A,
    .probe = m4a_metadata_probe,
    .parse = m4a_metadata_parse,
};

/* Get M4A metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_m4a_metadata_parser_ops(void)
{
    return &m4a_metadata_parser_ops;
}

