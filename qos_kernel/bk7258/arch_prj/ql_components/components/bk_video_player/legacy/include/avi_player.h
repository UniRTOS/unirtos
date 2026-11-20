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


#ifndef __AVI_PLAYER_H__
#define __AVI_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "modules/avilib.h"

// Keep legacy AVI player always buildable without depending on Kconfig macros.
// If you need a different limit, define AVI_VIDEO_MAX_JPEG_FRAME_LEN in the build flags.
#ifndef AVI_VIDEO_MAX_JPEG_FRAME_LEN
#define AVI_VIDEO_MAX_JPEG_FRAME_LEN    (102400)
#endif

typedef enum
{
    AVI_PLAYER_OUTPUT_FORMAT_RGB565 = 0,
    AVI_PLAYER_OUTPUT_FORMAT_YUYV = 1,
    AVI_PLAYER_OUTPUT_FORMAT_RGB888 = 2,
} bk_avi_player_format_t;

typedef struct
{
    char *file_path;
    bk_avi_player_format_t output_format;
    bool segment_flag;                      // Segment flag, it is suitable for dual-screen display.
    bool rgb565_byte_swap_flag;             // RGB565 byte swap flag.
} bk_avi_player_config_t;

typedef struct
{
    avi_t *avi;
    uint8_t *video_frame;
    uint32_t video_len;
    uint32_t video_num;

    uint8_t *framebuffer;
    uint8_t *segmentbuffer;

    uint32_t frame_size;
    uint32_t pos;
    bool segment_flag;
    bool swap_flag;
    bk_avi_player_format_t output_format;
} bk_avi_player_t;


bk_err_t bk_avi_player_video_parse(void);

bk_err_t bk_avi_player_open(bk_avi_player_config_t *player_config);

void bk_avi_player_close(void);

bk_avi_player_t *bk_avi_player_get_handle(void);

#ifdef __cplusplus
}
#endif
#endif /* __AUDIO_PLAY_H__ */

