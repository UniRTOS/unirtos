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


#ifndef __PLAY_MANAGER_H__
#define __PLAY_MANAGER_H__

#include <components/bk_audio_player/bk_audio_player.h>
#include "bk_audio_player_private.h"
#include "music_list.h"
#include "linked_list.h"
#include "player_osal.h"

//union player_control
typedef int (*cmd_handler_func)(int cmd, void *control);

enum PLAYER_STATE
{
    STATE_FIRST,

    STATE_STOPED,
    STATE_PLAYING,
    STATE_PAUSED,

    STATE_LAST,
};

struct bk_audio_player
{
    int cur_state;

    music_list_t *music_list;
    audio_player_mode_t play_mode;
    int cur_music_index;
    char *cur_url;

    int running;
    osal_sema_t thread_sem;

    osal_mutex_t mutex;
    int cur_cmd;
    int cur_cmd_option;
    int cmd_result;
    osal_sema_t sem;
    osal_sema_t sem_2;

    cmd_handler_func cmd_handler;
    audio_player_event_handler_func event_handler;
    audio_player_event_handler_func event_handler_app;
    void *event_handler_args;  /* User defined arguments for event handler */

    int output_to_file;
    char *output_file;

    int spk_gain;
    int seek_position;
    int seek_in_progress;
    int last_seek_second;
    int seek_deferred_pending;
    int seek_deferred_status;
    int seek_deferred_second;

    void *player_priv;

    /* Per-instance plugin lists; no global plugin state */
    list *source_list;
    list *codec_list;
    list *sink_list;
    list *parser_list;
    int parser_initialized;

    uint32_t task_stack;  /* Stack size for internal player task (from cfg) */
    int task_prio;        /* Priority for internal player task (from cfg) */
};

enum PLAYER_COMMAND
{
    CMD_NULL,

    CMD_PLAY,
    CMD_STOP,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_PREV,
    CMD_NEXT,
    CMD_JUMP,

    CMD_SEEK,

    CMD_EXIT,

    CMD_LIST_SET,
    CMD_LIST_CLEAR,
    CMD_LIST_ADD,
    CMD_LIST_RM_NAME,
    CMD_LIST_RM_URL,
};

#endif
