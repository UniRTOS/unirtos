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


#include<stdarg.h>

#include "player_osal.h"
#include "bk_audio_player_private.h"
#include "play_manager.h"
#include "play_sm.h"
#include "music_list.h"
#include "plugin_manager.h"
#include <os/str.h>
#include <driver/trng.h>


#define CHECK_HANDLE(h)  do { if (!(h)) return AUDIO_PLAYER_NOT_INIT; } while (0)

static int _advance_music_index(bk_audio_player_handle_t player, int direction)
{
    int index;
    int count;

    if (!player)
    {
        return AUDIO_PLAYER_INVALID;
    }

    count = music_list_get_count(player->music_list);
    if (count == 0)
    {
        return AUDIO_PLAYER_INVALID;
    }

    index = player->cur_music_index;
    index = direction > 0 ? index + 1 : index - 1;

    if (index < 0)
    {
        if (player->play_mode == AUDIO_PLAYER_MODE_SEQUENCE_LOOP)
        {
            index = count - 1;  //wrap around
        }
        else
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, the music is the first one, play_mode: %d, %d\n", __func__, player->play_mode, __LINE__);
            return AUDIO_PLAYER_INVALID;
        }
    }

    if (index > count - 1)
    {
        if (player->play_mode == AUDIO_PLAYER_MODE_SEQUENCE_LOOP)
        {
            index = 0;          //wrap around
        }
        else
        {
            BK_LOGI(AUDIO_PLAYER_TAG, "%s, the music is the last one, play_mode: %d, %d\n", __func__, player->play_mode, __LINE__);
            return AUDIO_PLAYER_INVALID;
        }
    }

    player->cur_music_index = index;

    return index;
}

static int _cur_music_index(bk_audio_player_handle_t player)
{
    int count;

    if (!player)
    {
        return AUDIO_PLAYER_INVALID;
    }

    count = music_list_get_count(player->music_list);
    if (count == 0)
    {
        return AUDIO_PLAYER_INVALID;
    }

    return player->cur_music_index;
}

static int _set_music_index(bk_audio_player_handle_t player, int index)
{
    int count;

    if (!player)
    {
        return AUDIO_PLAYER_INVALID;
    }

    count = music_list_get_count(player->music_list);
    if (count == 0)
    {
        return AUDIO_PLAYER_INVALID;
    }

    if (index < 0 || index >= count)
    {
        return AUDIO_PLAYER_INVALID;
    }

    player->cur_music_index = index;
    return index;
}

static int _set_music_index_by_random_number(bk_audio_player_handle_t player)
{
    int count;

    if (!player)
    {
        return AUDIO_PLAYER_INVALID;
    }

    count = music_list_get_count(player->music_list);

    if (count == 0)
    {
        return AUDIO_PLAYER_INVALID;
    }

    int random_num = bk_rand();
    if (random_num < 0)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "%s, get random number: %d fail, %d\n", __func__, random_num, __LINE__);
        return -1;
    }
    else
    {
        int id_num = random_num % count;
        return _set_music_index(player, id_num);
    }
}

static int _play_music_index(bk_audio_player_handle_t player, int index)
{
    int ret;
    music_info_t *info;

    if (!player)
    {
        return AUDIO_PLAYER_ERR;
    }

    if (index >= 0)
    {
        info = music_list_get_by_index(player->music_list, index);
        if (info && info->url)
        {
            ret = play_sm_play(player, info);
        }
        else
        {
            ret = AUDIO_PLAYER_ERR;
        }
    }
    else
    {
        ret = AUDIO_PLAYER_ERR;
    }

    return ret;
}

static int _bk_player_exit(bk_audio_player_handle_t player)
{
    player->running = 0;
    return AUDIO_PLAYER_OK;
}

static const char *event_strs[] =
{
    "SONG_START",
    "SONG_FINISH",
    "SONG_FAILURE",
    "SONG_PAUSE",
    "SONG_RESUME",
    "SONG_TICK",
    "SEEK_COMPLETE",
};

static const char *get_event_str(int event)
{
    const int n = (int)(sizeof(event_strs) / sizeof(event_strs[0]));

    if (event >= 0 && event < n && event < (int)AUDIO_PLAYER_EVENT_LAST)
    {
        return event_strs[event];
    }
    else
    {
        return "INVALID";
    }
}

static int _bk_cmd_dispatch(bk_audio_player_handle_t player, int cmd, void *control);

/* Internal event handler: app callback and auto next/loop; player is passed from notify */
static void _bk_event_handler(bk_audio_player_handle_t player, audio_player_event_type_t event, void *extra_info)
{
    int auto_cmd = CMD_NULL;

    if (!player)
    {
        return;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "got event (%d) : %s\n", event, get_event_str(event));

    if (player->event_handler_app)
    {
        player->event_handler_app(event, extra_info, player->event_handler_args);
    }

    if (event != AUDIO_PLAYER_EVENT_SONG_FINISH && event != AUDIO_PLAYER_EVENT_SONG_FAILURE)
    {
        return;
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "----------------------------------\n");

    switch (player->play_mode)
    {
        case AUDIO_PLAYER_MODE_ONE_SONG:
            break;
        case AUDIO_PLAYER_MODE_SEQUENCE:
            auto_cmd = CMD_NEXT;
            break;
        case AUDIO_PLAYER_MODE_ONE_SONG_LOOP:
            auto_cmd = CMD_PLAY;
            break;
        case AUDIO_PLAYER_MODE_SEQUENCE_LOOP:
            auto_cmd = CMD_NEXT;
            break;
        case AUDIO_PLAYER_MODE_RANDOM:
            _set_music_index_by_random_number(player);
            auto_cmd = CMD_PLAY;
            break;
    }

    if (auto_cmd != CMD_NULL)
    {
        _bk_cmd_dispatch(player, auto_cmd, NULL);
    }
}

static int _bk_cmd_dispatch(bk_audio_player_handle_t player, int cmd, void *control)
{
    int ret = AUDIO_PLAYER_ERR;

    if (!player)
    {
        return ret;
    }

    switch (cmd)
    {
        case CMD_PLAY:
        {
            if (player->play_mode == AUDIO_PLAYER_MODE_RANDOM)
            {
                _set_music_index_by_random_number(player);
            }
            ret = _play_music_index(player, _cur_music_index(player));
        }
        break;

        case CMD_STOP:
            ret = play_sm_stop(player);
            break;

        case CMD_PAUSE:
            ret = play_sm_pause(player);
            break;

        case CMD_RESUME:
            ret = play_sm_resume(player);
            break;

        case CMD_PREV:
        {
            int index = _advance_music_index(player, -1);
            ret = _play_music_index(player, index);
        }
        break;

        case CMD_NEXT:
        {
            int index = _advance_music_index(player, 1);
            ret = _play_music_index(player, index);
        }
        break;

        case CMD_JUMP:
        {
            int index = *((int *)control);
            index = _set_music_index(player, index);
            ret = _play_music_index(player, index);
        }
        break;

        case CMD_SEEK:
        {
            int miliseconds = *((int *)control);
            ret = play_sm_seek(player, miliseconds);
        }
        break;

        case CMD_EXIT:
            ret = _bk_player_exit(player);
            break;

        case CMD_LIST_SET:
            break;

        default:
            ret = AUDIO_PLAYER_ERR;
            break;
    }

    return ret;
}

static int enqueue_cmd(bk_audio_player_handle_t player, int cmd, void *control)
{
    player->cur_cmd = cmd;
    if (control)
    {
        player->cur_cmd_option = (*((int *)control));    // TODO : support real struct
    }
    osal_post_sema(&player->sem);
    return AUDIO_PLAYER_OK;
}

static int dequeue_cmd(bk_audio_player_handle_t player, int timeout, int *cmd_option)
{
    int ret;
    int cmd;

    ret = osal_wait_sema(&player->sem, timeout);
    if (ret == 0)
    {
        cmd = player->cur_cmd;
        *cmd_option = player->cur_cmd_option;
        player->cur_cmd = CMD_NULL;
    }
    else
    {
        cmd = CMD_NULL;
    }

    return cmd;
}

static void notify_cmd_finish(bk_audio_player_handle_t player, int result)
{
    player->cmd_result = result;
    osal_post_sema(&player->sem_2);
}

static int wait_cmd_finish(bk_audio_player_handle_t player)
{
    int ret;
    int result;

    ret = osal_wait_sema(&player->sem_2, -1);
    if (ret == 0)
    {
        result = player->cmd_result;
        player->cmd_result = AUDIO_PLAYER_ERR;
    }
    else
    {
        result = AUDIO_PLAYER_ERR;
    }

    return result;
}

static int _bk_cmd_handler(bk_audio_player_handle_t player, int cmd, void *control)
{
    int ret = AUDIO_PLAYER_ERR;

    if (!player)
    {
        return AUDIO_PLAYER_NOT_INIT;
    }

    osal_lock_mutex(&player->mutex);
    ret = enqueue_cmd(player, cmd, control);
    if (ret)
    {
        goto out;
    }

    ret = wait_cmd_finish(player);

out:
    osal_unlock_mutex(&player->mutex);
    return ret;
}

static int get_state(bk_audio_player_handle_t player)
{
    return player->cur_state;
}


static void _bk_player_loop(void *param)
{
    bk_audio_player_handle_t player;
    int cmd;
    int cmd_option;
    int ret;

    BK_LOGI(AUDIO_PLAYER_TAG, "_bk_player_loop enter\n");

    player = (bk_audio_player_handle_t)param;
    player->running = 1;

    while (player->running)
    {
        cmd = dequeue_cmd(player, -1, &cmd_option);
        if (cmd != CMD_NULL)
        {
            ret = _bk_cmd_dispatch(player, cmd, &cmd_option);
            notify_cmd_finish(player, ret);
        }
        if (get_state(player) != STATE_PLAYING)
        {
            continue;
        }

        while (player->running)
        {
            ret = play_sm_chunk(player);
            if (ret == CHUNK_CONTINUE)
            {
            }
            else if (ret == CHUNK_DONE)
            {
            }
            else
            {
            }

            cmd = dequeue_cmd(player, 0, &cmd_option);
            if (cmd != CMD_NULL)
            {
                ret = _bk_cmd_dispatch(player, cmd, &cmd_option);
                notify_cmd_finish(player, ret);
            }
            if (get_state(player) != STATE_PLAYING)
            {
                break;
            }
        }
    }

    BK_LOGI(AUDIO_PLAYER_TAG, "_bk_player_loop exit\n");

    osal_post_sema(&player->thread_sem);

    osal_delete_thread(NULL);
}

static int _bk_player_start_worker(bk_audio_player_handle_t player)
{
    osal_thread_t tid;
    int ret;

    ret = osal_create_thread(&tid, _bk_player_loop, player->task_stack, "player", player, player->task_prio);
    return ret;
}

static int _bk_player_stop_worker(bk_audio_player_handle_t player)
{
    if (!player)
    {
        return AUDIO_PLAYER_NOT_INIT;
    }
    return _bk_cmd_handler(player, CMD_EXIT, NULL);
}

int bk_audio_player_new(bk_audio_player_handle_t *handle, bk_audio_player_cfg_t *cfg)
{
    int ret;
    bk_audio_player_handle_t player;

    if (!handle || !cfg)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "invalid handle or cfg parameter\n");
        return AUDIO_PLAYER_INVALID;
    }

    *handle = NULL;

    player = player_malloc(sizeof(struct bk_audio_player));
    if (!player)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "can't alloc player\n");
        return AUDIO_PLAYER_ERR;
    }

    player->cur_state = STATE_STOPED;
    player->music_list = music_list_new();
    if (!player->music_list)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "can't alloc music list\n");
        player_free(player);
        player = NULL;
        return AUDIO_PLAYER_ERR;
    }
    player->play_mode = AUDIO_PLAYER_MODE_DEFAULT;
    player->cur_music_index = 0;
    player->cur_url = NULL;

    player->running = 0;

    osal_init_mutex(&player->mutex);
    player->cur_cmd = CMD_NULL;
    player->cur_cmd_option = 0;
    player->cmd_result = AUDIO_PLAYER_ERR;

    osal_init_sema(&player->sem, 1, 0);
    osal_init_sema(&player->sem_2, 1, 0);

    player->event_handler_app = cfg->event_handler;
    player->event_handler_args = cfg->args;

    player->task_stack = cfg->task_stack ? cfg->task_stack : AUDIO_PLAYER_DEFAULT_TASK_STACK;
    player->task_prio = cfg->task_prio;

    player->output_to_file = 0;
    player->output_file = NULL;

    player->spk_gain = 25;

    player->seek_position = -1;
    player->seek_in_progress = 0;
    player->last_seek_second = -1;
    player->seek_deferred_pending = 0;
    player->seek_deferred_status = AUDIO_PLAYER_ERR;
    player->seek_deferred_second = 0;

    player->player_priv = NULL;
    player->source_list = NULL;
    player->codec_list = NULL;
    player->sink_list = NULL;
    player->parser_list = NULL;
    player->parser_initialized = 0;

    ret = play_sm_init(player);
    if (ret)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "can't init player sm\n");
        player_free(player);
        player = NULL;
        return AUDIO_PLAYER_ERR;
    }

    ret = plugin_init(player);
    if (ret)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "can't init plugins\n");
        play_sm_deinit(player);
        player_free(player);
        player = NULL;
        return AUDIO_PLAYER_ERR;
    }

    osal_init_sema(&player->thread_sem, 1, 0);
    ret = _bk_player_start_worker(player);
    if (ret)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "can't start player loop\n");
        plugin_deinit(player);
        play_sm_deinit(player);
        player_free(player);
        player = NULL;
        return AUDIO_PLAYER_ERR;
    }

    *handle = (bk_audio_player_handle_t)player;

    BK_LOGI(AUDIO_PLAYER_TAG, "bk_audio_player_new ok\n");
    return AUDIO_PLAYER_OK;
}

int bk_audio_player_delete(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    if (player == NULL)
    {
        return AUDIO_PLAYER_INVALID;
    }

    _bk_player_stop_worker(player);
    osal_wait_sema(&player->thread_sem, -1);
    osal_deinit_sema(&player->thread_sem);

    plugin_deinit(player);

    if (player->player_priv)
    {
        play_sm_deinit(player);
        player->player_priv = NULL;
    }

    osal_deinit_mutex(&player->mutex);
    osal_deinit_sema(&player->sem);
    osal_deinit_sema(&player->sem_2);

    if (player->music_list)
    {
        music_list_free(player->music_list);
        player->music_list = NULL;
    }
    if (player->cur_url)
    {
        player_free(player->cur_url);
        player->cur_url = NULL;
    }
    if (player->output_file)
    {
        player_free(player->output_file);
        player->output_file = NULL;
    }

    player_free(player);

    BK_LOGI(AUDIO_PLAYER_TAG, "bk_audio_player_delete ok\n");
    return AUDIO_PLAYER_OK;
}

int bk_audio_player_set_play_mode(bk_audio_player_handle_t handle, audio_player_mode_t mode)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    if (mode < AUDIO_PLAYER_MODE_ONE_SONG || mode > AUDIO_PLAYER_MODE_RANDOM)
    {
        return AUDIO_PLAYER_INVALID;
    }

    player->play_mode = mode;
    return AUDIO_PLAYER_OK;
}

int bk_audio_player_set_volume(bk_audio_player_handle_t handle, int volume)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    if (volume < 0 || volume > 100)
    {
        return AUDIO_PLAYER_INVALID;
    }


    player->spk_gain = volume;
    volume = volume * 63 / 100;
    bk_printf("volume :%d\r\n", volume);

    play_sm_set_volume(player, volume);

    return AUDIO_PLAYER_OK;
}

int bk_audio_player_get_volume(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return player->spk_gain;
}

double bk_audio_player_get_duration(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    if (!player)
    {
        BK_LOGE(AUDIO_PLAYER_TAG, "Player not initialized\n");
        return 0.0;
    }

    return play_sm_get_total_time(player);
}

int bk_player_output_device(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);

    player->output_to_file = 0;
    if (player->output_file)
    {
        player_free(player->output_file);
        player->output_file = NULL;
    }
    return AUDIO_PLAYER_OK;
}

int bk_player_output_file(bk_audio_player_handle_t handle, char *file)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    if (!file)
    {
        return AUDIO_PLAYER_ERR;
    }

    player->output_to_file = 1;
    if (player->output_file)
    {
        player_free(player->output_file);
        player->output_file = NULL;
    }
    player->output_file = player_strdup(file);
    return AUDIO_PLAYER_OK;
}

//int bk_player_set_music_list(music_list_t *list) {}

int bk_audio_player_clear_music_list(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return music_list_clear(player->music_list);
}

int bk_audio_player_add_music(bk_audio_player_handle_t handle, char *name, char *uri)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return music_list_add(player->music_list, name, uri);
}

int bk_audio_player_del_music_by_name(bk_audio_player_handle_t handle, char *name)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return music_list_rm_by_name(player->music_list, name);
}

int bk_audio_player_del_music_by_uri(bk_audio_player_handle_t handle, char *uri)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return music_list_rm_by_url(player->music_list, uri);
}

int bk_audio_player_dump_music_list(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return music_list_dump(player->music_list);
}

int bk_audio_player_start(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return _bk_cmd_handler(player, CMD_PLAY, NULL);
}

int bk_player_play_position(bk_audio_player_handle_t handle, int miliseconds)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;
    if (!player)
    {
        return AUDIO_PLAYER_NOT_INIT;
    }

    if (miliseconds < 0)
    {
        miliseconds = 0;
    }

    if (player->seek_in_progress || player->seek_position != -1)
    {
        return AUDIO_PLAYER_PROGRESS;
    }

    player->seek_in_progress = 1;
    player->seek_position = miliseconds;

    return _bk_cmd_handler(player, CMD_PLAY, NULL);
}

int bk_audio_player_stop(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return _bk_cmd_handler(player, CMD_STOP, NULL);
}

int bk_audio_player_pause(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return _bk_cmd_handler(player, CMD_PAUSE, NULL);
}

int bk_audio_player_resume(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return _bk_cmd_handler(player, CMD_RESUME, NULL);
}

int bk_audio_player_prev(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return _bk_cmd_handler(player, CMD_PREV, NULL);
}

int bk_audio_player_next(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return _bk_cmd_handler(player, CMD_NEXT, NULL);
}

int bk_audio_player_jumpto(bk_audio_player_handle_t handle, int idx)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    CHECK_HANDLE(handle);
    return _bk_cmd_handler(player, CMD_JUMP, &idx);
}

int bk_player_get_time_pos(bk_audio_player_handle_t handle)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;

    if (!player)
    {
        return 0;
    }

    return play_sm_get_time_pos(player);
}

int bk_audio_player_seek(bk_audio_player_handle_t handle, int second)
{
    bk_audio_player_handle_t player = (bk_audio_player_handle_t)handle;
    if (!player)
    {
        return AUDIO_PLAYER_NOT_INIT;
    }

    if (second < 0)
    {
        second = 0;
    }

    if (player->seek_in_progress || player->seek_position != -1)
    {
        return AUDIO_PLAYER_PROGRESS;
    }

    player->seek_in_progress = 1;
    player->seek_position = second;
    return AUDIO_PLAYER_OK;
}

void bk_audio_player_notify(bk_audio_player_handle_t player, audio_player_event_type_t event, void *extra_info)
{
    if (!player)
    {
        return;
    }
    _bk_event_handler(player, event, extra_info);
}
