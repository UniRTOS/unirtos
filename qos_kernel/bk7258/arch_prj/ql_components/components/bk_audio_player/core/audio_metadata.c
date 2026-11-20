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

#include <components/bk_audio_player/bk_audio_player.h>
#include "bk_audio_player_private.h"
#include "audio_metadata_parser.h"
#include "audio_metadata_parser_common.h"
#include "play_manager.h"
#include "player_osal.h"

#include <fcntl.h>
#include <os/str.h>
#include <unistd.h>
#include <bk_posix.h>

void bk_audio_metadata_init(audio_metadata_t *metadata)
{
    if (metadata == NULL)
    {
        return;
    }

    os_memset(metadata, 0, sizeof(audio_metadata_t));
}

audio_format_t bk_audio_metadata_get_format(const char *filepath)
{
    const char *ext;

    if (filepath == NULL)
    {
        return AUDIO_FORMAT_UNKNOWN;
    }

    ext = os_strrchr(filepath, '.');
    if (ext == NULL)
    {
        return AUDIO_FORMAT_UNKNOWN;
    }

    ext++;

    if (os_strcasecmp(ext, "mp3") == 0)
    {
        return AUDIO_FORMAT_MP3;
    }
    if (os_strcasecmp(ext, "wav") == 0)
    {
        return AUDIO_FORMAT_WAV;
    }
    if (os_strcasecmp(ext, "aac") == 0)
    {
        return AUDIO_FORMAT_AAC;
    }
    if (os_strcasecmp(ext, "m4a") == 0 ||
        os_strcasecmp(ext, "m4b") == 0 ||
        os_strcasecmp(ext, "mp4") == 0)
    {
        /* m4a/m4b/mp4 containers carry AAC payload but require dedicated metadata parser */
        return AUDIO_FORMAT_M4A;
    }
    if (os_strcasecmp(ext, "flac") == 0)
    {
        return AUDIO_FORMAT_FLAC;
    }
    if (os_strcasecmp(ext, "ogg") == 0)
    {
        return AUDIO_FORMAT_OGG;
    }
    if (os_strcasecmp(ext, "opus") == 0)
    {
        return AUDIO_FORMAT_OPUS;
    }
    if (os_strcasecmp(ext, "amr") == 0)
    {
        return AUDIO_FORMAT_AMR;
    }

    return AUDIO_FORMAT_UNKNOWN;
}

int bk_audio_player_get_metadata_from_file(bk_audio_player_handle_t handle, const char *filepath, audio_metadata_t *metadata)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;
    const bk_audio_player_metadata_parser_ops_t *parser;
    audio_format_t format;
    int fd = -1;
    int parser_ret;

    if (handle == NULL || filepath == NULL || metadata == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "metadata: invalid parameter\n");
        return -3;
    }

    bk_audio_metadata_init(metadata);
    format = bk_audio_metadata_get_format(filepath);
    if (format == AUDIO_FORMAT_UNKNOWN)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "metadata: unsupported format for %s\n", filepath);
        return -2;
    }

    parser = audio_metadata_parser_find(player, format, filepath);
    if (parser == NULL)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "metadata: no parser registered for format %d\n", format);
        return -2;
    }

    fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "metadata: open %s failed\n", filepath);
        return -1;
    }

    parser_ret = parser->parse(fd, filepath, metadata);
    close(fd);

    if (parser_ret != AUDIO_PLAYER_OK)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "metadata: parser %s failed (%d)\n", parser->name, parser_ret);
        return -1;
    }

    metadata_fill_title_from_path(filepath, metadata);

    BK_LOGI(AUDIO_PLAYER_TAG, "metadata: title='%s', artist='%s', duration=%u s\n",
               metadata->title, metadata->artist, (uint32_t)(metadata->duration / 1000));
    return 0;
}

int bk_audio_metadata_get_field_string(const audio_metadata_t *metadata,
                                        audio_metadata_field_t field,
                                        char *buffer,
                                        int buf_size)
{
    if (metadata == NULL || buffer == NULL || buf_size <= 0)
    {
        return -1;
    }

    buffer[0] = '\0';

    switch (field)
    {
        case AUDIO_METADATA_TITLE:
            os_snprintf(buffer, buf_size, "%s", metadata->title);
            break;
        case AUDIO_METADATA_ARTIST:
            os_snprintf(buffer, buf_size, "%s", metadata->artist);
            break;
        case AUDIO_METADATA_ALBUM:
            os_snprintf(buffer, buf_size, "%s", metadata->album);
            break;
        case AUDIO_METADATA_ALBUM_ARTIST:
            os_snprintf(buffer, buf_size, "%s", metadata->album_artist);
            break;
        case AUDIO_METADATA_GENRE:
            os_snprintf(buffer, buf_size, "%s", metadata->genre);
            break;
        case AUDIO_METADATA_YEAR:
            os_snprintf(buffer, buf_size, "%s", metadata->year);
            break;
        case AUDIO_METADATA_COMPOSER:
            os_snprintf(buffer, buf_size, "%s", metadata->composer);
            break;
        case AUDIO_METADATA_TRACK_NUMBER:
            os_snprintf(buffer, buf_size, "%s", metadata->track_number);
            break;
        case AUDIO_METADATA_DURATION:
            os_snprintf(buffer, buf_size, "%u", (uint32_t)(metadata->duration / 1000));
            break;
        case AUDIO_METADATA_BITRATE:
            os_snprintf(buffer, buf_size, "%d", metadata->bitrate);
            break;
        case AUDIO_METADATA_SAMPLE_RATE:
            os_snprintf(buffer, buf_size, "%d", metadata->sample_rate);
            break;
        case AUDIO_METADATA_CHANNELS:
            os_snprintf(buffer, buf_size, "%d", metadata->channels);
            break;
        default:
            return -2;
    }

    return 0;
}

