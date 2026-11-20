#include "player_osal.h"
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


#include "play_manager.h"
#include "play_sm.h"
#include "plugin_manager.h"
#include "codec_api.h"
#include "linked_list.h"
#include "sink_api.h"

#include <unistd.h>
#include <stdbool.h>
static bool codec_ops_valid(bk_audio_player_handle_t player, bk_audio_player_decoder_ops_t *ops)
{
    list *codec_list;

    if (!ops || !player)
    {
        return false;
    }

    codec_list = audio_codecs_get(player);
    if (!codec_list)
    {
        return false;
    }

    for (int i = 0; i < get_size(codec_list); i++)
    {
        bk_audio_player_decoder_ops_t *registered = (bk_audio_player_decoder_ops_t *)get_index(codec_list, i);
        if (registered == ops)
        {
            return true;
        }
    }

    return false;
}

static void play_sm_notify_seek_result(bk_audio_player_handle_t player, int status, int second)
{
    if (!player)
    {
        return;
    }

    audio_player_seek_result_t result =
    {
        .second = second,
        .status = status,
    };

    if (result.second < 0)
    {
        if (player->last_seek_second >= 0)
        {
            result.second = player->last_seek_second;
        }
        else
        {
            result.second = 0;
        }
    }

    player->seek_position = -1;
    player->seek_in_progress = 0;
    player->seek_deferred_pending = 0;

    if (status == AUDIO_PLAYER_OK)
    {
        player->last_seek_second = result.second;
    }

    bk_audio_player_notify(player, AUDIO_PLAYER_EVENT_SEEK_COMPLETE, &result);
}

static bool sink_ops_valid(bk_audio_player_handle_t player, bk_audio_player_sink_ops_t *ops)
{
    list *sink_list;

    if (!ops || !player)
    {
        return false;
    }

    sink_list = audio_sinks_get(player);
    if (!sink_list)
    {
        return false;
    }

    for (int i = 0; i < get_size(sink_list); i++)
    {
        bk_audio_player_sink_ops_t *registered = (bk_audio_player_sink_ops_t *)get_index(sink_list, i);
        if (registered == ops)
        {
            return true;
        }
    }

    return false;
}


#define MAX_GAIN    128

typedef struct player_priv_s
{
    bk_audio_player_source_t *source;
    bk_audio_player_decoder_t *codec;
    bk_audio_player_sink_t *sink;

    char *buffer;
    int chunk_size;

    audio_info_t info;

    int consumed_bytes;
    int bytes_per_second;

    int spk_gain_internal;
    int gain_step;
} player_priv_t;

static const char *player_state_strs[] =
{
    "",         //STATE_FIRST
    "STOPED",
    "PLAYING",
    "PAUSED",
    "",         //STATE_LAST
};

static const char *get_state_str(int state)
{
    if (state > STATE_FIRST && state < STATE_LAST)
    {
        return player_state_strs[state];
    }
    else
    {
        return "INVALID";
    }
}

static int _play_sm_transfer_state(bk_audio_player_handle_t player, int new_state)
{
    BK_LOGI(AUDIO_PLAYER_TAG, "_play_sm_transfer_state : %s --> %s\n", get_state_str(player->cur_state), get_state_str(new_state));
    player->cur_state = new_state;
    return AUDIO_PLAYER_OK;
}

static int audio_pipeline_setup(bk_audio_player_handle_t player, char *url)
{
    int ret;
    player_priv_t *priv;
    bk_audio_player_source_t *source = NULL;
    bk_audio_player_decoder_t *codec = NULL;
    bk_audio_player_sink_t *sink = NULL;
    char *buffer = NULL;
    int codec_type;
    int chunk_size;
    audio_info_t info;

    BK_LOGI(AUDIO_PLAYER_TAG, "pipeline setup : url %s\n", url);

    priv = (player_priv_t *)player->player_priv;

    if (!url)
    {
        ret = -1;
        goto out;
    }

    if (player->cur_url)
    {
        os_free(player->cur_url);
    }
    player->cur_url = player_strdup(url);

    source = audio_source_open_url(player, url);
    if (!source)
    {
        ret = -2;
        goto out;
    }

    codec_type = audio_source_get_codec_type(source);
    BK_LOGI(AUDIO_PLAYER_TAG, "codec_type :%d\n", codec_type);

    if (codec_type == AUDIO_FORMAT_UNKNOWN)
    {
        ret = -3;
        goto out;
    }

    codec = audio_codec_open(player, codec_type, NULL, source);
    if (!codec)
    {
        ret = -4;
        goto out;
    }

    ret = audio_codec_get_info(codec, &info);
    if (ret)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, get codec info fail, ret=%d, %d \n", __func__, ret, __LINE__);
        ret = -5;
        goto out;
    }
    BK_LOGI(AUDIO_PLAYER_TAG, "channel_number=%d, sample_rate=%d, sample_bits=%d, total_bytes=%d\n",
               info.channel_number, info.sample_rate, info.sample_bits, info.total_bytes);

    chunk_size = audio_codec_get_chunk_size(codec);
    BK_LOGI(AUDIO_PLAYER_TAG, "chunk_size is %d\n", chunk_size);
    //info.frame_size = chunk_size;

    buffer = player_malloc(chunk_size);
    if (!buffer)
    {
        ret = -6;
        goto out;
    }

    if (player->output_to_file)
    {
        sink = audio_sink_open(player, AUDIO_SINK_FILE, player->output_file);
    }
    else
    {
        device_sink_param_t dev_param = { .info = &info, .player = player };
        sink = audio_sink_open(player, AUDIO_SINK_DEVICE, &dev_param);
    }
    if (!sink)
    {
        ret = -7;
        goto out;
    }

    priv->source = source;
    priv->codec = codec;
    priv->sink = sink;

    priv->buffer = buffer;
    priv->chunk_size = chunk_size;

    priv->info = info;

    priv->consumed_bytes = 0;
    priv->bytes_per_second = info.channel_number * info.sample_rate * info.sample_bits / 8;
    if (priv->bytes_per_second == 0)
    {
        priv->bytes_per_second = 96000 * 2 * 2;    //fake 96k
    }

    priv->spk_gain_internal = 0;
    //priv->gain_step = MAX_GAIN * 1000 * chunk_size / (priv->bytes_per_second * 200);  //MAX_GAIN / (bytes_per_second * 200 / 1000 / chunk_size)
    priv->gain_step = MAX_GAIN * 1000 * chunk_size / (priv->bytes_per_second * 200);    //MAX_GAIN / (bytes_per_second * 200 / 1000 / chunk_size)
    if (priv->gain_step == 0)
    {
        priv->gain_step = 1;
    }
    BK_LOGI(AUDIO_PLAYER_TAG, "gain_step is %d\n", priv->gain_step);

    bk_audio_player_notify(player, AUDIO_PLAYER_EVENT_SONG_START, NULL);
    return AUDIO_PLAYER_OK;

out :
    BK_LOGE(AUDIO_PLAYER_TAG, "pipeline setup : failed with ret=%d\n", ret);

    if (codec)
    {
        audio_codec_close(codec);
        codec = NULL;
    }
    if (source)
    {
        audio_source_close(source);
        source = NULL;
    }
    if (sink)
    {
        audio_sink_close(sink);
        sink =NULL;
    }

    if (buffer)
    {
        os_free(buffer);
        buffer = NULL;
    }

    return AUDIO_PLAYER_ERR;
}

static void audio_pipeline_teardown(bk_audio_player_handle_t player)
{
    player_priv_t *priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "pipeline teardown\n");

    player->seek_position = -1;
    player->seek_in_progress = 0;
    player->last_seek_second = -1;

    priv = (player_priv_t *)player->player_priv;

    /* mute audio dac before close audio to avoid "pop" voice */
    audio_sink_control(priv->sink, AUDIO_SINK_MUTE);

    bk_audio_player_decoder_t *codec = priv->codec;
    bk_audio_player_source_t *source = priv->source;
    bk_audio_player_sink_t *sink = priv->sink;

    priv->codec = NULL;
    priv->source = NULL;
    priv->sink = NULL;

    if (codec)
    {
        audio_codec_close(codec);
    }
    if (source)
    {
        audio_source_close(source);
    }
    if (sink)
    {
        audio_sink_close(sink);
    }

    if (priv->buffer)
    {
        os_free(priv->buffer);
    }

    priv->source = NULL;
    priv->codec = NULL;
    priv->sink = NULL;

    priv->buffer = NULL;

    //bk_audio_player_notify(AUDIO_PLAYER_EVENT_SONG_FINISH, NULL);    //move to chunk to distinct done/err
}

static int audio_pipeline_pause(bk_audio_player_handle_t player)
{
    player_priv_t *priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "pipeline pause\n");

    priv = (player_priv_t *)player->player_priv;

    audio_sink_control(priv->sink, AUDIO_SINK_PAUSE);
    bk_audio_player_notify(player, AUDIO_PLAYER_EVENT_SONG_PAUSE, NULL);

    return AUDIO_PLAYER_OK;
}

static int audio_pipeline_resume(bk_audio_player_handle_t player)
{
    player_priv_t *priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "pipeline resume\n");

    priv = (player_priv_t *)player->player_priv;

    audio_sink_control(priv->sink, AUDIO_SINK_RESUME);
    bk_audio_player_notify(player, AUDIO_PLAYER_EVENT_SONG_RESUME, NULL);

    return AUDIO_PLAYER_OK;
}

static int audio_pipeline_frame_info_change(bk_audio_player_handle_t player)
{
    player_priv_t *priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "frame_info change\n");

    priv = (player_priv_t *)player->player_priv;

    int ret = audio_sink_control(priv->sink, AUDIO_SINK_FRAME_INFO_CHANGE);
    if (ret != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, frame info change fail, ret: %d, %d\n", __func__, ret, __LINE__);
        return AUDIO_PLAYER_ERR;
    }

    return AUDIO_PLAYER_OK;
}

int play_sm_init(bk_audio_player_handle_t player)
{
    player_priv_t *priv;

    priv = player_malloc(sizeof(player_priv_t));
    if (!priv)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "can't alloc player priv\n");
        return AUDIO_PLAYER_ERR;
    }

    priv->source = NULL;
    priv->codec = NULL;
    priv->sink = NULL;
    priv->buffer = NULL;

    player->player_priv = priv;

    BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_init ok\n");
    return AUDIO_PLAYER_OK;
}

void play_sm_deinit(bk_audio_player_handle_t player)
{
    if (!player->player_priv)
    {
        return;
    }

    audio_pipeline_teardown(player);    //extra for sure

    player_free(player->player_priv);
    player->player_priv = NULL;

    BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_deinit ok\n");
}

int play_sm_play(bk_audio_player_handle_t player, music_info_t *info)
{
    int ret = AUDIO_PLAYER_INVALID;

    switch (player->cur_state)
    {
        case STATE_STOPED:
_state_stoped:
            ret = audio_pipeline_setup(player, info->url);
            if (ret == AUDIO_PLAYER_OK)
            {
                _play_sm_transfer_state(player, STATE_PLAYING);
                BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_play : play url=%s\n", info->url);
            }
            break;

        case STATE_PLAYING:
_state_playing:
            if (strcmp(player->cur_url, info->url) != 0)
            {
                audio_pipeline_teardown(player);
                _play_sm_transfer_state(player, STATE_STOPED);
                goto _state_stoped;
            }
            else
            {
                ret = AUDIO_PLAYER_OK;
            }
            break;

        case STATE_PAUSED:
            if (strcmp(player->cur_url, info->url) != 0)
            {
                //should change to PLAYING temply ???
                goto _state_playing;
            }
            else
            {
                ret = audio_pipeline_resume(player);
                if (ret == AUDIO_PLAYER_OK)
                {
                    _play_sm_transfer_state(player, STATE_PLAYING);
                    BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_play : resume url=%s\n", info->url);
                }
                else
                {
                    audio_pipeline_teardown(player);
                    _play_sm_transfer_state(player, STATE_STOPED);
                    BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_play : failed to resume url=%s\n", info->url);
                }
            }
            break;
    }

    return ret;
}

int play_sm_stop(bk_audio_player_handle_t player)
{
    int ret = AUDIO_PLAYER_INVALID;

    switch (player->cur_state)
    {
        case STATE_STOPED:
            ret = AUDIO_PLAYER_OK;    //SAME
            break;
        case STATE_PLAYING:
            audio_pipeline_teardown(player);
            ret = _play_sm_transfer_state(player, STATE_STOPED);
            BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_stop : stop url=%s\n", player->cur_url);
            break;
        case STATE_PAUSED:
            //should change to PLAYING temply ???
            audio_pipeline_teardown(player);
            ret = _play_sm_transfer_state(player, STATE_STOPED);
            BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_stop : stop url=%s\n", player->cur_url);
            break;
    }

    return ret;
}

int play_sm_pause(bk_audio_player_handle_t player)
{
    int ret = AUDIO_PLAYER_INVALID;

    switch (player->cur_state)
    {
        case STATE_STOPED:
            ret = AUDIO_PLAYER_INVALID;   //N/A
            break;
        case STATE_PLAYING:
            ret = audio_pipeline_pause(player);
            if (ret == AUDIO_PLAYER_OK)
            {
                _play_sm_transfer_state(player, STATE_PAUSED);
                BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_pause ok\n");
            }
            else
            {
                BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_pause failed\n");
            }
            break;
        case STATE_PAUSED:
            ret = AUDIO_PLAYER_OK;    //SAME
            break;
    }

    return ret;
}

int play_sm_resume(bk_audio_player_handle_t player)
{
    int ret = AUDIO_PLAYER_INVALID;

    switch (player->cur_state)
    {
        case STATE_STOPED:
            ret = AUDIO_PLAYER_INVALID;   //N/A
            break;
        case STATE_PLAYING:
            ret = AUDIO_PLAYER_OK;    //SAME
            break;
        case STATE_PAUSED:
            ret = audio_pipeline_resume(player);
            if (ret == AUDIO_PLAYER_OK)
            {
                _play_sm_transfer_state(player, STATE_PLAYING);
                BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_resume : resume url=%s\n", player->cur_url);
            }
            else
            {
                audio_pipeline_teardown(player);
                _play_sm_transfer_state(player, STATE_STOPED);
                BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_resume : failed to resume url=%s\n", player->cur_url);
            }
            break;
    }

    return ret;
}

int play_sm_set_volume(bk_audio_player_handle_t player, int volume)
{
    player_priv_t *priv = NULL;
    priv = (player_priv_t *)player->player_priv;

    int temp_volume = priv->sink->info.volume;

    audio_sink_set_volume(priv->sink, volume);

    int ret = audio_sink_control(priv->sink, AUDIO_SINK_SET_VOLUME);
    if (ret != 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, set volume fail, ret: %d, %d\n", __func__, ret, __LINE__);
        audio_sink_set_volume(priv->sink, temp_volume);
        return AUDIO_PLAYER_ERR;
    }

    return ret;
}

int play_sm_get_time_pos(bk_audio_player_handle_t player)
{
    player_priv_t *priv;

    if (player->cur_state != STATE_PLAYING && player->cur_state != STATE_PAUSED)
    {
        return 0;
    }

    priv = (player_priv_t *)player->player_priv;

    return priv->consumed_bytes / priv->bytes_per_second;
}

double play_sm_get_total_time(bk_audio_player_handle_t player)
{
    player_priv_t *priv;
    int file_size = 0;
    //int header_size = 0;

    if (player->cur_state != STATE_PLAYING && player->cur_state != STATE_PAUSED)
    {
        return -1;
    }

    priv = (player_priv_t *)player->player_priv;

    /* check whether duration was been caculated */
    if (priv->codec->info.duration == 0)
    {
        file_size = audio_source_get_total_bytes(priv->source);
        if (file_size == 0)
        {
            /* the url is not song in sdcard */
            BK_LOGI(AUDIO_PLAYER_TAG, "the url is not song in sdcard \n");
            return -1;
        }

        /* CBR: total_duration = (total_bytes - header_bytes) / bitrate * 8000 ms */
        BK_LOGI(AUDIO_PLAYER_TAG, "file_size: %d, header_bytes: %d, bps: %d \n", file_size, priv->codec->info.header_bytes, priv->codec->info.bps);
        priv->codec->info.duration = (double)(file_size - priv->codec->info.header_bytes) / (double)priv->codec->info.bps * 8000.0;
    }
    BK_LOGI(AUDIO_PLAYER_TAG, "the play url total time: %f ms\n", priv->codec->info.duration);

    return priv->codec->info.duration;
}

int play_sm_seek(bk_audio_player_handle_t player, int miliseconds)
{
    int ret = AUDIO_PLAYER_INVALID;

    switch (player->cur_state)
    {
        case STATE_STOPED:
            ret = AUDIO_PLAYER_INVALID;   //N/A
            break;
        case STATE_PLAYING:
            break;
        case STATE_PAUSED:
            break;
    }

    if (ret == AUDIO_PLAYER_OK)
    {
        BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_seek %dms\n", miliseconds);
    }
    return ret;
}

static void _play_sm_progress(bk_audio_player_handle_t player)
{
    static int old_second = 0;
    int new_second;

    new_second = play_sm_get_time_pos(player);
    if (old_second != new_second)
    {
        old_second = new_second;
        bk_audio_player_notify(player, AUDIO_PLAYER_EVENT_SONG_TICK, &new_second);
    }
}

static void _audio_fade_in(int *cur_gain, int step, signed short *buffer, int samples)
{
    int gain;
    int i;
    int value;

    gain = *cur_gain;

    for (i = 0; i < samples; i++)
    {
        value = *buffer;
        value = value * gain / MAX_GAIN;
        /*
        if (value > 0x7FFF)
            value = 0x7FFF;
        else if (value < -32768)
            value = -32768;
        */
        *buffer++ = value;
    }

    gain += step;
    if (gain > MAX_GAIN)
    {
        gain = MAX_GAIN;
    }
    *cur_gain = gain;

}

int play_sm_chunk(bk_audio_player_handle_t player)
{
    player_priv_t *priv;
    int ret;
    int len;
    int len2;
    int event = AUDIO_PLAYER_EVENT_LAST;

    //BK_LOGD(AUDIO_PLAYER_TAG, "play_sm_chunk\n");

    if (player->output_to_file)
    {
        osal_usleep(20 * 1000);
    }

    priv = (player_priv_t *)player->player_priv;

    if (!priv || !priv->codec || !priv->source || !priv->sink)
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, codec/source not ready, skip chunk\n", __func__);
        if (player->seek_in_progress || player->seek_position != -1)
        {
            int target = (player->seek_position != -1) ? player->seek_position : player->last_seek_second;
            play_sm_notify_seek_result(player, AUDIO_PLAYER_ERR, target);
        }
        osal_usleep(10 * 1000);
        return CHUNK_CONTINUE;
    }

    if (!codec_ops_valid(player, priv->codec->ops))
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, codec ops invalid, skip chunk\n", __func__);
        if (player->seek_in_progress || player->seek_position != -1)
        {
            int target = (player->seek_position != -1) ? player->seek_position : player->last_seek_second;
            play_sm_notify_seek_result(player, AUDIO_PLAYER_ERR, target);
        }
        priv->codec = NULL;
        osal_usleep(10 * 1000);
        return CHUNK_CONTINUE;
    }

    if (!sink_ops_valid(player, priv->sink->ops))
    {
        BK_LOGW(AUDIO_PLAYER_TAG, "%s, sink ops invalid, skip chunk\n", __func__);
        if (player->seek_in_progress || player->seek_position != -1)
        {
            int target = (player->seek_position != -1) ? player->seek_position : player->last_seek_second;
            play_sm_notify_seek_result(player, AUDIO_PLAYER_ERR, target);
        }
        priv->sink = NULL;
        osal_usleep(10 * 1000);
        return CHUNK_CONTINUE;
    }

    if (player->seek_position != -1)
    {
        int requested_second = player->seek_position;
        int status = AUDIO_PLAYER_ERR;
        int target_second = requested_second;
        bool defer_seek_notify = false;

        if (!priv->codec->ops || !priv->codec->ops->calc_position)
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, codec missing calc_position, drop seek %d\n",
                       __func__, player->seek_position);
        }
        else
        {
            int seek_offset = audio_codec_calc_position(priv->codec, player->seek_position);
            if (seek_offset < 0)
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, calc_position failed for %d\n", __func__, player->seek_position);
                status = seek_offset;
            }
            else
            {
                BK_LOGI(AUDIO_PLAYER_TAG, "%s, seek second:%d, start_offset:%d\n", __func__, requested_second, seek_offset);
                int seek_ret = audio_source_seek(priv->source, seek_offset, SEEK_SET);
                if (seek_ret != AUDIO_PLAYER_OK)
                {
                    BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_source_seek fail, ret:%d\n", __func__, seek_ret);
                    status = seek_ret;
                }
                else
                {
                    target_second = requested_second < 0 ? 0 : requested_second;
                    priv->consumed_bytes = target_second * priv->bytes_per_second;
                    status = AUDIO_PLAYER_OK;
                    defer_seek_notify = true;
                }
            }
        }
        if (status == AUDIO_PLAYER_OK && defer_seek_notify)
        {
            player->seek_position = -1;
            player->seek_deferred_pending = 1;
            player->seek_deferred_status = status;
            player->seek_deferred_second = target_second;
        }
        else
        {
            play_sm_notify_seek_result(player, status, target_second);
        }
    }

    len = audio_codec_get_data(priv->codec, priv->buffer, priv->chunk_size);

    if (player->seek_deferred_pending)
    {
        if (audio_codec_is_seek_ready(priv->codec))
        {
            play_sm_notify_seek_result(player, player->seek_deferred_status, player->seek_deferred_second);
        }
    }
    if (len == AUDIO_PLAYER_TIMEOUT)
    {
        ret = CHUNK_CONTINUE;
        event = AUDIO_PLAYER_EVENT_LAST;
    }
    else if (len < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "play_sm_chunk : codec return = %d\n", len);
        ret = CHUNK_CODEC_ERR;
        event = AUDIO_PLAYER_EVENT_SONG_FAILURE;
        if (player->seek_deferred_pending)
        {
            player->seek_deferred_status = AUDIO_PLAYER_ERR;
            play_sm_notify_seek_result(player, AUDIO_PLAYER_ERR, player->seek_deferred_second);
        }
    }
    else if (len == 0)
    {
        // During seek operation, codec may return 0 to indicate alignment/retry in progress
        // Also, after seek completes, decoder may need time to prepare data
        // Do not treat this as song finished
        bool is_seek_related = false;
        
        if (player->seek_in_progress || player->seek_deferred_pending)
        {
            is_seek_related = true;
        }
        else if (!audio_codec_is_seek_ready(priv->codec))
        {
            is_seek_related = true;
        }
        else if (player->last_seek_second >= 0)
        {
            // After seek completes, decoder may need time to prepare data
            // If last_seek_second is set and decoder returns 0, wait a bit more
            // Check if current position matches seek position (indicating we're at seek point)
            int current_pos = play_sm_get_time_pos(player);
            int seek_pos = player->last_seek_second;
            // If we're at or near the seek position, likely still preparing data after seek
            if (current_pos >= 0 && seek_pos >= 0)
            {
                int diff = (current_pos > seek_pos) ? (current_pos - seek_pos) : (seek_pos - current_pos);
                if (diff <= 2)
                {
                    is_seek_related = true;
                    BK_LOGD(AUDIO_PLAYER_TAG, "play_sm_chunk : len=0 after seek, current_pos=%d, seek_pos=%d, continue waiting\n", 
                              current_pos, seek_pos);
                }
            }
        }

        if (is_seek_related)
        {
            BK_LOGD(AUDIO_PLAYER_TAG, "play_sm_chunk : len=0 during/after seek, continue waiting\n");
            ret = CHUNK_CONTINUE;
            event = AUDIO_PLAYER_EVENT_LAST;
        }
        else
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "play_sm_chunk : done\n");
            ret = CHUNK_DONE;
            event = AUDIO_PLAYER_EVENT_SONG_FINISH;
        }
    }
    else
    {
        if (priv->spk_gain_internal < MAX_GAIN)
        {
            _audio_fade_in(&priv->spk_gain_internal, priv->gain_step, (signed short *)priv->buffer, len * 8 / priv->info.sample_bits);
        }

        /* check and change frame_info */
        if (priv->info.sample_rate != priv->codec->info.sample_rate
            || priv->info.sample_bits != priv->codec->info.sample_bits
            || priv->info.channel_number != priv->codec->info.channel_number)
        {
            if (!sink_ops_valid(player, priv->sink->ops))
            {
                BK_LOGW(AUDIO_PLAYER_TAG, "%s, sink ops invalid during frame change\n", __func__);
                priv->sink = NULL;
                if (player->seek_in_progress)
                {
                    play_sm_notify_seek_result(player, AUDIO_PLAYER_ERR, player->last_seek_second);
                }
                osal_usleep(10 * 1000);
                return CHUNK_CONTINUE;
            }

            audio_sink_set_info(priv->sink, priv->codec->info.sample_rate, priv->codec->info.sample_bits, priv->codec->info.channel_number);
            ret = audio_pipeline_frame_info_change(player);
            if (ret == AUDIO_PLAYER_OK)
            {
                priv->info.sample_rate = priv->codec->info.sample_rate;
                priv->info.sample_bits = priv->codec->info.sample_bits;
                priv->info.channel_number = priv->codec->info.channel_number;
                priv->info.bps = priv->codec->info.bps;
            }
            else
            {
                BK_LOGE(AUDIO_PLAYER_TAG, "%s, audio_pipeline_frame_info_change fail, %d\n", __func__, __LINE__);
            }
        }

        if (!sink_ops_valid(player, priv->sink->ops))
        {
            BK_LOGW(AUDIO_PLAYER_TAG, "%s, sink ops invalid before write\n", __func__);
            priv->sink = NULL;
            if (player->seek_in_progress)
            {
                play_sm_notify_seek_result(player, AUDIO_PLAYER_ERR, player->last_seek_second);
            }
            osal_usleep(10 * 1000);
            return CHUNK_CONTINUE;
        }

        len2 = audio_sink_write_data(priv->sink, priv->buffer, len);
        if (len2 <= 0)
        {
            BK_LOGE(AUDIO_PLAYER_TAG, "play_sm_chunk : sink return = %d\n", len2);
            ret = CHUNK_SINK_ERR;
            event = AUDIO_PLAYER_EVENT_SONG_FAILURE;
        }
        else
        {
            ret = CHUNK_CONTINUE;
            event = AUDIO_PLAYER_EVENT_LAST;
            priv->consumed_bytes += len2;
            _play_sm_progress(player);
        }
    }

    if (ret != CHUNK_CONTINUE)
    {
        play_sm_stop(player);
    }

    if (event != AUDIO_PLAYER_EVENT_LAST)
    {
        bk_audio_player_notify(player, event, NULL);
    }

    return ret;
}


