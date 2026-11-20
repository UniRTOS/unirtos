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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "components/avdk_utils/avdk_types.h"
#include "components/avdk_utils/avdk_check.h"
#include "components/bk_video_player/bk_video_player_engine.h"
#include "bk_video_player_ctlr.h"
#include "bk_video_player_buffer_pool.h"
#include "bk_video_player_pipeline.h"
#include "bk_video_player_container_parse.h"
#include "bk_video_player_audio_decode.h"
#include "bk_video_player_video_decode.h"

#define TAG "video_player_engine"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Engine structure (wraps the private controller)
struct bk_video_player_engine
{
    bk_video_player_ctlr_handle_t ctlr;  // Internal controller handle
    char *current_file_path;            // Current file path (set by set_file_path)
    uint8_t volume;                     // Volume level (0-100)
    bool is_muted;                      // Mute state
    bool audio_ctrl_applied;            // Whether default volume/mute has been applied to upper layer

    // Upper-layer audio output control callbacks (optional)
    video_player_audio_set_volume_cb_t audio_set_volume_cb;
    video_player_audio_set_mute_cb_t audio_set_mute_cb;
    void *audio_ctrl_user_data;
};

avdk_err_t bk_video_player_engine_new(bk_video_player_engine_handle_t *handle, bk_video_player_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, "config is NULL");

    bk_video_player_engine_handle_t engine = (bk_video_player_engine_handle_t)os_malloc(sizeof(struct bk_video_player_engine));
    AVDK_RETURN_ON_FALSE(engine, AVDK_ERR_NOMEM, TAG, "Failed to allocate engine handle");

    os_memset(engine, 0, sizeof(struct bk_video_player_engine));

    // Create internal controller
    bk_video_player_ctlr_handle_t ctlr_handle = NULL;
    avdk_err_t ret = bk_video_player_ctlr_new(&ctlr_handle, config);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: Failed to create controller, ret=%d\n", __func__, ret);
        os_free(engine);
        return ret;
    }
    engine->ctlr = ctlr_handle;

    engine->volume = 100;  // Default volume: 100%
    engine->is_muted = false;

    // Store upper-layer audio control callbacks (may be NULL).
    engine->audio_set_volume_cb = config->audio.audio_set_volume_cb;
    engine->audio_set_mute_cb = config->audio.audio_set_mute_cb;
    engine->audio_ctrl_user_data = (config->audio.audio_ctrl_user_data != NULL) ?
                                   config->audio.audio_ctrl_user_data :
                                   config->user_data;

    *handle = engine;
    LOGI("%s: Engine created\n", __func__);

    return AVDK_ERR_OK;
}

static void bk_video_player_engine_apply_default_audio_ctrl(bk_video_player_engine_handle_t handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->audio_ctrl_applied)
    {
        return;
    }

    // Apply default volume/mute to upper layer (best-effort).
    // Some applications may finish audio output initialization after engine_new() but before engine_open().
    if (handle->audio_set_volume_cb != NULL)
    {
        avdk_err_t ret = handle->audio_set_volume_cb(handle->audio_ctrl_user_data, handle->volume);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: apply default volume failed, ret=%d, volume=%u\n",
                 __func__, ret, (unsigned)handle->volume);
        }
    }

    if (handle->audio_set_mute_cb != NULL)
    {
        avdk_err_t ret = handle->audio_set_mute_cb(handle->audio_ctrl_user_data, handle->is_muted);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: apply default mute failed, ret=%d, mute=%u\n",
                 __func__, ret, (unsigned)handle->is_muted);
        }
    }

    handle->audio_ctrl_applied = true;
}

avdk_err_t bk_video_player_engine_delete(bk_video_player_engine_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    if (handle->ctlr != NULL)
    {
        if (handle->ctlr->delete_video_player != NULL)
        {
            handle->ctlr->delete_video_player(handle->ctlr);
        }
        handle->ctlr = NULL;
    }

    if (handle->current_file_path != NULL)
    {
        os_free(handle->current_file_path);
        handle->current_file_path = NULL;
    }

    os_free(handle);
    LOGI("%s: Engine deleted\n", __func__);

    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_engine_open(bk_video_player_engine_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->open, AVDK_ERR_INVAL, TAG, "open function is NULL");

    avdk_err_t ret = handle->ctlr->open(handle->ctlr);
    if (ret != AVDK_ERR_OK)
    {
        return ret;
    }

    bk_video_player_engine_apply_default_audio_ctrl(handle);
    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_engine_close(bk_video_player_engine_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->close, AVDK_ERR_INVAL, TAG, "close function is NULL");

    return handle->ctlr->close(handle->ctlr);
}

avdk_err_t bk_video_player_engine_set_file_path(bk_video_player_engine_handle_t handle, const char *file_path)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(file_path, AVDK_ERR_INVAL, TAG, "file_path is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->set_file_path, AVDK_ERR_INVAL, TAG, "set_file_path function is NULL");

    // Free existing file path
    if (handle->current_file_path != NULL)
    {
        os_free(handle->current_file_path);
        handle->current_file_path = NULL;
    }

    // Allocate and copy new file path
    size_t path_len = os_strlen(file_path) + 1;
    handle->current_file_path = (char *)os_malloc(path_len);
    AVDK_RETURN_ON_FALSE(handle->current_file_path, AVDK_ERR_NOMEM, TAG, "Failed to allocate file path");

    os_strncpy(handle->current_file_path, file_path, path_len - 1);
    handle->current_file_path[path_len - 1] = '\0';

    LOGI("%s: File path set: %s\n", __func__, file_path);

    // Keep controller in sync so that seek() can restart playback internally.
    avdk_err_t ret = handle->ctlr->set_file_path(handle->ctlr, file_path);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: controller set_file_path failed, ret=%d\n", __func__, ret);
        return ret;
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_engine_play(bk_video_player_engine_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->current_file_path, AVDK_ERR_INVAL, TAG, "file_path not set, call set_file_path first");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->play, AVDK_ERR_INVAL, TAG, "play function is NULL");

    return handle->ctlr->play(handle->ctlr, handle->current_file_path);
}

avdk_err_t bk_video_player_engine_stop(bk_video_player_engine_handle_t handle)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->stop, AVDK_ERR_INVAL, TAG, "stop function is NULL");

    return handle->ctlr->stop(handle->ctlr);
}

avdk_err_t bk_video_player_engine_set_pause(bk_video_player_engine_handle_t handle, bool pause)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->set_pause, AVDK_ERR_INVAL, TAG, "set_pause function is NULL");

    return handle->ctlr->set_pause(handle->ctlr, pause);
}

avdk_err_t bk_video_player_engine_seek(bk_video_player_engine_handle_t handle, uint64_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->current_file_path, AVDK_ERR_INVAL, TAG, "file_path not set, call set_file_path first");
    AVDK_RETURN_ON_FALSE(handle->ctlr->seek, AVDK_ERR_INVAL, TAG, "seek function is NULL");

    // Seek behavior is fully handled in controller:
    // - PAUSED: seek_preview one frame and keep paused, mark pending restart on resume
    // - PLAYING/FINISHED: stop->seek->play to keep playing
    // - OPENED/STOPPED: seek->play
    return handle->ctlr->seek(handle->ctlr, time_ms);
}

avdk_err_t bk_video_player_engine_play_file(bk_video_player_engine_handle_t handle, const char *file_path)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(file_path && file_path[0] != '\0', AVDK_ERR_INVAL, TAG, "file_path is NULL or empty");

    // Best-effort stop to ensure we always restart from STOPPED state (avoid PAUSED seek_preview behavior).
    (void)bk_video_player_engine_stop(handle);

    avdk_err_t ret = bk_video_player_engine_set_file_path(handle, file_path);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: set_file_path failed, ret=%d\n", __func__, ret);
        return ret;
    }

    ret = bk_video_player_engine_play(handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: play failed, ret=%d\n", __func__, ret);
        return ret;
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_engine_set_volume(bk_video_player_engine_handle_t handle, uint8_t volume)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    if (volume > 100)
    {
        volume = 100;
    }

    handle->volume = volume;

    /*
     * Volume control must be applied by upper layer (DAC/codec/voice service, etc.).
     * bk_video_player_engine only stores the value and notifies upper layer via callback.
     */
    if (handle->audio_set_volume_cb == NULL)
    {
        LOGW("%s: audio_set_volume_cb is NULL, volume not applied\n", __func__);
        return AVDK_ERR_UNSUPPORTED;
    }

    avdk_err_t ret = handle->audio_set_volume_cb(handle->audio_ctrl_user_data, volume);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: audio_set_volume_cb failed, ret=%d\n", __func__, ret);
        return ret;
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_engine_volume_up(bk_video_player_engine_handle_t handle, uint8_t step)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    if (step == 0)
    {
        return AVDK_ERR_OK;
    }

    uint32_t new_volume = (uint32_t)handle->volume + (uint32_t)step;
    if (new_volume > 100U)
    {
        new_volume = 100U;
    }

    return bk_video_player_engine_set_volume(handle, (uint8_t)new_volume);
}

avdk_err_t bk_video_player_engine_volume_down(bk_video_player_engine_handle_t handle, uint8_t step)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    if (step == 0)
    {
        return AVDK_ERR_OK;
    }

    uint8_t cur = handle->volume;
    uint8_t new_volume = 0;
    if (cur > step)
    {
        new_volume = (uint8_t)(cur - step);
    }

    return bk_video_player_engine_set_volume(handle, new_volume);
}

avdk_err_t bk_video_player_engine_set_mute(bk_video_player_engine_handle_t handle, bool mute)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");

    handle->is_muted = mute;

    /*
     * Mute control must be applied by upper layer (DAC/codec/voice service, etc.).
     * Prefer notifying upper layer via callback. If callback is not registered,
     * fall back to controller mute if supported for backward compatibility.
     */
    if (handle->audio_set_mute_cb != NULL)
    {
        avdk_err_t ret = handle->audio_set_mute_cb(handle->audio_ctrl_user_data, mute);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: audio_set_mute_cb failed, ret=%d\n", __func__, ret);
        }
        return ret;
    }

    return AVDK_ERR_OK;
}

avdk_err_t bk_video_player_engine_fast_forward(bk_video_player_engine_handle_t handle, uint32_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->fast_forward, AVDK_ERR_UNSUPPORTED, TAG, "fast_forward function is NULL");

    // Delegate to controller implementation (handles state transitions and duration clamp).
    return handle->ctlr->fast_forward(handle->ctlr, time_ms);
}

avdk_err_t bk_video_player_engine_rewind(bk_video_player_engine_handle_t handle, uint32_t time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->rewind, AVDK_ERR_UNSUPPORTED, TAG, "rewind function is NULL");

    // Delegate to controller implementation (handles state transitions and lower bound clamp).
    return handle->ctlr->rewind(handle->ctlr, time_ms);
}

avdk_err_t bk_video_player_engine_set_av_sync_offset_ms(bk_video_player_engine_handle_t handle, int32_t offset_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->ioctl, AVDK_ERR_UNSUPPORTED, TAG, "ioctl function is NULL");

    bk_video_player_av_sync_offset_param_t param;
    os_memset(&param, 0, sizeof(param));
    param.offset_ms = offset_ms;

    return handle->ctlr->ioctl(handle->ctlr, BK_VIDEO_PLAYER_IOCTL_CMD_SET_AV_SYNC_OFFSET_MS, &param);
}

avdk_err_t bk_video_player_engine_register_audio_decoder(bk_video_player_engine_handle_t handle,
                                                         const video_player_audio_decoder_ops_t *decoder_ops)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->register_audio_decoder, AVDK_ERR_INVAL, TAG, "register_audio_decoder function is NULL");

    return handle->ctlr->register_audio_decoder(handle->ctlr, decoder_ops);
}

avdk_err_t bk_video_player_engine_register_video_decoder(bk_video_player_engine_handle_t handle,
                                                         video_player_video_decoder_ops_t *decoder_ops)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->register_video_decoder, AVDK_ERR_INVAL, TAG, "register_video_decoder function is NULL");

    return handle->ctlr->register_video_decoder(handle->ctlr, decoder_ops);
}

avdk_err_t bk_video_player_engine_register_container_parser(bk_video_player_engine_handle_t handle,
                                                            video_player_container_parser_ops_t *parser_ops)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->register_container_parser, AVDK_ERR_INVAL, TAG, "register_container_parser function is NULL");

    return handle->ctlr->register_container_parser(handle->ctlr, parser_ops);
}

avdk_err_t bk_video_player_engine_get_current_time(bk_video_player_engine_handle_t handle, uint64_t *time_ms)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(time_ms, AVDK_ERR_INVAL, TAG, "time_ms is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->get_current_time, AVDK_ERR_INVAL, TAG, "get_current_time function is NULL");

    return handle->ctlr->get_current_time(handle->ctlr, time_ms);
}

avdk_err_t bk_video_player_engine_get_status(bk_video_player_engine_handle_t handle, video_player_status_t *status)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(status, AVDK_ERR_INVAL, TAG, "status is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->get_status, AVDK_ERR_INVAL, TAG, "get_status function is NULL");

    return handle->ctlr->get_status(handle->ctlr, status);
}

avdk_err_t bk_video_player_engine_get_media_info(bk_video_player_engine_handle_t handle,
                                                 const char *file_path,
                                                 video_player_media_info_t *media_info)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, "handle is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr, AVDK_ERR_INVAL, TAG, "controller is NULL");
    AVDK_RETURN_ON_FALSE(handle->ctlr->get_media_info, AVDK_ERR_UNSUPPORTED, TAG, "get_media_info function is NULL");

    return handle->ctlr->get_media_info(handle->ctlr, file_path, media_info);
}

