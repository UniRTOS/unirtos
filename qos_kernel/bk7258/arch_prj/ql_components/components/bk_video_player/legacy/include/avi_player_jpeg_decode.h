#ifndef __AVI_PLAYER_JPEG_DECODE_H_
#define __AVI_PLAYER_JPEG_DECODE_H_

#include "components/media_types.h"
#include "avi_player.h"

bk_err_t avi_player_jpeg_hw_decode_init(bk_avi_player_format_t output_format, uint32_t image_width);

bk_err_t avi_player_jpeg_hw_decode_deinit(bk_avi_player_format_t output_format);

bk_err_t avi_player_jpeg_hw_decode_start(bk_avi_player_t *avi_player);

#endif
