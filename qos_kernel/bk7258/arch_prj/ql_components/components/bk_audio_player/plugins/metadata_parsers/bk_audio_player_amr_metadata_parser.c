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
#include "player_osal.h"

#include <os/mem.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <bk_posix.h>

static int amr_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    static const int amr_frame_bytes[16] = {
        12, 13, 15, 17, 19, 20, 26, 31,
        5,  6,  5,  5,  0,  0,  0,  0
    };

    const char amr_magic[] = "#!AMR\n";
    uint8_t header[sizeof(amr_magic) - 1];
    uint64_t total_frame_bytes = 0;
    uint32_t frame_count = 0;
    off_t file_size;

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (read(fd, header, sizeof(header)) != (int)sizeof(header))
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "AMR metadata: header read failed\n");
        return AUDIO_PLAYER_ERR;
    }

    if (os_memcmp(header, amr_magic, sizeof(header)) != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "AMR metadata: invalid magic header\n");
        return AUDIO_PLAYER_ERR;
    }

    file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= (off_t)sizeof(header))
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "AMR metadata: file too small (%ld)\n", (long)file_size);
        return AUDIO_PLAYER_ERR;
    }

    if (lseek(fd, sizeof(header), SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    while (1)
    {
        uint8_t frame_header;
        ssize_t read_len;
        int frame_type;
        int payload_size;
        off_t current_pos;

        read_len = read(fd, &frame_header, 1);
        if (read_len == 0)
        {
            break;
        }
        if (read_len != 1)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "AMR metadata: frame header read error\n");
            break;
        }

        frame_type = (frame_header >> 3) & 0x0F;
        if (frame_type < 0 || frame_type >= 16)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "AMR metadata: invalid frame type %d\n", frame_type);
            break;
        }

        payload_size = amr_frame_bytes[frame_type];
        current_pos = lseek(fd, 0, SEEK_CUR);
        if (current_pos < 0)
        {
            break;
        }

        if (payload_size > 0)
        {
            if (current_pos + payload_size > file_size)
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "AMR metadata: truncated frame detected\n");
                break;
            }

            if (lseek(fd, payload_size, SEEK_CUR) < 0)
            {
                break;
            }
        }

        total_frame_bytes += (uint64_t)(payload_size + 1);
        frame_count++;
    }

    metadata->sample_rate = 8000;
    metadata->channels = 1;
    metadata->duration = frame_count ? (double)frame_count * 20.0 : 0.0;

    if (metadata->duration > 0 && total_frame_bytes > 0)
    {
        metadata->bitrate = (int)((total_frame_bytes * 8 * 1000) / metadata->duration);
    }

    metadata_fill_title_from_path(filepath, metadata);

    if (metadata->artist[0] == '\0')
    {
        metadata_safe_string_copy(metadata->artist, "Unknown Artist", AUDIO_METADATA_MAX_STRING_LEN);
    }
    if (metadata->album[0] == '\0')
    {
        metadata_safe_string_copy(metadata->album, "Unknown Album", AUDIO_METADATA_MAX_STRING_LEN);
    }
    if (metadata->genre[0] == '\0')
    {
        metadata_safe_string_copy(metadata->genre, "Speech", AUDIO_METADATA_MAX_STRING_LEN);
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "AMR metadata: frames=%u duration=%.0fms bitrate=%d\n",
               frame_count, metadata->duration, metadata->bitrate);
    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t amr_metadata_parser_ops = {
    .name = "amr",
    .format = AUDIO_FORMAT_AMR,
    .probe = NULL,
    .parse = amr_metadata_parse,
};

/* Get AMR metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_amr_metadata_parser_ops(void)
{
    return &amr_metadata_parser_ops;
}
