// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on "AS IS" BASIS,
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
#include <limits.h>

#define FLAC_MARKER_SIZE           4
#define FLAC_METADATA_BLOCK_HEADER_SIZE  4
#define ID3V2_HEADER_SIZE          10

// FLAC metadata block types
#define FLAC_METADATA_BLOCK_STREAMINFO    0
#define FLAC_METADATA_BLOCK_VORBIS_COMMENT 4

// FLAC file signature
#define FLAC_SIGNATURE             0x664C6143  // "fLaC"

// Helper functions to read big-endian values
static uint32_t read_be24(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 16) |
           ((uint32_t)buf[1] << 8) |
            (uint32_t)buf[2];
}

static uint32_t read_be32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
            (uint32_t)buf[3];
}

static uint64_t read_be64(const uint8_t *buf)
{
    uint64_t high = read_be32(buf);
    uint64_t low = read_be32(buf + 4);
    return (high << 32) | low;
}

// Read full data from file descriptor
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

// Parse FLAC STREAMINFO metadata block
static int flac_parse_streaminfo(int fd, audio_metadata_t *metadata)
{
    uint8_t streaminfo[34];
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
    uint64_t total_samples;
    uint64_t duration_ms = 0;

    if (read_full(fd, streaminfo, sizeof(streaminfo)) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to read STREAMINFO block\n");
        return AUDIO_PLAYER_ERR;
    }

    // Parse sample rate (20 bits, bits 0-19)
    sample_rate = ((uint32_t)streaminfo[10] << 12) |
                  ((uint32_t)streaminfo[11] << 4) |
                  ((uint32_t)streaminfo[12] >> 4);
    if (sample_rate == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: invalid sample rate\n");
        return AUDIO_PLAYER_ERR;
    }

    // Parse channels (3 bits, bits 20-22)
    channels = ((streaminfo[12] >> 1) & 0x07) + 1;
    if (channels == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: invalid channel count\n");
        return AUDIO_PLAYER_ERR;
    }

    // Parse bits per sample (5 bits, bits 24-28)
    bits_per_sample = (((streaminfo[12] & 0x01) << 4) | ((streaminfo[13] >> 4) & 0x0F)) + 1;
    if (bits_per_sample == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: invalid bits per sample\n");
        return AUDIO_PLAYER_ERR;
    }

    // Parse total samples (36 bits, bits 32-67)
    total_samples = ((uint64_t)(streaminfo[13] & 0x0F) << 32) |
                    ((uint64_t)streaminfo[14] << 24) |
                    ((uint64_t)streaminfo[15] << 16) |
                    ((uint64_t)streaminfo[16] << 8) |
                     (uint64_t)streaminfo[17];

    metadata->sample_rate = (int)sample_rate;
    metadata->channels = (int)channels;

    // Calculate duration if total_samples is available
    if (total_samples > 0 && sample_rate > 0)
    {
        duration_ms = (total_samples * 1000ULL + sample_rate / 2) / sample_rate;
        metadata->duration = (double)duration_ms;
    }

    // Calculate bitrate
    if (duration_ms > 0)
    {
        int file_size = lseek(fd, 0, SEEK_END);
        if (file_size > 0)
        {
            uint64_t bitrate = ((uint64_t)file_size * 8ULL * 1000ULL + duration_ms / 2) / duration_ms;
            if (bitrate > INT32_MAX)
            {
                bitrate = INT32_MAX;
            }
            metadata->bitrate = (int)bitrate;
        }
    }
    else
    {
        // Estimate bitrate from sample rate, channels, and bits per sample
        uint64_t bitrate = (uint64_t)sample_rate * channels * bits_per_sample;
        if (bitrate > INT32_MAX)
        {
            bitrate = INT32_MAX;
        }
        metadata->bitrate = (int)bitrate;
    }

    return AUDIO_PLAYER_OK;
}

// Parse VORBIS_COMMENT metadata block
static int flac_parse_vorbis_comment(int fd, audio_metadata_t *metadata, uint32_t block_length)
{
    uint8_t vendor_length_buf[4];
    uint32_t vendor_length;
    uint32_t comment_count;
    uint32_t i;
    uint8_t *comment_data = NULL;
    uint32_t comment_data_size = 0;
    uint32_t offset = 0;
    int ret = AUDIO_PLAYER_ERR;

    // Read vendor string length
    if (read_full(fd, vendor_length_buf, sizeof(vendor_length_buf)) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to read vendor length\n");
        return AUDIO_PLAYER_ERR;
    }
    vendor_length = read_be32(vendor_length_buf);

    // Skip vendor string
    if (vendor_length > 0)
    {
        if (lseek(fd, vendor_length, SEEK_CUR) < 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to skip vendor string\n");
            return AUDIO_PLAYER_ERR;
        }
    }

    // Read comment count
    if (read_full(fd, vendor_length_buf, sizeof(vendor_length_buf)) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to read comment count\n");
        return AUDIO_PLAYER_ERR;
    }
    comment_count = read_be32(vendor_length_buf);

    // Calculate remaining data size: block_length - 4(vendor_len) - vendor_length - 4(comment_count)
    if (block_length < 8 || vendor_length > block_length - 8)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: invalid block length or vendor length\n");
        return AUDIO_PLAYER_ERR;
    }

    comment_data_size = block_length - 8 - vendor_length;
    if (comment_data_size == 0)
    {
        // No comment data available
        return AUDIO_PLAYER_OK;
    }

    // Limit buffer size to prevent excessive memory allocation
    if (comment_data_size > 64 * 1024)
    {
        comment_data_size = 64 * 1024;
    }

    comment_data = (uint8_t *)player_malloc(comment_data_size);
    if (comment_data == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to allocate comment buffer\n");
        return AUDIO_PLAYER_NO_MEM;
    }

    if (read_full(fd, comment_data, comment_data_size) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to read comments\n");
        goto exit;
    }

    // Parse comments
    for (i = 0; i < comment_count && offset + 4 <= comment_data_size; i++)
    {
        uint32_t comment_length = read_be32(comment_data + offset);
        offset += 4;

        if (comment_length == 0 || offset + comment_length > comment_data_size)
        {
            break;
        }

        // Comments are in format "KEY=VALUE"
        char *comment = (char *)(comment_data + offset);
        char *equals = os_strchr(comment, '=');
        if (equals != NULL && equals > comment)
        {
            size_t key_len = (size_t)(equals - comment);
            char *value = equals + 1;
            size_t value_len = comment_length - key_len - 1;

            // Normalize key to uppercase for comparison
            char key[64];
            size_t j;
            if (key_len >= sizeof(key))
            {
                key_len = sizeof(key) - 1;
            }
            for (j = 0; j < key_len; j++)
            {
                char c = comment[j];
                if (c >= 'a' && c <= 'z')
                {
                    key[j] = c - 'a' + 'A';
                }
                else
                {
                    key[j] = c;
                }
            }
            key[key_len] = '\0';

            // Ensure value is null-terminated
            if (value_len > 0 && value[value_len - 1] == '\0')
            {
                value_len--;
            }

            // Map Vorbis comment keys to metadata fields
            if (os_strcmp(key, "TITLE") == 0 && metadata->title[0] == '\0')
            {
                metadata_safe_string_copy(metadata->title, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcmp(key, "ARTIST") == 0 && metadata->artist[0] == '\0')
            {
                metadata_safe_string_copy(metadata->artist, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcmp(key, "ALBUM") == 0 && metadata->album[0] == '\0')
            {
                metadata_safe_string_copy(metadata->album, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcmp(key, "ALBUMARTIST") == 0 && metadata->album_artist[0] == '\0')
            {
                metadata_safe_string_copy(metadata->album_artist, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcmp(key, "GENRE") == 0 && metadata->genre[0] == '\0')
            {
                metadata_safe_string_copy(metadata->genre, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcmp(key, "DATE") == 0 && metadata->year[0] == '\0')
            {
                // Extract year from date string (format: YYYY or YYYY-MM-DD)
                if (value_len >= 4)
                {
                    char year[5];
                    os_memcpy(year, value, 4);
                    year[4] = '\0';
                    metadata_safe_string_copy(metadata->year, year, AUDIO_METADATA_MAX_STRING_LEN);
                }
            }
            else if (os_strcmp(key, "COMPOSER") == 0 && metadata->composer[0] == '\0')
            {
                metadata_safe_string_copy(metadata->composer, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcmp(key, "TRACKNUMBER") == 0 && metadata->track_number[0] == '\0')
            {
                metadata_safe_string_copy(metadata->track_number, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
        }

        offset += comment_length;
    }

    ret = AUDIO_PLAYER_OK;

exit:
    if (comment_data != NULL)
    {
        player_free(comment_data);
    }

    return ret;
}

// Skip ID3v2 tag if present at the beginning of the file
static int flac_skip_id3v2(int fd)
{
    uint8_t header[ID3V2_HEADER_SIZE];
    int read_size;
    uint32_t tag_size;

    // Reset file position to beginning
    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    // Read ID3v2 header
    read_size = read(fd, header, sizeof(header));
    if (read_size != (int)sizeof(header))
    {
        return AUDIO_PLAYER_ERR;
    }

    // Check for ID3v2 tag
    if (header[0] != 'I' || header[1] != 'D' || header[2] != '3')
    {
        // No ID3v2 tag, reset to beginning
        if (lseek(fd, 0, SEEK_SET) < 0)
        {
            return AUDIO_PLAYER_ERR;
        }
        return AUDIO_PLAYER_OK;
    }

    // Parse ID3v2 tag size (synchsafe integer)
    tag_size = metadata_parse_synchsafe_int(&header[6]);
    if (tag_size == 0 || tag_size > (1024 * 1024))
    {
        // Invalid tag size, reset to beginning
        if (lseek(fd, 0, SEEK_SET) < 0)
        {
            return AUDIO_PLAYER_ERR;
        }
        return AUDIO_PLAYER_OK;
    }

    // Skip ID3v2 tag (header + tag data)
    if (lseek(fd, ID3V2_HEADER_SIZE + tag_size, SEEK_SET) < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to skip ID3v2 tag\n");
        return AUDIO_PLAYER_ERR;
    }

    BK_LOGD(AUDIO_PLAYER_TAG, "FLAC: skipped ID3v2 tag, size=%u\n", tag_size);
    return AUDIO_PLAYER_OK;
}

// Main FLAC metadata parsing function
static int flac_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    uint8_t marker[FLAC_MARKER_SIZE];
    uint8_t block_header[FLAC_METADATA_BLOCK_HEADER_SIZE];
    bool streaminfo_found = false;
    int read_size;
    int ret;

    if (metadata == NULL)
    {
        return AUDIO_PLAYER_ERR;
    }

    // Skip ID3v2 tag if present
    ret = flac_skip_id3v2(fd);
    if (ret != AUDIO_PLAYER_OK)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to skip ID3v2 tag\n");
        return ret;
    }

    // Read and verify FLAC marker
    read_size = read(fd, marker, sizeof(marker));
    if (read_size != (int)sizeof(marker))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to read file marker\n");
        return AUDIO_PLAYER_ERR;
    }

    if (read_be32(marker) != FLAC_SIGNATURE)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: invalid file marker\n");
        return AUDIO_PLAYER_ERR;
    }

    // Parse metadata blocks
    while (read(fd, block_header, sizeof(block_header)) == (int)sizeof(block_header))
    {
        bool is_last = (block_header[0] & 0x80) != 0;
        uint8_t block_type = block_header[0] & 0x7F;
        uint32_t block_length = read_be24(block_header + 1);

        if (block_length == 0 && block_type != FLAC_METADATA_BLOCK_STREAMINFO)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "FLAC: invalid block length\n");
            break;
        }

        switch (block_type)
        {
            case FLAC_METADATA_BLOCK_STREAMINFO:
                if (block_length != 34)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: invalid STREAMINFO block length\n");
                    return AUDIO_PLAYER_ERR;
                }
                if (flac_parse_streaminfo(fd, metadata) == AUDIO_PLAYER_OK)
                {
                    streaminfo_found = true;
                }
                break;

            case FLAC_METADATA_BLOCK_VORBIS_COMMENT:
                if (block_length > 8)
                {
                    flac_parse_vorbis_comment(fd, metadata, block_length);
                }
                else
                {
                    // Skip invalid block
                    if (lseek(fd, block_length, SEEK_CUR) < 0)
                    {
                        return AUDIO_PLAYER_ERR;
                    }
                }
                break;

            default:
                // Skip unknown metadata blocks
                if (lseek(fd, block_length, SEEK_CUR) < 0)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: failed to skip metadata block\n");
                    return AUDIO_PLAYER_ERR;
                }
                break;
        }

        if (is_last)
        {
            break;
        }
    }

    if (!streaminfo_found)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: STREAMINFO block not found\n");
        return AUDIO_PLAYER_ERR;
    }

    if (metadata->channels <= 0 || metadata->sample_rate == 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "FLAC: invalid audio parameters\n");
        return AUDIO_PLAYER_ERR;
    }

    // Fill title from filepath if not found in metadata
    metadata_fill_title_from_path(filepath, metadata);

    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t flac_metadata_parser_ops = {
    .name = "flac",
    .format = AUDIO_FORMAT_FLAC,
    .probe = NULL,
    .parse = flac_metadata_parse,
};

/* Get FLAC metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_flac_metadata_parser_ops(void)
{
    return &flac_metadata_parser_ops;
}
