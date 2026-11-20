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


#include "plugin_manager.h"
#include "player_osal.h"

struct codec_mapping
{
    char *ext_name;
    int codec_type;
};

static const struct codec_mapping codecs[] =
{
    {".wav",  AUDIO_FORMAT_WAV},
    {".mp3",  AUDIO_FORMAT_MP3},
    {".aac",  AUDIO_FORMAT_AAC},
    {".m4a",  AUDIO_FORMAT_M4A},
    {".ts",   AUDIO_FORMAT_TS},
    {".amr",  AUDIO_FORMAT_AMR},
    {".flac", AUDIO_FORMAT_FLAC},
    {".opus", AUDIO_FORMAT_OPUS},
    {".ogg",  AUDIO_FORMAT_OGG},
};

#define CODECS_CNT sizeof(codecs)/sizeof(codecs[0])

int audio_codec_get_type(char *ext_name)
{
    int i;
    int ret = AUDIO_FORMAT_UNKNOWN;

    if (!ext_name)
    {
        return ret;
    }

    for (i = 0; i < CODECS_CNT; i++)
    {
        if (strcasecmp(ext_name, codecs[i].ext_name) == 0)
        {
            ret = codecs[i].codec_type;
            break;
        }
    }

    return ret;
}

struct mime_mapping
{
    char *mime;
    int codec_type;
};

static const struct mime_mapping mimes[] =
{
    {"audio/wav",  AUDIO_FORMAT_WAV},
    {"audio/mp3",  AUDIO_FORMAT_MP3},
    {"audio/flac", AUDIO_FORMAT_FLAC},
    {"audio/ogg",  AUDIO_FORMAT_OGG},
    {"audio/opus", AUDIO_FORMAT_OPUS},
    {"audio/aac",  AUDIO_FORMAT_AAC},
    {"audio/mpeg", AUDIO_FORMAT_MP3},
    /* special format */
    {"video/MP2T", AUDIO_FORMAT_TS},
};

#define MIMES_CNT sizeof(mimes)/sizeof(mimes[0])

int audio_codec_get_mime_type(char *mime)
{
    int i;
    int ret = AUDIO_FORMAT_UNKNOWN;

    if (!mime)
    {
        return ret;
    }

    for (i = 0; i < MIMES_CNT; i++)
    {
        if (strcasecmp(mime, mimes[i].mime) == 0)
        {
            ret = mimes[i].codec_type;
            break;
        }
    }

    return ret;
}

bk_audio_player_decoder_t *audio_codec_new(int priv_size)
{
    bk_audio_player_decoder_t *decoder;

    decoder = player_malloc(sizeof(bk_audio_player_decoder_t) + priv_size);
    if (!decoder)
    {
        return NULL;
    }
    decoder->decoder_priv = decoder + 1;
    return decoder;
}

bk_audio_player_decoder_t *audio_codec_open(bk_audio_player_handle_t player, audio_format_t codec_type, void *param, bk_audio_player_source_t *source)
{
    list *llist;
    int count;
    bk_audio_player_decoder_ops_t *ops;
    bk_audio_player_decoder_t *decoder = NULL;
    int i;

    if (!player)
    {
        return NULL;
    }

    llist = audio_codecs_get(player);
    if (!llist)
    {
        return NULL;
    }

    count = get_size(llist);
    for (i = 0; i < count; i++)
    {
        ops = get_index(llist, i);
        if (ops && ops->open(codec_type, param, &decoder) == AUDIO_PLAYER_OK)
        {
            decoder->ops = ops;
            decoder->source = source;
            break;
        }
    }
    return decoder;
}

int audio_codec_close(bk_audio_player_decoder_t *decoder)
{
    int ret;

    if (decoder && decoder->ops->close)
    {
        ret = decoder->ops->close(decoder);
    }
    else
    {
        ret = AUDIO_PLAYER_OK;
    }

    player_free(decoder);

    return ret;
}

int audio_codec_get_info(bk_audio_player_decoder_t *decoder, audio_info_t *info)
{
    if (!decoder || !info)
    {
        return AUDIO_PLAYER_ERR;
    }

    os_memset(info, 0, sizeof(audio_info_t));
    return decoder->ops->get_info(decoder, info);
}

int audio_codec_get_chunk_size(bk_audio_player_decoder_t *decoder)
{
    if (decoder && decoder->ops->get_chunk_size)
    {
        return decoder->ops->get_chunk_size(decoder);
    }
    else
    {
        return DEFAULT_CHUNK_SIZE;
    }
}

int audio_codec_get_data(bk_audio_player_decoder_t *decoder, char *buffer, int len)
{
    if (!decoder || !buffer || !len)
    {
        return AUDIO_PLAYER_ERR;
    }

    return decoder->ops->get_data(decoder, buffer, len);
}


int audio_codec_calc_position(bk_audio_player_decoder_t *decoder, int second)
{
    if (!decoder)
    {
        return AUDIO_PLAYER_ERR;
    }

    return decoder->ops->calc_position(decoder, second);
}

int audio_codec_is_seek_ready(bk_audio_player_decoder_t *decoder)
{
    if (!decoder)
    {
        return 1;
    }

    if (decoder->ops->is_seek_ready)
    {
        return decoder->ops->is_seek_ready(decoder);
    }

    return 1;
}

