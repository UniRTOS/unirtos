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

#include <bk_posix.h>
#include <limits.h>
#include <os/mem.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define WAV_RIFF_HEADER_SIZE    12
#define WAV_CHUNK_HEADER_SIZE   8

typedef struct
{
    uint16_t audio_format;
    uint16_t block_align;
    uint16_t samples_per_block;
    uint32_t data_size;
    uint32_t byte_rate;
    uint16_t bits_per_sample;
} wav_format_info_t;

static int wav_parse_metadata(int fd, audio_metadata_t *metadata, wav_format_info_t *fmt_info)
{
    uint8_t header[WAV_RIFF_HEADER_SIZE];
    uint8_t chunk_header[WAV_CHUNK_HEADER_SIZE];
    int read_size;
    uint32_t chunk_size;
    bool fmt_found = false;
    bool data_found = false;

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    read_size = read(fd, header, sizeof(header));
    if (read_size != (int)sizeof(header))
    {
        return AUDIO_PLAYER_ERR;
    }

    if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F')
    {
        return AUDIO_PLAYER_ERR;
    }

    if (header[8] != 'W' || header[9] != 'A' || header[10] != 'V' || header[11] != 'E')
    {
        return AUDIO_PLAYER_ERR;
    }

    while (read(fd, chunk_header, sizeof(chunk_header)) == (int)sizeof(chunk_header))
    {
        chunk_size = chunk_header[4] |
                     (chunk_header[5] << 8) |
                     (chunk_header[6] << 16) |
                     (chunk_header[7] << 24);

        if (chunk_header[0] == 'f' &&
            chunk_header[1] == 'm' &&
            chunk_header[2] == 't' &&
            chunk_header[3] == ' ')
        {
            uint8_t stack_buf[32];
            uint8_t *fmt_buf = stack_buf;

            if (chunk_size > sizeof(stack_buf))
            {
                fmt_buf = (uint8_t *)os_malloc(chunk_size);
                if (!fmt_buf)
                {
                    return AUDIO_PLAYER_ERR;
                }
            }

            if (read(fd, fmt_buf, chunk_size) != (int)chunk_size)
            {
                if (fmt_buf != stack_buf)
                {
                    os_free(fmt_buf);
                }
                return AUDIO_PLAYER_ERR;
            }

            fmt_info->audio_format = fmt_buf[0] | (fmt_buf[1] << 8);
            metadata->channels = fmt_buf[2] | (fmt_buf[3] << 8);
            metadata->sample_rate = fmt_buf[4] |
                                    (fmt_buf[5] << 8) |
                                    (fmt_buf[6] << 16) |
                                    (fmt_buf[7] << 24);
            fmt_info->byte_rate = fmt_buf[8] |
                                  (fmt_buf[9] << 8) |
                                  (fmt_buf[10] << 16) |
                                  (fmt_buf[11] << 24);
            fmt_info->block_align = fmt_buf[12] | (fmt_buf[13] << 8);
            fmt_info->bits_per_sample = fmt_buf[14] | (fmt_buf[15] << 8);

            fmt_info->samples_per_block = 0;
            if (chunk_size > 16)
            {
                uint32_t offset = 16;
                uint16_t cb_size = fmt_buf[offset] | (fmt_buf[offset + 1] << 8);
                offset += 2;
                if (fmt_info->audio_format == 0x0011 && chunk_size >= offset + 2)
                {
                    fmt_info->samples_per_block = fmt_buf[offset] | (fmt_buf[offset + 1] << 8);
                }
                (void)cb_size;
            }

            if (fmt_buf != stack_buf)
            {
                os_free(fmt_buf);
            }

            fmt_found = true;
        }
        else if (chunk_header[0] == 'd' &&
                 chunk_header[1] == 'a' &&
                 chunk_header[2] == 't' &&
                 chunk_header[3] == 'a')
        {
            fmt_info->data_size = chunk_size;
            if (lseek(fd, chunk_size, SEEK_CUR) < 0)
            {
                return AUDIO_PLAYER_ERR;
            }
            data_found = true;
        }
        else
        {
            if (lseek(fd, chunk_size, SEEK_CUR) < 0)
            {
                return AUDIO_PLAYER_ERR;
            }
        }

        if (chunk_size & 1)
        {
            if (lseek(fd, 1, SEEK_CUR) < 0)
            {
                return AUDIO_PLAYER_ERR;
            }
        }
    }

    if (!fmt_found || !data_found)
    {
        return AUDIO_PLAYER_ERR;
    }

    return AUDIO_PLAYER_OK;
}

static int wav_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    int ret;
    wav_format_info_t fmt_info = {0};

    ret = wav_parse_metadata(fd, metadata, &fmt_info);
    if (ret != AUDIO_PLAYER_OK)
    {
        return ret;
    }

    if (metadata->channels <= 0 || metadata->sample_rate == 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    uint64_t bitrate = 0;
    uint64_t duration_ms = 0;

    if (fmt_info.audio_format == 0x0011) /* IMA ADPCM */
    {
        uint16_t samples_per_block = fmt_info.samples_per_block;
        if (samples_per_block == 0 && fmt_info.block_align > 0 && metadata->channels > 0)
        {
            uint32_t bytes_per_channel = fmt_info.block_align;
            if (bytes_per_channel >= (uint32_t)(4 * metadata->channels))
            {
                uint32_t payload = bytes_per_channel - (4 * metadata->channels);
                samples_per_block = (uint16_t)((payload * 2 / metadata->channels) + 1);
            }
        }

        if (samples_per_block > 0 && fmt_info.block_align > 0 && fmt_info.data_size > 0)
        {
            uint64_t full_blocks = fmt_info.data_size / fmt_info.block_align;
            uint64_t total_samples = full_blocks * samples_per_block;
            uint32_t remainder = fmt_info.data_size % fmt_info.block_align;

            if (remainder >= (uint32_t)(4 * metadata->channels))
            {
                uint32_t payload = remainder - (4 * metadata->channels);
                uint32_t extra_samples = (payload * 2 / metadata->channels) + 1;
                total_samples += extra_samples;
            }

            if (total_samples > 0)
            {
                duration_ms = (total_samples * 1000ULL + metadata->sample_rate / 2) / metadata->sample_rate;
                bitrate = ((uint64_t)fmt_info.data_size * 8ULL * metadata->sample_rate + (total_samples / 2)) / total_samples;
            }
        }
    }
    else if (fmt_info.audio_format == 0x0001) /* Linear PCM */
    {
        uint16_t bits_per_sample = fmt_info.bits_per_sample ? fmt_info.bits_per_sample : 16;
        bitrate = (uint64_t)metadata->sample_rate * metadata->channels * bits_per_sample;
        if (fmt_info.data_size > 0 && bitrate > 0)
        {
            duration_ms = (uint64_t)fmt_info.data_size * 8ULL * 1000ULL / bitrate;
        }
    }

    if (bitrate == 0 && fmt_info.byte_rate > 0)
    {
        bitrate = (uint64_t)fmt_info.byte_rate * 8ULL;
    }

    if (duration_ms == 0 && fmt_info.data_size > 0 && bitrate > 0)
    {
        duration_ms = (uint64_t)fmt_info.data_size * 8ULL * 1000ULL / bitrate;
    }

    if (bitrate > INT32_MAX)
    {
        bitrate = INT32_MAX;
    }

    metadata->bitrate = (int)bitrate;
    metadata->duration = (double)duration_ms;

    metadata_fill_title_from_path(filepath, metadata);
    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t wav_metadata_parser_ops = {
    .name = "wav",
    .format = AUDIO_FORMAT_WAV,
    .probe = NULL,
    .parse = wav_metadata_parse,
};

/* Get WAV metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_wav_metadata_parser_ops(void)
{
    return &wav_metadata_parser_ops;
}
