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
//#include "play_manager.h"
#include <stdint.h>
#include <os/mem.h>
#include <os/str.h>
#include <unistd.h>
#include <bk_posix.h>

#define ID3V2_HEADER_SIZE       10
#define ID3V2_FRAME_HEADER_SIZE 10
#define ID3V2_MAX_TAG_SIZE      (1024 * 1024)

uint32_t metadata_parse_synchsafe_int(const uint8_t *data)
{
    if (data == NULL)
    {
        return 0;
    }

    return ((uint32_t)data[0] << 21) |
           ((uint32_t)data[1] << 14) |
           ((uint32_t)data[2] << 7) |
           ((uint32_t)data[3]);
}

void metadata_safe_string_copy(char *dst, const char *src, int max_len)
{
    int i;
    int last_non_space = -1;

    if (dst == NULL || src == NULL || max_len <= 0)
    {
        return;
    }

    for (i = 0; i < (max_len - 1) && src[i] != '\0'; i++)
    {
        dst[i] = src[i];
        if (src[i] != ' ')
        {
            last_non_space = i;
        }
    }

    if (last_non_space >= 0)
    {
        dst[last_non_space + 1] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }
}

char *metadata_extract_id3v2_text(const uint8_t *data, uint32_t size)
{
    uint8_t encoding;
    const uint8_t *text_data;
    uint32_t text_size;
    char *result;
    uint32_t i;

    if (data == NULL || size < 1)
    {
        return NULL;
    }

    encoding = data[0];
    text_data = data + 1;
    text_size = size - 1;

    if (text_size == 0)
    {
        return NULL;
    }

    result = (char *)player_malloc(text_size + 1);
    if (result == NULL)
    {
        return NULL;
    }

    switch (encoding)
    {
        case 0:
        case 3:
            os_memcpy(result, text_data, text_size);
            result[text_size] = '\0';
            break;

        case 1:
        case 2:
            for (i = 0; i < text_size / 2; i++)
            {
                result[i] = text_data[i * 2 + 1];
            }
            result[text_size / 2] = '\0';
            break;

        default:
            result[0] = '\0';
            break;
    }

    return result;
}

void metadata_fill_title_from_path(const char *filepath, audio_metadata_t *metadata)
{
    const char *base;
    size_t len;
    char temp[AUDIO_METADATA_MAX_STRING_LEN];
    char *dot;

    if (filepath == NULL || metadata == NULL)
    {
        return;
    }

    if (metadata->title[0] != '\0')
    {
        return;
    }

    base = os_strrchr(filepath, '/');
    base = base ? (base + 1) : filepath;
    len = os_strlen(base);

    if (len >= sizeof(temp))
    {
        len = sizeof(temp) - 1;
    }

    os_memcpy(temp, base, len);
    temp[len] = '\0';

    dot = os_strrchr(temp, '.');
    if (dot && dot > temp)
    {
        *dot = '\0';
    }

    metadata_safe_string_copy(metadata->title, temp, AUDIO_METADATA_MAX_STRING_LEN);
}

int metadata_parse_id3v2(int fd, audio_metadata_t *metadata)
{
    uint8_t header[ID3V2_HEADER_SIZE];
    uint8_t *tag_data = NULL;
    uint32_t tag_size;
    uint32_t offset = 0;
    uint8_t version_major;
    int read_size;
    int ret = -1;

    if (metadata == NULL)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return AUDIO_PLAYER_ERR;
    }

    read_size = read(fd, header, sizeof(header));
    if (read_size != (int)sizeof(header))
    {
        return AUDIO_PLAYER_ERR;
    }

    if (header[0] != 'I' || header[1] != 'D' || header[2] != '3')
    {
        return AUDIO_PLAYER_ERR;
    }

    tag_size = metadata_parse_synchsafe_int(&header[6]);
    if (tag_size == 0 || tag_size > ID3V2_MAX_TAG_SIZE)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "ID3v2: invalid tag size %u\n", tag_size);
        return AUDIO_PLAYER_ERR;
    }

    tag_data = (uint8_t *)player_malloc(tag_size);
    if (tag_data == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ID3v2: memory allocation failed, size=%u\n", tag_size);
        return AUDIO_PLAYER_NO_MEM;
    }

    read_size = read(fd, tag_data, tag_size);
    if (read_size != (int)tag_size)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "ID3v2: payload read failed (%d/%u)\n", read_size, tag_size);
        goto exit;
    }

    version_major = header[3];

    while (offset + ID3V2_FRAME_HEADER_SIZE <= tag_size)
    {
        uint8_t *frame_header = tag_data + offset;
        char frame_id[5];
        uint32_t frame_size;
        uint8_t *frame_data;
        char *text;

        if (frame_header[0] == 0)
        {
            break;
        }

        os_memcpy(frame_id, frame_header, 4);
        frame_id[4] = '\0';

        if (version_major == 4)
        {
            frame_size = metadata_parse_synchsafe_int(&frame_header[4]);
        }
        else
        {
            frame_size = (frame_header[4] << 24) |
                         (frame_header[5] << 16) |
                         (frame_header[6] << 8) |
                         frame_header[7];
        }

        if (frame_size == 0 || offset + ID3V2_FRAME_HEADER_SIZE + frame_size > tag_size)
        {
            break;
        }

        frame_data = frame_header + ID3V2_FRAME_HEADER_SIZE;

        if (os_strcmp(frame_id, "TIT2") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata_safe_string_copy(metadata->title, text, AUDIO_METADATA_MAX_STRING_LEN);
                player_free(text);
            }
        }
        else if (os_strcmp(frame_id, "TPE1") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata_safe_string_copy(metadata->artist, text, AUDIO_METADATA_MAX_STRING_LEN);
                player_free(text);
            }
        }
        else if (os_strcmp(frame_id, "TALB") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata_safe_string_copy(metadata->album, text, AUDIO_METADATA_MAX_STRING_LEN);
                player_free(text);
            }
        }
        else if (os_strcmp(frame_id, "TDRC") == 0 || os_strcmp(frame_id, "TYER") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata_safe_string_copy(metadata->year, text, AUDIO_METADATA_MAX_STRING_LEN);
                player_free(text);
            }
        }
        else if (os_strcmp(frame_id, "TCOM") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata_safe_string_copy(metadata->composer, text, AUDIO_METADATA_MAX_STRING_LEN);
                player_free(text);
            }
        }
        else if (os_strcmp(frame_id, "TRCK") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata_safe_string_copy(metadata->track_number, text, AUDIO_METADATA_MAX_STRING_LEN);
                player_free(text);
            }
        }
        else if (os_strcmp(frame_id, "TCON") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata_safe_string_copy(metadata->genre, text, AUDIO_METADATA_MAX_STRING_LEN);
                player_free(text);
            }
        }
        else if (os_strcmp(frame_id, "TLEN") == 0)
        {
            text = metadata_extract_id3v2_text(frame_data, frame_size);
            if (text)
            {
                metadata->duration = (double)os_strtoul(text, NULL, 10);
                player_free(text);
            }
        }

        offset += ID3V2_FRAME_HEADER_SIZE + frame_size;
    }

    metadata->has_id3v2 = 1;
    ret = AUDIO_PLAYER_OK;

exit:
    if (tag_data)
    {
        player_free(tag_data);
    }

    return ret;
}

