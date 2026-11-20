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

//#include "audio_metadata_parser.h"
#include "audio_metadata_parser_common.h"
#include "player_osal.h"
//#include "play_manager.h"

#include <modules/ogg.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>
#include <unistd.h>
#include <bk_posix.h>

#define OPUS_METADATA_READ_CHUNK        4096
#define OPUS_GRANULE_RATE               48000U

typedef struct opus_metadata_ctx_s
{
    int fd;
    audio_metadata_t *metadata;
    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    bool stream_initialized;
    bool head_parsed;
    bool tags_parsed;
    uint32_t input_sample_rate;
    uint32_t pre_skip;
    uint8_t channels;
    ogg_int64_t last_granule;
    uint64_t file_size_bytes;
} opus_metadata_ctx_t;

static uint32_t opus_parser_read_le32(const uint8_t *data)
{
    return (uint32_t)data[0]
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

static int opus_metadata_read_chunk(opus_metadata_ctx_t *ctx)
{
    char *buffer = ogg_sync_buffer(&ctx->oy, OPUS_METADATA_READ_CHUNK);
    if (!buffer)
    {
        return AUDIO_PLAYER_ERR;
    }

    int bytes = read(ctx->fd, buffer, OPUS_METADATA_READ_CHUNK);
    if (bytes < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus metadata: read failed (%d)", bytes);
        return AUDIO_PLAYER_ERR;
    }

    ogg_sync_wrote(&ctx->oy, bytes);
    return bytes;
}

static void opus_metadata_apply_comment(audio_metadata_t *metadata,
                                        const char *key,
                                        const char *value)
{
    if (!metadata || !key || !value || value[0] == '\0')
    {
        return;
    }

    if (os_strcasecmp(key, "TITLE") == 0 && metadata->title[0] == '\0')
    {
        metadata_safe_string_copy(metadata->title, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
    if (os_strcasecmp(key, "ARTIST") == 0 && metadata->artist[0] == '\0')
    {
        metadata_safe_string_copy(metadata->artist, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
    if (os_strcasecmp(key, "ALBUM") == 0 && metadata->album[0] == '\0')
    {
        metadata_safe_string_copy(metadata->album, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
    if ((os_strcasecmp(key, "ALBUMARTIST") == 0 ||
         os_strcasecmp(key, "ALBUM_ARTIST") == 0) &&
        metadata->album_artist[0] == '\0')
    {
        metadata_safe_string_copy(metadata->album_artist, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
    if (os_strcasecmp(key, "GENRE") == 0 && metadata->genre[0] == '\0')
    {
        metadata_safe_string_copy(metadata->genre, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
    if ((os_strcasecmp(key, "DATE") == 0 || os_strcasecmp(key, "YEAR") == 0) &&
        metadata->year[0] == '\0')
    {
        metadata_safe_string_copy(metadata->year, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
    if (os_strcasecmp(key, "TRACKNUMBER") == 0 && metadata->track_number[0] == '\0')
    {
        metadata_safe_string_copy(metadata->track_number, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
    if (os_strcasecmp(key, "COMPOSER") == 0 && metadata->composer[0] == '\0')
    {
        metadata_safe_string_copy(metadata->composer, value, AUDIO_METADATA_MAX_STRING_LEN);
        return;
    }
}

static int opus_metadata_parse_head(opus_metadata_ctx_t *ctx, const uint8_t *payload, uint32_t size)
{
    if (size < 19 || os_memcmp(payload, "OpusHead", 8) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus metadata: invalid OpusHead");
        return AUDIO_PLAYER_ERR;
    }

    ctx->channels = payload[9];
    ctx->pre_skip = (uint32_t)payload[10] | ((uint32_t)payload[11] << 8);
    ctx->input_sample_rate = opus_parser_read_le32(&payload[12]);
    if (ctx->input_sample_rate == 0)
    {
        ctx->input_sample_rate = OPUS_GRANULE_RATE;
    }

    ctx->metadata->channels = ctx->channels;
    ctx->metadata->sample_rate = (int)ctx->input_sample_rate;
    ctx->head_parsed = true;
    return AUDIO_PLAYER_OK;
}

static int opus_metadata_parse_tags(opus_metadata_ctx_t *ctx, const uint8_t *payload, uint32_t size)
{
    uint32_t cursor = 0;
    uint32_t vendor_len;
    uint32_t comment_count;
    uint32_t i;

    if (size < 8 || os_memcmp(payload, "OpusTags", 8) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus metadata: invalid OpusTags");
        return AUDIO_PLAYER_ERR;
    }

    cursor = 8;
    if (size < cursor + 4)
    {
        return AUDIO_PLAYER_ERR;
    }

    vendor_len = opus_parser_read_le32(payload + cursor);
    cursor += 4;
    if (cursor + vendor_len > size)
    {
        return AUDIO_PLAYER_ERR;
    }

    cursor += vendor_len;
    if (cursor + 4 > size)
    {
        return AUDIO_PLAYER_ERR;
    }

    comment_count = opus_parser_read_le32(payload + cursor);
    cursor += 4;

    for (i = 0; i < comment_count; i++)
    {
        uint32_t comment_len;
        const uint8_t *comment_data;
        const char *sep;
        char *temp;

        if (cursor + 4 > size)
        {
            break;
        }

        comment_len = opus_parser_read_le32(payload + cursor);
        cursor += 4;
        if (comment_len == 0 || cursor + comment_len > size)
        {
            break;
        }

        comment_data = payload + cursor;
        cursor += comment_len;

        temp = (char *)player_malloc(comment_len + 1);
        if (!temp)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "opus metadata: alloc comment failed");
            continue;
        }

        os_memcpy(temp, comment_data, comment_len);
        temp[comment_len] = '\0';

        sep = os_strchr(temp, '=');
        if (sep && sep != temp)
        {
            size_t key_len = sep - temp;
            char key_buf[64];
            char *value = (char *)(sep + 1);

            if (key_len >= sizeof(key_buf))
            {
                key_len = sizeof(key_buf) - 1;
            }
            os_memcpy(key_buf, temp, key_len);
            key_buf[key_len] = '\0';

            opus_metadata_apply_comment(ctx->metadata, key_buf, value);
        }

        player_free(temp);
    }

    ctx->tags_parsed = true;
    return AUDIO_PLAYER_OK;
}

static void opus_metadata_update_duration(opus_metadata_ctx_t *ctx)
{
    if (ctx->last_granule <= 0)
    {
        return;
    }

    ogg_int64_t samples = ctx->last_granule;
    if (samples > (ogg_int64_t)ctx->pre_skip)
    {
        samples -= ctx->pre_skip;
    }

    ctx->metadata->duration = ((double)samples * 1000.0) / (double)OPUS_GRANULE_RATE;

    if (ctx->metadata->bitrate == 0 &&
        ctx->metadata->duration > 0.0 &&
        ctx->file_size_bytes > 0)
    {
        double bits = (double)ctx->file_size_bytes * 8.0;
        ctx->metadata->bitrate = (int)(bits * 1000.0 / ctx->metadata->duration);
    }
}

static void opus_metadata_cleanup(opus_metadata_ctx_t *ctx)
{
    if (ctx->stream_initialized)
    {
        ogg_stream_clear(&ctx->os);
        ctx->stream_initialized = false;
    }
    ogg_sync_clear(&ctx->oy);
}

static int opus_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    opus_metadata_ctx_t ctx;
    int ret = AUDIO_PLAYER_ERR;
    off_t current_pos;

    if (!metadata)
    {
        return AUDIO_PLAYER_ERR;
    }

    os_memset(&ctx, 0, sizeof(ctx));
    ctx.fd = fd;
    ctx.metadata = metadata;

    current_pos = lseek(fd, 0, SEEK_END);
    if (current_pos > 0)
    {
        ctx.file_size_bytes = (uint64_t)current_pos;
    }
    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    ogg_sync_init(&ctx.oy);

    while (1)
    {
        int page_status = ogg_sync_pageout(&ctx.oy, &ctx.og);
        if (page_status == 1)
        {
            if (!ctx.stream_initialized)
            {
                if (ogg_stream_init(&ctx.os, ogg_page_serialno(&ctx.og)) != 0)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "opus metadata: stream init failed");
                    goto exit;
                }
                ctx.stream_initialized = true;
            }

            if (ogg_stream_pagein(&ctx.os, &ctx.og) != 0)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "opus metadata: pagein failed");
                continue;
            }

            if (ogg_page_granulepos(&ctx.og) >= 0)
            {
                ctx.last_granule = ogg_page_granulepos(&ctx.og);
            }

            while (ogg_stream_packetout(&ctx.os, &ctx.op) > 0)
            {
                if (!ctx.head_parsed)
                {
                    if (opus_metadata_parse_head(&ctx, ctx.op.packet, (uint32_t)ctx.op.bytes) != AUDIO_PLAYER_OK)
                    {
                        goto exit;
                    }
                    continue;
                }

                if (!ctx.tags_parsed)
                {
                    if (opus_metadata_parse_tags(&ctx, ctx.op.packet, (uint32_t)ctx.op.bytes) != AUDIO_PLAYER_OK)
                    {
                        goto exit;
                    }
                    continue;
                }
            }
        }
        else if (page_status == 0)
        {
            int bytes = opus_metadata_read_chunk(&ctx);
            if (bytes == AUDIO_PLAYER_ERR)
            {
                goto exit;
            }

            if (bytes == 0)
            {
                break;
            }
        }
        else
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "opus metadata: dropping corrupt ogg page");
        }
    }

    if (!ctx.head_parsed)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "opus metadata: OpusHead missing");
        goto exit;
    }

    opus_metadata_update_duration(&ctx);
    metadata_fill_title_from_path(filepath, metadata);
    ret = AUDIO_PLAYER_OK;

exit:
    opus_metadata_cleanup(&ctx);
    return ret;
}

const bk_audio_player_metadata_parser_ops_t opus_metadata_parser_ops =
{
    .name = "opus",
    .format = AUDIO_FORMAT_OPUS,
    .probe = NULL,
    .parse = opus_metadata_parse,
};

/* Get OPUS metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_opus_metadata_parser_ops(void)
{
    return &opus_metadata_parser_ops;
}
