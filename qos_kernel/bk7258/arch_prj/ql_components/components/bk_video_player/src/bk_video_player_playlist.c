// Copyright 2020-2021 Beken
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

#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_playlist.h"
#include "components/bk_video_player/bk_video_player_engine.h"
#include "bk_video_player_file_list.h"

#define TAG "video_player_playlist"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Playlist player structure (wraps the engine and adds playlist management)
struct bk_video_player_playlist
{
    bk_video_player_engine_handle_t engine;      // Engine handle (single-file playback)
    video_player_file_list_t file_list;          // Playlist
    bk_video_player_playlist_play_mode_t play_mode; // Play mode (STOP/REPEAT/LOOP)

    // Optional: reset audio output path when switching tracks (next/prev/play).
    video_player_audio_output_reset_cb_t audio_output_reset_cb;
    void *audio_output_reset_user_data;
};

static void bk_video_player_playlist_prepare_track_switch(bk_video_player_playlist_handle_t pl)
{
    if (pl == NULL || pl->engine == NULL)
    {
        return;
    }

    // Best-effort stop before switching.
    (void)bk_video_player_engine_stop(pl->engine);

    // Best-effort reset audio output to drop stale buffered audio.
    if (pl->audio_output_reset_cb != NULL)
    {
        avdk_err_t ret = pl->audio_output_reset_cb(pl->audio_output_reset_user_data);
        if (ret != AVDK_ERR_OK)
        {
            LOGW("%s: audio_output_reset_cb failed, ret=%d\n", __func__, ret);
        }
    }
}

static avdk_err_t bk_video_player_playlist_play_internal(bk_video_player_playlist_handle_t pl, const char *file_path)
{
    AVDK_RETURN_ON_FALSE(pl, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(pl->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");

    const char *path_to_play = file_path;
    if (path_to_play == NULL || path_to_play[0] == '\0')
    {
        video_player_file_node_t *current = file_list_get_current(&pl->file_list);
        if (current == NULL || current->file_path == NULL)
        {
            LOGE("%s: No current file in playlist\n", __func__);
            return AVDK_ERR_NODEV;
        }
        path_to_play = current->file_path;
    }
    else
    {
        // Add to playlist if not already present, and set as current.
        // Ignore add failure if already exists; we still try to set current.
        (void)file_list_add(&pl->file_list, path_to_play);
        (void)file_list_set_current(&pl->file_list, path_to_play);
    }

    // Validate file existence before starting playback.
    avdk_err_t ret = file_list_check_file_valid(path_to_play);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    ret = bk_video_player_engine_set_file_path(pl->engine, path_to_play);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    return bk_video_player_engine_play(pl->engine);
}

static void bk_video_player_playlist_finished_cb(void *user_data, const char *file_path)
{
    bk_video_player_playlist_handle_t pl = (bk_video_player_playlist_handle_t)user_data;
    if (pl == NULL || pl->engine == NULL)
    {
        return;
    }

    LOGI("%s: playback finished, file=%s, action=%d\n",
         __func__,
         (file_path != NULL) ? file_path : "(null)",
         (int)pl->play_mode);

    switch (pl->play_mode)
    {
        case BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_REPEAT:
            (void)bk_video_player_playlist_play_file(pl, NULL);
            break;
        case BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_LOOP:
        {
            // Loop playlist: play next if possible, otherwise wrap to the first item.
            bool has_next = false;
            rtos_lock_mutex(&pl->file_list.mutex);
            if (pl->file_list.current != NULL && pl->file_list.current->next != NULL)
            {
                has_next = true;
            }
            rtos_unlock_mutex(&pl->file_list.mutex);

            if (has_next)
            {
                (void)bk_video_player_playlist_play_next(pl);
            }
            else
            {
                (void)bk_video_player_playlist_play_at_index(pl, 0);
            }
            break;
        }
        case BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_STOP:
        default:
            break;
    }
}

avdk_err_t bk_video_player_playlist_new(bk_video_player_playlist_handle_t *handle, bk_video_player_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    bk_video_player_playlist_handle_t pl = (bk_video_player_playlist_handle_t)os_malloc(sizeof(struct bk_video_player_playlist));
    AVDK_RETURN_ON_FALSE(pl, AVDK_ERR_NOMEM, TAG, "Failed to allocate playlist handle");
    os_memset(pl, 0, sizeof(struct bk_video_player_playlist));

    avdk_err_t ret = file_list_init(&pl->file_list);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: file_list_init failed, ret=%d\n", __func__, ret);
        os_free(pl);
        return ret;
    }

    // Install playlist-layer finished callback so playlist can decide play mode.
    bk_video_player_config_t engine_cfg;
    os_memcpy(&engine_cfg, config, sizeof(engine_cfg));
    engine_cfg.playback_finished_cb = bk_video_player_playlist_finished_cb;
    engine_cfg.playback_finished_user_data = pl;

    ret = bk_video_player_engine_new(&pl->engine, &engine_cfg);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: engine_new failed, ret=%d\n", __func__, ret);
        file_list_deinit(&pl->file_list);
        os_free(pl);
        return ret;
    }

    pl->play_mode = BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_STOP;
    pl->audio_output_reset_cb = config->audio.audio_output_reset_cb;
    pl->audio_output_reset_user_data = (config->audio.audio_output_reset_user_data != NULL) ?
                                       config->audio.audio_output_reset_user_data :
                                       config->user_data;

    *handle = pl;
    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_playlist_delete(bk_video_player_playlist_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    if (handle->engine != NULL)
    {
        bk_video_player_engine_delete(handle->engine);
        handle->engine = NULL;
    }

    file_list_deinit(&handle->file_list);
    os_free(handle);
    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_playlist_open(bk_video_player_playlist_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_open(handle->engine);
}

avdk_err_t bk_video_player_playlist_close(bk_video_player_playlist_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_close(handle->engine);
}

avdk_err_t bk_video_player_playlist_add_file(bk_video_player_playlist_handle_t handle, const char *file_path)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");

    avdk_err_t ret = file_list_check_file_valid(file_path);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    bool was_empty = false;
    rtos_lock_mutex(&handle->file_list.mutex);
    was_empty = (handle->file_list.head == NULL);
    rtos_unlock_mutex(&handle->file_list.mutex);

    ret = file_list_add(&handle->file_list, file_path);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    if (was_empty)
    {
        (void)file_list_set_current(&handle->file_list, file_path);
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_playlist_remove_file(bk_video_player_playlist_handle_t handle, const char *file_path)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");
    return file_list_remove(&handle->file_list, file_path);
}

avdk_err_t bk_video_player_playlist_clear(bk_video_player_playlist_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    file_list_clear(&handle->file_list);
    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_playlist_get_file_count(bk_video_player_playlist_handle_t handle, uint32_t *count)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(count, AVDK_ERR_INVAL, TAG, "count is NULL");

    rtos_lock_mutex(&handle->file_list.mutex);
    uint32_t file_count = 0;
    video_player_file_node_t *node = handle->file_list.head;
    while (node != NULL)
    {
        file_count++;
        node = node->next;
    }
    rtos_unlock_mutex(&handle->file_list.mutex);

    *count = file_count;
    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_playlist_get_current_file(bk_video_player_playlist_handle_t handle,
                                                     char *file_path,
                                                     uint32_t file_path_size,
                                                     uint32_t *index)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");
    AVDK_RETURN_ON_FALSE(file_path_size > 0, AVDK_ERR_INVAL, TAG, "file_path_size is 0");

    rtos_lock_mutex(&handle->file_list.mutex);

    video_player_file_node_t *current = handle->file_list.current;
    if (current == NULL || current->file_path == NULL)
    {
        rtos_unlock_mutex(&handle->file_list.mutex);
        return AVDK_ERR_NODEV;
    }

    if (index != NULL)
    {
        uint32_t idx = 0;
        video_player_file_node_t *node = handle->file_list.head;
        while (node != NULL && node != current)
        {
            idx++;
            node = node->next;
        }
        if (node == NULL)
        {
            rtos_unlock_mutex(&handle->file_list.mutex);
            return AVDK_ERR_NODEV;
        }
        *index = idx;
    }

    size_t path_len = os_strlen(current->file_path);
    if (path_len >= file_path_size)
    {
        rtos_unlock_mutex(&handle->file_list.mutex);
        return AVDK_ERR_INVAL;
    }

    os_strncpy(file_path, current->file_path, file_path_size - 1);
    file_path[file_path_size - 1] = '\0';

    rtos_unlock_mutex(&handle->file_list.mutex);
    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_playlist_play_file(bk_video_player_playlist_handle_t handle, const char *file_path)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");

    bk_video_player_playlist_prepare_track_switch(handle);
    return bk_video_player_playlist_play_internal(handle, file_path);
}

avdk_err_t bk_video_player_playlist_stop(bk_video_player_playlist_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_stop(handle->engine);
}

avdk_err_t bk_video_player_playlist_set_pause(bk_video_player_playlist_handle_t handle, bool pause)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_set_pause(handle->engine, pause);
}

avdk_err_t bk_video_player_playlist_seek(bk_video_player_playlist_handle_t handle, uint64_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_seek(handle->engine, time_ms);
}

avdk_err_t bk_video_player_playlist_fast_forward(bk_video_player_playlist_handle_t handle, uint32_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_fast_forward(handle->engine, time_ms);
}

avdk_err_t bk_video_player_playlist_rewind(bk_video_player_playlist_handle_t handle, uint32_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_rewind(handle->engine, time_ms);
}

avdk_err_t bk_video_player_playlist_set_volume(bk_video_player_playlist_handle_t handle, uint8_t volume)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_set_volume(handle->engine, volume);
}

avdk_err_t bk_video_player_playlist_volume_up(bk_video_player_playlist_handle_t handle, uint8_t step)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_volume_up(handle->engine, step);
}

avdk_err_t bk_video_player_playlist_volume_down(bk_video_player_playlist_handle_t handle, uint8_t step)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_volume_down(handle->engine, step);
}

avdk_err_t bk_video_player_playlist_set_mute(bk_video_player_playlist_handle_t handle, bool mute)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_set_mute(handle->engine, mute);
}

avdk_err_t bk_video_player_playlist_set_av_sync_offset_ms(bk_video_player_playlist_handle_t handle, int32_t offset_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_set_av_sync_offset_ms(handle->engine, offset_ms);
}

avdk_err_t bk_video_player_playlist_get_media_info(bk_video_player_playlist_handle_t handle,
                                                   const char *file_path,
                                                   video_player_media_info_t *media_info)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    AVDK_RETURN_ON_FALSE(media_info, AVDK_ERR_INVAL, TAG, "media_info is NULL");
    return bk_video_player_engine_get_media_info(handle->engine, file_path, media_info);
}

avdk_err_t bk_video_player_playlist_play_next(bk_video_player_playlist_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");

    video_player_file_node_t *next_file = file_list_find_next(&handle->file_list);
    if (next_file == NULL || next_file->file_path == NULL)
    {
        LOGE("%s: No next file in playlist\n", __func__);
        return AVDK_ERR_NODEV;
    }

    bk_video_player_playlist_prepare_track_switch(handle);

    avdk_err_t ret = bk_video_player_engine_set_file_path(handle->engine, next_file->file_path);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    return bk_video_player_engine_play(handle->engine);
}

avdk_err_t bk_video_player_playlist_play_prev(bk_video_player_playlist_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");

    video_player_file_node_t *prev_file = file_list_find_prev(&handle->file_list);
    if (prev_file == NULL || prev_file->file_path == NULL)
    {
        LOGE("%s: No previous file in playlist\n", __func__);
        return AVDK_ERR_NODEV;
    }

    bk_video_player_playlist_prepare_track_switch(handle);

    avdk_err_t ret = bk_video_player_engine_set_file_path(handle->engine, prev_file->file_path);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    return bk_video_player_engine_play(handle->engine);
}

avdk_err_t bk_video_player_playlist_play_at_index(bk_video_player_playlist_handle_t handle, uint32_t index)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");

    rtos_lock_mutex(&handle->file_list.mutex);

    video_player_file_node_t *node = handle->file_list.head;
    uint32_t cur = 0;
    if (node == NULL)
    {
        rtos_unlock_mutex(&handle->file_list.mutex);
        LOGE("%s: Playlist is empty\n", __func__);
        return AVDK_ERR_NODEV;
    }

    while (node != NULL && cur < index)
    {
        node = node->next;
        cur++;
    }

    if (node == NULL)
    {
        node = handle->file_list.tail;
    }

    if (node == NULL || node->file_path == NULL)
    {
        rtos_unlock_mutex(&handle->file_list.mutex);
        return AVDK_ERR_NODEV;
    }

    handle->file_list.current = node;
    const char *file_path = node->file_path;

    rtos_unlock_mutex(&handle->file_list.mutex);

    bk_video_player_playlist_prepare_track_switch(handle);

    avdk_err_t ret = bk_video_player_engine_set_file_path(handle->engine, file_path);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }
    return bk_video_player_engine_play(handle->engine);
}

avdk_err_t bk_video_player_playlist_set_play_mode(bk_video_player_playlist_handle_t handle,
                                                  bk_video_player_playlist_play_mode_t mode)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(mode <= BK_VIDEO_PLAYER_PLAYLIST_PLAY_MODE_LOOP, AVDK_ERR_INVAL, TAG, "invalid play mode");
    handle->play_mode = mode;
    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_playlist_register_audio_decoder(bk_video_player_playlist_handle_t handle,
                                                           const video_player_audio_decoder_ops_t *decoder_ops)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_register_audio_decoder(handle->engine, decoder_ops);
}

avdk_err_t bk_video_player_playlist_register_video_decoder(bk_video_player_playlist_handle_t handle,
                                                           video_player_video_decoder_ops_t *decoder_ops)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_register_video_decoder(handle->engine, decoder_ops);
}

avdk_err_t bk_video_player_playlist_register_container_parser(bk_video_player_playlist_handle_t handle,
                                                              video_player_container_parser_ops_t *parser_ops)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_register_container_parser(handle->engine, parser_ops);
}

avdk_err_t bk_video_player_playlist_get_current_time(bk_video_player_playlist_handle_t handle, uint64_t *time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_get_current_time(handle->engine, time_ms);
}

avdk_err_t bk_video_player_playlist_get_status(bk_video_player_playlist_handle_t handle, video_player_status_t *status)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->engine, AVDK_ERR_INVAL, TAG, "engine is NULL");
    return bk_video_player_engine_get_status(handle->engine, status);
}

