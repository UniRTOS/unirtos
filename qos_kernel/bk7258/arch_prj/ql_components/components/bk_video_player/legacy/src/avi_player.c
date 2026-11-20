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

#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "avi_player.h"
#include "avi_player_jpeg_decode.h"


#define TAG "avi_player"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static bk_avi_player_t *avi_player = NULL;

static void bk_avi_player_free(void)
{
    if (avi_player->avi) {
        AVI_close(avi_player->avi);
        avi_player->avi = NULL;
    }

    if (avi_player->video_frame) {
        psram_free(avi_player->video_frame);
        avi_player->video_frame = NULL;
    }

    if (avi_player->framebuffer) {
        psram_free(avi_player->framebuffer);
        avi_player->framebuffer = NULL;
    }

    if (avi_player->segmentbuffer) {
        psram_free(avi_player->segmentbuffer);
        avi_player->segmentbuffer = NULL;
    }
}

bk_err_t bk_avi_player_video_parse(void)
{
    bk_err_t ret = BK_OK;

    if (avi_player == NULL) {
        LOGE("%s %d avi_player is NULL\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    ret = AVI_set_video_read_index(avi_player->avi, avi_player->pos, (long *)&avi_player->video_len);
    if (ret != BK_OK) {
        LOGE("%s %d AVI_set_video_read_index failed\r\n", __func__, __LINE__);
        return ret;
    }

    AVI_read_next_video_frame(avi_player->avi, (char *)avi_player->video_frame, avi_player->video_len);

    if (avi_player->video_len == 0) {
        avi_player->pos = avi_player->pos + 1;
        ret = AVI_set_video_read_index(avi_player->avi, avi_player->pos, (long *)&avi_player->video_len);
        if (ret != BK_OK) {
            LOGE("%s %d AVI_set_video_read_index failed\r\n", __func__, __LINE__);
            return ret;
        }

        AVI_read_next_video_frame(avi_player->avi, (char *)avi_player->video_frame, avi_player->video_len);
    }

    ret = avi_player_jpeg_hw_decode_start(avi_player);
    if (ret != BK_OK) {
        LOGE("%s %d avi_player_jpeg_hw_decode_start failed\r\n", __func__, __LINE__);
        return ret;
    }

    if (avi_player->segment_flag == true) {
        for (int i = 0; i < avi_player->avi->height; i++) {
            os_memcpy(avi_player->segmentbuffer + i * avi_player->avi->width, avi_player->framebuffer + i * avi_player->avi->width * 2, avi_player->avi->width);
            os_memcpy(avi_player->segmentbuffer + (avi_player->avi->width >> 1) * avi_player->avi->height * 2 + i * avi_player->avi->width, avi_player->framebuffer + i * avi_player->avi->width * 2 + avi_player->avi->width, avi_player->avi->width);
        }
    }

    return ret;
}

bk_err_t bk_avi_player_open(bk_avi_player_config_t *player_config)
{
    bk_err_t ret = BK_OK;

    if (player_config == NULL) {
        LOGE("%s %d player_config is NULL\r\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if (avi_player != NULL) {
        LOGE("%s %d avi_player has opened\r\n", __func__, __LINE__);
        return BK_OK;
    }

    avi_player = os_malloc(sizeof(bk_avi_player_t));
    if (avi_player == NULL) {
        LOGE("%s %d avi_player malloc failed\r\n", __func__, __LINE__);
        goto out;
    }
    os_memset(avi_player, 0x00, sizeof(bk_avi_player_t));

    avi_player->avi = AVI_open_input_file((const char *)player_config->file_path, 1, AVI_MEM_PSRAM);
    if (avi_player->avi == NULL) {
        LOGE("%s %d open avi file failed\r\n", __func__, __LINE__);
        goto out;
    } else {
        avi_player->video_num = AVI_video_frames(avi_player->avi);
        if (player_config->output_format == AVI_PLAYER_OUTPUT_FORMAT_RGB888) {
            avi_player->frame_size = avi_player->avi->width * avi_player->avi->height * 3;
        } else {
            avi_player->frame_size = avi_player->avi->width * avi_player->avi->height * 2;
        }
        LOGI("avi video_num: %d, width: %d, height: %d, frame_size: %d, fps: %d\r\n", avi_player->video_num, avi_player->avi->width, avi_player->avi->height, avi_player->frame_size, (uint32_t)avi_player->avi->fps);
    }

    avi_player->pos = 0;
    avi_player->segment_flag = player_config->segment_flag;
    avi_player->swap_flag = player_config->rgb565_byte_swap_flag;
    avi_player->output_format = player_config->output_format;

    avi_player->video_frame = psram_malloc(AVI_VIDEO_MAX_JPEG_FRAME_LEN);
    if (avi_player->video_frame == NULL) {
        LOGE("%s video_frame malloc fail\r\n", __func__);
        goto out;
    }

    avi_player->framebuffer = psram_malloc(avi_player->frame_size);
    if (avi_player->framebuffer == NULL) {
        LOGE("%s %d framebuffer malloc fail\r\n", __func__, __LINE__);
        goto out;
    }

    if (avi_player->segment_flag == true) {
        avi_player->segmentbuffer = psram_malloc(avi_player->frame_size);
        if (avi_player->segmentbuffer == NULL) {
            LOGE("%s %d segmentbuffer malloc fail\r\n", __func__, __LINE__);
            goto out;
        }
    }

    ret = avi_player_jpeg_hw_decode_init(avi_player->output_format, avi_player->avi->width);
    if (ret != BK_OK) {
        LOGE("%s %d failed\r\n", __func__, __LINE__);
        goto out;
    }

    ret = bk_avi_player_video_parse();
    if (ret != BK_OK) {
        LOGE("%s %d failed\r\n", __func__, __LINE__);
        avi_player_jpeg_hw_decode_deinit(avi_player->output_format);
        goto out;
    }

    LOGI("%s complete\r\n", __func__);

    return BK_OK;

out:
    bk_avi_player_free();

    if (avi_player) {
        os_free(avi_player);
        avi_player = NULL;
    }

    return BK_FAIL;
}

void bk_avi_player_close(void)
{
    if (avi_player == NULL) {
        LOGE("%s %d avi_player is NULL\r\n", __func__, __LINE__);
        return;
    }

    avi_player_jpeg_hw_decode_deinit(avi_player->output_format);

    bk_avi_player_free();

    if (avi_player) {
        os_free(avi_player);
        avi_player = NULL;
    }

    LOGI("%s complete\r\n", __func__);
}

bk_avi_player_t *bk_avi_player_get_handle(void)
{
    if (avi_player == NULL) {
        LOGE("%s %d avi_player is NULL\r\n", __func__, __LINE__);
        return NULL;
    }

    return avi_player;
}