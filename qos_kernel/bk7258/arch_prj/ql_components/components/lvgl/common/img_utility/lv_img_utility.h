#ifndef __LV_IMG_UTILITY_H
#define __LV_IMG_UTILITY_H

#include "lvgl.h"

int lv_img_get_filelen(char *filename);

#if CONFIG_LVGL_V8
bk_err_t lv_jpeg_img_load_with_sw_dec(char *filename, lv_img_dsc_t *img_dst, bool byte_swap);
#else
bk_err_t lv_jpeg_img_load_with_sw_dec(char *filename, lv_image_dsc_t *img_dst, bool byte_swap);
#endif

#if CONFIG_LVGL_V8
bk_err_t lv_jpeg_img_load_with_hw_dec(char *filename, lv_img_dsc_t *img_dst, bool byte_swap);
#else
bk_err_t lv_jpeg_img_load_with_hw_dec(char *filename, lv_image_dsc_t *img_dst, bool byte_swap);
#endif

#if CONFIG_LVGL_V8
bk_err_t lv_png_img_load(char *filename, lv_img_dsc_t *img_dst);
#else
bk_err_t lv_png_img_load(char *filename, lv_image_dsc_t *img_dst);
#endif

#if CONFIG_LVGL_V8
void lv_img_decode_unload(lv_img_dsc_t *img_dst);
#else
void lv_img_decode_unload(lv_image_dsc_t *img_dst);
#endif

#endif

