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
#include <os/str.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <bk_posix.h>

static int aac_parse_adts(int fd, audio_metadata_t *metadata)
{
    uint8_t buffer[7];
    uint8_t id3_header[10];
    int read_size;
    int i;
    uint32_t sync_word;
    uint8_t profile;
    uint8_t sample_rate_index;
    uint8_t channel_config;
    uint32_t id3_size = 0;
    off_t start_pos = 0;

    static const int aac_sample_rates[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
        16000, 12000, 11025, 8000, 7350, 0, 0, 0
    };

    if (metadata->has_id3v2)
    {
        if (lseek(fd, 0, SEEK_SET) >= 0 && read(fd, id3_header, sizeof(id3_header)) == (int)sizeof(id3_header))
        {
            if (id3_header[0] == 'I' && id3_header[1] == 'D' && id3_header[2] == '3')
            {
                id3_size = metadata_parse_synchsafe_int(&id3_header[6]);
                start_pos = 10 + id3_size;
            }
        }
    }

    if (lseek(fd, start_pos, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    for (i = 0; i < 4096; i++)
    {
        if (lseek(fd, start_pos + i, SEEK_SET) < 0)
        {
            break;
        }

        read_size = read(fd, buffer, sizeof(buffer));
        if (read_size < (int)sizeof(buffer))
        {
            break;
        }

        sync_word = (buffer[0] << 4) | (buffer[1] >> 4);
        if (sync_word != 0x0FFF)
        {
            continue;
        }

        profile = (buffer[2] >> 6) & 0x03;
        (void)profile;
        sample_rate_index = (buffer[2] >> 2) & 0x0F;
        channel_config = ((buffer[2] & 0x01) << 2) | ((buffer[3] >> 6) & 0x03);

        if (sample_rate_index < (int)(sizeof(aac_sample_rates) / sizeof(aac_sample_rates[0])))
        {
            metadata->sample_rate = aac_sample_rates[sample_rate_index];
        }

        metadata->channels = channel_config;
        return AUDIO_PLAYER_OK;
    }

    BK_LOGW(AUDIO_PLAYER_TAG, "AAC metadata: ADTS sync not found, use defaults\n");
    if (metadata->sample_rate == 0)
    {
        metadata->sample_rate = 44100;
    }
    if (metadata->channels == 0)
    {
        metadata->channels = 2;
    }

    return AUDIO_PLAYER_ERR;
}

static int aac_calculate_duration(int fd, audio_metadata_t *metadata)
{
    int file_size;
    uint64_t temp;
    uint32_t audio_data_size;
    uint32_t duration_ms;
    uint32_t estimated_bitrate;
    uint32_t id3_size = 0;
    uint8_t id3_header[10];

    file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "AAC metadata: invalid file size %d\n", file_size);
        return AUDIO_PLAYER_ERR;
    }

    if (metadata->has_id3v2)
    {
        if (lseek(fd, 0, SEEK_SET) >= 0 && read(fd, id3_header, sizeof(id3_header)) == (int)sizeof(id3_header))
        {
            if (id3_header[0] == 'I' && id3_header[1] == 'D' && id3_header[2] == '3')
            {
                id3_size = 10 + metadata_parse_synchsafe_int(&id3_header[6]);
            }
        }
    }

    audio_data_size = (uint32_t)(file_size - id3_size);

    if (metadata->bitrate == 0)
    {
        estimated_bitrate = (metadata->channels <= 1) ? 64000 : 128000;
        metadata->bitrate = estimated_bitrate;
    }

    temp = (uint64_t)audio_data_size * 8000;
    duration_ms = (uint32_t)(temp / metadata->bitrate);
    metadata->duration = (double)duration_ms;
    return AUDIO_PLAYER_OK;
}

static int aac_metadata_parse(int fd, const char *filepath, audio_metadata_t *metadata)
{
    (void)filepath;

    if (metadata_parse_id3v2(fd, metadata) != AUDIO_PLAYER_OK)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "AAC metadata: ID3v2 tag not found\n");
    }

    aac_parse_adts(fd, metadata);
    aac_calculate_duration(fd, metadata);

    return AUDIO_PLAYER_OK;
}

const bk_audio_player_metadata_parser_ops_t aac_metadata_parser_ops = {
    .name = "aac",
    .format = AUDIO_FORMAT_AAC,
    .probe = NULL,
    .parse = aac_metadata_parse,
};

/* Get AAC metadata parser operations structure */
const bk_audio_player_metadata_parser_ops_t *bk_audio_player_get_aac_metadata_parser_ops(void)
{
    return &aac_metadata_parser_ops;
}

