#ifndef __LV_JPEG_HW_DECODE_H_
#define __LV_JPEG_HW_DECODE_H_

#include "lvgl.h"
#include "components/media_types.h"

bk_err_t lv_jpeg_hw_decode_init(uint32_t image_width);

bk_err_t lv_jpeg_hw_decode_deinit(void);

#if CONFIG_LVGL_V8
bk_err_t lv_jpeg_hw_decode_start(frame_buffer_t *jpeg_frame, lv_img_dsc_t *img_dst, bool byte_swap);
#else
bk_err_t lv_jpeg_hw_decode_start(frame_buffer_t *jpeg_frame, lv_image_dsc_t *img_dst, bool byte_swap);
#endif

#endif
