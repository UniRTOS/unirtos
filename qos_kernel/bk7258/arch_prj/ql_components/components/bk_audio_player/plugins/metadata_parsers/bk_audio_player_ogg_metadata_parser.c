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
#include "player_mem.h"
#include "player_osal.h"
//#include "play_manager.h"

#include <bk_posix.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdbool.h>
#include <stdint.h>

#define OGG_SYNC_HEADER       "OggS"
#define OGG_SYNC_HEADER_SIZE  4
#define OGG_PAGE_HEADER_SIZE  27
#define OGG_MAX_PAGE_SIZE     65536

/* The nominator of the timebase (48kHz for Opus, typically 48000) */
#define OGG_DEFAULT_RATE      48000

typedef struct ogg_page_s
{
    uint8_t       header[OGG_PAGE_HEADER_SIZE];
    uint8_t       segments;
    uint8_t       lacing_vals[255];
    uint32_t      payload_bytes;
    uint64_t      file_offset;
} ogg_page_t;

static int ogg_read_page(int fd, ogg_page_t *page)
{
    uint8_t marker[OGG_SYNC_HEADER_SIZE];
    ssize_t rd;
    off_t start;

    while (1)
    {
        start = lseek(fd, 0, SEEK_CUR);
        if (start < 0)
        {
            return -1;
        }

        rd = read(fd, marker, OGG_SYNC_HEADER_SIZE);
        if (rd != OGG_SYNC_HEADER_SIZE)
        {
            return -1;
        }

        if (os_memcmp(marker, OGG_SYNC_HEADER, OGG_SYNC_HEADER_SIZE) != 0)
        {
            if (lseek(fd, start + 1, SEEK_SET) < 0)
            {
                return -1;
            }
            continue;
        }

        page->file_offset = (uint64_t)start;
        os_memcpy(page->header, marker, OGG_SYNC_HEADER_SIZE);
        rd = read(fd, page->header + OGG_SYNC_HEADER_SIZE, OGG_PAGE_HEADER_SIZE - OGG_SYNC_HEADER_SIZE);
        if (rd != (ssize_t)(OGG_PAGE_HEADER_SIZE - OGG_SYNC_HEADER_SIZE))
        {
            return -1;
        }

        page->segments = page->header[26];
        if (page->segments > 0)
        {
            rd = read(fd, page->lacing_vals, page->segments);
            if (rd != page->segments)
            {
                return -1;
            }
        }
        uint32_t sum = 0;
        for (uint32_t i = 0; i < page->segments; ++i)
        {
            sum += page->lacing_vals[i];
        }
        page->payload_bytes = sum;
        if (lseek(fd, sum, SEEK_CUR) < 0)
        {
            return -1;
        }

        return 0;
    }
}

static int64_t ogg_page_granule(const ogg_page_t *page)
{
    int64_t value = 0;
    for (int i = 0; i < 8; ++i)
    {
        value |= ((int64_t)page->header[6 + i]) << (i * 8);
    }
    return value;
}

static int ogg_parse_comments(int fd, audio_metadata_t *metadata, uint8_t segments, uint32_t header_skip)
{
    uint32_t size;
    uint8_t *buf = NULL;
    ssize_t rd;

    size = 0;
    for (uint32_t i = 0; i < segments; ++i)
    {
        uint8_t len;
        rd = read(fd, &len, 1);
        if (rd != 1)
        {
            return -1;
        }
        size += len;
    }

    if (size == 0 || size > OGG_MAX_PAGE_SIZE || size < header_skip)
    {
        return -1;
    }

    buf = (uint8_t *)player_malloc(size);
    if (!buf)
    {
        return -1;
    }

    if (lseek(fd, -(off_t)size, SEEK_CUR) < 0)
    {
        player_free(buf);
        return -1;
    }

    rd = read(fd, buf, size);
    if (rd != (ssize_t)size)
    {
        player_free(buf);
        return -1;
    }

    // Skip header (OpusTags: 8 bytes, Vorbis: 7 bytes)
    const uint8_t *ptr = buf + header_skip;
    size -= header_skip;
    uint32_t vendor_len;
    if (ptr + 4 > buf + size)
    {
        player_free(buf);
        return -1;
    }
    vendor_len = (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) | ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
    ptr += 4;
    if (ptr + vendor_len > buf + size)
    {
        player_free(buf);
        return -1;
    }
    ptr += vendor_len;

    if (ptr + 4 > buf + size)
    {
        player_free(buf);
        return -1;
    }
    uint32_t comment_count = (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) |
                             ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
    ptr += 4;

    for (uint32_t i = 0; i < comment_count; ++i)
    {
        if (ptr + 4 > buf + size)
        {
            break;
        }
        uint32_t len = (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) |
                       ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
        ptr += 4;
        if (len == 0 || ptr + len > buf + size)
        {
            break;
        }

        char *entry = (char *)player_malloc(len + 1);
        if (!entry)
        {
            break;
        }
        os_memcpy(entry, ptr, len);
        entry[len] = '\0';
        ptr += len;

        char *eq = os_strchr(entry, '=');
        if (eq)
        {
            *eq = '\0';
            const char *key = entry;
            const char *value = eq + 1;

            if (os_strcasecmp(key, "TITLE") == 0)
            {
                metadata_safe_string_copy(metadata->title, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcasecmp(key, "ARTIST") == 0)
            {
                metadata_safe_string_copy(metadata->artist, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcasecmp(key, "ALBUM") == 0)
            {
                metadata_safe_string_copy(metadata->album, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcasecmp(key, "ALBUMARTIST") == 0 || os_strcasecmp(key, "ALBUM_ARTIST") == 0)
            {
                metadata_safe_string_copy(metadata->album_artist, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcasecmp(key, "GENRE") == 0)
            {
                metadata_safe_string_copy(metadata->genre, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcasecmp(key, "DATE") == 0 || os_strcasecmp(key, "YEAR") == 0)
            {
                metadata_safe_string_copy(metadata->year, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcasecmp(key, "TRACKNUMBER") == 0 || os_strcasecmp(key, "TRACK") == 0)
            {
                metadata_safe_string_copy(metadata->track_number, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
            else if (os_strcasecmp(key, "COMPOSER") == 0)
            {
                metadata_safe_string_copy(metadata->composer, value, AUDIO_METADATA_MAX_STRING_LEN);
            }
        }

        player_free(entry);
    }

    player_free(buf);
    return 0;
}

static int ogg_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    ogg_page_t page;
    bool header_seen = false;
    bool comments_done = false;
    uint32_t page_count = 0;
    int64_t total_granule = -1;
    uint32_t pre_skip = 0;
    uint32_t sample_rate = OGG_DEFAULT_RATE; // Default to 48kHz, will be updated from header
    bool is_vorbis = false;
    off_t saved_pos = 0;

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    // Parse header and comments from first few pages
    while (page_count < 8 && (!header_seen || !comments_done))
    {
        if (ogg_read_page(fd, &page) != 0)
        {
            break;
        }
        ++page_count;

        // Track granule position for duration calculation
        int64_t granule = ogg_page_granule(&page);
        if (granule >= 0)
        {
            total_granule = granule;
        }

        if (!header_seen)
        {
            // Check if this page contains OpusHead by reading first packet
            // Note: ogg_read_page already skipped payload, so we need to go back
            if (page.segments > 0)
            {
                // Calculate payload start position
                off_t payload_start = (off_t)(page.file_offset + OGG_PAGE_HEADER_SIZE + page.segments);
                saved_pos = lseek(fd, 0, SEEK_CUR);

                // Go back to payload start
                if (lseek(fd, payload_start, SEEK_SET) >= 0)
                {
                    uint8_t head_buf[30];
                    ssize_t rd = read(fd, head_buf, sizeof(head_buf));
                    if (rd >= 19)
                    {
                        // Check for OpusHead magic
                        if (os_memcmp(head_buf, "OpusHead", 8) == 0)
                        {
                            // Parse pre_skip from OpusHead (bytes 10-11, little-endian)
                            pre_skip = (uint32_t)head_buf[10] | ((uint32_t)head_buf[11] << 8);
                            header_seen = true;
                            BK_LOGI(AUDIO_PLAYER_TAG, "ogg metadata: OpusHead found at offset=%lld, pre_skip=%u\n",
                                       (long long)page.file_offset, pre_skip);
                        }
                        // Check for Vorbis identification header (packet type 0x01, then "vorbis")
                        else if (head_buf[0] == 0x01 && rd >= 30)
                        {
                            if (os_memcmp(head_buf + 1, "vorbis", 6) == 0)
                            {
                                is_vorbis = true;
                                // Parse sample rate from Vorbis identification header (bytes 12-15, little-endian)
                                sample_rate = (uint32_t)head_buf[12] |
                                             ((uint32_t)head_buf[13] << 8) |
                                             ((uint32_t)head_buf[14] << 16) |
                                             ((uint32_t)head_buf[15] << 24);
                                header_seen = true;
                                BK_LOGI(AUDIO_PLAYER_TAG, "ogg metadata: Vorbis header found at offset=%lld, sample_rate=%u\n",
                                           (long long)page.file_offset, sample_rate);
                            }
                        }
                    }
                }

                // Restore file position to after this page (saved_pos)
                lseek(fd, saved_pos, SEEK_SET);
            }
            continue;
        }

        if (!comments_done)
        {
            // Check if this page contains OpusTags or Vorbis comments
            if (page.segments > 0)
            {
                saved_pos = lseek(fd, 0, SEEK_CUR);
                off_t payload_start = (off_t)(page.file_offset + OGG_PAGE_HEADER_SIZE + page.segments);
                if (lseek(fd, payload_start, SEEK_SET) >= 0)
                {
                    uint8_t magic[8];
                    if (read(fd, magic, 8) == 8)
                    {
                        if (os_memcmp(magic, "OpusTags", 8) == 0)
                        {
                            // Parse Opus comments from this page (skip 8-byte "OpusTags" header)
                            lseek(fd, payload_start, SEEK_SET);
                            if (ogg_parse_comments(fd, metadata, page.segments, 8) == 0)
                            {
                                comments_done = true;
                            }
                        }
                        else if (is_vorbis && magic[0] == 0x03 && os_memcmp(magic + 1, "vorbis", 6) == 0)
                        {
                            // Parse Vorbis comments from this page (skip 7-byte header: 0x03 + "vorbis")
                            lseek(fd, payload_start, SEEK_SET);
                            if (ogg_parse_comments(fd, metadata, page.segments, 7) == 0)
                            {
                                comments_done = true;
                            }
                        }
                    }
                    lseek(fd, saved_pos, SEEK_SET);
                }
            }
            continue;
        }
    }

    // Continue scanning all pages to find the last granule position
    // Keep scanning until we reach end of file
    uint32_t scan_count = 0;
    int consecutive_failures = 0;
    const int max_consecutive_failures = 10; // Increased to handle files with minor corruption
    off_t file_size = lseek(fd, 0, SEEK_END);

    // Continue from current position
    lseek(fd, saved_pos, SEEK_SET);

    while (scan_count < 100000 && consecutive_failures < max_consecutive_failures)
    {
        off_t before_read = lseek(fd, 0, SEEK_CUR);
        if (before_read < 0 || before_read >= file_size)
        {
            break;
        }

        if (ogg_read_page(fd, &page) != 0)
        {
            consecutive_failures++;
            // If we failed, try to continue from next byte
            // Skip up to 27 bytes (OGG page header size) to find next page
            off_t skip_bytes = consecutive_failures < 27 ? 1 : 27;
            if (lseek(fd, before_read + skip_bytes, SEEK_SET) < 0)
            {
                break;
            }
            continue;
        }

        consecutive_failures = 0;
        scan_count++;

        int64_t granule = ogg_page_granule(&page);
        // Update if granule is valid (non-negative) and larger
        // Note: granulepos can be -1 for pages with no audio data
        if (granule >= 0)
        {
            if (total_granule < 0 || granule > total_granule)
            {
                total_granule = granule;
            }
        }

        // Check if we're near end of file (within last 1KB)
        off_t after_read = lseek(fd, 0, SEEK_CUR);
        if (after_read < 0 || after_read >= file_size - 1024)
        {
            // Near end, but continue a bit more to ensure we get the last page
            if (after_read >= file_size - 27)
            {
                break;
            }
        }
    }

    // Try to find last page by scanning backwards from file end
    // This is more reliable for getting the final granule position
    BK_LOGI(AUDIO_PLAYER_TAG, "ogg metadata: forward scan complete, granule=%lld, scanned %u pages, file_size=%lld\n",
               (long long)total_granule, scan_count, (long long)file_size);

    if (file_size > 0)
    {
        // Ogg pages are typically at most 64KB, scan from file_size - 128KB to be safe
        off_t scan_size = file_size > 131072 ? 131072 : file_size;
        off_t scan_start = file_size > scan_size ? (file_size - scan_size) : 0;
        int64_t best_granule = total_granule;
        off_t best_offset = 0;
        const off_t coarse_step = 1024; // Coarse scan: 1KB steps for speed
        const off_t fine_step = 256;    // Fine scan: 256-byte steps

        BK_LOGD(AUDIO_PLAYER_TAG, "ogg metadata: backward scan from offset %lld to %lld\n",
                   (long long)scan_start, (long long)file_size);

        // Phase 1: Coarse backward scan - find potential last pages quickly
        for (off_t offset = file_size - 1; offset >= scan_start && offset >= 0; offset -= coarse_step)
        {
            if (lseek(fd, offset, SEEK_SET) < 0)
            {
                break;
            }

            uint8_t marker[OGG_SYNC_HEADER_SIZE];
            if (read(fd, marker, OGG_SYNC_HEADER_SIZE) == OGG_SYNC_HEADER_SIZE)
            {
                if (os_memcmp(marker, OGG_SYNC_HEADER, OGG_SYNC_HEADER_SIZE) == 0)
                {
                    // Found potential page start, try to read it
                    if (lseek(fd, offset, SEEK_SET) >= 0)
                    {
                        ogg_page_t test_page;
                        if (ogg_read_page(fd, &test_page) == 0)
                        {
                            int64_t granule = ogg_page_granule(&test_page);
                            if (granule >= 0)
                            {
                                if (granule > best_granule || best_granule < 0)
                                {
                                    best_granule = granule;
                                    best_offset = offset;
                                    BK_LOGD(AUDIO_PLAYER_TAG, "ogg metadata: coarse backward scan found granule=%lld at offset=%lld\n",
                                               (long long)granule, (long long)offset);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Phase 2: Fine backward scan around the best candidate and from file end
        // Scan the last 256KB more carefully to ensure we find the absolute last page
        off_t fine_scan_size = file_size > 262144 ? 262144 : file_size;
        off_t fine_scan_start = file_size > fine_scan_size ? (file_size - fine_scan_size) : 0;

        // Fine scan: check around the best offset and from file end
        for (off_t offset = file_size - 1; offset >= fine_scan_start && offset >= 0; offset -= fine_step)
        {
            if (lseek(fd, offset, SEEK_SET) < 0)
            {
                break;
            }

            uint8_t marker[OGG_SYNC_HEADER_SIZE];
            if (read(fd, marker, OGG_SYNC_HEADER_SIZE) == OGG_SYNC_HEADER_SIZE)
            {
                if (os_memcmp(marker, OGG_SYNC_HEADER, OGG_SYNC_HEADER_SIZE) == 0)
                {
                    if (lseek(fd, offset, SEEK_SET) >= 0)
                    {
                        ogg_page_t test_page;
                        if (ogg_read_page(fd, &test_page) == 0)
                        {
                            int64_t granule = ogg_page_granule(&test_page);
                            if (granule >= 0 && granule > best_granule)
                            {
                                best_granule = granule;
                                best_offset = offset;
                                BK_LOGD(AUDIO_PLAYER_TAG, "ogg metadata: fine backward scan found granule=%lld at offset=%lld\n",
                                           (long long)granule, (long long)offset);
                            }
                        }
                    }
                }
            }
        }

        if (best_granule > total_granule)
        {
            total_granule = best_granule;
            BK_LOGI(AUDIO_PLAYER_TAG, "ogg metadata: backward scan found larger granule=%lld at offset=%lld\n",
                       (long long)total_granule, (long long)best_offset);
        }

        (void)best_offset; // Suppress unused variable warning
    }

    // Calculate duration from last granule position
    // Subtract pre_skip as it represents the number of samples to skip at the start (Opus only)
    if (total_granule >= 0 && sample_rate > 0)
    {
        int64_t samples = total_granule;
        if (is_vorbis)
        {
            // Vorbis doesn't use pre_skip, granulepos directly represents total samples
            // No need to subtract pre_skip
        }
        else
        {
            // Opus: subtract pre_skip
            if (samples > (int64_t)pre_skip)
            {
                samples -= (int64_t)pre_skip;
            }
        }
        metadata->duration = ((double)samples * 1000.0) / (double)sample_rate;

        BK_LOGI(AUDIO_PLAYER_TAG, "ogg metadata: format=%s, granule=%lld, pre_skip=%u, sample_rate=%u, samples=%lld, duration=%.1fms (%.1fs)\n",
                   is_vorbis ? "Vorbis" : "Opus",
                   (long long)total_granule, pre_skip, sample_rate, (long long)samples, 
                   metadata->duration, metadata->duration / 1000.0);
    }
    else
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "ogg metadata: failed to find last granule position, scanned %u pages\n", scan_count);
    }

    metadata_fill_title_from_path(filepath, metadata);
    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t ogg_metadata_parser_ops =
{
    .name = "ogg",
    .format = AUDIO_FORMAT_OGG,
    .probe = NULL,
    .parse = ogg_metadata_parse,
};

/* Get OGG metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_ogg_metadata_parser_ops(void)
{
    return &ogg_metadata_parser_ops;
}
